#ifndef AGAVELONGRUNNING_H
#define AGAVELONGRUNNING_H

#include "../remotedatainterface.h"

#include <QObject>
#include <QMultiMap>

#include <QJsonDocument>
#include <QJsonObject>

class AgaveHandler;

class AgaveLongRunning : public LongRunningTask
{
    Q_OBJECT

public:
    AgaveLongRunning(QMultiMap<QString, QString> * newParamList, AgaveHandler * manager);
    ~AgaveLongRunning();

    virtual void cancelTask();
    virtual void purgeTaskData();
    virtual LongRunningState getState();

    virtual QString getIDstr();
    virtual QString getRawDataStr();

    virtual QMultiMap<QString, QString> * getTaskParamList();

    void setIDstr(QString newID);
    void changeState(LongRunningState newState);
    void setRawDataStr(QString newRawData);
    void parseJSONdescription(QJsonDocument taskJSONdesc);

signals:
    void stateChange(LongRunningState oldState, LongRunningState newState);

private:
    AgaveHandler * myManager;
    QMultiMap<QString, QString> * taskParamList = NULL;

    QString myIDstr;
    QString myRawData;

    LongRunningState myState;
};

#endif // AGAVELONGRUNNING_H
