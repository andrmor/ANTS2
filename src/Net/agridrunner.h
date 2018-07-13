#ifndef AGRIDRUNNER_H
#define AGRIDRUNNER_H

#include <QObject>
#include <QJsonObject>
#include <QHostAddress>

class EventsDataClass;
class APmHub;
class ARemoteServerRecord;
class AWebSocketSession;
class AWebSocketWorker_Base;
class QJsonObject;

class AGridRunner : public QObject
{
    Q_OBJECT
public:
    AGridRunner(EventsDataClass* EventsDataHub, APmHub *PMs);

    void CheckStatus(QVector<ARemoteServerRecord *> &Servers);
    const QString Simulate(QVector<ARemoteServerRecord *> &Servers, const QJsonObject* config);
    void Reconstruct(QVector<ARemoteServerRecord *> &Servers, const QJsonObject* config);
    void RateServers(QVector<ARemoteServerRecord *> &Servers, const QJsonObject* config);

    void SetTimeout(int timeout) {TimeOut = timeout;}

public slots:
    void onRequestTextLog(int index, const QString message);

private:
    EventsDataClass* EventsDataHub;
    APmHub *PMs;
    int TimeOut = 5000;
    QVector<AWebSocketSession*> Sockets;

private:
    AWebSocketWorker_Base* startCheckStatusOfServer(int index, ARemoteServerRecord *serverRecord);
    AWebSocketWorker_Base* startSim(int index, ARemoteServerRecord *serverRecord, const QJsonObject* config);
    AWebSocketWorker_Base* startRec(int index, ARemoteServerRecord *server, const QJsonObject* config);

    void startInNewThread(AWebSocketWorker_Base *worker);

    void waitForWorkersToFinish(QVector<AWebSocketWorker_Base *> &workers);
    void waitForWorkersToPauseOrFinish(QVector<AWebSocketWorker_Base *> &workers);

    void regularToCustomNodes(const QJsonObject & RegularScanOptions, QJsonArray & toArray);

signals:
    void requestTextLog(int index, const QString message);
    void requestStatusLog(const QString message);
    void requestDelegateGuiUpdate();

};


class AWebSocketWorker_Base : public QObject
{
    Q_OBJECT
public:
    AWebSocketWorker_Base(int index, ARemoteServerRecord* rec, int timeOut,  const QJsonObject* config = 0);
    virtual ~AWebSocketWorker_Base() {}

    bool isRunning() const {return bRunning;}
    bool isPausedOrFinished() const {return bPaused || !bRunning;}
    void setStarted() {bRunning = true;}
    void setPaused(bool flag) {bPaused = flag;}

    void setExtraScript(const QString& script) {extraScript = script;}

    ARemoteServerRecord* getRecord() {return rec;}

public slots:
    virtual void run() = 0;

protected:
    int index;
    ARemoteServerRecord* rec;
    int TimeOut = 5000;

    bool bRunning = false;
    bool bPaused = false;
    bool bExternalAbort = false;

    const QJsonObject* config;
    AWebSocketSession* ants2socket = 0;

    QString extraScript; //e.g. script to modify config according to distribution of sim events

    AWebSocketSession* connectToServer(int port);
    bool               allocateAntsServer();
    AWebSocketSession* connectToAntsServer();
    bool               establishSession();

signals:
    void finished();
    void requestTextLog(int index, const QString message);
};

class AWebSocketWorker_Check : public AWebSocketWorker_Base
{
    Q_OBJECT
public:
    AWebSocketWorker_Check(int index, ARemoteServerRecord* rec, int timeOut);

public slots:
    virtual void run() override;

private:
    int  timerInterval = 250; //ms
    int  timeElapsed = 0;

private slots:
    void onTimer();

};

class AWebSocketWorker_Sim : public AWebSocketWorker_Base
{
    Q_OBJECT
public:
    AWebSocketWorker_Sim(int index, ARemoteServerRecord* rec, int timeOut, const QJsonObject* config);

public slots:
    virtual void run() override;

private:    
    void runSimulation();
};

class AWebSocketWorker_Rec : public AWebSocketWorker_Base
{
    Q_OBJECT
public:
    AWebSocketWorker_Rec(int index, ARemoteServerRecord* rec, int timeOut, const QJsonObject* config);

public slots:
    virtual void run() override;

private:
    void runReconstruction();
};

#endif // AGRIDRUNNER_H
