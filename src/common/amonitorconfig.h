#ifndef AMONITORCONFIG_H
#define AMONITORCONFIG_H

class QJsonObject;

class AMonitorConfig
{
public:
    int    shape = 0;             // 0 - rectangular; 1 - round
    double size1 = 100.0;
    double size2 = 100.0;         //xy sizes or radius
    double dz = 0.001;            // constant! - half-thickness

    int    PhotonOrParticle = 0;  //0 - photon, 1 - particle
    bool   bUpper = true;         // direction: z>0 is upper, z<0 is lower
    bool   bLower = true;
    bool   bStopTracking = false;

    //particle specific
    int    ParticleIndex = 0;
    bool   bPrimary = true;
    bool   bSecondary = true;
    bool   bDirect = true;
    bool   bIndirect = true;

    //histogram properties
    int    xbins = 100;
    int    ybins = 100;
    int    timeBins = 100;
    double timeFrom = 0;
    double timeTo = 0;
    int    angleBins = 100;
    double angleFrom = 0;
    double angleTo = 90.0;
    int    waveBins = 150;
    double waveFrom = -1;
    double waveTo = 148;
    int    energyBins = 100;
    double energyFrom = 0;
    double energyTo = 0;
    int    energyUnitsInHist = 2; // 0,1,2,3 -> meV, eV, keV, MeV;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
};

#endif // AMONITORCONFIG_H
