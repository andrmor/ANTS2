#ifdef USE_ROOT_HTML

#ifndef AROOTHTTPSERVER_H
#define AROOTHTTPSERVER_H

#include <QString>

class THttpServer;
class TGeoManager;
class TGeoNode;
class TObjArray;

class ARootHttpServer
{
public:
    ARootHttpServer(unsigned int port, QString OptionalUrlJsRoot = "");
    ~ARootHttpServer();

    void UpdateGeo(TGeoManager* GeoManager);

private:
    THttpServer* server;
    //TGeoNode* GeoWorld;
    TGeoManager* GeoWorld;
    TObjArray* GeoTracks;
};

#endif // AROOTHTTPSERVER_H

#endif
