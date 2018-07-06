#ifndef AWEBSERVERINTERFACE_H
#define AWEBSERVERINTERFACE_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVariant>

class AWebSocketSessionServer;
class EventsDataClass;

class AWebServerInterface: public AScriptInterface
{
  Q_OBJECT

public:
    AWebServerInterface(AWebSocketSessionServer& Server, EventsDataClass* EventsDataHub);
    ~AWebServerInterface() {}

public slots:
    void           SendText(const QString& message);
    void           SendFile(const QString& fileName);
    void           SendObject(const QVariant& object);
    void           SendObjectAsJSON(const QVariant& object);
    void           SendReconstructionData();

    bool           IsBufferEmpty() const;
    void           ClearBuffer();

    const QVariant GetBufferAsObject() const;
    void           GetBufferAsEvents();  //abort on fail, otherwise reply with OK
    bool           SaveBufferToFile(const QString& fileName);

    void           SendProgressReport(int percents);
    void           SetAcceptExternalProgressReport(bool flag);

private:
    AWebSocketSessionServer& Server;
    EventsDataClass* EventsDataHub;
};

#endif // AWEBSERVERINTERFACE_H
