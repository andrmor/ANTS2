#ifndef AWEBSOCKETSESSIONSERVER_H
#define AWEBSOCKETSESSIONSERVER_H

#include <QObject>
#include <QByteArray>
#include <QJsonObject>

class QWebSocketServer;
class QWebSocket;

class AWebSocketSessionServer : public QObject
{
    Q_OBJECT
public:
    explicit AWebSocketSessionServer(quint16 port, QObject *parent = 0);
    ~AWebSocketSessionServer();

    void ReplyWithText(const QString& message);
    void ReplyWithBinaryFile(const QString& fileName);
    void ReplyWithBinaryObject(const QVariant& object);
    void ReplyWithBinaryObject_asJSON(const QVariant& object);

    bool isReplied() const {return bReplied;}
    bool isBinaryEmpty() const {return ReceivedBinary.isEmpty();}
    void clearBinary() {ReceivedBinary.clear();}
    const QByteArray& getBinary() const {return ReceivedBinary;}

    const QString GetUrl() const;
    int GetPort() const;
    void PauseAccepting(bool flag);

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);
    void onSocketDisconnected();

signals:
    void textMessageReceived(const QString &message);
    void clientDisconnected();
    void closed();
    void reportToGUI(const QString& text);

private:
    QWebSocketServer *server = 0;
    QWebSocket* client = 0;

    bool bDebug = true;
    //bool bCompressBinary = false;
    //int CompressionLevel = -1;

    QByteArray ReceivedBinary;

    bool bReplied = false;

private:
    bool assureCanReply();
};

#endif // AWEBSOCKETSESSIONSERVER_H
