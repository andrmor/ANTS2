#include "awebsocketstandalonemessanger.h"

#include <QWebSocket>
#include <QElapsedTimer>
#include <QApplication>
#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>

AWebSocketStandaloneMessanger::AWebSocketStandaloneMessanger() : QObject()
{
    socket = new QWebSocket();
    connect(socket, &QWebSocket::connected, this, &AWebSocketStandaloneMessanger::onConnect);
    connect(socket, &QWebSocket::disconnected, this, &AWebSocketStandaloneMessanger::onDisconnect);
    connect(socket, &QWebSocket::textMessageReceived, this, &AWebSocketStandaloneMessanger::onTextMessageReceived);
}

AWebSocketStandaloneMessanger::~AWebSocketStandaloneMessanger()
{
    socket->deleteLater();
}

bool AWebSocketStandaloneMessanger::sendTextMessage(const QString &Url, const QVariant &message, bool WaitForAnswer)
{
    Error.clear();
    MessageReceived.clear();
    fExternalAbort = false;

    if (State == WaitingForAnswer)
    {
        Error = "Already waiting for answer";
        return false;
    }

    qDebug() << "-->Attempting to send to Url:\n" << Url
             << "\n->Message:\n" << message
             << "\n<-";

    State = Connecting;
    MessageToSend = variantToString(message);
    bWaitForAnswer = WaitForAnswer;

    QElapsedTimer timer;
    timer.start();

    socket->open( QUrl(Url) );
    do
    {
        qApp->processEvents();
        if (timer.elapsed() > timeout || fExternalAbort)
        {
            socket->abort();
            State = Idle;
            Error = "Timeout";
            return false;
        }
    }
    while (State != Idle);

    socket->close();
    TimeMs = timer.elapsed();

    return true;
}

int AWebSocketStandaloneMessanger::ping(const QString& Url)
{
    bool bOK = sendTextMessage(Url, "", true);

    if (bOK) return TimeMs;
    else     return -1;
}

void AWebSocketStandaloneMessanger::externalAbort()
{
    State = Aborted;
    socket->abort();
    fExternalAbort = true;
}

void AWebSocketStandaloneMessanger::onConnect()
{
    qDebug() << "Connected";

    socket->sendTextMessage(MessageToSend);
    if (!bWaitForAnswer) State = Idle;
    else State = WaitingForAnswer;
}

void AWebSocketStandaloneMessanger::onDisconnect()
{
    if (State = Aborted)
    {
        qDebug() << "compatibility socket: Disconnect on abort";
        State = Idle;
    }
    else if (bWaitForAnswer)
    {
        Error = "Disconnected";
        State = Idle;
    }
}

void AWebSocketStandaloneMessanger::onTextMessageReceived(const QString &message)
{
    qDebug() << "Message received:" << message;
    MessageReceived = message;

    State = Idle;
}

QString AWebSocketStandaloneMessanger::variantToString(const QVariant &val)
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
