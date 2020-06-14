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

    bool bOnlyPhotons = true;

    AGeneralSimSettings   genSimSet;
    APhotonSimSettings    photSimSet;
    AParticleSimSettings  partSimSet;

    //runtime


};

#endif // ASIMSETTINGS_H
