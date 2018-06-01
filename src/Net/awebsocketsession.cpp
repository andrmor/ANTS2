#include "awebsocketsession.h"

#include <QWebSocket>
#include <QElapsedTimer>
#include <QApplication>
#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

AWebSocketSession::AWebSocketSession() : QObject()
{
    socket = new QWebSocket();
    QObject::connect(socket, &QWebSocket::connected, this, &AWebSocketSession::onConnect);
    QObject::connect(socket, &QWebSocket::disconnected, this, &AWebSocketSession::onDisconnect);
    QObject::connect(socket, &QWebSocket::textMessageReceived, this, &AWebSocketSession::onTextMessageReceived);
    QObject::connect(socket, &QWebSocket::binaryMessageReceived, this, &AWebSocketSession::onBinaryMessageReceived);
}

AWebSocketSession::~AWebSocketSession()
{
    socket->deleteLater();
}

bool AWebSocketSession::connect(const QString &Url)
{
    fExternalAbort = false;
    Error.clear();
    TextReply.clear();
    BinaryReply.clear();

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

bool AWebSocketSession::confirmSendPossible()
{
    Error.clear();
    TextReply.clear();

    if (State != Connected)
    {
        Error = "Not connected to server";
        return false;
    }
    return true;
}

bool AWebSocketSession::waitForReply()
{
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

bool AWebSocketSession::sendText(const QString &message)
{
    if ( !confirmSendPossible() ) return false;

    socket->sendTextMessage(message);

    return waitForReply();
}

bool AWebSocketSession::sendJson(const QJsonObject &json)
{
    if ( !confirmSendPossible() ) return false;

    QJsonDocument doc(json);
    QByteArray ba = doc.toBinaryData();

    socket->sendBinaryMessage(ba);

    return waitForReply();
}

bool AWebSocketSession::sendFile(const QString &fileName)
{
    if ( !confirmSendPossible() ) return false;

    QFile file(fileName);
    if ( !file.open(QIODevice::ReadOnly) )
    {
        Error = "Cannot open file: ";
        Error += fileName;
        return false;
    }
    QByteArray ba = file.readAll();

    socket->sendBinaryMessage(ba);

    return waitForReply();
}

void AWebSocketSession::clearReply()
{
    TextReply.clear();
    BinaryReply.clear();
}

void AWebSocketSession::externalAbort()
{
    State = Aborted;
    socket->abort();
    fExternalAbort = true; //paranoic
}

void AWebSocketSession::onConnect()
{
    qDebug() << "Connected to server, checking busy status...";
    bWaitForAnswer = true;
}

void AWebSocketSession::onDisconnect()
{
    if (State == Aborted)
    {
        qDebug() << "Disconnect on abort";
        State = Idle;
    }
    else if (bWaitForAnswer)
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
        qDebug() << "Clinet disconnected";
        State = Idle; //paranoic
    }
}

void AWebSocketSession::onTextMessageReceived(const QString &message)
{
    qDebug() << "Text message received:" << message;
    TextReply = message;
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

void AWebSocketSession::onBinaryMessageReceived(const QByteArray &message)
{
    qDebug() << "Binary message received. Size = " << message.length();
    TextReply = "#binary";
    BinaryReply = message;
    bWaitForAnswer = false;
}
