/*********************************************************************************
**
** Copyright (c) 2017 The University of Notre Dame
** Copyright (c) 2017 The Regents of the University of California
**
** Redistribution and use in source and binary forms, with or without modification,
** are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
** list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice, this
** list of conditions and the following disclaimer in the documentation and/or other
** materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its contributors may
** be used to endorse or promote products derived from this software without specific
** prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
** EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
** SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
** IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
**
***********************************************************************************/

// Contributors:
// Written by Peter Sempolinski, for the Natural Hazard Modeling Laboratory, director: Ahsan Kareem, at Notre Dame

#ifndef AGAVEHANDLER_H
#define AGAVEHANDLER_H

//Note: In order to insure that this work continues to function, even without Agave,
//This is a subclass of a more generic abstract class

#include "../remotedatainterface.h"

#include <QtGlobal>
#include <QObject>
#include <QNetworkAccessManager>
#include <QSslConfiguration>
#include <QJsonDocument>
#include <QFile>
#include <QBuffer>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFileInfo>
#include <QStringList>
#include <QList>
#include <QMultiMap>

enum class AgaveRequestType {AGAVE_GET, AGAVE_POST, AGAVE_DELETE, AGAVE_UPLOAD, AGAVE_PIPE_UPLOAD, AGAVE_DOWNLOAD, AGAVE_PUT, AGAVE_NONE, AGAVE_APP};
enum class AgaveParamType {PARAM_STRING, PARAM_INT, PARAM_BOOL};

class QNetworkReply;
class AgaveTaskGuide;
class AgaveTaskReply;

class AgaveHandler : public RemoteDataInterface
{
    Q_OBJECT

public:
    explicit AgaveHandler(QObject *parent);
    ~AgaveHandler();

    virtual RemoteDataReply * closeAllConnections();

    //Defaults to directory root,
    //Subsequent commands with remote folder names are either absolute paths
    //or reletive to the current working directory
    virtual RemoteDataReply * setCurrentRemoteWorkingDirectory(QString cd);

    //Remote tasks to be implemented in subclasses:
    //Returns a RemoteDataReply, which should have the correct signal attached to an appropriate slot
    virtual RemoteDataReply * performAuth(QString uname, QString passwd);

    virtual RemoteDataReply * remoteLS(QString dirPath);

    virtual RemoteDataReply * deleteFile(QString toDelete);
    virtual RemoteDataReply * moveFile(QString from, QString to);
    virtual RemoteDataReply * copyFile(QString from, QString to);
    virtual RemoteDataReply * renameFile(QString fullName, QString newName);

    virtual RemoteDataReply * mkRemoteDir(QString loc, QString newName);

    virtual RemoteDataReply * uploadFile(QString loc, QString localFileName);
    virtual RemoteDataReply * downloadFile(QString localDest, QString remoteName);

    virtual RemoteDataReply * runRemoteJob(QString jobName, QMultiMap<QString, QString> jobParameters, QString remoteWorkingDir);

    QString getTenantURL();
    void forwardAgaveError(QString errorText);
    bool inShutdownMode();

    //On Agave Apps:
    //There are two ways to invoke Agave Apps,
    //1) (Advanced) Directly, by using:
    RemoteDataReply * invokeAgaveApp(QJsonDocument rawJSONinput);
    //This also requires interfacing with the AgaveHandler at every invoke, and cuts down on the potential for polymorphism

    //2)(Basic) (and allows for polymorphism for code other than setup)
    //Register info on the Agave App's parameters, using:
    void registerAgaveAppInfo(QString agaveAppName, QString fullAgaveName, QStringList parameterList, QStringList inputList, QString workingDirParameter);
    //After that, use the standard runRemoteJob, where jobName is the agaveAppName,
    //the job parameters are a list matching the inputs/parameters given by parameterList and inputList
    //and the remoteWorkingDir will be used as a input/parameter named in remoteDirParameter (optional)

    //For debugging purposes, to retrive the list of availalbe Agave Apps:
    RemoteDataReply * getAgaveAppList();
signals:
    void finishedAllTasks();

private slots:
    void handleInternalTask(AgaveTaskReply *agaveReply, QNetworkReply * rawReply);
    void finishedOneTask(QNetworkReply *reply);

private:
    AgaveTaskReply * performAgaveQuery(QString queryName, QObject * parentReq = NULL);
    AgaveTaskReply * performAgaveQuery(QString queryName, QString param1, QObject * parentReq = NULL);
    AgaveTaskReply * performAgaveQuery(QString queryName, QString param1, QString param2, QObject * parentReq = NULL);
    AgaveTaskReply * performAgaveQuery(QString queryName, QStringList * paramList0 = NULL, QStringList * paramList1 = NULL, QObject * parentReq = NULL);
    QNetworkReply * internalQueryMethod(AgaveTaskGuide * theGuide, QStringList * paramList1 = NULL, QStringList * paramList2 = NULL);
    QNetworkReply * finalizeAgaveRequest(AgaveTaskGuide * theGuide, QString urlAppend, QByteArray * authHeader = NULL, QByteArray postData = "", QIODevice * fileHandle = NULL);

    void forwardReplyToParent(AgaveTaskReply * agaveReply, RequestState replyState, QString * param1 = NULL);

    void clearAllAuthTokens();

    void setupTaskGuideList();
    void insertAgaveTaskGuide(AgaveTaskGuide * newGuide);
    AgaveTaskGuide * retriveTaskGuide(QString taskID);

    QString getPathReletiveToCWD(QString inputPath);

    QNetworkAccessManager networkHandle;
    QSslConfiguration SSLoptions;
    const QString tenantURL = "https://agave.designsafe-ci.org";
    const QString clientName = "SimCenterWindGUI";
    const QString storageNode = "designsafe.storage.default";

    QByteArray authEncloded;
    QByteArray clientEncoded;
    QByteArray token;
    QByteArray tokenHeader;
    QByteArray refreshToken;

    QString authUname;
    QString authPass;
    QString clientKey;
    QString clientSecret;

    QMap<QString, AgaveTaskGuide*> validTaskList;

    QString pwd = "";

    int pendingRequestCount = 0;
    bool performingShutdown = false;
    bool authGained = false;
    bool attemptingAuth = false;
};

#endif // AGAVEHANDLER_H
