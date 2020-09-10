#ifndef PHOTON_GENERATOR_H
#define PHOTON_GENERATOR_H

class DetectorClass;
class APhoton;
class AGeneralSimSettings;
class ASimulationStatistics;
class AOneEvent;
class TRandom2;

class Photon_Generator
{
public:
    explicit Photon_Generator(const DetectorClass & Detector, TRandom2 & RandGen);

    void GenerateDirection(APhoton *Photon) const;
    void GenerateWave(APhoton *Photon, int materialId) const;
    void GenerateTime(APhoton *Photon, int materialId) const;

    void configure(const AGeneralSimSettings *simSet, ASimulationStatistics* detStat) {SimSet = simSet; DetStat = detStat;}

    void GenerateSignalsForLrfMode(int NumPhotons, double *r, AOneEvent* OneEvent);

    ASimulationStatistics * DetStat = nullptr;
    const AGeneralSimSettings * SimSet = nullptr;

private:
    const DetectorClass & Detector;
    TRandom2 & RandGen;
};

#endif // PHOTON_GENERATOR_H
