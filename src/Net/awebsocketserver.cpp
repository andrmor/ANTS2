#include "awebsocketserver.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QDebug>

AWebSocketServer::AWebSocketServer(quint16 port, bool debug, QObject *parent) :
    QObject(parent),
    server(new QWebSocketServer(QStringLiteral("Echo Server"), QWebSocketServer::NonSecureMode, this)),
    clients(), replyClient(0),
    fDebug(debug)
{
    if (server->listen(QHostAddress::Any, port))
    {
        if (fDebug)
          {
            qDebug() << "ANTS2 web socket server is now listening";
            qDebug() << "--Address:"<<server->serverAddress();
            qDebug() << "--Port:"<<server->serverPort();
            qDebug() << "--URL:" << server->serverUrl().toString();

          }
        connect(server, &QWebSocketServer::newConnection, this, &AWebSocketServer::onNewConnection);
        connect(server, &QWebSocketServer::closed, this, &AWebSocketServer::closed);
    }
}

AWebSocketServer::~AWebSocketServer()
{
    server->close();
    qDeleteAll(clients.begin(), clients.end());
}

void AWebSocketServer::ReplyAndClose(QString message)
{
    qDebug() << "Reply and close request";
    qDebug() << "Message:"<<message<<"client:"<<replyClient;

    if (replyClient)
    {
        replyClient->sendTextMessage(message);
        replyClient->close();
        replyClient = 0;
    }
}

QString AWebSocketServer::GetUrl()
{
  return server->serverUrl().toString();
}

int AWebSocketServer::GetPort()
{
  return server->serverPort();
}

void AWebSocketServer::onNewConnection()
{
    if (fDebug) qDebug() << "New connection";
    QWebSocket *pSocket = server->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &AWebSocketServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &AWebSocketServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &AWebSocketServer::socketDisconnected);

    clients << pSocket;
}

void AWebSocketServer::processTextMessage(QString message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (fDebug) qDebug() << "Server - message received:" << message << "client:"<<pClient;
    replyClient = pClient;

    emit NotifyTextMessageReceived(message);

    //if (pClient) pClient->sendTextMessage("Echo:"+message);
//    if (pClient)
//    {
//        pClient->sendTextMessage("UpdateGeometry");
//        pClient->close();
//    }

}

void AWebSocketServer::processBinaryMessage(QByteArray message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (fDebug) qDebug() << "Binary Message received:" << message;

    //if (pClient) pClient->sendBinaryMessage(message);
}

void AWebSocketServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (fDebug) qDebug() << "socketDisconnected:" << pClient;

    if (pClient)
    {
        clients.removeAll(pClient);
        pClient->deleteLater();
    }

    replyClient = 0;
}

void AWebSocketServer::PauseAccepting(bool flag)
{
  if (flag) server->pauseAccepting();
  else server->resumeAccepting();
}

