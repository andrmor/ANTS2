#include "anetworkmodule.h"
#include "ajavascriptmanager.h"

//#include "awebsocketserver.h"
#include "awebsocketsessionserver.h"
#ifdef USE_ROOT_HTML
#include "aroothttpserver.h"
#endif

#include <QDebug>

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

int ANetworkModule::getRootServerPort() const
{
    return RootServerPort;
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

void ANetworkModule::StartWebSocketServer(quint16 port)
{    
    WebSocketServer->StartListen(port);
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

void ANetworkModule::StartRootHttpServer(unsigned int port, QString OptionalUrlJsRoot)
{
#ifdef USE_ROOT_HTML
    delete RootHttpServer;
    RootHttpServer = new ARootHttpServer(port, OptionalUrlJsRoot);

    JSROOT = OptionalUrlJsRoot;
    RootServerPort = port;
    qDebug() << "ANTS2 root server is now listening";
    emit StatusChanged();
    emit RootServerStarted(); //to update current geometry on the server
#endif
}
void ANetworkModule::StopRootHttpServer()
{
#ifdef USE_ROOT_HTML
    delete RootHttpServer;
    RootHttpServer = 0;

    qDebug() << "ANTS2 root server has stopped listening";
    emit StatusChanged();
#endif
}

void ANetworkModule::onNewGeoManagerCreated(TObject *GeoManager)
{
#ifdef USE_ROOT_HTML
    if (!RootHttpServer) return;
    RootHttpServer->UpdateGeoWorld(GeoManager);
#endif
}

void ANetworkModule::OnWebSocketTextMessageReceived(QString message)
{
    if (bSingleConnectionMode) IdleTimer.stop();

    if (message.startsWith("#"))
    {
        qDebug() << "Command received:"<<message;
        QStringList sl = message.split('#', QString::SkipEmptyParts);

        if (sl.size() >= 2 && sl.first().simplified() == "ticket")
        {
            QString receivedTicket = sl.at(1);
            qDebug() << "Received ticket" << receivedTicket;
            if ( receivedTicket == Ticket || Ticket.isEmpty() )
            {
                bTicketChecked = true;
                qDebug() << "  Ticket is valid";
                WebSocketServer->sendOK();
            }
            else
            {
                qDebug() << "  Invalid ticket!";
                WebSocketServer->sendError("Invalid ticket");
                WebSocketServer->DisconnectClient();
            }
        }
        else
        {
            qDebug() << "Unknown command";
            WebSocketServer->sendError("Invalid command");
            WebSocketServer->DisconnectClient();
        }
    }
    else
    {
        qDebug() << "Message received by websocket server";
        if (!bTicketChecked)
        {
            qDebug() << "  Ticket is required, but was not validated, disconnecting";
            WebSocketServer->sendError("Ticket was not validated");
            WebSocketServer->DisconnectClient();
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
                    else WebSocketServer->ReplyWithText("{ \"result\" : false, \"evaluation\" : \"" + res + "\" }");
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
