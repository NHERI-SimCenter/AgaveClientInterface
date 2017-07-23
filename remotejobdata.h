#ifndef REMOTEJOBDATA_H
#define REMOTEJOBDATA_H

#include <QObject>
#include <QString>

enum class LongRunningState {INIT, PENDING, RUNNING, DONE, ERROR, PURGING, INVALID}; //Add more if needed

class RemoteJobData
{
public:
    RemoteJobData();

    QString getID();
    void setJobID(QString newID);
    LongRunningState getState();
    void setState(LongRunningState newState);

private:
    QString myID;
    LongRunningState myState;
};

#endif // REMOTEJOBDATA_H
