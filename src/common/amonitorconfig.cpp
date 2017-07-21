#include "amonitorconfig.h"
#include "ajsontools.h"

#include <QJsonObject>

AMonitorConfig::AMonitorConfig()
{
    shape = 0;
    size1 = size2 = 100.0;
    dz = 0.001;  // constant! - half-thickness

    PhotonOrParticle = 0;
    bUpper = true;
    bLower = true;
    bStopTracking = false;

    ParticleIndex = 0;
    bPrimary = true;
    bSecondary = false;

    xbins = ybins = timeBins = angleBins = waveBins = energyBins = 100;
    timeFrom = timeTo = angleFrom = waveFrom = waveTo = energyFrom = energyTo = 0;
    angleTo = 90.0;
}

void AMonitorConfig::writeToJson(QJsonObject &json)
{
   json["shape"] = shape;
   json["size1"] = size1;
   json["size2"] = size2;
   json["dz"] = dz;

   json["PhotonOrParticle"] = PhotonOrParticle;
   json["bUpper"] = bUpper;
   json["bLower"] = bLower;
   json["bStopTracking"] = bStopTracking;

   json["ParticleIndex"] = ParticleIndex;
   json["bPrimary"] = bPrimary;
   json["bSecondary"] = bSecondary;

   json["xbins"] = xbins;
   json["ybins"] = ybins;

   json["timeBins"] = timeBins;
   json["timeFrom"] = timeFrom;
   json["timeTo"] = timeTo;

   json["angleBins"] = angleBins;
   json["angleFrom"] = angleFrom;
   json["angleTo"] = angleTo;

   json["waveBins"] = waveBins;
   json["waveFrom"] = waveFrom;
   json["waveTo"] = waveTo;

   json["energyBins"] = energyBins;
   json["energyFrom"] = energyFrom;
   json["energyTo"] = energyTo;
}

void AMonitorConfig::readFromJson(QJsonObject &json)
{
    parseJson(json, "shape", shape);
    parseJson(json, "size1", size1);
    parseJson(json, "size2", size2);
    parseJson(json, "dz", dz);

    parseJson(json, "PhotonOrParticle", PhotonOrParticle);
    parseJson(json, "bUpper", bUpper);
    parseJson(json, "bLower", bLower);
    parseJson(json, "bStopTracking", bStopTracking);

    parseJson(json, "ParticleIndex", ParticleIndex);
    parseJson(json, "bPrimary", bPrimary);
    parseJson(json, "bSecondary", bSecondary);

    parseJson(json, "xbins", xbins);
    parseJson(json, "ybins", ybins);

    parseJson(json, "timeBins", timeBins);
    parseJson(json, "timeFrom", timeFrom);
    parseJson(json, "timeTo", timeTo);

    parseJson(json, "angleBins", angleBins);
    parseJson(json, "angleFrom", angleFrom);
    parseJson(json, "angleTo", angleTo);

    parseJson(json, "waveBins", waveBins);
    parseJson(json, "waveFrom", waveFrom);
    parseJson(json, "waveTo", waveTo);

    parseJson(json, "energyBins", energyBins);
    parseJson(json, "energyFrom", energyFrom);
    parseJson(json, "energyTo", energyTo);
}
