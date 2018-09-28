#ifndef AWEBSOCKETSTANDALONEMESSANGER_H
#define AWEBSOCKETSTANDALONEMESSANGER_H

#include <QObject>

class QWebSocket;

class AWebSocketStandaloneMessanger : public QObject
{
    Q_OBJECT

public:
    AWebSocketStandaloneMessanger();
    ~AWebSocketStandaloneMessanger();

    void  setTimeout(int milliseconds) {timeout = milliseconds;}

    bool  sendTextMessage(const QString& Url, const QVariant &message, bool WaitForAnswer = false);
    int   ping(const QString &Url);

    const QString& getReceivedMessage() {return MessageReceived;}
    const QString& getError() {return Error;}

    void  externalAbort();

public:
    enum  ServerState {Idle = 0, Connecting, WaitingForAnswer, Aborted};

private slots:
    void  onConnect();
    void  onDisconnect();
    void  onTextMessageReceived(const QString& message);

private:
    QWebSocket* socket = 0;
    int timeout = 3000;

    ServerState State = Idle;
    QString MessageToSend;
    QString MessageReceived;
    QString Error;
    bool bWaitForAnswer = false;
    int TimeMs = 0;
    bool fExternalAbort = false;

    QString variantToString(const QVariant &val);
};

#endif // AWEBSOCKETSTANDALONEMESSANGER_H
