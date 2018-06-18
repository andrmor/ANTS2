#ifndef ANETWORKMODULE_H
#define ANETWORKMODULE_H

#include <QObject>
#include <QTimer>

//class AWebSocketServer;
class AWebSocketSessionServer;
class ARootHttpServer;
class TObject;
class AJavaScriptManager;

class ANetworkModule : public QObject
{
    Q_OBJECT
public:
    ANetworkModule();
    ~ANetworkModule();

    void SetDebug(bool flag) {fDebug = flag;}
    void SetScriptManager(AJavaScriptManager* man);

    bool isWebSocketServerRunning() const;
    int getWebSocketPort() const;
    const QString getWebSocketServerURL() const;

    bool isRootServerRunning() const;
    int getRootServerPort() const;
    const QString getJSROOTstring() const {return JSROOT;}

    const QString getWebSocketServerURL();

    void SetExitOnDisconnect(bool flag) {bSingleConnectionMode = flag;} //important: do it before start listen!
    void SetTicket(const QString& ticket);

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
  void OnClientDisconnected();

signals:
  void StatusChanged();
  void RootServerStarted();
  void ReportTextToGUI(const QString text);
  void ProgressReport(int percents); //retranslator to AWebSocketSessionServer

private slots:
  void onIdleTimerTriggered();

private:
  AJavaScriptManager* ScriptManager = 0;
  bool fDebug = true;
  QString JSROOT = "https://root.cern/js/latest/";
  unsigned int RootServerPort = 0;

  QString Ticket;
  bool bTicketChecked = true;
  bool bSingleConnectionMode = false;
  int  SelfDestructOnIdle = 10000; //milliseconds
  QTimer IdleTimer;
};

#endif // ANETWORKMODULE_H
