#ifndef PHOTON_GENERATOR_H
#define PHOTON_GENERATOR_H

#include <QObject>
#include <QVector>

class DetectorClass;
class APhoton;
class GeneralSimSettings;
class ASimulationStatistics;
class AOneEvent;
class TRandom2;

class Photon_Generator : public QObject
{
    Q_OBJECT
public:
    explicit Photon_Generator(const DetectorClass* Detector, TRandom2* RandGen, QObject *parent = 0);
    ~Photon_Generator();

    void GenerateDirectionPrimary(APhoton *Photon);
    void GenerateDirectionSecondary(APhoton *Photon);
    void GenerateWave(APhoton *Photon, int materialId);
    void GenerateTime(APhoton *Photon, int materialId);

    void configure(const GeneralSimSettings *simSet, ASimulationStatistics* detStat) {SimSet = simSet; DetStat = detStat;}

    void GenerateSignalsForLrfMode(int NumPhotons, double *r, AOneEvent* OneEvent);

    ASimulationStatistics* DetStat;
    const GeneralSimSettings* SimSet;

private:
    const DetectorClass *Detector;
    TRandom2* RandGen;
};

#endif // PHOTON_GENERATOR_H
