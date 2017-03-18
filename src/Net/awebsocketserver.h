#ifndef AWEBSOCKETSERVER_H
#define AWEBSOCKETSERVER_H

#include <QObject>
#include <QList>
#include <QByteArray>

class QWebSocketServer;
class QWebSocket;

class AWebSocketServer : public QObject
{
    Q_OBJECT
public:
    explicit AWebSocketServer(quint16 port, bool debug = false, QObject *parent = 0);
    ~AWebSocketServer();

    void ReplyAndClose(QString message);

    QString GetUrl();
    int GetPort();
    void PauseAccepting(bool flag);

signals:
    void closed();
    void NotifyTextMessageReceived(QString message);

private slots:
    void onNewConnection();
    void processTextMessage(QString message);
    void processBinaryMessage(QByteArray message);
    void socketDisconnected();

private:
    QWebSocketServer *server;
    QList<QWebSocket *> clients;
    QWebSocket* replyClient;
    bool fDebug;
};

#endif // AWEBSOCKETSERVER_H
