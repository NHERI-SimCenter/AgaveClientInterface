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

#include "agavehandler.h"

#include "agavetaskguide.h"
#include "agavetaskreply.h"
#include "agavepipebuffer.h"

#include "../filemetadata.h"

//TODO: need to do more double checking of valid file paths

AgaveHandler::AgaveHandler() :
        RemoteDataInterface(), networkHandle(0), SSLoptions()
{
    SSLoptions.setProtocol(QSsl::SecureProtocols);
    clearAllAuthTokens();

    setupTaskGuideList();
    QObject::connect(&networkHandle, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedOneTask()));
}

void AgaveHandler::finishedOneTask()
{
    pendingRequestCount--;
    if (pendingRequestCount < 0)
    {
        qDebug("Internal Error: Request count becoming negative");
        pendingRequestCount = 0;
    }
    if (pendingRequestCount == 0)
    {
        emit finishedAllTasks();
    }
}

AgaveHandler::~AgaveHandler()
{
    if ((performingShutdown == false) || (authGained == true))
    {
        qDebug("ERROR: Agave Handler destroyed without proper shutdown");
    }
    foreach (AgaveTaskGuide * aTaskGuide , validTaskList)
    {
        delete aTaskGuide;
    }
}

QString AgaveHandler::getUserName()
{
    if (authGained)
    {
        return authUname;
    }
    return QString();
}

bool AgaveHandler::inShutdownMode()
{
    return performingShutdown;
}

RemoteDataReply * AgaveHandler::setCurrentRemoteWorkingDirectory(QString cd)
{
    QString tmp = getPathReletiveToCWD(cd);

    AgaveTaskReply * passThru = new AgaveTaskReply(retriveTaskGuide("changeDir"),NULL,this, (QObject *)this);
    if (passThru == NULL) return NULL;

    passThru->getTaskParamList()->insert("cd",cd.toLatin1());

    if (tmp.isEmpty())
    {
        passThru->delayedPassThruReply(RequestState::NO_CHANGE_DIR, pwd);
    }
    else
    {
        pwd = tmp;
        passThru->delayedPassThruReply(RequestState::GOOD, pwd);
    }

    return (RemoteDataReply *) passThru;
}

QString AgaveHandler::getPathReletiveToCWD(QString inputPath)
{
    //TODO: check validity of input path
    QString cleanedInput = FileMetaData::cleanPathSlashes(inputPath);

    QStringList retList;

    if (inputPath.at(0) == '/')
    {
        QStringList oldList = cleanedInput.split('/');

        for (auto itr = oldList.cbegin(); itr != oldList.cend(); itr++)
        {
            if ((*itr) == ".") {}
            else if ((*itr) == "..")
            {
                retList.removeLast();
            }
            else if (!(*itr).isEmpty())
            {
                retList.append(*itr);
            }
        }
    }
    else
    {
        QStringList oldList = pwd.split('/');
        QStringList newList = cleanedInput.split('/');

        for (auto itr = oldList.cbegin(); itr != oldList.cend(); itr++)
        {
            if ((*itr) == ".") {}
            else if ((*itr) == "..")
            {
                retList.removeLast();
            }
            else if (!(*itr).isEmpty())
            {
                retList.append(*itr);
            }
        }

        for (auto itr = newList.cbegin(); itr != newList.cend(); itr++)
        {
            if ((*itr) == ".") {}
            else if ((*itr) == "..")
            {
                retList.removeLast();
            }
            else if (!(*itr).isEmpty())
            {
                retList.append(*itr);
            }
        }
    }

    QString ret;

    for (auto itr = retList.cbegin(); itr != retList.cend(); itr++)
    {
        if (!(*itr).isEmpty())
        {
            ret.append('/');
            ret.append(*itr);
        }
    }

    return ret;
}

RemoteDataReply * AgaveHandler::performAuth(QString uname, QString passwd)
{   
    if (attemptingAuth || authGained)
    {
        return NULL;
    }
    authUname = uname;
    authPass = passwd;

    authEncoded = "Basic ";
    QByteArray rawAuth(uname.toLatin1());
    rawAuth.append(":");
    rawAuth.append(passwd);
    authEncoded.append(rawAuth.toBase64());

    AgaveTaskReply * parentReply = new AgaveTaskReply(retriveTaskGuide("fullAuth"),NULL,this,(QObject *)this);
    QMap<QString, QByteArray> taskVars;
    parentReply->getTaskParamList()->insert("uname", uname.toLatin1());
    parentReply->getTaskParamList()->insert("passwd", passwd.toLatin1());
    AgaveTaskReply * tmp = performAgaveQuery("authStep1", taskVars, (QObject *)parentReply);

    if (tmp == NULL)
    {
        parentReply->deleteLater();
        return NULL;
    }
    attemptingAuth = true;

    return (RemoteDataReply *) parentReply;
}

RemoteDataReply * AgaveHandler::remoteLS(QString dirPath)
{
    QString tmp = getPathReletiveToCWD(dirPath);
    if ((tmp.isEmpty()) || (tmp == "/") || (tmp == ""))
    {
        tmp = "/";
        tmp.append(authUname);
    }

    QMap<QString, QByteArray> taskVars;
    taskVars.insert("dirPath", tmp.toLatin1());

    AgaveTaskReply * theReply = performAgaveQuery("dirListing", taskVars);
    return (RemoteDataReply *) theReply;
}

RemoteDataReply * AgaveHandler::deleteFile(QString toDelete)
{
    QString toCheck = getPathReletiveToCWD(toDelete);

    QMap<QString, QByteArray> taskVars;
    taskVars.insert("toDelete", toCheck.toLatin1());

    AgaveTaskReply * theReply = performAgaveQuery("fileDelete", taskVars);
    return (RemoteDataReply *) theReply;
}

RemoteDataReply * AgaveHandler::moveFile(QString from, QString to)
{
    QString fromCheck = getPathReletiveToCWD(from);
    QString toCheck = getPathReletiveToCWD(to);
    //TODO: check stuff is valid

    QMap<QString, QByteArray> taskVars;
    taskVars.insert("from", fromCheck.toLatin1());
    taskVars.insert("to", toCheck.toLatin1());

    AgaveTaskReply * theReply = performAgaveQuery("fileMove", taskVars);
    return (RemoteDataReply *) theReply;
}

RemoteDataReply * AgaveHandler::copyFile(QString from, QString to)
{
    QString fromCheck = getPathReletiveToCWD(from);
    QString toCheck = getPathReletiveToCWD(to);
    //TODO: check stuff is valid

    QMap<QString, QByteArray> taskVars;
    taskVars.insert("from", fromCheck.toLatin1());
    taskVars.insert("to", toCheck.toLatin1());

    AgaveTaskReply * theReply = performAgaveQuery("fileCopy", taskVars);
    return (RemoteDataReply *) theReply;
}

RemoteDataReply * AgaveHandler::renameFile(QString fullName, QString newName)
{
    QString toCheck = getPathReletiveToCWD(fullName);
    //TODO: check that path and new name is valid

    QMap<QString, QByteArray> taskVars;
    taskVars.insert("fullName", toCheck.toLatin1());
    taskVars.insert("newName", newName.toLatin1());

    AgaveTaskReply * theReply = performAgaveQuery("renameFile", taskVars);
    return (RemoteDataReply *) theReply;
}

RemoteDataReply * AgaveHandler::mkRemoteDir(QString location, QString newName)
{
    QString toCheck = getPathReletiveToCWD(location);
    //TODO: check that path and new name is valid

    QMap<QString, QByteArray> taskVars;
    taskVars.insert("location", toCheck.toLatin1());
    taskVars.insert("newName", newName.toLatin1());

    AgaveTaskReply * theReply = performAgaveQuery("newFolder", taskVars);
    return (RemoteDataReply *) theReply;
}

RemoteDataReply * AgaveHandler::uploadFile(QString location, QString localFileName)
{
    QString toCheck = getPathReletiveToCWD(location);
    //TODO: check that path and local file exists

    QMap<QString, QByteArray> taskVars;
    taskVars.insert("location", toCheck.toLatin1());
    taskVars.insert("localFileName", localFileName.toLatin1());

    AgaveTaskReply * theReply = performAgaveQuery("fileUpload", taskVars);
    return (RemoteDataReply *) theReply;
}

RemoteDataReply * AgaveHandler::uploadBuffer(QString location, QByteArray fileData, QString newFileName)
{
    QString toCheck = getPathReletiveToCWD(location);
    //TODO: check that path and local file exists

    QMap<QString, QByteArray> taskVars;
    taskVars.insert("location", toCheck.toLatin1());
    taskVars.insert("newFileName", newFileName.toLatin1());
    taskVars.insert("fileData", fileData);

    AgaveTaskReply * theReply = performAgaveQuery("filePipeUpload",taskVars);
    return (RemoteDataReply *) theReply;
}

RemoteDataReply * AgaveHandler::downloadFile(QString localDest, QString remoteName)
{
    //TODO: check path and local path
    QString toCheck = getPathReletiveToCWD(remoteName);

    QMap<QString, QByteArray> taskVars;
    taskVars.insert("remoteName", toCheck.toLatin1());
    taskVars.insert("localDest", localDest.toLatin1());

    AgaveTaskReply * theReply = performAgaveQuery("fileDownload", taskVars);
    return (RemoteDataReply *) theReply;
}

RemoteDataReply * AgaveHandler::downloadBuffer(QString remoteName)
{
    //TODO: check path
    QString toCheck = getPathReletiveToCWD(remoteName);

    QMap<QString, QByteArray> taskVars;
    taskVars.insert("remoteName", toCheck.toLatin1());

    AgaveTaskReply * theReply = performAgaveQuery("filePipeDownload", taskVars);
    return (RemoteDataReply *) theReply;
}

AgaveTaskReply *AgaveHandler::getAgaveAppList()
{
    return performAgaveQuery("getAgaveList");
}

RemoteDataReply * AgaveHandler::runRemoteJob(QString jobName, ParamMap jobParameters, QString remoteWorkingDir, QString indivJobName)
{
    //This function is only for Agave Jobs
    AgaveTaskGuide * guideToCheck = retriveTaskGuide(jobName);
    if (guideToCheck == NULL)
    {
        qDebug("ERROR: Agave App not configured");
        return NULL;
    }
    if (guideToCheck->getRequestType() != AgaveRequestType::AGAVE_APP)
    {
        qDebug("ERROR: Agave App not configured as Agave App");
        return NULL;
    }
    QString fullAgaveName = guideToCheck->getAgaveFullName();
    if (fullAgaveName.isEmpty())
    {
        qDebug("ERROR: Agave App does not have a full name");
        return NULL;
    }

    QJsonDocument rawJSONinput;
    QJsonObject rootObject;
    rootObject.insert("appId",QJsonValue(fullAgaveName));
    if (indivJobName.isEmpty())
    {
        rootObject.insert("name",QJsonValue(fullAgaveName.append("-run")));
    }
    else
    {
        rootObject.insert("name",QJsonValue(indivJobName));
    }


    if (jobName.startsWith("cwe-"))
    {
        QString stageName = jobParameters.value("stage");
        if (!stageName.isEmpty())
        {
            QString simDir = remoteWorkingDir;
            simDir = simDir.append("/");
            simDir = simDir.append(stageName);
            rootObject.insert("archivePath",QJsonValue(simDir));
        }
    }

    QJsonObject inputList;
    QJsonObject paramList;

    QStringList expectedInputs = guideToCheck->getAgaveInputList();
    QStringList expectedParams = guideToCheck->getAgaveParamList();

    QMap<QString, QByteArray> taskVars;
    taskVars.insert("jobName", jobName.toLatin1());

    if ((!guideToCheck->getAgavePWDparam().isEmpty()) && (!remoteWorkingDir.isEmpty()))
    {
        //TODO: check path
        QString realPath = getPathReletiveToCWD(remoteWorkingDir);
        jobParameters.insert(guideToCheck->getAgavePWDparam(),realPath);
        taskVars.insert("remoteWorkingDir", realPath.toLatin1());
    }

    for (auto itr = jobParameters.cbegin(); itr != jobParameters.cend(); itr++)
    {
        taskVars.insert(itr.key(), (*itr).toLatin1());

        QJsonObject * objectToAddTo;

        if (expectedParams.contains(itr.key()))
        {
            objectToAddTo = &paramList;
        }
        else if (expectedInputs.contains(itr.key()))
        {
            objectToAddTo = &inputList;
        }
        else
        {
            qDebug("ERROR: Agave App given invalid parameter");
            return NULL;
        }

        objectToAddTo->insert(itr.key(),QJsonValue(*itr));
    }
    QJsonValue inputListValue(inputList);
    QJsonValue paramListValue(paramList);
    rootObject.insert("inputs",inputListValue);
    rootObject.insert("parameters",paramListValue);
    rawJSONinput.setObject(rootObject);

    taskVars.insert("rawJSONinput", rawJSONinput.toJson());
    taskVars.insert("fileData", rawJSONinput.toJson());

    qDebug("%s",qPrintable(rawJSONinput.toJson()));

    AgaveTaskReply * theReply = performAgaveQuery("agaveAppStart", taskVars);
    return (RemoteDataReply *) theReply;
}

RemoteDataReply * AgaveHandler::runRemoteJob(QJsonDocument rawJobJSON)
{
    //TODO: urgent
    return NULL;
}

RemoteDataReply * AgaveHandler::getListOfJobs()
{
    return (RemoteDataReply *) performAgaveQuery("getJobList");
}

RemoteDataReply * AgaveHandler::getJobDetails(QString IDstr)
{
    QMap<QString, QByteArray> taskVars;
    taskVars.insert("IDstr", IDstr.toLatin1());

    AgaveTaskReply * theReply = performAgaveQuery("getJobDetails", taskVars);
    return (RemoteDataReply *) theReply;
}

RemoteDataReply * AgaveHandler::stopJob(QString IDstr)
{
    QMap<QString, QByteArray> taskVars;
    taskVars.insert("IDstr", IDstr.toLatin1());

    AgaveTaskReply * theReply = performAgaveQuery("stopJob", taskVars);
    return (RemoteDataReply *) theReply;
}

void AgaveHandler::registerAgaveAppInfo(QString agaveAppName, QString fullAgaveName, QStringList parameterList, QStringList inputList, QString workingDirParameter)
{
    AgaveTaskGuide * toInsert = new AgaveTaskGuide(agaveAppName, AgaveRequestType::AGAVE_APP);
    toInsert->setAgaveFullName(fullAgaveName);
    toInsert->setAgaveParamList(parameterList);
    toInsert->setAgaveInputList(inputList);
    toInsert->setAgavePWDparam(workingDirParameter);
    insertAgaveTaskGuide(toInsert);
}

RemoteDataReply * AgaveHandler::closeAllConnections()
{
    //Note: relogin is not yet supported
    AgaveTaskReply * waitHandle = new AgaveTaskReply(retriveTaskGuide("waitAll"),NULL,this,(QObject *)this);
    performingShutdown = true;
    if (waitHandle == NULL)
    {
        return NULL;
    }
    if ((clientEncoded != "") && (token != ""))
    {
        qDebug("Closing all connections sequence begins");
        QMap<QString, QByteArray> taskVars;
        taskVars.insert("token", token);
        performAgaveQuery("authRevoke", taskVars);
        //maybe TODO: Remove client entry?
    }
    else
    {
        qDebug("Not logged in: quick shutdown");
        clearAllAuthTokens();
    }
    QObject::connect(this, SIGNAL(finishedAllTasks()), waitHandle, SLOT(rawTaskComplete()));
    if (pendingRequestCount == 0)
    {
        waitHandle->delayedPassThruReply(RequestState::GOOD);
    }

    return waitHandle;
}

void AgaveHandler::clearAllAuthTokens()
{
    attemptingAuth = false;
    authGained = false;

    authEncoded = "";
    clientEncoded = "";
    token = "";
    refreshToken = "";

    authUname = "";
    authPass = "";
    clientKey = "";
    clientSecret = "";
}

void AgaveHandler::setupTaskGuideList()
{
    AgaveTaskGuide * toInsert = NULL;

    toInsert = new AgaveTaskGuide("changeDir", AgaveRequestType::AGAVE_NONE);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("fullAuth", AgaveRequestType::AGAVE_NONE);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("waitAll", AgaveRequestType::AGAVE_NONE);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("authStep1", AgaveRequestType::AGAVE_GET);
    toInsert->setURLsuffix(QString("/clients/v2/%1").arg(clientName));
    toInsert->setHeaderType(AuthHeaderType::PASSWD);
    toInsert->setAsInternal();
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("authStep1a", AgaveRequestType::AGAVE_DELETE);
    toInsert->setURLsuffix(QString("/clients/v2/%1").arg(clientName));
    toInsert->setHeaderType(AuthHeaderType::PASSWD);
    toInsert->setAsInternal();
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("authStep2", AgaveRequestType::AGAVE_POST);
    toInsert->setURLsuffix(QString("/clients/v2/"));
    toInsert->setHeaderType(AuthHeaderType::PASSWD);
    toInsert->setPostParams(QString("clientName=%1&description=Client ID for SimCenter Wind GUI App").arg(clientName));
    toInsert->setAsInternal();
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("authStep3", AgaveRequestType::AGAVE_POST);
    toInsert->setURLsuffix(QString("/token"));
    toInsert->setHeaderType(AuthHeaderType::CLIENT);
    toInsert->setPostParams("username=%1&password=%2&grant_type=password&scope=PRODUCTION", {"authUname", "authPass"});
    toInsert->setTokenFormat(true);
    toInsert->setAsInternal();
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("authRefresh", AgaveRequestType::AGAVE_POST);
    toInsert->setURLsuffix(QString("/token"));
    toInsert->setHeaderType(AuthHeaderType::CLIENT);
    toInsert->setPostParams("grant_type=refresh_token&scope=PRODUCTION&refresh_token=%1",{"token"});
    toInsert->setTokenFormat(true);
    toInsert->setAsInternal();
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("authRevoke", AgaveRequestType::AGAVE_POST);
    toInsert->setURLsuffix(QString("/revoke"));
    toInsert->setHeaderType(AuthHeaderType::CLIENT);
    toInsert->setPostParams("token=%1",{"token"});
    toInsert->setAsInternal();
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("dirListing", AgaveRequestType::AGAVE_GET);
    toInsert->setURLsuffix((QString("/files/v2/listings/system/%1/")).arg(storageNode));
    toInsert->setDynamicURLParams("%1",{"dirPath"});
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("fileUpload", AgaveRequestType::AGAVE_UPLOAD);
    toInsert->setURLsuffix((QString("/files/v2/media/system/%1/")).arg(storageNode));
    toInsert->setDynamicURLParams("%1",{"location"});
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("fileDownload", AgaveRequestType::AGAVE_DOWNLOAD);
    toInsert->setURLsuffix((QString("/files/v2/media/system/%1/")).arg(storageNode));
    //toInsert->setURLsuffix(QString("/files/v2/media/"));
    toInsert->setDynamicURLParams("%1",{"remoteName"});
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("filePipeUpload", AgaveRequestType::AGAVE_PIPE_UPLOAD);
    toInsert->setURLsuffix((QString("/files/v2/media/system/%1/")).arg(storageNode));
    toInsert->setDynamicURLParams("%1",{"location"});
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("filePipeDownload", AgaveRequestType::AGAVE_PIPE_DOWNLOAD);
    toInsert->setURLsuffix((QString("/files/v2/media/system/%1/")).arg(storageNode));
    toInsert->setDynamicURLParams("%1",{"remoteName"});
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("fileDelete", AgaveRequestType::AGAVE_DELETE);
    toInsert->setURLsuffix((QString("/files/v2/media/system/%1/")).arg(storageNode));
    toInsert->setDynamicURLParams("%1",{"toDelete"});
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("newFolder", AgaveRequestType::AGAVE_PUT);
    toInsert->setURLsuffix((QString("/files/v2/media/system/%1/")).arg(storageNode));
    toInsert->setDynamicURLParams("%1",{"location"});
    toInsert->setPostParams("action=mkdir&path=%1",{"newName"});
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("renameFile", AgaveRequestType::AGAVE_PUT);
    toInsert->setURLsuffix((QString("/files/v2/media/system/%1/")).arg(storageNode));
    toInsert->setDynamicURLParams("%1",{"fullName"});
    toInsert->setPostParams("action=rename&path=%1",{"newName"});
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("fileCopy", AgaveRequestType::AGAVE_PUT);
    toInsert->setURLsuffix((QString("/files/v2/media/system/%1/")).arg(storageNode));
    toInsert->setDynamicURLParams("%1",{"from"});
    toInsert->setPostParams("action=copy&path=%1",{"to"});
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("fileMove", AgaveRequestType::AGAVE_PUT);
    toInsert->setURLsuffix((QString("/files/v2/media/system/%1/")).arg(storageNode));
    toInsert->setDynamicURLParams("%1",{"from"});
    toInsert->setPostParams("action=move&path=%1",{"to"});
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("agaveAppStart", AgaveRequestType::AGAVE_PIPE_UPLOAD);
    toInsert->setURLsuffix(QString("/jobs/v2"));
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("getAgaveList", AgaveRequestType::AGAVE_GET);
    toInsert->setURLsuffix(QString("/apps/v2"));
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("getJobList", AgaveRequestType::AGAVE_GET);
    toInsert->setURLsuffix(QString("/jobs/v2"));
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("getJobDetails", AgaveRequestType::AGAVE_GET);
    toInsert->setURLsuffix(QString("/jobs/v2/"));
    toInsert->setDynamicURLParams("%1",{"IDstr"});
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);

    toInsert = new AgaveTaskGuide("stopJob", AgaveRequestType::AGAVE_POST);
    toInsert->setURLsuffix(QString("/jobs/v2/"));
    toInsert->setDynamicURLParams("%1",{"IDstr"});
    toInsert->setPostParams("action=stop");
    toInsert->setHeaderType(AuthHeaderType::TOKEN);
    insertAgaveTaskGuide(toInsert);
}

void AgaveHandler::insertAgaveTaskGuide(AgaveTaskGuide * newGuide)
{
    QString taskName = newGuide->getTaskID();
    if (validTaskList.contains(taskName))
    {
        qDebug("ERROR: Invalid Task Guide List: Duplicate Name");
        return;
    }
    validTaskList.insert(taskName,newGuide);
}

AgaveTaskGuide * AgaveHandler::retriveTaskGuide(QString taskID)
{
    AgaveTaskGuide * ret;
    if (!validTaskList.contains(taskID))
    {
        qDebug("ERROR: Non-existant request requested.1");
        return NULL;
    }
    ret = validTaskList.value(taskID);
    if (taskID != ret->getTaskID())
    {
        qDebug("ERROR: Task Guide format error.");
    }
    return ret;
}

void AgaveHandler::forwardReplyToParent(AgaveTaskReply * agaveReply, RequestState replyState)
{
    AgaveTaskReply * parentReply = (AgaveTaskReply*)agaveReply->parent();
    if (parentReply == NULL)
    {
        return;
    }
    parentReply->delayedPassThruReply(replyState);
}

void AgaveHandler::forwardReplyToParent(AgaveTaskReply * agaveReply, RequestState replyState, QString param1)
{
    AgaveTaskReply * parentReply = (AgaveTaskReply*)agaveReply->parent();
    if (parentReply == NULL)
    {
        return;
    }
    parentReply->delayedPassThruReply(replyState, param1);
}

void AgaveHandler::handleInternalTask(AgaveTaskReply * agaveReply, QNetworkReply * rawReply)
{
    if (agaveReply->getTaskGuide()->getTaskID() == "authRevoke")
    {
        qDebug("Auth revoke procedure complete");
        clearAllAuthTokens();
        return;
    }

    const QByteArray replyText = rawReply->readAll();

    QJsonParseError parseError;
    QJsonDocument parseHandler = QJsonDocument::fromJson(replyText, &parseError);

    if (parseHandler.isNull())
    {
        forwardReplyToParent(agaveReply, RequestState::JSON_PARSE_ERROR);
        return;
    }

    if (rawOutputDebugEnabled())
    {
        qDebug("%s",qPrintable(parseHandler.toJson()));
    }

    RequestState prelimResult = AgaveTaskReply::standardSuccessFailCheck(agaveReply->getTaskGuide(), &parseHandler);

    QString taskID = agaveReply->getTaskGuide()->getTaskID();

    if ((prelimResult != RequestState::GOOD) && (prelimResult != RequestState::EXPLICIT_ERROR))
    {
        if ((taskID == "authStep1") || (taskID == "authStep1a") || (taskID == "authStep2") || (taskID == "authStep3"))
        {
            clearAllAuthTokens();
        }
        forwardReplyToParent(agaveReply, prelimResult);
        return;
    }

    if (taskID == "authStep1")
    {
        if (prelimResult == RequestState::GOOD)
        {
            QMap<QString, QByteArray> varList;
            if (performAgaveQuery("authStep1a", varList, agaveReply->parent()) == NULL)
            {
                forwardReplyToParent(agaveReply, RequestState::INTERNAL_ERROR);
                clearAllAuthTokens();
            }
        }
        else
        {
            QString messageData = AgaveTaskReply::retriveMainAgaveJSON(&parseHandler, "message").toString();
            if (messageData == "Application not found")
            {
                QMap<QString, QByteArray> varList;
                if (performAgaveQuery("authStep2", varList, agaveReply->parent()) == NULL)
                {
                    forwardReplyToParent(agaveReply, RequestState::INTERNAL_ERROR);
                    clearAllAuthTokens();
                }
            }
            else if (messageData == "Login failed.Please recheck the username and password and try again.")
            {
                clearAllAuthTokens();
                forwardReplyToParent(agaveReply, RequestState::EXPLICIT_ERROR);
            }
            else
            {
                clearAllAuthTokens();
                forwardReplyToParent(agaveReply, prelimResult);
            }
        }
    }
    else if (taskID == "authStep1a")
    {
        if (prelimResult == RequestState::GOOD)
        {
            QMap<QString, QByteArray> varList;
            if (performAgaveQuery("authStep2", varList, agaveReply->parent()) == NULL)
            {
                forwardReplyToParent(agaveReply, RequestState::INTERNAL_ERROR);
                clearAllAuthTokens();
            }
        }
        else
        {
            clearAllAuthTokens();
            forwardReplyToParent(agaveReply, prelimResult);
        }
    }
    else if (taskID == "authStep2")
    {
        if (prelimResult == RequestState::GOOD)
        {
            clientKey = AgaveTaskReply::retriveMainAgaveJSON(&parseHandler, {"result", "consumerKey"}).toString();
            clientSecret = AgaveTaskReply::retriveMainAgaveJSON(&parseHandler, {"result", "consumerSecret"}).toString();

            if (clientKey.isEmpty() || clientSecret.isEmpty())
            {
                forwardReplyToParent(agaveReply, RequestState::JSON_PARSE_ERROR);
                clearAllAuthTokens();
                return;
            }

            clientEncoded = "Basic ";
            QByteArray rawAuth(clientKey.toLatin1());
            rawAuth.append(":");
            rawAuth.append(clientSecret);
            clientEncoded.append(rawAuth.toBase64());

            QMap<QString, QByteArray> varList;
            varList.insert("authUname", authUname.toLatin1());
            varList.insert("authPass", authPass.toLatin1());

            if (performAgaveQuery("authStep3", varList, agaveReply->parent()) == NULL)
            {
                forwardReplyToParent(agaveReply, RequestState::INTERNAL_ERROR);
                clearAllAuthTokens();
            }
        }
        else
        {
            clearAllAuthTokens();
            forwardReplyToParent(agaveReply, prelimResult);
        }
    }
    else if (taskID == "authStep3")
    {
        if (prelimResult == RequestState::GOOD)
        {
            token = AgaveTaskReply::retriveMainAgaveJSON(&parseHandler, "access_token").toString().toLatin1();
            refreshToken = AgaveTaskReply::retriveMainAgaveJSON(&parseHandler, "refresh_token").toString().toLatin1();

            if (token.isEmpty() || refreshToken.isEmpty())
            {
                clearAllAuthTokens();
                forwardReplyToParent(agaveReply, RequestState::JSON_PARSE_ERROR);
            }
            else
            {
                tokenHeader = (QString("Bearer ").append(token)).toLatin1();

                authGained = true;
                attemptingAuth = false;

                forwardReplyToParent(agaveReply, RequestState::GOOD);
                qDebug("Login success.");
            }
        }
        else
        {
            clearAllAuthTokens();
            forwardReplyToParent(agaveReply, prelimResult);
        }
    }
    else if (taskID == "authRefresh")
    {
        qDebug("Auth refresh occurred without being implemented.");
        forwardReplyToParent(agaveReply, RequestState::NOT_IMPLEMENTED);

        /*
        if (prelimResult == RequestState::GOOD)
        {
            token = AgaveTaskReply::retriveMainAgaveJSON(&parseHandler, "access_token").toString().toLatin1();
            refreshToken = AgaveTaskReply::retriveMainAgaveJSON(&parseHandler, "refresh_token").toString().toLatin1();

            if (token.isEmpty() || refreshToken.isEmpty())
            {
                emit sendFatalErrorMessage("Token refresh failure.");
                return;
            }
            //TODO: Will need more info here based on when, how and where refreshes are requested
        }
        */
    }
    else
    {
        qDebug("Non-existant internal request requested.");
        forwardReplyToParent(agaveReply, RequestState::UNKNOWN_TASK);
    }
}

AgaveTaskReply * AgaveHandler::performAgaveQuery(QString queryName)
{
    QMap<QString, QByteArray> taskVars;
    return performAgaveQuery(queryName, taskVars);
}

AgaveTaskReply * AgaveHandler::performAgaveQuery(QString queryName, QMap<QString, QByteArray> varList, QObject * parentReq)
{
    //The network availabilty flag seems innacurate cross-platform
    //Failed task invocations return NULL from this function.
    /*
    if (networkHandle.networkAccessible() == QNetworkAccessManager::NotAccessible)
    {
        emit sendFatalErrorMessage("Network not available");
        return NULL;
    }
    */

    if ((performingShutdown) && (queryName != "authRevoke"))
    {
        qDebug("Rejecting request given during shutdown.");
        return NULL;
    }

    AgaveTaskGuide * taskGuide = retriveTaskGuide(queryName);

    if ((!authGained) && (taskGuide->getHeaderType() == AuthHeaderType::TOKEN))
    {
        return NULL;
    }

    QNetworkReply * qReply = distillRequestData(taskGuide, &varList);

    if (qReply == NULL)
    {
        return NULL;
    }
    pendingRequestCount++;

    QObject * parentObj = (QObject *) this;
    if (parentReq != NULL)
    {
        parentObj = parentReq;
    }

    AgaveTaskReply * ret = new AgaveTaskReply(taskGuide,qReply,this, parentObj);

    for (auto itr = varList.cbegin(); itr != varList.cend(); itr++)
    {
        ret->getTaskParamList()->insert(itr.key(), *itr);
    }

    return ret;
}

QNetworkReply * AgaveHandler::distillRequestData(AgaveTaskGuide * taskGuide, QMap<QString, QByteArray> * varList)
{
    QByteArray * authHeader = NULL;
    if (taskGuide->getHeaderType() == AuthHeaderType::CLIENT)
    {
        authHeader = &clientEncoded;
    }
    else if (taskGuide->getHeaderType() == AuthHeaderType::PASSWD)
    {
        authHeader = &authEncoded;
    }
    else if (taskGuide->getHeaderType() == AuthHeaderType::REFRESH)
    {
        //TODO
    }
    else if (taskGuide->getHeaderType() == AuthHeaderType::TOKEN)
    {
        authHeader = &tokenHeader;
    }

    if ((taskGuide->getRequestType() == AgaveRequestType::AGAVE_POST) || (taskGuide->getRequestType() == AgaveRequestType::AGAVE_PUT))
    {
        //Note: For a put, the post data for this function is used as the put data for the HTTP request
        //qDebug("Post data: %s", qPrintable(taskGuide->fillPostArgList(varList)));
        return finalizeAgaveRequest(taskGuide, taskGuide->getArgAndURLsuffix(varList),
                         authHeader, taskGuide->fillPostArgList(varList));
    }
    else if ((taskGuide->getRequestType() == AgaveRequestType::AGAVE_GET) || (taskGuide->getRequestType() == AgaveRequestType::AGAVE_DELETE))
    {
        qDebug("URL Req: %s", qPrintable(taskGuide->getArgAndURLsuffix(varList)));
        return finalizeAgaveRequest(taskGuide, taskGuide->getArgAndURLsuffix(varList),
                         authHeader);
    }
    else if (taskGuide->getRequestType() == AgaveRequestType::AGAVE_UPLOAD)
    {
        //For agave upload, instead of post params, we have the full local file name
        QString fullFileName = QString::fromLatin1(varList->value("localFileName"));
        QFile * fileHandle = new QFile(fullFileName);
        if (!fileHandle->open(QIODevice::ReadOnly))
        {
            fileHandle->deleteLater();
            return NULL;
        }
        qDebug("URL Req: %s", qPrintable(taskGuide->getArgAndURLsuffix(varList)));

        return finalizeAgaveRequest(taskGuide, taskGuide->getArgAndURLsuffix(varList),
                         authHeader, fullFileName.toLatin1(), fileHandle);
    }
    else if (taskGuide->getRequestType() == AgaveRequestType::AGAVE_PIPE_UPLOAD)
    {
        QByteArray pipedFileData = varList->value("fileData");
        qDebug("New File Name: %s\n", qPrintable(varList->value("newFileName")));

        AgavePipeBuffer * pipedData = new AgavePipeBuffer(&pipedFileData);
        pipedData->open(QBuffer::ReadOnly);
        qDebug("URL Req: %s", qPrintable(taskGuide->getArgAndURLsuffix(varList)));

        return finalizeAgaveRequest(taskGuide, taskGuide->getArgAndURLsuffix(varList),
                         authHeader, varList->value("newFileName"), pipedData);
    }
    else if (taskGuide->getRequestType() == AgaveRequestType::AGAVE_DOWNLOAD)
    {
        //For agave download, instead of post params, we have the full local file name
        QString fullFileName = varList->value("localDest");
        QFile * fileHandle = new QFile(fullFileName);
        if (fileHandle->open(QIODevice::ReadOnly))
        {
            //If the file already exists, we do not overwrite
            //This should be checked by calling client, but we check it here too
            fileHandle->deleteLater();
            return NULL;
        }
        fileHandle->deleteLater();
        qDebug("URL Req: %s", qPrintable(taskGuide->getArgAndURLsuffix(varList)));

        return finalizeAgaveRequest(taskGuide, taskGuide->getArgAndURLsuffix(varList),
                         authHeader);
    }
    else if (taskGuide->getRequestType() == AgaveRequestType::AGAVE_PIPE_DOWNLOAD)
    {
        return finalizeAgaveRequest(taskGuide, taskGuide->getArgAndURLsuffix(varList), authHeader);
    }
    else
    {
        qDebug("ERROR: Non-existant Agave request type requested.");
        return NULL;
    }

    return NULL;
}

QNetworkReply * AgaveHandler::finalizeAgaveRequest(AgaveTaskGuide * theGuide, QString urlAppend, QByteArray * authHeader, QByteArray postData, QIODevice * fileHandle)
{
    QNetworkReply * clientReply = NULL;

    QString activeURL = tenantURL;
    activeURL.append(removeDoubleSlashes(urlAppend));

    QNetworkRequest * clientRequest = new QNetworkRequest();
    clientRequest->setUrl(QUrl(activeURL));

    //clientRequest->setRawHeader("User-Agent", "SimCenterWindGUI");
    if (theGuide->getRequestType() == AgaveRequestType::AGAVE_POST)
    {
        clientRequest->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    }

    if (authHeader != NULL)
    {
        if (authHeader->isEmpty())
        {
            qDebug("ERROR: Authorization request reply has no data in it");
            return NULL;
        }
        clientRequest->setRawHeader(QByteArray("Authorization"), *authHeader);
    }

    //Note: to suppress SSL warning for not having obsolete SSL versions, use
    // QT_LOGGING_RULES in the project build environment variables. Set to:
    // qt.network.ssl.warning=false
    clientRequest->setSslConfiguration(SSLoptions);

    qDebug("%s", qPrintable(clientRequest->url().url()));

    if ((theGuide->getRequestType() == AgaveRequestType::AGAVE_GET) || (theGuide->getRequestType() == AgaveRequestType::AGAVE_DOWNLOAD)
            || (theGuide->getRequestType() == AgaveRequestType::AGAVE_PIPE_DOWNLOAD))
    {
        clientReply = networkHandle.get(*clientRequest);
    }
    else if (theGuide->getRequestType() == AgaveRequestType::AGAVE_POST)
    {
        clientReply = networkHandle.post(*clientRequest, postData);
    }
    else if (theGuide->getRequestType() == AgaveRequestType::AGAVE_PUT)
    {
        clientReply = networkHandle.put(*clientRequest, postData);
    }
    else if (theGuide->getRequestType() == AgaveRequestType::AGAVE_DELETE)
    {
        clientReply = networkHandle.deleteResource(*clientRequest);
    }
    else if ((theGuide->getRequestType() == AgaveRequestType::AGAVE_UPLOAD) || (theGuide->getRequestType() == AgaveRequestType::AGAVE_PIPE_UPLOAD))
    {
        QHttpMultiPart * fileUpload = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        QHttpPart filePart;
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-strem"));
        QString tempString = "form-data; name=\"fileToUpload\"; filename=\"%1\"";
        tempString = tempString.arg(QString(postData));
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(tempString));

        filePart.setBodyDevice(fileHandle);
        //Following line is to insure deletion of the file handle later, when the parent is deleted
        fileHandle->setParent(fileUpload);

        fileUpload->append(filePart);

        clientReply = networkHandle.post(*clientRequest, fileUpload);

        //Following line insures Mulipart object deleted when the network reply is
        fileUpload->setParent(clientReply);
    }

    return clientReply;
}

QString AgaveHandler::getTenantURL()
{
    return tenantURL;
}

QString AgaveHandler::removeDoubleSlashes(QString stringIn)
{
    QString ret;

    for (int i = 0; i < stringIn.size(); i++)
    {
        if (stringIn[i] != '/')
        {
            ret.append(stringIn[i]);
        }
        else if ((i == stringIn.length() - 1) || (stringIn[i + 1] != '/'))
        {
            ret.append(stringIn[i]);
        }
    }
    return ret;
}
