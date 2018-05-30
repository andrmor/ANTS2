#ifndef AINTERFACETOWEBSOCKET_H
#define AINTERFACETOWEBSOCKET_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVariant>

class QWebSocketServer;
class QWebSocket;
class AWebSocketStandaloneMessanger;
class AWebSocketSession;

class AInterfaceToWebSocket: public AScriptInterface
{
  Q_OBJECT

public:
    AInterfaceToWebSocket();
    ~AInterfaceToWebSocket();

public slots:    
    //standalone - no persistent connection
    const QString  SendTextMessage(const QString& Url, const QVariant &message, bool WaitForAnswer = false);
    int            Ping(const QString& Url);

    //with persistent connection
    void           Connect(const QString& Url);
    void           Disconnect();
    const QString  SendText(const QString& message);
    const QString  SendObject(const QVariant& object);
    const QString  SendFile(const QString& fileName);
    const QVariant GetBinaryReplyAsObject() const;
    bool           SaveBinaryReplyToFile(const QString& fileName);

    /*
    //server side
    void           ServerReplyText(const QString& message);
    void           ServerReplyFile(const QString& fileName);
    void           ServerReplyObject(const QVariant& object);
    */

    //misc
    void           SetTimeout(int milliseconds);

private:
    AWebSocketStandaloneMessanger* standaloneMessenger;
    AWebSocketSession* sessionMessenger;
};

#endif // AINTERFACETOWEBSOCKET_H
