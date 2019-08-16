#ifndef AROOTHTTPSERVER_H
#define AROOTHTTPSERVER_H

#include <QString>

class THttpServer;
class TGeoManager;

class ARootHttpServer
{
public:
    ARootHttpServer(unsigned int port, QString OptionalUrlJsRoot = "");
    ~ARootHttpServer();

    void UpdateGeo(TGeoManager* GeoManager);

private:
    THttpServer * server = nullptr;
    TGeoManager * GeoWorld = nullptr;
};

#endif // AROOTHTTPSERVER_H
