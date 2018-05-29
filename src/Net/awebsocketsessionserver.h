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
    //void ReplyWithJsonCompressed(const QJsonObject& message);

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
    void jsonMessageReceived(const QJsonObject& json);
    void clientDisconnected();
    void closed();

private:
    QWebSocketServer *server = 0;
    QWebSocket* client = 0;

    bool bDebug = true;
    bool bCompressBinary = false;
    int CompressionLevel = -1;

    QJsonObject JsonReceived;
};

#endif // AWEBSOCKETSESSIONSERVER_H
