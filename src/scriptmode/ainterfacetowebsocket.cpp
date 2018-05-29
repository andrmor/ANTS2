#include "ainterfacetowebsocket.h"
#include "awebsocketstandalonemessanger.h"
#include "awebsocketsession.h"

#include <QDebug>

AInterfaceToWebSocket::AInterfaceToWebSocket()
{
    standaloneMessenger = new AWebSocketStandaloneMessanger();
    sessionMessenger    = new AWebSocketSession();
}

AInterfaceToWebSocket::~AInterfaceToWebSocket()
{
    standaloneMessenger->deleteLater();
    sessionMessenger->deleteLater();
}

void AInterfaceToWebSocket::SetTimeout(int milliseconds)
{
    standaloneMessenger->setTimeout(milliseconds);
    sessionMessenger->setTimeout(milliseconds);
}

const QString AInterfaceToWebSocket::SendTextMessage(const QString &Url, const QVariant& message, bool WaitForAnswer)
{
   bool bOK = standaloneMessenger->sendTextMessage(Url, message, WaitForAnswer);

   if (!bOK)
   {
       abort(standaloneMessenger->getError());
       return "";
   }
   return standaloneMessenger->getReceivedMessage();
}

int AInterfaceToWebSocket::Ping(const QString &Url)
{
    int ping = standaloneMessenger->ping(Url);

    if (ping < 0)
    {
        abort(standaloneMessenger->getError());
        return -1;
    }
    return ping;
}

void AInterfaceToWebSocket::Connect(const QString &Url)
{
    bool bOK = sessionMessenger->connect(Url);
    if (!bOK)
        abort(sessionMessenger->getError());
}

void AInterfaceToWebSocket::Disconnect()
{
    sessionMessenger->disconnect();
}

const QVariant AInterfaceToWebSocket::SendText(const QString &message)
{
    bool bOK = sessionMessenger->sendText(message);
    if (bOK)
        return sessionMessenger->getAnswer();
    else
    {
        abort(sessionMessenger->getError());
        return 0;
    }
}
