#ifndef AWEBSERVERINTERFACE_H
#define AWEBSERVERINTERFACE_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVariant>

class AWebSocketSessionServer;

class AWebServerInterface: public AScriptInterface
{
  Q_OBJECT

public:
    AWebServerInterface(AWebSocketSessionServer& Server);
    ~AWebServerInterface() {}

public slots:
    void           SendText(const QString& message);
    void           SendFile(const QString& fileName);
    void           SendObject(const QVariant& object);
    void           SendObjectAsJSON(const QVariant& object);

    bool           IsBufferEmpty() const;
    void           ClearBuffer();

    const QVariant GetBufferAsObject() const;
    bool           SaveBufferToFile(const QString& fileName);

    void           SendProgressReport(int percents);
    void           SetAcceptExternalProgressReport(bool flag);

private:
    AWebSocketSessionServer& Server;
};

#endif // AWEBSERVERINTERFACE_H
