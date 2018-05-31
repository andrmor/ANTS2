#include "awebsocketsessionserver.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QDebug>
#include <QNetworkInterface>
#include <QFile>

AWebSocketSessionServer::AWebSocketSessionServer(quint16 port, QObject *parent) :
    QObject(parent),
    server(new QWebSocketServer(QStringLiteral("ANTS2"), QWebSocketServer::NonSecureMode, this))
{
    //if (server->listen(QHostAddress::Any, port))
    if (server->listen(QHostAddress::AnyIPv4, port))
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

bool AWebSocketSessionServer::assureCanReply()
{
    if (client) return true;

    qDebug() << "AWebSocketSessionServer: cannot reply - connection not established";
    return false;
}

void AWebSocketSessionServer::ReplyWithText(const QString &message)
{
    if ( !assureCanReply() ) return;

    qDebug() << "Reply text message:"<<message;

    client->sendTextMessage(message);
    bReplied = true;
}

void AWebSocketSessionServer::ReplyWithBinaryFile(const QString &fileName)
{
    if ( !assureCanReply() ) return;

    qDebug() << "Binary reply from file:" << fileName;

    QFile file(fileName);
    if ( !file.open(QIODevice::ReadOnly) )
    {
        QString err = "Cannot open file: ";
        err += fileName;
        qDebug() << err;
        client->sendTextMessage(err);
    }
    else
    {
        QByteArray ba = file.readAll();
        client->sendBinaryMessage(ba);
    }
    bReplied = true;
}

void AWebSocketSessionServer::ReplyWithBinaryObject(const QVariant &object)
{
    if ( !assureCanReply() ) return;

    qDebug() << "Binary reply from object";

    if (object.type() == QMetaType::QVariantMap)
    {
        QVariantMap vm = object.toMap();
        QJsonObject js = QJsonObject::fromVariantMap(vm);
        QJsonDocument doc(js);
        client->sendBinaryMessage(doc.toBinaryData());
    }
    else
    {
        QString err = "Error: Reply with object failed";
        qDebug() << err;
        client->sendTextMessage("Error: ReplyWithBinaryObject argument is not object");
    }
    bReplied = true;
}

void AWebSocketSessionServer::ReplyWithBinaryObject_asJSON(const QVariant &object)
{
    if ( !assureCanReply() ) return;

    qDebug() << "Binary reply from object as JSON";

    if (object.type() == QMetaType::QVariantMap)
    {
        QVariantMap vm = object.toMap();
        QJsonObject js = QJsonObject::fromVariantMap(vm);
        QJsonDocument doc(js);
        client->sendBinaryMessage(doc.toJson());
    }
    else
    {
        qDebug() << "Reply from object failed";
        client->sendTextMessage("Error: ReplyWithBinaryObject_asJSON argument is not object");
    }
    bReplied = true;
}

void AWebSocketSessionServer::onNewConnection()
{
    if (bDebug) qDebug() << "New connection attempt";
    QWebSocket *pSocket = server->nextPendingConnection();

    emit reportToGUI(QString("--- Connection request from ") + pSocket->peerAddress().toString());

    if (client)
    {
        //deny - exclusive connections!
        if (bDebug) qDebug() << "Connection denied: another client is already connected";
        emit reportToGUI("--X Denied: another session is already active");
        pSocket->sendTextMessage("Error: another client is already connected");
        pSocket->close();
    }
    else
    {
        if (bDebug) qDebug() << "Connection established with" << pSocket->peerAddress().toString();
        client = pSocket;

        emit reportToGUI("--> Connection established");

        connect(pSocket, &QWebSocket::textMessageReceived, this, &AWebSocketSessionServer::onTextMessageReceived);
        connect(pSocket, &QWebSocket::binaryMessageReceived, this, &AWebSocketSessionServer::onBinaryMessageReceived);
        connect(pSocket, &QWebSocket::disconnected, this, &AWebSocketSessionServer::onSocketDisconnected);

        if (client) client->sendTextMessage("OK: Connection established");
    }
}

void AWebSocketSessionServer::onTextMessageReceived(const QString &message)
{    
    bReplied = false;

    if (bDebug) qDebug() << "Text message received:\n--->\n"<<message << "\n<---";

    //emit reportToGUI("    Text message received");

    if (message.isEmpty())
    {
        if (client)
            client->sendTextMessage("PONG"); //used for watchdogs
    }
    else
       emit textMessageReceived(message);
}

void AWebSocketSessionServer::onBinaryMessageReceived(const QByteArray &message)
{
    ReceivedBinary = message;

    //emit reportToGUI("    Binary message received");

    if (bDebug) qDebug() << "Binary message received. Length =" << message.length();

    if (client)
        client->sendTextMessage("OK");
}

void AWebSocketSessionServer::onSocketDisconnected()
{
    if (bDebug) qDebug() << "Client disconnected!";

    emit reportToGUI("<-- Client disconnected");

    //if (client) client->deleteLater();
    if (client) delete client;
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
