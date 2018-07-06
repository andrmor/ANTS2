#include "ainterfacetowebsocket.h"
#include "anetworkmodule.h"
#include "awebsocketstandalonemessanger.h"
#include "awebsocketsession.h"
#include "awebsocketsessionserver.h"
#include "eventsdataclass.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

const QJsonObject strToObject(const QString& s)
{
    QJsonDocument doc = QJsonDocument::fromJson(s.toUtf8());
    return doc.object();
}

AInterfaceToWebSocket::AInterfaceToWebSocket(EventsDataClass *EventsDataHub) :
    AScriptInterface(), EventsDataHub(EventsDataHub) {}

AInterfaceToWebSocket::AInterfaceToWebSocket(const AInterfaceToWebSocket &other)
{
    compatibilitySocket = 0;
    socket = 0;
    EventsDataHub = other.EventsDataHub;
}

AInterfaceToWebSocket::~AInterfaceToWebSocket()
{
    if (compatibilitySocket) compatibilitySocket->deleteLater();
    if (socket) socket->deleteLater();
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
        return "";
    }
}

void AInterfaceToWebSocket::Disconnect()
{
    if (socket) socket->Disconnect();
}

int AInterfaceToWebSocket::GetAvailableThreads(const QString &IP, int port, bool ShowOutput)
{
    QString url = "ws://" + IP + ':' + QString::number(port);

    if (ShowOutput) emit clearTextOnMessageWindow();
    if (ShowOutput) emit showTextOnMessageWindow(QString("Connecting to dispatcher ") + url);

    QString reply = Connect(url, true);
    if (reply.isEmpty() || !strToObject(reply)["result"].toBool())
    {
        if (ShowOutput) emit showTextOnMessageWindow( "Connection failed!" );
        return 0;
    }

    if (ShowOutput) emit showTextOnMessageWindow( "Requesting status" );
    reply = SendText( "{ \"command\" : \"report\" }" );
    if (ShowOutput) emit showTextOnMessageWindow( QString("Dispatcher reply: ") + reply );
    Disconnect();

    int availableThreads = 0;
    QJsonObject ro = strToObject(reply);
    if (ro.contains("threads"))
         availableThreads = ro["threads"].toInt();
    if (ShowOutput) emit showTextOnMessageWindow( QString("Available threads: ") + QString::number(availableThreads) );

    return availableThreads;
}

const QString AInterfaceToWebSocket::OpenSession(const QString &IP, int port, int threads, bool ShowOutput)
{
    QString url = "ws://" + IP + ':' + QString::number(port);
    RequestedThreads = threads;

    if (ShowOutput) emit clearTextOnMessageWindow();
    if (ShowOutput) emit showTextOnMessageWindow(QString("Connecting to dispatcher ") + url);

    QString reply = Connect(url, true);
    if (reply.isEmpty())
    {
        if (ShowOutput) emit showTextOnMessageWindow( "Connection failed!" );
        return "";
    }

    //  if (ShowOutput) emit showTextOnMessageWindow( QString("Dispatcher reply onConnect: ") + reply );
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

    if (ShowOutput) emit showTextOnMessageWindow( "Requesting ants2 server" );

    reply = SendText( strCn ); //   SendText( '{ "command":"new", "threads":4 }' )
    //  if (ShowOutput) emit showTextOnMessageWindow( QString("Dispatcher reply: ") + reply );
    Disconnect();

    QJsonObject ro = strToObject(reply);
    if ( !ro["result"].toBool())
    {
        abort("Dispatcher rejected request for the new server: " + reply);
        return "";
    }

    port = ro["port"].toInt();
    QString ticket = ro["ticket"].toString();
    if (ShowOutput) emit showTextOnMessageWindow( QString("Dispatcher allocated port ") + QString::number(port) + "  and ticket " + ticket);

    url = "ws://" + IP + ':' + QString::number(port);
    if (ShowOutput) emit showTextOnMessageWindow( QString("Connecting to ants2 server at ") + url);
    reply = Connect(url, true);
    //  if (ShowOutput) emit showTextOnMessageWindow( QString("Server reply onConnect: ") + reply );
    if ( !strToObject(reply)["result"].toBool())
    {
        abort("Failed to connect to the ants2 server");
        return "";
    }

    if (ShowOutput) emit showTextOnMessageWindow("Sending ticket " + ticket);
    reply = SendTicket( ticket );
    //  if (ShowOutput) emit showTextOnMessageWindow( QString("Server reply message: ") + reply);
    if ( !strToObject(reply)["result"].toBool() )
    {
        abort("Server rejected the ticket!");
        return "";
    }
    if (ShowOutput) emit showTextOnMessageWindow( "Connected!");

    return QString("Connected to ants2 server with ticket ") + ticket;
}

bool AInterfaceToWebSocket::SendConfig(QVariant config)
{
    if (!socket)
    {
        abort("Web socket was not connected");
        return false;
    }

    QString reply = SendObject(config);
    QJsonObject obj = strToObject(reply);
    if ( !obj.contains("result") || !obj["result"].toBool() )
    {
        abort("Failed to send config");
        return false;
    }

    QString Script = "var c = server.GetBufferAsObject();"
                     "var ok = config.SetConfig(c);"
                     "if (!ok) core.abort(\"Failed to set config\");";
    reply = SendText(Script);
    obj = strToObject(reply);
    if ( !obj.contains("result") || !obj["result"].toBool() )
    {
        abort("Failed to set config at the remote server");
        return false;
    }
    return true;
}

bool AInterfaceToWebSocket::RemoteSimulatePhotonSources(const QString& SimTreeFileNamePattern, bool ShowOutput)
{
    return remoteSimulate(true, SimTreeFileNamePattern, ShowOutput);
}

bool AInterfaceToWebSocket::RemoteSimulateParticleSources(const QString &SimTreeFileNamePattern, bool ShowOutput)
{
    return remoteSimulate(false, SimTreeFileNamePattern, ShowOutput);
}

bool AInterfaceToWebSocket::remoteSimulate(bool bPhotonSource, const QString& LocalSimTreeFileName, bool ShowOutput)
{
    if (!socket)
    {
        abort("Web socket was not connected");
        return false;
    }

    const QString RemoteSimTreeFileName = QString("SimTree-") + QString::number(socket->GetPeerPort()) + ".root";

    QString Script;
    Script += "server.SetAcceptExternalProgressReport(true);"; //even if not showing to the user, still want to send reports to see that the server is alive
    if (bPhotonSource)
        Script += "sim.RunPhotonSources(" + QString::number(RequestedThreads) + ");";
    else
        Script += "sim.RunParticleSources(" + QString::number(RequestedThreads) + ");";
    Script += "var fileName = \"" + RemoteSimTreeFileName + "\";";
    Script += "var ok = sim.SaveAsTree(fileName);";
    Script += "if (!ok) core.abort(\"Failed to save simulation data\");";
    if (ShowOutput) Script += "server.SetAcceptExternalProgressReport(false);";
    Script += "server.SendFile(fileName);";
    qDebug() << Script;

    //execute the script on remote server
    if (ShowOutput) emit showTextOnMessageWindow("Sending script to the server...");
    QString reply = SendText(Script);
    if (reply.isEmpty())
    {
        if (ShowOutput)
        {
            emit showTextOnMessageWindow("Script execution failed!");
            emit showTextOnMessageWindow(socket->GetError());
        }
        return false; //this is on local abort, e.g. timeout
    }
    QJsonObject obj = strToObject(reply);
    if (obj.contains("error"))
    {
        const QString err = obj["error"].toString();
        emit showTextOnMessageWindow(err);
        abort(err);
        return false;
    }
    while ( !obj.contains("binary") ) //after sending the file, the reply is "{ \"binary\" : \"file\" }"
    {
        if (ShowOutput) emit showTextOnMessageWindow(reply);
        reply = ResumeWaitForAnswer();
        if (reply.isEmpty())
        {
            emit showTextOnMessageWindow("Script execution failed!");
            emit showTextOnMessageWindow(socket->GetError());
            return false; //this is on local abort, e.g. timeout
        }
        obj = strToObject(reply);
    }

    if (ShowOutput) emit showTextOnMessageWindow("Server has finished simulation");
    bool bOK = SaveBinaryReplyToFile(LocalSimTreeFileName);
    if (!bOK)
    {
        const QString err = QString("Cannot save tree in file ") + LocalSimTreeFileName;
        emit showTextOnMessageWindow(err);
        abort(err);
        return false;
    }
    emit showTextOnMessageWindow("Sim results saved in " + LocalSimTreeFileName);
    return true;
}

bool AInterfaceToWebSocket::RemoteReconstructEvents(int eventsFrom, int eventsTo, bool ShowOutput)
{
    if (!socket)
    {
        abort("Web socket was not connected");
        return false;
    }

    if (eventsFrom < 0 || eventsTo > EventsDataHub->countEvents() || eventsFrom >= eventsTo)
    {
        abort("Bad from/to events");
        return false;
    }

    //sending events
    QByteArray ba;
    EventsDataHub->packEventsToByteArray(eventsFrom, eventsTo, ba);

    if (ShowOutput) emit showTextOnMessageWindow("Sending events to the server...");
    QString reply = sendQByteArray(ba);
    QJsonObject obj = strToObject(reply);
    if ( !obj.contains("result") || !obj["result"].toBool() )
    {
        abort("Failed to send events");
        return false;
    }

    if (ShowOutput) emit showTextOnMessageWindow("Setting event signals at the remote server...");
    QString Script = "server.GetBufferAsEvents()";
    reply = SendText(Script);
    obj = strToObject(reply);
    if ( !obj.contains("result") || !obj["result"].toBool() )
    {
        abort("Failed to set events on server. Check that configuration was transferred");
        return false;
    }

    if (ShowOutput) emit showTextOnMessageWindow("Starting reconstruction...");
    Script = "server.SetAcceptExternalProgressReport(true);"; //even if not showing to the user, still want to send reports to see that the server is alive
    Script += "rec.ReconstructEvents(" + QString::number(RequestedThreads) + ", false);";
    Script += "server.SendReconstructionData()";
    reply = SendText(Script);
    obj = strToObject(reply);
    if (obj.contains("error"))
    {
        const QString err = obj["error"].toString();
        emit showTextOnMessageWindow(err);
        abort(err);
        return false;
    }
    while ( !obj.contains("binary") ) //after sending the file, the reply is "{ \"binary\" : \"qbytearray\" }"
    {
        if (ShowOutput) emit showTextOnMessageWindow(reply);
        reply = ResumeWaitForAnswer();
        if (reply.isEmpty())
        {
            emit showTextOnMessageWindow("Script execution failed!");
            emit showTextOnMessageWindow(socket->GetError());
            return false; //this is on local abort, e.g. timeout
        }
        obj = strToObject(reply);
    }
    if (ShowOutput) emit showTextOnMessageWindow("Remote reconstruction finished");

    if (ShowOutput) emit showTextOnMessageWindow("Setting local reconstruction data...");
    const QByteArray& baIn = socket->GetBinaryReply();
    bool bOK = EventsDataHub->unpackReconstructedFromByteArray(eventsFrom, eventsTo, baIn);
    if (!bOK)
    {
        QString err = "Failed to set reconstruction data using the obtained data";
        emit showTextOnMessageWindow(err);
        abort(err);
        return false;
    }

    if (ShowOutput) emit showTextOnMessageWindow("Done!");

    /*
    Script += "server.SendFile(fileName);";
    qDebug() << Script;

    //execute the script on remote server
    if (ShowOutput) emit showTextOnMessageWindow("Sending script to the server...");
    reply = SendText(Script);
    if (reply.isEmpty())
    {
        if (ShowOutput)
        {
            emit showTextOnMessageWindow("Script execution failed!");
            emit showTextOnMessageWindow(socket->GetError());
        }
        return false; //this is on local abort, e.g. timeout
    }
    obj = strToObject(reply);
    if (obj.contains("error"))
    {
        const QString err = obj["error"].toString();
        emit showTextOnMessageWindow(err);
        abort(err);
        return false;
    }
*/

    return true;
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
        return "";
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
    if (object.type() != QMetaType::QVariantMap)
    {
        abort("Argument type of SendObject() method should be object!");
        return "";
    }
    QVariantMap vm = object.toMap();
    QJsonObject js = QJsonObject::fromVariantMap(vm);

    return sendQJsonObject(js);
}

const QString AInterfaceToWebSocket::sendQJsonObject(const QJsonObject& json)
{
    if (!socket)
    {
        abort("Web socket was not connected");
        return "";
    }

    bool bOK = socket->SendJson(json);
    if (bOK)
        return socket->GetTextReply();
    else
    {
        abort(socket->GetError());
        return "";
    }
}

const QString AInterfaceToWebSocket::sendQByteArray(const QByteArray &ba)
{
    if (!socket)
    {
        abort("Web socket was not connected");
        return "";
    }

    bool bOK = socket->SendQByteArray(ba);
    if (bOK)
        return socket->GetTextReply();
    else
    {
        abort(socket->GetError());
        return "";
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
        return "";
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
        return "";
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
