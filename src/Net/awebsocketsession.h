#ifndef AWEBSOCKETSESSION_H
#define AWEBSOCKETSESSION_H

#include <QObject>
#include <QVariant>

class QWebSocket;

class AWebSocketSession : public QObject
{
    Q_OBJECT

public:
    AWebSocketSession();
    ~AWebSocketSession();

    void  setTimeout(int milliseconds) {timeout = milliseconds;}

    bool  connect(const QString& Url);
    void  disconnect();
    int   ping();
    bool  sendText(const QString& message);

    const QVariant& getAnswer() {return Answer;}
    const QString&  getError()  {return Error;}

    void  externalAbort() {fExternalAbort = true;}

public:
    enum  ServerState {Idle = 0, Connecting, WaitingForAnswer, ConnectionFailed, Connected};

private slots:
    void  onConnect();
    void  onDisconnect();
    void  onTextMessageReceived(const QString& message);

private:
    QWebSocket* socket = 0;
    int timeout = 3000;

    ServerState State = Idle;
    QString Error;
    bool bWaitForAnswer = false;
    int TimeMs = 0;
    bool fExternalAbort = false;

    QVariant Answer;
};

#endif // AWEBSOCKETSESSION_H
