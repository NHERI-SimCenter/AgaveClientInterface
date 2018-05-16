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

#ifndef REMOTEDATAINTERFACE_H
#define REMOTEDATAINTERFACE_H

#include "filemetadata.h"
#include "remotejobdata.h"

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QMap>
#include <QJsonDocument>
#include <QLoggingCategory>

typedef QMap<QString, QString> ParamMap;
Q_DECLARE_METATYPE(ParamMap)

Q_DECLARE_LOGGING_CATEGORY(remoteInterface)
Q_DECLARE_LOGGING_CATEGORY(rawHTTP)

//Good means the request was good and
//Fail means the remote service replied, but did not like the request, for some reason
//No Connect means that the request did not get thru to the remote service at all
enum class RequestState {GOOD, PENDING,
                         UNKNOWN_TASK, INTERNAL_ERROR,
                         SIGNAL_OBJ_MISMATCH, SERVICE_UNAVAILABLE,
                         LOST_INTERNET, DROPPED_CONNECTION,
                         NO_CHANGE_DIR, FILE_NOT_FOUND,
                         JOB_SYSTEM_DOWN, BAD_HTTP_REQUEST,
                         GENERIC_NETWORK_ERROR, REMOTE_SERVER_ERROR,
                         LOCAL_FILE_ERROR, JSON_PARSE_ERROR,
                         EXPLICIT_ERROR, MISSING_REPLY_STATUS,
                         MISSING_REPLY_DATA, STOPPED_BY_USER,
                         INVALID_PARAM, NOT_READY,
                         NOT_IMPLEMENTED, UNCLASSIFIED};
//If RemoteDataReply returned is NULL, then the request was invalid due to internal error

class RemoteDataReply : public QObject
{
    Q_OBJECT

public:
    RemoteDataReply(QObject * parent);

signals:
    //All referenced values should be copied by the reciever or they will be discarded
    void haveCurrentRemoteDir(RequestState replyState, QString pwd);
    void connectionsClosed(RequestState replyState);

    void haveAuthReply(RequestState authReply);
    void haveLSReply(RequestState replyState, QList<FileMetaData> fileDataList);

    void haveDeleteReply(RequestState replyState, QString toDelete);
    void haveMoveReply(RequestState replyState, FileMetaData revisedFileData, QString from);
    void haveCopyReply(RequestState replyState, FileMetaData newFileData);
    void haveRenameReply(RequestState replyState, FileMetaData newFileData, QString oldName);

    void haveMkdirReply(RequestState replyState, FileMetaData newFolderData);

    void haveUploadReply(RequestState replyState, FileMetaData newFileData);
    void haveDownloadReply(RequestState replyState, QString localDest);
    void haveBufferDownloadReply(RequestState replyState, QByteArray fileBuffer);

    //Job replys should be in an intelligble format, JSON is used by Agave and AWS for various things
    void haveJobReply(RequestState replyState, QJsonDocument rawJobReply);

    void haveJobList(RequestState replyState, QList<RemoteJobData> jobList);
    void haveJobDetails(RequestState replyState, RemoteJobData jobData);
    void haveStoppedJob(RequestState replyState);
};

class RemoteDataInterface : public QObject
{
    Q_OBJECT

public:
    RemoteDataInterface(QObject * parent = NULL);

public slots:
    virtual QString getUserName() = 0;
    virtual bool isDisconnected() = 0;

    //Defaults to directory root,
    //Subsequent commands with remote folder names are either absolute paths
    //or reletive to the current working directory
    virtual RemoteDataReply * setCurrentRemoteWorkingDirectory(QString cd) = 0;
    virtual RemoteDataReply * closeAllConnections() = 0;

    //Remote tasks to be implemented in subclasses:
    //Returns a RemoteDataReply, which should have the correct signal attached to an appropriate slot
    virtual RemoteDataReply * performAuth(QString uname, QString passwd) = 0;

    virtual RemoteDataReply * remoteLS(QString dirPath) = 0;

    virtual RemoteDataReply * deleteFile(QString toDelete) = 0;
    virtual RemoteDataReply * moveFile(QString from, QString to) = 0;
    virtual RemoteDataReply * copyFile(QString from, QString to) = 0;
    virtual RemoteDataReply * renameFile(QString fullName, QString newName) = 0;

    virtual RemoteDataReply * mkRemoteDir(QString location, QString newName) = 0;

    virtual RemoteDataReply * uploadFile(QString location, QString localFileName) = 0;
    virtual RemoteDataReply * uploadBuffer(QString location, QByteArray fileData, QString newFileName) = 0;
    virtual RemoteDataReply * downloadFile(QString localDest, QString remoteName) = 0;
    virtual RemoteDataReply * downloadBuffer(QString remoteName) = 0;

    virtual RemoteDataReply * runRemoteJob(QString jobName, ParamMap jobParameters, QString remoteWorkingDir, QString indivJobName = "") = 0;

    virtual RemoteDataReply * getListOfJobs() = 0;
    virtual RemoteDataReply * getJobDetails(QString IDstr) = 0;
    virtual RemoteDataReply * stopJob(QString IDstr) = 0;

    static QString interpretRequestState(RequestState theState);
};

class RemoteDataThread : public QThread
{
    Q_OBJECT

public:
    RemoteDataThread(QObject * parent);

    bool interfaceReady();

    //All of the following meta-invoke the same using the relevant
    //RemoteDataInterface within this QThread
    //Must invoke start before calling any of them

    //Also, this assumes that the underlying RemoteDataInterface protects the given slots
    //With mutexes or Read/Write locks if needed.
    //If, however, nothing can touch the RemoteDataInterface outside this thread, that should not be needed,
    //Since the event queue provides protection

    //Also, to be safe, RemoteDataReply objects show wait if not connected, in case they finish before
    //connection is made
    QString getUserName();
    bool isDisconnected();

    RemoteDataReply * setCurrentRemoteWorkingDirectory(QString cd);
    RemoteDataReply * closeAllConnections();

    RemoteDataReply * performAuth(QString uname, QString passwd);

    RemoteDataReply * remoteLS(QString dirPath);

    RemoteDataReply * deleteFile(QString toDelete);
    RemoteDataReply * moveFile(QString from, QString to);
    RemoteDataReply * copyFile(QString from, QString to);
    RemoteDataReply * renameFile(QString fullName, QString newName);

    RemoteDataReply * mkRemoteDir(QString location, QString newName);

    RemoteDataReply * uploadFile(QString location, QString localFileName);
    RemoteDataReply * uploadBuffer(QString location, QByteArray fileData, QString newFileName);
    RemoteDataReply * downloadFile(QString localDest, QString remoteName);
    RemoteDataReply * downloadBuffer(QString remoteName);

    RemoteDataReply * runRemoteJob(QString jobName, QMap<QString, QString> jobParameters, QString remoteWorkingDir, QString indivJobName = "");

    RemoteDataReply * getListOfJobs();
    RemoteDataReply * getJobDetails(QString IDstr);
    RemoteDataReply * stopJob(QString IDstr);

protected:
    //In subclass, run() method should point this to a
    //stack variable within run
    RemoteDataInterface * myInterface = NULL;
    QMutex readyLock;

    virtual void run();
    bool remoteThreadReady();

private:
    bool connectIsReady = false;
};

#endif // REMOTEDATAINTERFACE_H
