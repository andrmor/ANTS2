#include "awebsocketsessionserver.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QDebug>
#include <QNetworkInterface>

AWebSocketSessionServer::AWebSocketSessionServer(quint16 port, QObject *parent) :
    QObject(parent),
    server(new QWebSocketServer(QStringLiteral("ANTS2"), QWebSocketServer::NonSecureMode, this))
{
    if (server->listen(QHostAddress::Any, port))
    {
        if (bDebug)
          {
            qDebug() << "ANTS2 is operating in the servermode";
            //qDebug() << "--Address:"<<server->serverAddress();
            qDebug() << "--Port:"<<server->serverPort();
            qDebug() << "--URL:" << server->serverUrl().toString();

//            foreach (const QHostAddress &address, QNetworkInterface::allAddresses())
//            {
//                if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
//                     qDebug() << address.toString();
//            }

          }
        connect(server, &QWebSocketServer::newConnection, this, &AWebSocketSessionServer::onNewConnection);
        connect(server, &QWebSocketServer::closed, this, &AWebSocketSessionServer::closed);
    }
}

AWebSocketSessionServer::~AWebSocketSessionServer()
{
    server->close();
}

void AWebSocketSessionServer::ReplyWithText(const QString &message)
{
    if (client)
    {
        qDebug() << "Reply message:"<<message;
        client->sendTextMessage(message);
    }
    else
        qDebug() << "AWebSocketSessionServer: cannot reply - connection not established";
}

void AWebSocketSessionServer::onNewConnection()
{
    if (bDebug) qDebug() << "New connection attempt";
    QWebSocket *pSocket = server->nextPendingConnection();

    if (client)
    {
        //deny - exclusive connections!
        if (bDebug) qDebug() << "Connection denied: another client is already connected";
        pSocket->sendTextMessage("Error: another client is already connected");
        pSocket->close();
    }
    else
    {
        if (bDebug) qDebug() << "Connection established with" << pSocket->peerAddress().toString();
        client = pSocket;


        connect(pSocket, &QWebSocket::textMessageReceived, this, &AWebSocketSessionServer::onTextMessageReceived);
        connect(pSocket, &QWebSocket::binaryMessageReceived, this, &AWebSocketSessionServer::onBinaryMessageReceived);
        connect(pSocket, &QWebSocket::disconnected, this, &AWebSocketSessionServer::onSocketDisconnected);

        //if (client) client->sendTextMessage("OK: Connection established");
    }
}

void AWebSocketSessionServer::onTextMessageReceived(const QString &message)
{
    JsonReceived = QJsonObject();

    if (bDebug) qDebug() << "Text message received:\n--->\n"<<message << "\n<---";

    if (message.isEmpty())
    {
        if (client)
            client->sendTextMessage("PING"); //used for watchdogs
    }
    else
       emit textMessageReceived(message);
}

void AWebSocketSessionServer::onBinaryMessageReceived(const QByteArray &message)
{
    JsonReceived = QJsonObject();

    // bCompressBinary
    // ***!!!

    QJsonDocument itemDoc = QJsonDocument::fromJson(message);
    JsonReceived = itemDoc.object();

    if (bDebug) qDebug() << "Json message received. Json contains" << JsonReceived.count() << "properties";

    if (JsonReceived.isEmpty())
        client->sendTextMessage("Server: Empty json received"); //used for pinging
    else
        emit jsonMessageReceived(JsonReceived);
}

void AWebSocketSessionServer::onSocketDisconnected()
{
    if (bDebug) qDebug() << "Client disconnected!";

    if (client) client->deleteLater();
    client = 0;

    emit clientDisconnected();
}

const QString AWebSocketSessionServer::GetUrl() const
{
  return server->serverUrl().toString();
}

int AWebSocketSessionServer::GetPort() const
{
  return server->serverPort();
}
