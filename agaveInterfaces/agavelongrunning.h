#ifndef AGAVELONGRUNNING_H
#define AGAVELONGRUNNING_H

#include "../remotedatainterface.h"

#include <QObject>

class AgaveHandler;

class AgaveLongRunning : public LongRunningTask
{
    Q_OBJECT

public:
    AgaveLongRunning(QString taskID, AgaveHandler * manager);

    virtual void cancelTask();
    virtual LongRunningState getState();
    virtual QString getIDstr();

    virtual QString getRawDataStr();

    void changeState(LongRunningState newState);
    void setRawDataStr(QString);

signals:
    void stateChange(LongRunningState oldState, LongRunningState newState);

private:
    virtual void deleteSelf();

    AgaveHandler * myManager;

    QString myIDstr;
    QString myRawData;

    LongRunningState myState;
};

#endif // AGAVELONGRUNNING_H
