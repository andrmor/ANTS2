#ifndef PMLOCALCACHE_H
#define PMLOCALCACHE_H

#include <QVector>

class PMsensor;
class LRF2;
class LRFaxial;
class LRFcAxial;
class LRFxy;
class LRFxyz;
class LRFcomposite;
class LRFaxial3d;
class LRFsliced3D;
struct AReconRecord;
struct AScanRecord;
class ALrfFitSettings;
//template<typename T> class QVector;

class SensorLocalCache
{
public:
    SensorLocalCache(int numGoodEvents, bool fDataRecon, bool fScaleByEnergy, const QVector<AReconRecord*> reconData,
                     const QVector<AScanRecord*> *scan, const QVector< QVector <float> > *events, ALrfFitSettings* LRFsettings);

    ~SensorLocalCache();

    void uncacheGroup();
    bool cacheGroup(const std::vector<PMsensor> *sensors, bool adjustGains, int igrp_);
    void calcRelativeGains2D(int ngrid, unsigned int pmsCount);

    LRF2 *fitLRF(LRF2 *lrf) const;
    LRF2 *mkLRFaxial(int nodes, double *compr) const;
    LRF2 *mkLRFxy(int nodesx, int nodesy) const;
    LRF2 *mkLRFcomposite(int nodesr, int nodesxy, double *compr) const;
    LRF2 *mkLRFaxial3d(int nodesr, int nodesz, double *compr) const;
    LRF2 *mkLRFsliced3D(int nodesr, int nodesz) const;
    LRF2 *mkLRFxyz(int nodesr, int nodesz) const;

    void expandDomain(double fraction);

    const double *getGains() const { return gains; }

    ALrfFitSettings* LRFsettings;
    //bool fUseGrid;
    //bool fFitError;
    //bool fFitOnlyLast; //for composite

private:
    int igrp; // Current Group ID  - needed for debugging (VS 20/10/14)
    int numGoodEvents, dataSize;
    const QVector<float> * const *goodEvents;
    const double * const *r;
    const double *factors;
    double *xx, minx, maxx;
    double *yy, miny, maxy;
    double *zz, minz, maxz;
    double *sigsig;
    double *gains;
    double maxr, maxr2;
};

#endif // PMLOCALCACHE_H
