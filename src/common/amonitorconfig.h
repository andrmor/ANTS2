#ifndef AMONITORCONFIG_H
#define AMONITORCONFIG_H

class QJsonObject;

class AMonitorConfig
{
public:
    AMonitorConfig();

    int shape; // 0 - rectangular; 1 - round
    double size1, size2; //xy sizes or radius
    double dz;  // constant! - half-thickness

    int  PhotonOrParticle;  //0 - photon, 1 - particle
    bool bUpper; // direction: z>0 is upper, z<0 is lower
    bool bLower;
    bool bStopTracking;

    //particle specific
    int ParticleIndex;
    bool bPrimary;
    bool bSecondary;

    //histogram properties
    int xbins, ybins;
    int timeBins; double timeFrom; double timeTo;
    int angleBins; double angleFrom; double angleTo;
    int waveBins; double waveFrom; double waveTo;
    int energyBins; double energyFrom; double energyTo;

    void writeToJson(QJsonObject &json);
    void readFromJson(QJsonObject &json);
};

#endif // AMONITORCONFIG_H
