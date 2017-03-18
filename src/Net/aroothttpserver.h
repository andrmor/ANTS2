#ifdef USE_ROOT_HTML

#ifndef AROOTHTTPSERVER_H
#define AROOTHTTPSERVER_H

#include <QString>

class THttpServer;
class TObject;

class ARootHttpServer
{
public:
    ARootHttpServer(unsigned int port, QString OptionalUrlJsRoot = "");
    ~ARootHttpServer();

    void UpdateGeoWorld(TObject* NewGeoWorld);
    void Register(QString name, TObject* obj);
    void Unregister(TObject* obj);

private:
    THttpServer* server;
    TObject* GeoWorld;
};

#endif // AROOTHTTPSERVER_H

#endif
