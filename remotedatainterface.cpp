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

#include "remotedatainterface.h"

RemoteDataInterface::RemoteDataInterface(QObject *parent):QObject(parent) {}

bool RemoteDataInterface::rawOutputDebugEnabled()
{
    return showRawOutputInDebug;
}

void RemoteDataInterface::setRawDebugOutput(bool newSetting)
{
    showRawOutputInDebug = newSetting;
}

RemoteDataReply::RemoteDataReply(QObject * parent):QObject(parent) {}

RemoteDataThread::RemoteDataThread(QObject *parent):QThread(parent)
{
    QObject::connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

bool RemoteDataThread::interfaceReady()
{
    QMutexLocker lock(&readyLock);
    return remoteThreadReady();
}

void RemoteDataThread::fatalErrorPassthru(QString errorText)
{
    emit sendFatalErrorMessage(errorText);
}

void RemoteDataThread::run()
{
    if (myInterface == NULL)
    {
        emit sendFatalErrorMessage("Internal ERROR: Remote Data Thread not subclassed properly.");
        return;
    }
    QObject::connect(myInterface, SIGNAL(sendFatalErrorMessage(QString)),
                     this, SLOT(fatalErrorPassthru(QString)));

    readyLock.lock();
    connectIsReady = true;
    readyLock.unlock();

    exec();

    readyLock.lock();
    connectIsReady = false;
    readyLock.unlock();
}

bool RemoteDataThread::remoteThreadReady()
{
    if (connectIsReady && isRunning()) return true;
    return false;
}

QString RemoteDataThread::getUserName()
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return QString();
    QString retVal;
    QMetaObject::invokeMethod(myInterface, "getUserName", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QString, retVal));
    return retVal;
}

RemoteDataReply * RemoteDataThread::setCurrentRemoteWorkingDirectory(QString cd)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "setCurrentRemoteWorkingDirectory", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, cd));
    return retVal;
}

RemoteDataReply * RemoteDataThread::closeAllConnections()
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "closeAllConnections", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal));
    return retVal;
}

RemoteDataReply * RemoteDataThread::performAuth(QString uname, QString passwd)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "performAuth", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, uname),
                              Q_ARG(QString, passwd));
    return retVal;
}

RemoteDataReply * RemoteDataThread::remoteLS(QString dirPath)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "remoteLS", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, dirPath));
    return retVal;
}

RemoteDataReply * RemoteDataThread::deleteFile(QString toDelete)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "deleteFile", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, toDelete));
    return retVal;
}

RemoteDataReply * RemoteDataThread::moveFile(QString from, QString to)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "moveFile", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, from),
                              Q_ARG(QString, to));
    return retVal;
}

RemoteDataReply * RemoteDataThread::copyFile(QString from, QString to)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "copyFile", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, from),
                              Q_ARG(QString, to));
    return retVal;
}

RemoteDataReply * RemoteDataThread::renameFile(QString fullName, QString newName)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "renameFile", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, fullName),
                              Q_ARG(QString, newName));
    return retVal;
}

RemoteDataReply * RemoteDataThread::mkRemoteDir(QString location, QString newName)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "mkRemoteDir", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, location),
                              Q_ARG(QString, newName));
    return retVal;
}

RemoteDataReply * RemoteDataThread::uploadFile(QString location, QString localFileName)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "uploadFile", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, location),
                              Q_ARG(QString, localFileName));
    return retVal;
}

RemoteDataReply * RemoteDataThread::uploadBuffer(QString location, QByteArray fileData, QString newFileName)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "uploadBuffer", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, location),
                              Q_ARG(QByteArray, fileData),
                              Q_ARG(QString, newFileName));
    return retVal;
}

RemoteDataReply * RemoteDataThread::downloadFile(QString localDest, QString remoteName)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "downloadFile", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, localDest),
                              Q_ARG(QString, remoteName));
    return retVal;
}

RemoteDataReply * RemoteDataThread::downloadBuffer(QString remoteName)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "downloadBuffer", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, remoteName));
    return retVal;
}

RemoteDataReply * RemoteDataThread::runRemoteJob(QString jobName, QMap<QString, QString> jobParameters, QString remoteWorkingDir, QString indivJobName)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "runRemoteJob", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, jobName),
                              Q_ARG(ParamMap, jobParameters),
                              Q_ARG(QString, remoteWorkingDir),
                              Q_ARG(QString, indivJobName));
    return retVal;
}

RemoteDataReply * RemoteDataThread::runRemoteJob(QJsonDocument rawJobJSON)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "runRemoteJob", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QJsonDocument, rawJobJSON));
    return retVal;
}

RemoteDataReply * RemoteDataThread::getListOfJobs()
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "getListOfJobs", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal));
    return retVal;
}

RemoteDataReply * RemoteDataThread::getJobDetails(QString IDstr)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "getJobDetails", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, IDstr));
    return retVal;
}

RemoteDataReply * RemoteDataThread::stopJob(QString IDstr)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    RemoteDataReply * retVal = NULL;
    QMetaObject::invokeMethod(myInterface, "stopJob", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(RemoteDataReply *, retVal),
                              Q_ARG(QString, IDstr));
    return retVal;
}

bool RemoteDataThread::rawOutputDebugEnabled()
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return NULL;
    bool retVal;
    QMetaObject::invokeMethod(myInterface, "rawOutputDebugEnabled", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, retVal));
    return retVal;
}

void RemoteDataThread::setRawDebugOutput(bool newSetting)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return;
    QMetaObject::invokeMethod(myInterface, "rawOutputDebugEnabled", Qt::BlockingQueuedConnection,
                              Q_ARG(bool, newSetting));
}
