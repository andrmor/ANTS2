#ifndef APHOTONNODEDISTRIBUTOR_H
#define APHOTONNODEDISTRIBUTOR_H

#include <QVector>
#include <QString>
#include "a3dposprob.h"

class APhotonSim_SpatDistSettings;
class APhoton;

class APhotonNodeDistributor
{
public:
    bool init(const APhotonSim_SpatDistSettings & Settings);
    void releaseResources();

    void apply(APhoton & photon, double * center, double rnd) const;

    QString ErrorString;

private:
    QVector<A3DPosProb> Matrix;

    bool calculateCumulativeProbabilities();
};

#endif // APHOTONNODEDISTRIBUTOR_H
