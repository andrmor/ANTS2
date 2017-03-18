#include "ainterfacetowebsocket.h"

#include <QWebSocketServer>
#include <QWebSocket>

#include <QApplication>
#include <QDebug>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>

AInterfaceToWebSocket::AInterfaceToWebSocket()
{
    timeout = 3000;
    ClientSocket = new QWebSocket();
    connect(ClientSocket, &QWebSocket::connected, this, &AInterfaceToWebSocket::onClientConnected);
    connect(ClientSocket, &QWebSocket::textMessageReceived, this, &AInterfaceToWebSocket::onTextMessageReceived);
    State = Idle;
}

AInterfaceToWebSocket::~AInterfaceToWebSocket()
{
    ClientSocket->deleteLater();
}
#include <QElapsedTimer>
QString AInterfaceToWebSocket::SendTextMessage(QString Url, QVariant message, bool WaitForAnswer)
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
   fWaitForAnswer = WaitForAnswer;

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
    ClientSocket->sendTextMessage(MessageToSend);

    if (!fWaitForAnswer) State = Idle;
    else State = WaitingForAnswer;
}

void AInterfaceToWebSocket::onTextMessageReceived(QString message)
{
    qDebug() << "Message received:" << message;
    MessageReceived = message;
    State = Idle;
}

QString AInterfaceToWebSocket::variantToString(QVariant val)
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
