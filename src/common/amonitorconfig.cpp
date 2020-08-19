#include "amonitorconfig.h"
#include "ajsontools.h"
#include "ageoconsts.h"

#include <QJsonObject>
#include <QDebug>

void AMonitorConfig::writeToJson(QJsonObject &json) const
{
   json["shape"] = shape;
   json["size1"] = size1;
   json["size2"] = size2;
   json["dz"] = dz;

   if (!str2size1.isEmpty()) json["str2size1"] = str2size1;
   if (!str2size2.isEmpty()) json["str2size2"] = str2size2;

   json["PhotonOrParticle"] = PhotonOrParticle;
   json["bUpper"] = bUpper;
   json["bLower"] = bLower;
   json["bStopTracking"] = bStopTracking;

   json["ParticleIndex"] = ParticleIndex;
   json["bPrimary"]   = bPrimary;
   json["bSecondary"] = bSecondary;
   json["bDirect"]    = bDirect;
   json["bIndirect"]  = bIndirect;

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

void AMonitorConfig::readFromJson(const QJsonObject & json)
{
    parseJson(json, "shape", shape);
    parseJson(json, "size1", size1);
    parseJson(json, "size2", size2);
    parseJson(json, "dz", dz);

    str2size1.clear();
    parseJson(json, "str2size1", str2size1);
    str2size2.clear();
    parseJson(json, "str2size2", str2size2);

    parseJson(json, "PhotonOrParticle", PhotonOrParticle);
    parseJson(json, "bUpper", bUpper);
    parseJson(json, "bLower", bLower);
    parseJson(json, "bStopTracking", bStopTracking);

    parseJson(json, "ParticleIndex", ParticleIndex);
    parseJson(json, "bPrimary",   bPrimary);
    parseJson(json, "bSecondary", bSecondary);
    parseJson(json, "bDirect",    bDirect);
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

    updateFromGeoConstants();
}

void AMonitorConfig::updateFromGeoConstants()
{
    const AGeoConsts & GC = AGeoConsts::getConstInstance();
    QString errorStr;

    bool ok;
    ok = GC.updateParameter(errorStr, str2size1, size1); if (!ok) {qWarning() << errorStr;}
    ok = GC.updateParameter(errorStr, str2size2, size2); if (!ok) {qWarning() << errorStr;}
}
