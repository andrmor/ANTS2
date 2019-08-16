#include "aroothttpserver.h"

#include <QDebug>

#include "THttpServer.h"
#include "TGeoManager.h"

ARootHttpServer::ARootHttpServer(unsigned int port, QString OptionalUrlJsRoot)
{
    QString s = "http:" + QString::number(port);
    server = new THttpServer(s.toLatin1());
    if (!OptionalUrlJsRoot.isEmpty()) server->SetJSROOT(OptionalUrlJsRoot.toLatin1());
}

ARootHttpServer::~ARootHttpServer()
{
    delete server;
}

void ARootHttpServer::UpdateGeo(TGeoManager * GeoManager)
{
    //qDebug() << "Root html server: registering new GeoManager";
    if (GeoWorld) server->Unregister(GeoWorld);
    GeoManager->SetName("world");
    server->Register("GeoWorld", GeoManager);
    GeoWorld = GeoManager;

    server->SetItemField("/","_drawitem","world");
}
