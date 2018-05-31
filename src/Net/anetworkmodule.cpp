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

const QString ANetworkModule::getWebSocketURL() const
{
  if (!WebSocketServer) return "";
  return WebSocketServer->GetUrl();
}

int ANetworkModule::getRootServerPort() const
{
  return RootServerPort;
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
       WebSocketServer->ReplyWithText("Error: Syntax check failed");
    }
    else
    {
        QString res = ScriptManager->Evaluate(message);
        qDebug() << "Script evaluation result:"<<res;

        if (ScriptManager->isEvalAborted())
        {
            WebSocketServer->ReplyWithText("Error: aborted -> " + ScriptManager->getLastError());
        }
        else
        {
            if ( !WebSocketServer->isReplied() )
            {
                if (res == "undefined") WebSocketServer->ReplyWithText("OK");
                else WebSocketServer->ReplyWithText(res);
            }
        }
        //WebSocketServer->ReplyWithText("UpdateGeometry");
    }
}


