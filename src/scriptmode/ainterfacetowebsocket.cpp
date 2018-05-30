#include "ainterfacetowebsocket.h"
#include "anetworkmodule.h"
#include "awebsocketstandalonemessanger.h"
#include "awebsocketsession.h"
#include "awebsocketsessionserver.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

AInterfaceToWebSocket::AInterfaceToWebSocket(ANetworkModule &NetworkModule) : AScriptInterface(), NetworkModule(NetworkModule)
{
    standaloneMessenger = new AWebSocketStandaloneMessanger();
    sessionMessenger    = new AWebSocketSession();
}

AInterfaceToWebSocket::~AInterfaceToWebSocket()
{
    standaloneMessenger->deleteLater();
    sessionMessenger->deleteLater();
}

void AInterfaceToWebSocket::SetTimeout(int milliseconds)
{
    standaloneMessenger->setTimeout(milliseconds);
    sessionMessenger->setTimeout(milliseconds);
}

const QString AInterfaceToWebSocket::SendTextMessage(const QString &Url, const QVariant& message, bool WaitForAnswer)
{
   bool bOK = standaloneMessenger->sendTextMessage(Url, message, WaitForAnswer);

   if (!bOK)
   {
       abort(standaloneMessenger->getError());
       return "";
   }
   return standaloneMessenger->getReceivedMessage();
}

int AInterfaceToWebSocket::Ping(const QString &Url)
{
    int ping = standaloneMessenger->ping(Url);

    if (ping < 0)
    {
        abort(standaloneMessenger->getError());
        return -1;
    }
    return ping;
}

void AInterfaceToWebSocket::Connect(const QString &Url)
{
    bool bOK = sessionMessenger->connect(Url);
    if (!bOK)
        abort(sessionMessenger->getError());
}

void AInterfaceToWebSocket::Disconnect()
{
    sessionMessenger->disconnect();
}

const QString AInterfaceToWebSocket::SendText(const QString &message)
{
    bool bOK = sessionMessenger->sendText(message);
    if (bOK)
        return sessionMessenger->getTextReply();
    else
    {
        abort(sessionMessenger->getError());
        return 0;
    }
}

const QString AInterfaceToWebSocket::SendObject(const QVariant &object)
{
    if (object.type() != QMetaType::QVariantMap)
    {
        abort("Argument type of SendObject() method should be object!");
        return false;
    }
    QVariantMap vm = object.toMap();
    QJsonObject js = QJsonObject::fromVariantMap(vm);

    bool bOK = sessionMessenger->sendJson(js);
    if (bOK)
        return sessionMessenger->getTextReply();
    else
    {
        abort(sessionMessenger->getError());
        return 0;
    }
}

const QString AInterfaceToWebSocket::SendFile(const QString &fileName)
{
    bool bOK = sessionMessenger->sendFile(fileName);
    if (bOK)
        return sessionMessenger->getTextReply();
    else
    {
        abort(sessionMessenger->getError());
        return 0;
    }
}

const QVariant AInterfaceToWebSocket::GetBinaryReplyAsObject() const
{
    const QByteArray& ba = sessionMessenger->getBinaryReply();
    QJsonDocument doc = QJsonDocument::fromBinaryData(ba);
    QJsonObject json = doc.object();

    QVariantMap vm = json.toVariantMap();
    return vm;
}

bool AInterfaceToWebSocket::SaveBinaryReplyToFile(const QString &fileName)
{
    const QByteArray& ba = sessionMessenger->getBinaryReply();
    QJsonDocument doc = QJsonDocument::fromBinaryData(ba);

    QFile saveFile(fileName);
    if ( !saveFile.open(QIODevice::WriteOnly) )
    {
        abort( QString("Server: Cannot save binary to file: ") + fileName );
        return false;
    }
    saveFile.write(doc.toJson());
    saveFile.close();
    return true;
}

void AInterfaceToWebSocket::ServerSendText(const QString &message)
{
    NetworkModule.WebSocketServer->ReplyWithText(message);
}

void AInterfaceToWebSocket::ServerSendBinaryFile(const QString &fileName)
{
    NetworkModule.WebSocketServer->ReplyWithBinaryFile(fileName);
}

void AInterfaceToWebSocket::ServerSendBinaryObject(const QVariant &object)
{
    NetworkModule.WebSocketServer->ReplyWithBinaryObject(object);
}

void AInterfaceToWebSocket::ServerSendBinaryObject_asJSON(const QVariant &object)
{
    NetworkModule.WebSocketServer->ReplyWithBinaryObject_asJSON(object);
}

bool AInterfaceToWebSocket::ServerIsBinaryInputEmpty() const
{
    return NetworkModule.WebSocketServer->isBinaryEmpty();
}

void AInterfaceToWebSocket::ServerClearBinaryInput()
{
    NetworkModule.WebSocketServer->clearBinary();
}

const QVariant AInterfaceToWebSocket::ServerGetBinaryInputAsObject() const
{
    const QByteArray& ba = NetworkModule.WebSocketServer->getBinary();
    QJsonDocument doc =  QJsonDocument::fromBinaryData(ba);
    QJsonObject json = doc.object();

    QVariantMap vm = json.toVariantMap();
    return vm;
}

bool AInterfaceToWebSocket::ServerSaveBinaryInputToFile(const QString& fileName)
{
    const QByteArray& ba = NetworkModule.WebSocketServer->getBinary();
    QJsonDocument doc = QJsonDocument::fromBinaryData(ba);

    QFile saveFile(fileName);
    if ( !saveFile.open(QIODevice::WriteOnly) )
    {
        abort( QString("Cannot save binary reply to file: ") + fileName );
        return false;
    }
    saveFile.write(doc.toJson());
    saveFile.close();
    return true;
}
