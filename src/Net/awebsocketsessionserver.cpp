#include "awebsocketsessionserver.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QDebug>
#include <QNetworkInterface>
#include <QFile>

AWebSocketSessionServer::AWebSocketSessionServer(QObject *parent) :
    QObject(parent),
    server(new QWebSocketServer(QStringLiteral("ANTS2"), QWebSocketServer::NonSecureMode, this))
{
    connect(server, &QWebSocketServer::newConnection, this, &AWebSocketSessionServer::onNewConnection);
    connect(server, &QWebSocketServer::closed, this, &AWebSocketSessionServer::closed);
}

AWebSocketSessionServer::~AWebSocketSessionServer()
{
    server->close();
}

bool AWebSocketSessionServer::StartListen(quint16 port)
{
    //if ( !server->listen(QHostAddress::Any, port) )
    if ( !server->listen(QHostAddress::AnyIPv4, port) )
    {
        qCritical("WebSocket server was unable to starrt listen!");
        return false;
    }

    if (bDebug)
    {
        qDebug() << "ANTS2 is operating in the servermode";
        //qDebug() << "--Port:" << server->serverPort();
        qDebug() << "--URL:" << GetUrl();
    }

    emit reportToGUI("> Server started listening -> URL: "+GetUrl());

    return true;
}

void AWebSocketSessionServer::StopListen()
{
    server->close();
    emit reportToGUI("< Server stopped listening\n");
}

bool AWebSocketSessionServer::IsRunning()
{
    return server->isListening();
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
        client->sendTextMessage("##binary#file#");
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
        client->sendTextMessage("##binary#object#");
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
        client->sendTextMessage("##binary#JSON#");
    }
    else
    {
        qDebug() << "Reply from object failed";
        client->sendTextMessage("Error: ReplyWithBinaryObject_asJSON argument is not object");
    }
    bReplied = true;
}

void AWebSocketSessionServer::ReplyProgress(int percents)
{
    if ( !assureCanReply() ) return;

    QString s = QStringLiteral("##progress#") + QString::number(percents) + "#";
    qDebug() << "Sending progress: "<<s;

    client->sendTextMessage(s);
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
  //return server->serverUrl().toString();

    QString pre = "ws://";
    QString aft = QString(":") + QString::number(server->serverPort());

    QString local = QHostAddress(QHostAddress::LocalHost).toString();

    QString url = pre + local + aft;

    QString ip;
    for (const QHostAddress &address : QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
        {
            ip = address.toString();
            break;
        }
    }
    if ( !ip.isEmpty() )
        url += QStringLiteral("  or  ") + pre + ip + aft;

    return url;
}

int AWebSocketSessionServer::GetPort() const
{
    return server->serverPort();
}

void AWebSocketSessionServer::onProgressChanged(int percents)
{
    if (bRetranslateProgress) ReplyProgress(percents);
}
