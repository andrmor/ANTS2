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

AInterfaceToWebSocket::~AInterfaceToWebSocket()
{
    compatibilitySocket->deleteLater();
    socket->deleteLater();
}

void AInterfaceToWebSocket::ForceStop()
{
    socket->ExternalAbort();
    compatibilitySocket->externalAbort();
}

void AInterfaceToWebSocket::SetTimeout(int milliseconds)
{
    compatibilitySocket->setTimeout(milliseconds);
    socket->SetTimeout(milliseconds);
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

const QString AInterfaceToWebSocket::Connect(const QString &Url, bool GetAnswerOnConnection)
{
    bool bOK = socket->Connect(Url, GetAnswerOnConnection);
    if (bOK)
    {
        return socket->GetTextReply();
    }
    else
    {
        abort(socket->GetError());
        return "";
    }
}

void AInterfaceToWebSocket::Disconnect()
{
    socket->Disconnect();
}

const QString AInterfaceToWebSocket::SendText(const QString &message)
{
    bool bOK = socket->SendText(message);
    if (bOK)
        return socket->GetTextReply();
    else
    {
        abort(socket->GetError());
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

    bool bOK = socket->SendJson(js);
    if (bOK)
        return socket->GetTextReply();
    else
    {
        abort(socket->GetError());
        return 0;
    }
}

const QString AInterfaceToWebSocket::SendFile(const QString &fileName)
{
    bool bOK = socket->SendFile(fileName);
    if (bOK)
        return socket->GetTextReply();
    else
    {
        abort(socket->GetError());
        return 0;
    }
}

const QString AInterfaceToWebSocket::ResumeWaitForAnswer()
{
    bool bOK = socket->ResumeWaitForAnswer();
    if (bOK)
        return socket->GetTextReply();
    else
    {
        abort(socket->GetError());
        return 0;
    }
}

const QVariant AInterfaceToWebSocket::GetBinaryReplyAsObject()
{
    const QByteArray& ba = socket->GetBinaryReply();
    QJsonDocument doc = QJsonDocument::fromBinaryData(ba);
    QJsonObject json = doc.object();

    QVariantMap vm = json.toVariantMap();
    return vm;
}

bool AInterfaceToWebSocket::SaveBinaryReplyToFile(const QString &fileName)
{
    const QByteArray& ba = socket->GetBinaryReply();
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
