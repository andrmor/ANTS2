#ifndef AGRIDRUNNER_H
#define AGRIDRUNNER_H

#include <QObject>
#include <QHostAddress>

class ARemoteServerRecord;
class AWebSocketSession;
class AWebSocketWorker;

class AGridRunner : public QObject
{
    Q_OBJECT
public:
    void CheckStatus(QVector<ARemoteServerRecord *> &Servers);

public slots:
    void onRequestTextLog(int index, const QString message);

private:
    int TimeOut = 5000;
    QVector<AWebSocketSession*> Sockets;

private:
    AWebSocketWorker* startCheckStatusOfServer(int index, ARemoteServerRecord *Server);

signals:
    void requestTextLog(int index, const QString message);
    void requestGuiUpdate();

};

class AWebSocketWorker : public QObject
{
    Q_OBJECT
public:
    AWebSocketWorker(int index, ARemoteServerRecord* rec, int timeOut);

    bool isRunning() const {return bRunning;}
    void setStarted() {bRunning = true;}

private:
    int index;
    ARemoteServerRecord* rec;
    int TimeOut = 5000;

    bool bRunning = false;
    int  timerInterval = 250; //ms
    int  timeElapsed = 0;

private slots:
    void onTimer();

public slots:
    void checkStatus();

signals:
    void finished();
    void requestTextLog(int index, const QString message);
};

#endif // AGRIDRUNNER_H
