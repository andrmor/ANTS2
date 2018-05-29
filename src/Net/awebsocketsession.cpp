#include "awebsocketsession.h"

#include <QWebSocket>
#include <QElapsedTimer>
#include <QApplication>
#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>

AWebSocketSession::AWebSocketSession() : QObject()
{
    socket = new QWebSocket();
    QObject::connect(socket, &QWebSocket::connected, this, &AWebSocketSession::onConnect);
    QObject::connect(socket, &QWebSocket::disconnected, this, &AWebSocketSession::onDisconnect);
    QObject::connect(socket, &QWebSocket::textMessageReceived, this, &AWebSocketSession::onTextMessageReceived);
}

AWebSocketSession::~AWebSocketSession()
{
    socket->deleteLater();
}

bool AWebSocketSession::connect(const QString &Url)
{
    Error.clear();
    Answer.clear();

    if (State != Idle)
    {
        Error = "Cannot connect: not idle";
        return false;
    }

    QElapsedTimer timer;
    timer.start();

    State = Connecting;
    socket->open( QUrl(Url) );

    //waiting for connection
    do
    {
        qApp->processEvents();
        if (timer.elapsed() > timeout || fExternalAbort)
        {
            socket->abort();
            State = Idle;
            Error = "Connection timeout!";
            return false;
        }
    }
    while (State == Connecting);

    if (Error.isEmpty()) return true;
    else
    {
        socket->close();
        State = Idle;
        return false;
    }
}

void AWebSocketSession::disconnect()
{
    if (State == Connected)
        socket->close();
    //status change on actual disconnect! see slot onClientDisconnected()
}

int AWebSocketSession::ping()
{
    if (State == Connected)
    {
        bool bOK = sendText("");
        if (bOK)
            return TimeMs;
        else
            return false;
    }
    else
    {
        Error = "Not connected";
        return -1;
    }
}

bool AWebSocketSession::sendText(const QString &message)
{
    Error.clear();

    if (State != Connected)
    {
        Error = "Not connected to server";
        return false;
    }

    socket->sendTextMessage(message);

    bWaitForAnswer = true;
    QElapsedTimer timer;
    timer.start();
    do
    {
        qApp->processEvents();
        if (timer.elapsed() > timeout || fExternalAbort)
        {
            socket->abort();
            State = Idle;
            Error = "Timeout!";
            return false;
        }
    }
    while (bWaitForAnswer);
    TimeMs = timer.elapsed();

    return true;
}

void AWebSocketSession::onConnect()
{
    qDebug() << "Connected to server, checking busy status...";
}

void AWebSocketSession::onDisconnect()
{
    if (WaitingForAnswer)
    {
        qDebug() << "Abnormal disconnect detected";
        if (State == Connecting)
            Error = "Server disconnected before confirming connection";
        else
            Error = "Server disconnected before reply";
        State = ConnectionFailed;
    }
    else
    {
        qDebug() << "Normal disconnect detected";
        State = Idle; //paranoic
    }
}

void AWebSocketSession::onTextMessageReceived(const QString &message)
{
    qDebug() << "Text message received:" << message;
    Answer = message;
    bWaitForAnswer = false;

    if (State == Connecting)
    {
        if (message.startsWith("Error"))
        {
            State = ConnectionFailed;
            Error = "Connection failed";
            if ( message.contains("another client is already connected") )
                Error += ": another client is connected to the server";
        }
        else
            State = Connected;
    }
}
