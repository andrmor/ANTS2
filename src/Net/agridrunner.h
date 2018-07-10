#ifndef AGRIDRUNNER_H
#define AGRIDRUNNER_H

#include <QObject>
#include <QHostAddress>

class ARemoteServerRecord;
class AWebSocketSession;
class AWebSocketWorker_Base;
class QJsonObject;

class AGridRunner : public QObject
{
    Q_OBJECT
public:
    void CheckStatus(QVector<ARemoteServerRecord *> &Servers);
    void Simulate(QVector<ARemoteServerRecord *> &Servers, const QJsonObject* config);

public slots:
    void onRequestTextLog(int index, const QString message);

private:
    int TimeOut = 5000;
    QVector<AWebSocketSession*> Sockets;

private:
    AWebSocketWorker_Base* startCheckStatusOfServer(int index, ARemoteServerRecord *server);
    AWebSocketWorker_Base* startSimulationOnServer(int index, ARemoteServerRecord *server, const QJsonObject* config);

    void startInNewThread(AWebSocketWorker_Base *worker);
    void waitForWorkersToFinish(QVector<AWebSocketWorker_Base *> &workers);

signals:
    void requestTextLog(int index, const QString message);
    void requestGuiUpdate();

};


class AWebSocketWorker_Base : public QObject
{
    Q_OBJECT
public:
    AWebSocketWorker_Base(int index, ARemoteServerRecord* rec, int timeOut);
    virtual ~AWebSocketWorker_Base() {}

    bool isRunning() const {return bRunning;}
    void setStarted() {bRunning = true;}

protected:
    int index;
    ARemoteServerRecord* rec;
    int TimeOut = 5000;

    bool bRunning = false;

    AWebSocketSession* connectToServer(int port);
    bool               allocateAntsServer();

public slots:
    virtual void run() = 0;

signals:
    void finished();
    void requestTextLog(int index, const QString message);
};

class AWebSocketWorker_Check : public AWebSocketWorker_Base
{
    Q_OBJECT
public:
    AWebSocketWorker_Check(int index, ARemoteServerRecord* rec, int timeOut);

private:
    int  timerInterval = 250; //ms
    int  timeElapsed = 0;

private slots:
    void onTimer();

public slots:
    virtual void run() override;
};

class AWebSocketWorker_Sim : public AWebSocketWorker_Base
{
    Q_OBJECT
public:
    AWebSocketWorker_Sim(int index, ARemoteServerRecord* rec, int timeOut, const QJsonObject* config);

private:
    const QJsonObject* config;

public slots:
    virtual void run() override;
};



#endif // AGRIDRUNNER_H
