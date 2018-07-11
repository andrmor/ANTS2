#include "aremoteserverrecord.h"

#include "ajsontools.h"

const QJsonObject ARemoteServerRecord::WriteToJson()
{
    QJsonObject json;

    json["Name"] = Name;
    json["IP"] = IP;
    json["Port"] = Port;
    json["Enabled"] = bEnabled;

    return json;
}

void ARemoteServerRecord::ReadFromJson(const QJsonObject &json)
{
    parseJson(json, "Name", Name);
    parseJson(json, "IP", IP);
    parseJson(json, "Port", Port);
    parseJson(json, "Enabled", bEnabled);
}
