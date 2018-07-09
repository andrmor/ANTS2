#ifndef AGRIDRUNNER_H
#define AGRIDRUNNER_H

#include <QObject>
#include <QHostAddress>

class AWebSocketSession;

class ARemoteServerRecord
{
public:
    ARemoteServerRecord(QString IP, int Port) : IP(IP), Port(Port) {}
    ARemoteServerRecord() {}

    QString IP;
    int     Port;

    int     NumThreads;
    QString error;
};

class AGridRunner : public QObject
{
    Q_OBJECT
public:
    void CheckStatus(QVector<ARemoteServerRecord> &Servers);

public slots:
    void onRequestTextLog(int index, const QString message);

private:
    int TimeOut = 5000;
    QVector<AWebSocketSession*> Sockets;

private:
    void startCheckStatusOfServer(int index, ARemoteServerRecord& Server);

signals:
    void requestTextLog(int index, const QString message);

};

class AWebSocketWorker : public QObject
{
    Q_OBJECT
public:
    AWebSocketWorker(int index, ARemoteServerRecord& rec, int timeOut);

    bool isRunning() const {return bRunning;}
    void setStarted() {bRunning = true;}

private:
    int index;
    ARemoteServerRecord& rec;
    int TimeOut = 3000;

    bool bRunning = false;

public slots:
    void checkStatus();

signals:
    void finished();
    void requestTextLog(int index, const QString message);
};

#endif // AGRIDRUNNER_H
