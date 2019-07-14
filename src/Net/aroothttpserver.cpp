#ifdef USE_ROOT_HTML

#include "aroothttpserver.h"

#include <QDebug>

#include "THttpServer.h"
#include "TGeoManager.h"
#include "TObjArray.h"

ARootHttpServer::ARootHttpServer(unsigned int port, QString OptionalUrlJsRoot)
{
    QString s = "http:" + QString::number(port);
    server = new THttpServer(s.toLatin1());
    if (!OptionalUrlJsRoot.isEmpty()) server->SetJSROOT(OptionalUrlJsRoot.toLatin1());

    GeoWorld = 0;
    GeoTracks = 0;
}

ARootHttpServer::~ARootHttpServer()
{
    delete server;
}

void ARootHttpServer::UpdateGeo(TGeoManager * GeoManager)
{
    //qDebug() << "In root html server: registering new TopNode and GeoTracks";

    if (GeoWorld) server->Unregister(GeoWorld);
    GeoManager->SetName("world");
    server->Register("GeoWorld", GeoManager);//->GetTopNode());
    GeoWorld = GeoManager;//->GetTopNode();

    //if (GeoTracks) server->Unregister(GeoTracks);
    //server->Register("GeoTracks", GeoManager->GetListOfTracks());
    //GeoTracks = GeoManager->GetListOfTracks();

    //qDebug() << "Num tracks:"<< GeoTracks->GetEntries();

    //server->SetItemField("/","_drawitem","world");
    //server->SetItemField("/","_drawopt","tracks");
}

#endif

