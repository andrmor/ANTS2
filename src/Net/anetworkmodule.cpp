#include "anetworkmodule.h"
#include "ajavascriptmanager.h"
#include "agridrunner.h"
#include "awebsocketsessionserver.h"

#ifdef USE_ROOT_HTML
    #include "aroothttpserver.h"
#endif

#include <QDebug>

#include "TGeoManager.h"

ANetworkModule::ANetworkModule()
{
    WebSocketServer = new AWebSocketSessionServer();

    QObject::connect(WebSocketServer, &AWebSocketSessionServer::textMessageReceived, this, &ANetworkModule::OnWebSocketTextMessageReceived);
    QObject::connect(WebSocketServer, &AWebSocketSessionServer::reportToGUI, this, &ANetworkModule::ReportTextToGUI);
    QObject::connect(WebSocketServer, &AWebSocketSessionServer::clientDisconnected, this, &ANetworkModule::OnClientDisconnected);

    QObject::connect(this, &ANetworkModule::ProgressReport, WebSocketServer, &AWebSocketSessionServer::onProgressChanged);
}

ANetworkModule::~ANetworkModule()
{
    delete WebSocketServer;

#ifdef USE_ROOT_HTML
    delete RootHttpServer;
#endif

    delete GridRunner;
}

void ANetworkModule::SetScriptManager(AJavaScriptManager* man)
{
  ScriptManager = man;
}

bool ANetworkModule::isWebSocketServerRunning() const
{
    return WebSocketServer->IsRunning();
}

int ANetworkModule::getWebSocketPort() const
{
    if (!WebSocketServer) return 0;
    return WebSocketServer->GetPort();
}

const QString ANetworkModule::getWebSocketServerURL() const
{
  if (!WebSocketServer) return "";
  return WebSocketServer->GetUrl();
}

const QString ANetworkModule::getWebSocketServerURL()
{
    return WebSocketServer->GetUrl();
}

void ANetworkModule::SetTicket(const QString &ticket)
{
    Ticket = ticket;
    bTicketChecked = false;
}

bool ANetworkModule::isRootServerRunning() const
{
#ifdef USE_ROOT_HTML
    return (bool)RootHttpServer;
#endif
  return false;
}

void ANetworkModule::StartWebSocketServer(QHostAddress ip, quint16 port)
{    
    WebSocketServer->StartListen(ip, port);
    emit StatusChanged();

    if (bSingleConnectionMode)
    {
        IdleTimer.setInterval(SelfDestructOnIdle);
        IdleTimer.setSingleShot(true);
        QObject::connect(&IdleTimer, &QTimer::timeout, this, &ANetworkModule::onIdleTimerTriggered);
        IdleTimer.start();
    }
}

void ANetworkModule::StopWebSocketServer()
{
    WebSocketServer->StopListen();
    qDebug() << "ANTS2 web socket server has stopped listening";
    emit StatusChanged();
}

#include "agridrunner.h"
void ANetworkModule::initGridRunner(EventsDataClass & EventsDataHub, const APmHub & PMs, ASimulationManager & simMan)
{
    GridRunner = new AGridRunner(EventsDataHub, PMs, simMan);
}

#include "aglobalsettings.h"
bool ANetworkModule::StartRootHttpServer()
{
#ifdef USE_ROOT_HTML
    const AGlobalSettings & GlobSet = AGlobalSettings::getInstance();

    delete RootHttpServer;
    RootHttpServer = new ARootHttpServer(GlobSet.RootServerPort, GlobSet.ExternalJSROOT);

    if (RootHttpServer->isRunning())
    {
        qDebug() << "ANTS2 root server is now listening";
        emit StatusChanged();
        emit RootServerStarted(); //to update current geometry on the server
        return true;
    }
    else
    {
        delete RootHttpServer; RootHttpServer = nullptr;
        qDebug() << "Root http server failed to start!\nCheck if another server is already listening at this port";
        return false;
    }
#endif
    return false;
}

void ANetworkModule::StopRootHttpServer()
{
#ifdef USE_ROOT_HTML
    delete RootHttpServer;
    RootHttpServer = nullptr;

    qDebug() << "ANTS2 root server has stopped listening";
    emit StatusChanged();
#endif
}

void ANetworkModule::onNewGeoManagerCreated()
{
#ifdef USE_ROOT_HTML
    if (!RootHttpServer) return;
    RootHttpServer->UpdateGeo(gGeoManager);
#endif
}

#include <QJsonObject>
#include <QJsonDocument>
void ANetworkModule::OnWebSocketTextMessageReceived(QString message)
{
    if (bSingleConnectionMode) IdleTimer.stop();

    if (message.startsWith("__"))
    {
        qDebug() << "WebSocket server received system message:"<<message;
        message.remove(0, 2);
        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
        QJsonObject json = doc.object();

        if (json.contains("ticket"))
        {
            QString receivedTicket = json["ticket"].toString();
            qDebug() << "  Ticket presented" << receivedTicket;
            if ( receivedTicket == Ticket || Ticket.isEmpty() )
            {
                bTicketChecked = true;
                qDebug() << "  Ticket is valid!";
                WebSocketServer->sendOK();
            }
            else
            {
                qDebug() << "  Invalid ticket!";
                WebSocketServer->sendError("Invalid ticket");
                WebSocketServer->DisconnectClient();
                return;
            }
        }
        else
        {
            qDebug() << "  System message not recognized!";
            WebSocketServer->sendError("Unknown system message");
            WebSocketServer->DisconnectClient();
            return;
        }
    }
    else
    {
        qDebug() << "Websocket server: Message (script) received";
        if (!bTicketChecked)
        {
            qDebug() << "  Ticket is required, but has not been validated, disconnecting";
            WebSocketServer->sendError("Ticket was not validated");
            WebSocketServer->DisconnectClient();
            return;
        }

        qDebug() << "  Evaluating as JavaScript";

        int line = ScriptManager->FindSyntaxError(message);
        if (line != -1)
        {
           qDebug() << "Syntaxt error!";
           WebSocketServer->sendError("  Syntax check failed - message is not a valid JavaScript");
        }
        else
        {
            QString res = ScriptManager->Evaluate(message);
            qDebug() << "  Script evaluation result:"<<res;

            if (ScriptManager->isEvalAborted())
            {
                qDebug() << "  Was aborted:" << ScriptManager->getLastError();
                WebSocketServer->sendError("Aborted -> " + ScriptManager->getLastError());
            }
            else
            {
                if ( !WebSocketServer->isReplied() )
                {
                    if (res == "undefined") WebSocketServer->sendOK();
                    else WebSocketServer->ReplyWithText("{ \"result\" : true, \"evaluation\" : \"" + res + "\" }");
                }
            }
        }
    }

    if (bSingleConnectionMode) IdleTimer.start();
}

void ANetworkModule::OnClientDisconnected()
{
    if (bSingleConnectionMode) exit(0);
}

void ANetworkModule::onIdleTimerTriggered()
{
    qDebug() << "Idle time limit reached, exiting";
    exit(1);
}
