#include "awebserverinterface.h"
#include "awebsocketsessionserver.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

AWebServerInterface::AWebServerInterface(AWebSocketSessionServer &Server) :
    AScriptInterface(), Server(Server) {}

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
