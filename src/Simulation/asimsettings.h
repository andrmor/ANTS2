#ifndef ASIMSETTINGS_H
#define ASIMSETTINGS_H

#include "ageneralsimsettings.h"
#include "aphotonsimsettings.h"
#include "aparticlesimsettings.h"

class QJsonObject;

class ASimSettings
{
public:

    void writeToJson(QJsonObject & json) const;
    bool readFromJson(const QJsonObject & json);

    AGeneralSimSettings   genSimSet;
    APhotonSimSettings    photSimSet;
    AParticleSimSettings  partSimSet;

};

#endif // ASIMSETTINGS_H
