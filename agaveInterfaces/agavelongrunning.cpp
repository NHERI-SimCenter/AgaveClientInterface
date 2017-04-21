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

void AgaveLongRunning::parseJSONdescription(QJsonDocument taskJSONdesc)
{
    myRawData = "";
    if (!taskJSONdesc.isObject())
    {
        myRawData = "Job Info Error";
        changeState(LongRunningState::ERROR);
        return;
    }
    QJsonObject jobData = taskJSONdesc.object();

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
