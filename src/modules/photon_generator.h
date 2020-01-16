#ifndef PHOTON_GENERATOR_H
#define PHOTON_GENERATOR_H

#include <QVector>

class DetectorClass;
class APhoton;
class GeneralSimSettings;
class ASimulationStatistics;
class AOneEvent;
class TRandom2;

class Photon_Generator
{
public:
    explicit Photon_Generator(const DetectorClass & Detector, TRandom2* RandGen);
    ~Photon_Generator();

    void GenerateDirectionPrimary(APhoton *Photon);
    void GenerateDirectionSecondary(APhoton *Photon);
    void GenerateWave(APhoton *Photon, int materialId);
    void GenerateTime(APhoton *Photon, int materialId);

    void configure(const GeneralSimSettings *simSet, ASimulationStatistics* detStat) {SimSet = simSet; DetStat = detStat;}

    void GenerateSignalsForLrfMode(int NumPhotons, double *r, AOneEvent* OneEvent);

    ASimulationStatistics * DetStat = nullptr;
    const GeneralSimSettings * SimSet = nullptr;

private:
    const DetectorClass & Detector;
    TRandom2 * RandGen = nullptr;
};

#endif // PHOTON_GENERATOR_H
