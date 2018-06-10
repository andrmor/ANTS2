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
    qDebug() << "Message received by websocket server -> running as script";
    int line = ScriptManager->FindSyntaxError(message);
    if (line != -1)
    {
       qDebug() << "Syntaxt error!";
       WebSocketServer->sendError("Syntax check failed - message is not a valid JavaScript");
    }
    else
    {
        QString res = ScriptManager->Evaluate(message);
        qDebug() << "Script evaluation result:"<<res;

        if (ScriptManager->isEvalAborted())
        {
            qDebug() << "Was aborted:" << ScriptManager->getLastError();
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
        //WebSocketServer->ReplyWithText("UpdateGeometry");
    }
}
