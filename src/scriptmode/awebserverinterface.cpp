#include "awebserverinterface.h"
#include "awebsocketsessionserver.h"
#include "eventsdataclass.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

AWebServerInterface::AWebServerInterface(AWebSocketSessionServer &Server, EventsDataClass *EventsDataHub) :
    AScriptInterface(), Server(Server), EventsDataHub(EventsDataHub)
{
    QObject::connect(&Server, &AWebSocketSessionServer::requestAbort, this, &AWebServerInterface::AbortScriptEvaluation);
}

void AWebServerInterface::SendText(const QString &message)
{
    Server.ReplyWithText(message);
}

void AWebServerInterface::SendFile(const QString &fileName)
{
    Server.ReplyWithBinaryFile(fileName);
}

void AWebServerInterface::SendObject(const QVariant &object)
{
    Server.ReplyWithBinaryObject(object);
}

void AWebServerInterface::SendObjectAsJSON(const QVariant &object)
{
    Server.ReplyWithBinaryObject_asJSON(object);
}

void AWebServerInterface::SendReconstructionData()
{
    QByteArray ba;
    EventsDataHub->packReconstructedToByteArray(ba);

    Server.ReplyWithQByteArray(ba);
}

bool AWebServerInterface::IsBufferEmpty() const
{
    return Server.isBinaryEmpty();
}

void AWebServerInterface::ClearBuffer()
{
    Server.clearBinary();
}

const QVariant AWebServerInterface::GetBufferAsObject() const
{
    const QByteArray& ba = Server.getBinary();
    QJsonDocument doc =  QJsonDocument::fromBinaryData(ba);
    QJsonObject json = doc.object();

    QVariantMap vm = json.toVariantMap();
    return vm;
}

void AWebServerInterface::GetBufferAsEvents()
{
    const QByteArray& ba = Server.getBinary();

    bool bOK = EventsDataHub->unpackEventsFromByteArray(ba);
    if (!bOK)
        abort("Failed to set events from the binary buffer.");
    else Server.sendOK();
}

bool AWebServerInterface::SaveBufferToFile(const QString &fileName)
{
    const QByteArray& ba = Server.getBinary();
    qDebug() << "Preparing to save, ba size = " << ba.size();

    QFile saveFile(fileName);
    if ( !saveFile.open(QIODevice::WriteOnly) )
    {
        abort( QString("Cannot save binary reply to file: ") + fileName );
        return false;
    }
    saveFile.write(ba);
    saveFile.close();
    return true;
}

void AWebServerInterface::SendProgressReport(int percents)
{
    Server.ReplyProgress(percents);
}

void AWebServerInterface::SetAcceptExternalProgressReport(bool flag)
{
    Server.SetCanRetranslateProgress(flag);
}
