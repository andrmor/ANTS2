#ifndef ANETWORKMODULE_H
#define ANETWORKMODULE_H

#include <QObject>

//class AWebSocketServer;
class AWebSocketSessionServer;
class ARootHttpServer;
class TObject;
class AJavaScriptManager;

class ANetworkModule : public QObject
{
    Q_OBJECT
public:
    ANetworkModule() {}
    ~ANetworkModule();

    void SetDebug(bool flag) {fDebug = flag;}
    void SetScriptManager(AJavaScriptManager* man);

    bool isWebSocketServerRunning() const {return (bool)WebSocketServer;}
    int getWebSocketPort() const;
    const QString getWebSocketURL() const;

    bool isRootServerRunning() const;
    int getRootServerPort() const;
    const QString getJSROOTstring() const {return JSROOT;}

    AWebSocketSessionServer* WebSocketServer = 0;
#ifdef USE_ROOT_HTML
public: ARootHttpServer* RootHttpServer = 0;
#endif

public slots:
  void StartWebSocketServer(quint16 port);
  void StopWebSocketServer();
  void StartRootHttpServer(unsigned int port = 8080, QString OptionalUrlJsRoot = "https://root.cern/js/latest/");
  void StopRootHttpServer();

  void onNewGeoManagerCreated(TObject* GeoManager);
  void OnWebSocketTextMessageReceived(QString message);

signals:
  void StatusChanged();
  void RootServerStarted();
  void ReportTextToGUI(const QString text);

private:
  AJavaScriptManager* ScriptManager = 0;
  bool fDebug = true;
  QString JSROOT = "https://root.cern/js/latest/";
  unsigned int RootServerPort = 0;

};

#endif // ANETWORKMODULE_H
