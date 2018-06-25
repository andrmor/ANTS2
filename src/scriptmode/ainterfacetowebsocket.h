#ifndef AINTERFACETOWEBSOCKET_H
#define AINTERFACETOWEBSOCKET_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVariant>
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
    const QString  Connect(const QString& Url, bool GetAnswerOnConnection);
    void           Disconnect();

    const QString  OpenSession(const QString& IP, int port, int threads, bool ShowOutput = true);
    bool           SendConfig(QVariant config);
    bool           RemoteSimulatePhotonSources(int NumThreads, const QString& LocalSimTreeFileName, bool ShowOutput);

    const QString  SendText(const QString& message);
    const QString  SendTicket(const QString& ticket);
    const QString  SendObject(const QVariant& object);
    const QString  SendFile(const QString& fileName);

    const QString  ResumeWaitForAnswer();

    const QVariant GetBinaryReplyAsObject();
    bool           SaveBinaryReplyToFile(const QString& fileName);

    void           SetTimeout(int milliseconds);

    //compatibility mode -> standalone - no persistent connection
    const QString  SendTextMessage(const QString& Url, const QVariant &message, bool WaitForAnswer = false);
    int            Ping(const QString& Url);

signals:
    void showTextOnMessageWindow(const QString& text);
    void clearTextOnMessageWindow();

private:
    AWebSocketStandaloneMessanger* compatibilitySocket = 0;
    AWebSocketSession* socket = 0;

    int TimeOut = 5000; //milliseconds
};

#endif // AINTERFACETOWEBSOCKET_H
