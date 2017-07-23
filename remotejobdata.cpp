#include "remotejobdata.h"

RemoteJobData::RemoteJobData()
{
    myID = "ERROR";
    myState = LongRunningState::INVALID;
}

QString RemoteJobData::getID()
{
    return myID;
}

void RemoteJobData::setJobID(QString newID)
{
    myID = newID;
}

LongRunningState RemoteJobData::getState()
{
    return myState;
}

void RemoteJobData::setState(LongRunningState newState)
{
    myState = newState;
}
