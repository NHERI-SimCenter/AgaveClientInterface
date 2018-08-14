/*********************************************************************************
**
** Copyright (c) 2018 The University of Notre Dame
** Copyright (c) 2018 The Regents of the University of California
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

#include "agavethread.h"
#include "agavehandler.h"

AgaveThread::AgaveThread(QObject *parent) : RemoteDataThread(parent) {}

void AgaveThread::registerAgaveAppInfo(QString agaveAppName, QString fullAgaveName, QStringList parameterList, QStringList inputList, QString workingDirParameter)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return;
    QMetaObject::invokeMethod(myInterface, "registerAgaveAppInfo", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, agaveAppName),
                              Q_ARG(QString, fullAgaveName),
                              Q_ARG(QStringList, parameterList),
                              Q_ARG(QStringList, inputList),
                              Q_ARG(QString, workingDirParameter));
}

void AgaveThread::setAgaveConnectionParams(QString tenant, QString clientId, QString storage)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return;
    QMetaObject::invokeMethod(myInterface, "setAgaveConnectionParams", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, tenant),
                              Q_ARG(QString, clientId),
                              Q_ARG(QString, storage));
}

AgaveTaskReply * AgaveThread::getAgaveAppList()
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return nullptr;
    AgaveTaskReply * retVal = nullptr;
    QMetaObject::invokeMethod(myInterface, "getAgaveAppList", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(AgaveTaskReply *, retVal));
    return retVal;
}

AgaveTaskReply * AgaveThread::runAgaveJob(QJsonDocument rawJobJSON)
{
    QMutexLocker lock(&readyLock);

    if (!remoteThreadReady()) return nullptr;
    AgaveTaskReply * retVal = nullptr;
    QMetaObject::invokeMethod(myInterface, "runAgaveJob", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(AgaveTaskReply *, retVal),
                              Q_ARG(QJsonDocument, rawJobJSON));
    return retVal;
}

void AgaveThread::run()
{
    QNetworkAccessManager theNetManager;

    AgaveHandler theInterface(&theNetManager);
    myInterface = &theInterface;

    RemoteDataThread::run();

    myInterface = nullptr;
}
