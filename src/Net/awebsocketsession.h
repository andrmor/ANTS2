#ifndef AWEBSOCKETSESSION_H
#define AWEBSOCKETSESSION_H

#include <QObject>
#include <QVariant>
#include <QByteArray>

class QWebSocket;
class QJsonObject;

class AWebSocketSession : public QObject
{
    Q_OBJECT

public:
    AWebSocketSession();
    ~AWebSocketSession();

    bool  connect(const QString& Url);
    void  disconnect();
    int   ping();
    bool  sendText(const QString& message);
    bool  sendJson(const QJsonObject& json);
    bool  sendFile(const QString& fileName);

    bool  resumeWaitForAnswer();

    void  clearReply();

    const QString&    getError() const {return Error;}
    const QString&    getTextReply() const {return TextReply;}
    const QByteArray& getBinaryReply() const {return BinaryReply;}
    bool              isBinaryReplyEmpty() const {return BinaryReply.isEmpty();}

    void  externalAbort();

    void  setTimeout(int milliseconds) {timeout = milliseconds;}

public:
    enum  ServerState {Idle = 0, Connecting, ConnectionFailed, Connected, Aborted};

private slots:
    void  onConnect();
    void  onDisconnect();
    void  onTextMessageReceived(const QString& message);
    void  onBinaryMessageReceived(const QByteArray &message);

private:
    QWebSocket* socket = 0;
    int timeout = 3000;

    ServerState State = Idle;
    QString Error;
    bool bWaitForAnswer = false;
    int TimeMs = 0;
    bool fExternalAbort = false;

    QString TextReply;
    QByteArray BinaryReply;

private:
    bool confirmSendPossible();
    bool waitForReply();
};

#endif // AWEBSOCKETSESSION_H
