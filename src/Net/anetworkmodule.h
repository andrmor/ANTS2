#ifndef ANETWORKMODULE_H
#define ANETWORKMODULE_H

#include <QObject>
#include <QTimer>
#include <QHostAddress>

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

    void SetScriptManager(AJavaScriptManager* man);

    bool isWebSocketServerRunning() const;
    int getWebSocketPort() const;
    const QString getWebSocketServerURL() const;

    bool isRootServerRunning() const;

    const QString getWebSocketServerURL();

    void SetExitOnDisconnect(bool flag) {bSingleConnectionMode = flag;} //important: do it before start listen!
    void SetTicket(const QString& ticket);

    AWebSocketSessionServer* WebSocketServer = 0;
#ifdef USE_ROOT_HTML
public: ARootHttpServer* RootHttpServer = 0;
#endif

    void StartWebSocketServer(QHostAddress ip, quint16 port);
    void StopWebSocketServer();

public slots:
  void StartRootHttpServer();
  void StopRootHttpServer();

  void onNewGeoManagerCreated();
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
  AJavaScriptManager* ScriptManager = nullptr;

  QString Ticket;
  bool bTicketChecked = true;
  bool bSingleConnectionMode = false;
  int  SelfDestructOnIdle = 10000; //milliseconds
  QTimer IdleTimer;
};

#endif // ANETWORKMODULE_H
