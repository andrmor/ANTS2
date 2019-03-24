#include "aserver_si.h"
#include "awebsocketsessionserver.h"
#include "eventsdataclass.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

AServer_SI::AServer_SI(AWebSocketSessionServer &Server, EventsDataClass *EventsDataHub) :
    AScriptInterface(), Server(Server), EventsDataHub(EventsDataHub)
{
    QObject::connect(&Server, &AWebSocketSessionServer::requestAbort, this, &AServer_SI::AbortScriptEvaluation);
}

void AServer_SI::SendText(const QString &message)
{
    Server.ReplyWithText(message);
}

void AServer_SI::SendFile(const QString &fileName)
{
    Server.ReplyWithBinaryFile(fileName);
}

void AServer_SI::SendObject(const QVariant &object)
{
    Server.ReplyWithBinaryObject(object);
}

void AServer_SI::SendObjectAsJSON(const QVariant &object)
{
    Server.ReplyWithBinaryObject_asJSON(object);
}

void AServer_SI::SendReconstructionData()
{
    QByteArray ba;
    EventsDataHub->packReconstructedToByteArray(ba);

    Server.ReplyWithQByteArray(ba);
}

bool AServer_SI::IsBufferEmpty() const
{
    return Server.isBinaryEmpty();
}

void AServer_SI::ClearBuffer()
{
    Server.clearBinary();
}

const QVariant AServer_SI::GetBufferAsObject() const
{
    const QByteArray& ba = Server.getBinary();
    QJsonDocument doc =  QJsonDocument::fromBinaryData(ba);
    QJsonObject json = doc.object();

    QVariantMap vm = json.toVariantMap();
    return vm;
}

void AServer_SI::GetBufferAsEvents()
{
    const QByteArray& ba = Server.getBinary();

    bool bOK = EventsDataHub->unpackEventsFromByteArray(ba);
    if (!bOK)
        abort("Failed to set events from the binary buffer.");
    else Server.sendOK();
}

bool AServer_SI::SaveBufferToFile(const QString &fileName)
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

void AServer_SI::SendProgressReport(int percents)
{
    Server.ReplyProgress(percents);
}

void AServer_SI::SetAcceptExternalProgressReport(bool flag)
{
    Server.SetCanRetranslateProgress(flag);
}
