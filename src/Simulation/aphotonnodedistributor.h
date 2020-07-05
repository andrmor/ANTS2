#ifndef APHOTONNODEDISTRIBUTOR_H
#define APHOTONNODEDISTRIBUTOR_H

#include <QVector>
#include <vector>
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
    std::vector<double> CumulativeProb;
    double SumProb = 1.0;

    bool calculateCumulativeProbabilities();
};

#endif // APHOTONNODEDISTRIBUTOR_H
