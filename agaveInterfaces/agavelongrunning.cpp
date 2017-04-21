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

#include "agavelongrunning.h"

#include "agavehandler.h"

AgaveLongRunning::AgaveLongRunning(QMultiMap<QString, QString> * newParamList, AgaveHandler * manager) : LongRunningTask(manager)
{
    myIDstr = "";
    myState = LongRunningState::INIT;
    myManager = manager;
    taskParamList = newParamList;
}

AgaveLongRunning::~AgaveLongRunning()
{
    if (taskParamList != NULL)
    {
        delete taskParamList;
    }
}

void AgaveLongRunning::cancelTask()
{
    myManager->stopLongRunnging(this);
}

void AgaveLongRunning::purgeTaskData()
{
    myManager->purgeLongRunning(this);
}

LongRunningState AgaveLongRunning::getState()
{
    return myState;
}

void AgaveLongRunning::setIDstr(QString newID)
{
    if (myState != LongRunningState::INIT)
    {
        return;
    }
    myIDstr = newID;
    changeState(LongRunningState::PENDING);
}

QString AgaveLongRunning::getIDstr()
{
    return myIDstr;
}

QString AgaveLongRunning::getRawDataStr()
{
    return myRawData;
}

QMultiMap<QString, QString> * AgaveLongRunning::getTaskParamList()
{
    return taskParamList;
}

void AgaveLongRunning::changeState(LongRunningState newState)
{
    LongRunningState oldState = myState;
    myState = newState;
    emit stateChange(oldState, newState);
}

void AgaveLongRunning::setRawDataStr(QString newRawData)
{
    myRawData = newRawData;
    //Note: updating of data members, if any, will go here
}

void AgaveLongRunning::parseJSONdescription(QJsonObject jobData)
{
    if (!jobData.contains("id") || !jobData.contains("appId") || !jobData.contains("status"))
    {
        myRawData = "Job Info Error";
        changeState(LongRunningState::ERROR);
        return;
    }

    myRawData = myRawData.append(jobData.value("id").toString());
    myRawData = myRawData.append(" - ");
    myRawData = myRawData.append(jobData.value("appId").toString());
    myRawData = myRawData.append(" - ");
    myRawData = myRawData.append(jobData.value("status").toString());

    //TODO: fill out this listing of state changes
    if (jobData.value("status").toString() == "FINISHED")
    {
        changeState(LongRunningState::DONE);
    }
}
