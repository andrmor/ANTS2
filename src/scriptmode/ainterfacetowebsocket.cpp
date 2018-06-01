#include "ainterfacetowebsocket.h"
#include "anetworkmodule.h"
#include "awebsocketstandalonemessanger.h"
#include "awebsocketsession.h"
#include "awebsocketsessionserver.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

AInterfaceToWebSocket::AInterfaceToWebSocket() : AScriptInterface()
{
    compatibilitySocket = new AWebSocketStandaloneMessanger();
    socket = new AWebSocketSession();
}

AInterfaceToWebSocket::AInterfaceToWebSocket(const AInterfaceToWebSocket &)
{
    compatibilitySocket = new AWebSocketStandaloneMessanger();
    socket = new AWebSocketSession();
}

/*
void AInterfaceToWebSocket::initSocket()
{
    if ( !sockets.contains(QThread::currentThread()))
        sockets.insert( QThread::currentThread(), new AWebSocketSession() );
}
*/

AInterfaceToWebSocket::~AInterfaceToWebSocket()
{
    //standaloneMessenger->deleteLater();
    delete compatibilitySocket;
    //socket->deleteLater();
    delete socket;

    /*
    for(auto e : sockets.keys())
       delete sockets.value(e);
    sockets.clear();
    */
}

void AInterfaceToWebSocket::ForceStop()
{
    socket->externalAbort();
    compatibilitySocket->externalAbort();
}

void AInterfaceToWebSocket::SetTimeout(int milliseconds)
{
    compatibilitySocket->setTimeout(milliseconds);
    socket->setTimeout(milliseconds);

    /*
    AWebSocketSession* socket = getSocket();
    if (socket) socket->setTimeout(milliseconds);
    */
}

const QString AInterfaceToWebSocket::SendTextMessage(const QString &Url, const QVariant& message, bool WaitForAnswer)
{
   bool bOK = compatibilitySocket->sendTextMessage(Url, message, WaitForAnswer);

   if (!bOK)
   {
       abort(compatibilitySocket->getError());
       return "";
   }
   return compatibilitySocket->getReceivedMessage();
}

int AInterfaceToWebSocket::Ping(const QString &Url)
{
    int ping = compatibilitySocket->ping(Url);

    if (ping < 0)
    {
        abort(compatibilitySocket->getError());
        return -1;
    }
    return ping;
}

/*
AWebSocketSession *AInterfaceToWebSocket::getSocket() const
{
    if (sockets.contains(QThread::currentThread()))
        return sockets[QThread::currentThread()];
    else return 0;
}
*/

void AInterfaceToWebSocket::Connect(const QString &Url)
{
    /*
    initSocket();
    AWebSocketSession* socket = getSocket();
    if (!socket)
    {
        abort("Web socket interface system error: socket not found for this thread");
        return;
    }
    */

    bool bOK = socket->connect(Url);
    if (!bOK)
        abort(socket->getError());
}

void AInterfaceToWebSocket::Disconnect()
{
    /*
    AWebSocketSession* socket = getSocket();
    if (!socket)
    {
        abort("Web socket interface system error: socket not found for this thread");
        return;
    }
    */

    socket->disconnect();
}

const QString AInterfaceToWebSocket::SendText(const QString &message)
{
    /*
    AWebSocketSession* socket = getSocket();
    if (!socket)
    {
        abort("Not connected or socket not found for this thread");
        return "";
    }
    */

    bool bOK = socket->sendText(message);
    if (bOK)
        return socket->getTextReply();
    else
    {
        abort(socket->getError());
        return 0;
    }
}

const QString AInterfaceToWebSocket::SendObject(const QVariant &object)
{
    /*
    AWebSocketSession* socket = getSocket();
    if (!socket)
    {
        abort("Not connected or socket not found for this thread");
        return "";
    }
    */

    if (object.type() != QMetaType::QVariantMap)
    {
        abort("Argument type of SendObject() method should be object!");
        return false;
    }
    QVariantMap vm = object.toMap();
    QJsonObject js = QJsonObject::fromVariantMap(vm);

    bool bOK = socket->sendJson(js);
    if (bOK)
        return socket->getTextReply();
    else
    {
        abort(socket->getError());
        return 0;
    }
}

const QString AInterfaceToWebSocket::SendFile(const QString &fileName)
{
    /*
    AWebSocketSession* socket = getSocket();
    if (!socket)
    {
        abort("Not connected or socket not found for this thread");
        return "";
    }
    */

    bool bOK = socket->sendFile(fileName);
    if (bOK)
        return socket->getTextReply();
    else
    {
        abort(socket->getError());
        return 0;
    }
}

const QString AInterfaceToWebSocket::ResumeWaitForAnswer()
{
    return socket->getTextReply();
}

const QVariant AInterfaceToWebSocket::GetBinaryReplyAsObject()
{
    /*
    AWebSocketSession* socket = getSocket();
    if (!socket)
    {
        abort("There was no connection yet or socket not found for this thread");
        return 0;
    }
    */

    const QByteArray& ba = socket->getBinaryReply();
    QJsonDocument doc = QJsonDocument::fromBinaryData(ba);
    QJsonObject json = doc.object();

    QVariantMap vm = json.toVariantMap();
    return vm;
}

bool AInterfaceToWebSocket::SaveBinaryReplyToFile(const QString &fileName)
{
    /*
    AWebSocketSession* socket = getSocket();
    if (!socket)
    {
        abort("There was no connection yet or socket not found for this thread");
        return false;
    }
    */

    const QByteArray& ba = socket->getBinaryReply();
    qDebug() << "ByteArray to save size:"<<ba.size();

    QFile saveFile(fileName);
    if ( !saveFile.open(QIODevice::WriteOnly) )
    {
        abort( QString("Server: Cannot save binary to file: ") + fileName );
        return false;
    }
    saveFile.write(ba);
    saveFile.close();
    return true;
}
