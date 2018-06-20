#include "ainterfacetowebsocket.h"
#include "anetworkmodule.h"
#include "awebsocketstandalonemessanger.h"
#include "awebsocketsession.h"
#include "awebsocketsessionserver.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

AInterfaceToWebSocket::AInterfaceToWebSocket() : AScriptInterface() {}

AInterfaceToWebSocket::AInterfaceToWebSocket(const AInterfaceToWebSocket &)
{
    compatibilitySocket = 0;
    socket = 0;
}

AInterfaceToWebSocket::~AInterfaceToWebSocket()
{
    compatibilitySocket->deleteLater();
    socket->deleteLater();
}

void AInterfaceToWebSocket::ForceStop()
{
    if (socket) socket->ExternalAbort();
    if (compatibilitySocket) compatibilitySocket->externalAbort();
}

void AInterfaceToWebSocket::SetTimeout(int milliseconds)
{
    TimeOut = milliseconds;

    if (socket) socket->SetTimeout(milliseconds);
    if (compatibilitySocket) compatibilitySocket->setTimeout(milliseconds);
}

const QString AInterfaceToWebSocket::SendTextMessage(const QString &Url, const QVariant& message, bool WaitForAnswer)
{
    if (!compatibilitySocket)
    {
        compatibilitySocket = new AWebSocketStandaloneMessanger();
        compatibilitySocket->setTimeout(TimeOut);
    }

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
    if (!compatibilitySocket)
    {
        compatibilitySocket = new AWebSocketStandaloneMessanger();
        compatibilitySocket->setTimeout(TimeOut);
    }
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
    if (!socket)
    {
        socket = new AWebSocketSession();
        socket->SetTimeout(TimeOut);
    }

    bool bOK = socket->Connect(Url, GetAnswerOnConnection);
    if (bOK)
    {
        return socket->GetTextReply();
    }
    else
    {
        abort(socket->GetError());
        return "error";
    }
}

void AInterfaceToWebSocket::Disconnect()
{
    if (socket) socket->Disconnect();
}

const QJsonObject strToObject(const QString& s)
{
    QJsonDocument doc = QJsonDocument::fromJson(s.toUtf8());
    return doc.object();
}

const QString AInterfaceToWebSocket::OpenSession(const QString &IP, int port, int threads)
{
    QString url = "ws://" + IP + ':' + QString::number(port);
    QString reply = Connect(url, true);
    if (reply == "error") return "";
    qDebug() << "Dispatcher reply onConnect: " << reply;
    if ( !strToObject(reply)["result"].toBool() )
    {
        abort("Failed to connect to the ants2 dispatcher");
        return "";
    }

    QJsonObject cn;
    cn["command"] = "new";
    cn["threads"] = threads;
    QJsonDocument doc(cn);
    QString strCn(doc.toJson(QJsonDocument::Compact));
    reply = SendText( strCn ); //   SendText( '{ "command":"new", "threads":4 }' )
    qDebug() << reply;
    Disconnect();

    QJsonObject ro = strToObject(reply);
    if ( !ro["result"].toBool())
    {
        abort("Dispatcher rejected request for the new server: " + reply);
        return "";
    }

    port = ro["port"].toInt();
    QString ticket = ro["ticket"].toString();
    qDebug() << "Dispatcher allocated port: " << port << "  and ticket: "<< ticket;
    qDebug() << "\nConnecting to ants2 server...";

    url = "ws://" + IP + ':' + QString::number(port);
    reply = Connect(url, true);
    qDebug() << "Server reply onConnect: " << reply;
    if ( !strToObject(reply)["result"].toBool())
    {
        abort("Failed to connect to the ants2 server");
        return "";
    }

    reply = SendTicket( ticket );
    qDebug() << "ants2 server reply message: " << reply;
    if ( !strToObject(reply)["result"].toBool() )
    {
        abort("Server rejected the ticket!");
        return "";
    }
    qDebug() << "   Connected!";

    return QString("Connected to ants2 server with ticket ") + ticket;
}

const QString AInterfaceToWebSocket::SendText(const QString &message)
{
    if (!socket)
    {
        abort("Web socket was not connected");
        return "";
    }

    bool bOK = socket->SendText(message);
    if (bOK)
        return socket->GetTextReply();
    else
    {
        abort(socket->GetError());
        return 0;
    }
}

const QString AInterfaceToWebSocket::SendTicket(const QString &ticket)
{
    QString m = "__";
    m += "{\"ticket\" : \"";
    m += ticket;
    m += "\"}";

    return SendText(m);
}

const QString AInterfaceToWebSocket::SendObject(const QVariant &object)
{
    if (!socket)
    {
        abort("Web socket was not connected");
        return "";
    }

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
    if (!socket)
    {
        abort("Web socket was not connected");
        return "";
    }

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
    if (!socket)
    {
        abort("Web socket was not connected");
        return "";
    }

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
    if (!socket)
    {
        abort("Web socket was not connected");
        return "";
    }
    const QByteArray& ba = socket->GetBinaryReply();
    QJsonDocument doc = QJsonDocument::fromBinaryData(ba);
    QJsonObject json = doc.object();

    QVariantMap vm = json.toVariantMap();
    return vm;
}

bool AInterfaceToWebSocket::SaveBinaryReplyToFile(const QString &fileName)
{
    if (!socket)
    {
        abort("Web socket was not connected");
        return "";
    }
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
