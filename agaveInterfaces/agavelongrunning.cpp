#include "agavelongrunning.h"

#include "agavehandler.h"

AgaveLongRunning::AgaveLongRunning(QString taskID, AgaveHandler * manager) : LongRunningTask(manager)
{
    taskID = myIDstr;
    myState = LongRunningState::PENDING;
    myManager = manager;
}

void AgaveLongRunning::cancelTask()
{
    myManager->stopLongRunnging(this);
}

LongRunningState AgaveLongRunning::getState()
{
    return myState;
}

QString AgaveLongRunning::getIDstr()
{
    return myIDstr;
}

QString AgaveLongRunning::getRawDataStr()
{
    return myRawData;
}

void AgaveLongRunning::deleteSelf()
{
    cancelTask();
    myManager->purgeLongRunning(this);
    this->deleteLater();
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
