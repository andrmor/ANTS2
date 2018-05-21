#ifndef PHOTON_GENERATOR_H
#define PHOTON_GENERATOR_H

#include <QObject>
#include <QVector>

class DetectorClass;
class APhoton;
class GeneralSimSettings;
class ASimulationStatistics;
class AOneEvent;

class Photon_Generator : public QObject
{
    Q_OBJECT
public:
    explicit Photon_Generator(const DetectorClass* Detector, QObject *parent = 0);
    ~Photon_Generator();

    void GenerateDirectionPrimary(APhoton *Photon);
    void GenerateDirectionSecondary(APhoton *Photon);
    void GenerateWaveTime(APhoton *Photon, int materialId);

    void configure(const GeneralSimSettings *simSet, ASimulationStatistics* detStat) {SimSet = simSet; DetStat = detStat;}

    void GenerateSignalsForLrfMode(int NumPhotons, double *r, AOneEvent* OneEvent);

    ASimulationStatistics* DetStat;
    const GeneralSimSettings* SimSet;

private:
    const DetectorClass *Detector;
};

#endif // PHOTON_GENERATOR_H
