#ifndef AREMOTESERVERRECORD_H
#define AREMOTESERVERRECORD_H

#include <QString>
#include <QJsonObject>
#include <QByteArray>

class AWebSocketSession;
class QThread;

class ARemoteServerRecord
{
public:
    ARemoteServerRecord(QString Name, QString IP, int Port) : Name(Name), IP(IP), Port(Port) {}
    ARemoteServerRecord() {}

    enum ServerStatus {Unknown = 0, Connecting, Alive, Dead, Busy};

    ServerStatus Status = Unknown;

    QString Name = "_name_";
    QString IP = "000.000.000.000";
    int     Port = 1234;
    bool    bEnabled = true;
    double  SpeedFactor = 1.0;
    int     NumThreads_Possible = 1;

    // all properties below are runtime-only
    int     NumThreads_Allocated = 0;
    int     Progress = 0;
    int     AntsServerPort = -1;
    QString AntsServerTicket;
    QString FileName; //local file where sim tree is saved
    int     EventsFrom;
    int     EventsTo;
    QByteArray ByteArrayToSend;
    QByteArray ByteArrayReceived;
    qint64  TimeElapsed; //for rating

    QString Error;

public:
    const QJsonObject WriteToJson();
    void              ReadFromJson(const QJsonObject& json);
};

#endif // AREMOTESERVERRECORD_H
