#ifndef AINTERFACETOWEBSOCKET_H
#define AINTERFACETOWEBSOCKET_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVariant>

class QWebSocketServer;
class QWebSocket;

class AInterfaceToWebSocket: public AScriptInterface
{
  Q_OBJECT

public:
    enum ServerState {Idle=0, Sending, WaitingForAnswer,
                      TryingToConnect, Connected, ConnectionFailed};
    AInterfaceToWebSocket();
    ~AInterfaceToWebSocket();

public slots:
    void setTimeout(int milliseconds) {timeout = milliseconds;}

    //standalone - no persistent connection
    QString SendTextMessage(const QString& Url, const QVariant &message, bool WaitForAnswer=false);
    int Ping(QString Url);

    //with persistent connection
    void Connect(const QString& Url);
    void Disconnect();
    const QVariant SendText(const QString& message, bool WaitForAnswer=false);
    //const QVariant SendJson(const QString& message, const QVariant& json, bool WaitForAnswer=false);

private slots:
    void onClientConnected();
    void onClientDisconnected();
    void onTextMessageReceived(QString message);

private:
    QWebSocket *ClientSocket = 0;
    bool bConnected = false;

    int timeout = 3000;
    int lastExchangeDuration;

    ServerState State = Idle;
    QString MessageToSend;
    QString MessageReceived;
    bool bWaitForAnswer;
    bool bServerWasBusy = false;

    QString variantToString(const QVariant &val);
};

#endif // AINTERFACETOWEBSOCKET_H
