#include "amonitorconfig.h"
#include "ajsontools.h"

#include <QJsonObject>

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
   json["bDirect"] = bDirect;
   json["bIndirect"] = bIndirect;

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
   json["energyUnitsInHist"] = energyUnitsInHist;
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
    parseJson(json, "bPrimary", bDirect); // compatibility
    parseJson(json, "bDirect",  bDirect);
    parseJson(json, "bSecondary", bIndirect);
    parseJson(json, "bIndirect",  bIndirect);

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
    parseJson(json, "energyUnitsInHist", energyUnitsInHist);
}
