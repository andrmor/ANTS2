#ifndef AREMOTESERVERRECORD_H
#define AREMOTESERVERRECORD_H

#include <QString>

class ARemoteServerRecord
{
public:
    ARemoteServerRecord(QString Name, QString IP, int Port) : Name(Name), IP(IP), Port(Port) {}
    ARemoteServerRecord() {}

    enum ServerStatus {Unknown = 0, Connecting, Alive, Dead};

    ServerStatus Status = Unknown;

    QString Name = "_name_";
    QString IP = "000.000.000.000";
    int     Port = 1234;
    bool    bEnabled = true;

    // all properties below are runtime-only
    int     NumThreads = -1;
    int     Progress = 0;

    bool    AntsServerAllocated = false;
    int     AntsServerPort = -1;
    QString AntsServerTicket = "---";

    QString Error;
};

#endif // AREMOTESERVERRECORD_H
