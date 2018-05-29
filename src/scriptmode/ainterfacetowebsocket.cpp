#include "ainterfacetowebsocket.h"

#include <QWebSocketServer>
#include <QWebSocket>

#include <QApplication>
#include <QDebug>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QElapsedTimer>

AInterfaceToWebSocket::AInterfaceToWebSocket()
{
    ClientSocket = new QWebSocket();
    connect(ClientSocket, &QWebSocket::connected, this, &AInterfaceToWebSocket::onClientConnected);
    connect(ClientSocket, &QWebSocket::disconnected, this, &AInterfaceToWebSocket::onClientDisconnected);
    connect(ClientSocket, &QWebSocket::textMessageReceived, this, &AInterfaceToWebSocket::onTextMessageReceived);
}

AInterfaceToWebSocket::~AInterfaceToWebSocket()
{
    ClientSocket->deleteLater();
}

QString AInterfaceToWebSocket::SendTextMessage(const QString &Url, const QVariant& message, bool WaitForAnswer)
{
   if (State != Idle)
   {
       qDebug() << "Not ready for this yet....";
       exit(678);
   }

   qDebug() << "-->Attempting to send to Url:\n" << Url
            << "\n->Message:\n" << message
            << "\n<-";

   State = Sending;
   MessageToSend = variantToString(message);
   MessageReceived = "";
   bWaitForAnswer = WaitForAnswer;

   QElapsedTimer timer;
   timer.start();

   ClientSocket->open(QUrl(Url));   
   do
   {
       qApp->processEvents();
       if (timer.elapsed() > timeout)
       {
           ClientSocket->abort();
           State = Idle;
           abort("Timeout!");
           return "";
       }
   }
   while (State != Idle);

   ClientSocket->close();
   lastExchangeDuration = timer.elapsed();
   return MessageReceived;
}

int AInterfaceToWebSocket::Ping(QString Url)
{
  SendTextMessage(Url, "", true);
  return lastExchangeDuration;
}

void AInterfaceToWebSocket::onClientConnected()
{
    qDebug() << "ClientSocket connected";

    if (State == TryingToConnect)
    {
        //session mode
        //waiting for confirmation that server is not busy
    }
    else
    {
        //standalone
        ClientSocket->sendTextMessage(MessageToSend);
        if (!bWaitForAnswer) State = Idle;
        else State = WaitingForAnswer;
    }
}

void AInterfaceToWebSocket::onClientDisconnected()
{
    if (State == Connected)
    {
        if (State = TryingToConnect)
        {
            State = ConnectionFailed;
            return; //handled outside
        }
        else
            abort("Lost connection");
    }
    else
        State = Idle;
}

void AInterfaceToWebSocket::onTextMessageReceived(QString message)
{
    qDebug() << "Message received:" << message;
    MessageReceived = message;

    if (State == TryingToConnect)
    {
        //session mode
        if (MessageReceived.startsWith("Error"))
        {
            State = ConnectionFailed;
            bServerWasBusy = MessageReceived.contains("another client is already connected");
        }
        else
            State = Connected;
    }
    else if (State == Connected)
    {
        bWaitForAnswer = false;
    }
    else
    {
        //standalone
        State = Idle;
    }
}

QString AInterfaceToWebSocket::variantToString(const QVariant& val)
{
    QString type = val.typeName();

    QJsonValue jv;
    QString rep;
    if (type == "int")
      {
        jv = QJsonValue(val.toInt());
        rep = QString::number(val.toInt());
      }
    else if (type == "double")
      {
        jv = QJsonValue(val.toDouble());
        rep = QString::number(val.toDouble());
      }
    else if (type == "bool")
      {
        jv = QJsonValue(val.toBool());
        rep = val.toBool() ? "true" : "false";
      }
    else if (type == "QString")
      {
        jv = QJsonValue(val.toString());
        rep = val.toString();
      }
//    else if (type == "QVariantList")
//      {
//        QVariantList vl = val.toList();
//        QJsonArray ar = QJsonArray::fromVariantList(vl);
//        jv = ar;
//        rep = "-Array-";
//      }
    else if (type == "QVariantMap")
        {
          QVariantMap mp = val.toMap();
          QJsonObject json = QJsonObject::fromVariantMap(mp);
          QJsonDocument doc(json);
          rep = QString(doc.toJson(QJsonDocument::Compact));
        }

    return rep;
}

// -------------- persistent ---------------------
void AInterfaceToWebSocket::Connect(const QString &Url)
{
    if (State != Idle)
    {
        abort("Cannot connect: network module is not idle");
        return;
    }

    QElapsedTimer timer;
    timer.start();

    State = TryingToConnect;
    ClientSocket->open(QUrl(Url));

    //waiting for connection
    do
    {
        qApp->processEvents();
        if (timer.elapsed() > timeout)
        {
            ClientSocket->abort();
            State = Idle;
            abort("Connection timeout!");
            return;
        }
    }
    while (State == TryingToConnect);

    if (State == ConnectionFailed)
    {
        ClientSocket->close();
        State = Idle;
        QString err = (bServerWasBusy ? "Connection failed: another client is connected to the server" : "Connection failed");
        abort(err);
    }
}

void AInterfaceToWebSocket::Disconnect()
{
    if (State == Connected)
        ClientSocket->close();
    //status change on actual disconnect! see slot onClientDisconnected()
}

const QVariant AInterfaceToWebSocket::SendText(const QString &message, bool WaitForAnswer)
{
    if (State != Connected)
    {
        abort("Not connected to server");
        return 0;
    }

    ClientSocket->sendTextMessage(message);
    if (WaitForAnswer)
    {
        bWaitForAnswer = true;

        QElapsedTimer timer;
        timer.start();
        do
        {
            qApp->processEvents();
            if (timer.elapsed() > timeout)
            {
                ClientSocket->abort();
                State = Idle;
                abort("Timeout!");
                return "";
            }
        }
        while (bWaitForAnswer);

        return MessageReceived;
    }
    return QVariant();
}
