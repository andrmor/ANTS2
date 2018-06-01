#ifndef AINTERFACETOWEBSOCKET_H
#define AINTERFACETOWEBSOCKET_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVariant>
#include <QThread>
#include <QMap>

class QWebSocketServer;
class QWebSocket;
class AWebSocketStandaloneMessanger;
class AWebSocketSession;

class AInterfaceToWebSocket: public AScriptInterface
{
  Q_OBJECT

public:
    AInterfaceToWebSocket();
    AInterfaceToWebSocket(const AInterfaceToWebSocket& other);
    ~AInterfaceToWebSocket();

    virtual bool IsMultithreadCapable() const {return true;}
    virtual void ForceStop();

public slots:    
    void           Connect(const QString& Url);
    void           Disconnect();

    const QString  SendText(const QString& message);
    const QString  SendObject(const QVariant& object);
    const QString  SendFile(const QString& fileName);

    const QVariant GetBinaryReplyAsObject();
    bool           SaveBinaryReplyToFile(const QString& fileName);

    void           SetTimeout(int milliseconds);

    //compatibility mode -> standalone - no persistent connection
    const QString  SendTextMessage(const QString& Url, const QVariant &message, bool WaitForAnswer = false);
    int            Ping(const QString& Url);

private:
    AWebSocketStandaloneMessanger* compatibilitySocket;
    AWebSocketSession* socket;

    //QMap<QThread*, AWebSocketSession*> sockets;

private:
    //void initSocket();
    //AWebSocketSession* getSocket() const;
};

#endif // AINTERFACETOWEBSOCKET_H
