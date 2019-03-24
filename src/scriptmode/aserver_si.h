#ifndef AWEBSERVERINTERFACE_H
#define AWEBSERVERINTERFACE_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVariant>

class AWebSocketSessionServer;
class EventsDataClass;

class AServer_SI: public AScriptInterface
{
  Q_OBJECT

public:
    AServer_SI(AWebSocketSessionServer& Server, EventsDataClass* EventsDataHub);
    ~AServer_SI() {}

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
