#ifdef USE_ROOT_HTML

#include "aroothttpserver.h"

#include <QDebug>

#include "THttpServer.h"

ARootHttpServer::ARootHttpServer(unsigned int port, QString OptionalUrlJsRoot)
{
    QString s = "http:" + QString::number(port);
    server = new THttpServer(s.toLatin1());
    if (!OptionalUrlJsRoot.isEmpty()) server->SetJSROOT(OptionalUrlJsRoot.toLatin1());

    GeoWorld = 0;
}

ARootHttpServer::~ARootHttpServer()
{
    delete server;
}

void ARootHttpServer::UpdateGeoWorld(TObject *NewGeoWorld)
{
    qDebug() << "In root html server: registering new TopNode";
    if (GeoWorld) server->Unregister(GeoWorld);
    server->Register("GeoWorld", NewGeoWorld);
    GeoWorld = NewGeoWorld;
}

void ARootHttpServer::Register(QString name, TObject *obj)
{
    server->Register(name.toLatin1(), obj);
}

void ARootHttpServer::Unregister(TObject *obj)
{
    server->Unregister(obj);
}

#endif
