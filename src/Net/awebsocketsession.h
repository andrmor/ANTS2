#ifndef AWEBSOCKETSESSION_H
#define AWEBSOCKETSESSION_H

#include <QObject>
#include <QVariant>
#include <QByteArray>
#include <QAbstractSocket>

class QWebSocket;
class QJsonObject;

class AWebSocketSession : public QObject
{
    Q_OBJECT

public:
    AWebSocketSession();
    ~AWebSocketSession();

    bool  Connect(const QString& Url, bool WaitForAnswer = true);
    void  Disconnect();
    int   Ping();
    bool  SendText(const QString& message);
    bool  SendJson(const QJsonObject& json);
    bool  SendQByteArray(const QByteArray& ba);
    bool  SendFile(const QString& fileName);

    bool  ResumeWaitForAnswer();

    void  ClearReply();

    const QString&    GetError() const {return Error;}
    const QString&    GetTextReply() const {return TextReply;}
    const QByteArray& GetBinaryReply() const {return BinaryReply;}
    bool              IsBinaryReplyEmpty() const {return BinaryReply.isEmpty();}

    void  ExternalAbort();

    void  SetTimeout(int milliseconds) {timeout = milliseconds;}
    void  SetIntervalBetweenEventProcessing(int milliseconds) {sleepDuration = milliseconds;}
    quint16 GetPeerPort() {return peerPort;} //used as part of the unique names for remote files

    bool ConfirmSendPossible();

public:
    enum  ServerState {Idle = 0, Connecting, ConnectionFailed, Connected, Aborted};

private slots:
    void  onConnect();
    void  onDisconnect();
    void  onTextMessageReceived(const QString& message);
    void  onBinaryMessageReceived(const QByteArray &message);

    //void  onStateChanged(QAbstractSocket::SocketState state);

private:
    QWebSocket* socket = 0;
    int timeout = 3000;
    int timeoutForDisconnect = 3000;
    ulong sleepDuration = 50;

    ServerState State = Idle;
    QString Error;
    bool bWaitForAnswer = false;
    int TimeMs = 0;
    bool fExternalAbort = false;

    QString TextReply;
    QByteArray BinaryReply;

    quint16 peerPort = 0;

private:    
    bool waitForReply();
};

#endif // AWEBSOCKETSESSION_H
