#ifndef ASIMULATIONSTATISTICS_H
#define ASIMULATIONSTATISTICS_H

#include "aphotonhistorylog.h"

#include <QVector>
#include <QSet>

#include "TString.h"

class TH1I;
class TH1D;

class ASimulationStatistics
{
public:
    ASimulationStatistics(const TString nameID = "");
    ~ASimulationStatistics();

    void initialize(int nBins = 0, int waveNodes = 0); //0 - default (100) or previously set value will be used
    bool isEmpty();

    void registerWave(int iWave);
    void registerTime(double Time);
    void registerAngle(double CosAngle);
    void registerNumTrans(int NumTransitions);

    //since every thread has its own statistics container:
    void AppendSimulationStatistics(const ASimulationStatistics *from);

    //read-outs
    TH1I* getWaveSpectrum() {return WaveSpectrum;}
    TH1D* getTimeSpectrum() {return TimeSpectrum;}
    TH1I* getCosAngleSpectrum() {return CosAngleSpectrum;}
    TH1I* getTransitionSpectrum() {return TransitionSpectrum;}

    //photon loss statistics
    long Absorbed, OverrideLoss, HitPM, HitDummy, Escaped, LossOnGrid, TracingSkipped, MaxCyclesReached, GeneratedOutsideGeometry;

    //statistics for optical processes
    long FresnelTransmitted, FresnelReflected, BulkAbsorption, Rayleigh, Reemission; //general bulk
    long OverrideBack, OverrideForward; //general override
      //specific overrides
    long OverrideSimplisticAbsorption, OverrideSimplisticReflection, OverrideSimplisticScatter; //simplistic
    long OverrideFSNPabs, OverrideFSNlambert, OverrideFSNPspecular; //FSNP
    long OverrideMetalAbs, OverrideMetalReflection; //on metal
    long OverrideClaudioAbs, OverrideClaudioSpec, OverrideClaudioLamb; //Claudio's
    long OverrideWLSabs, OverrideWLSshift;

    //only affects script unit "photon" tracing!
    QVector< QVector <APhotonHistoryLog> > PhotonHistoryLog;    
    QSet<int> MustNotInclude_Processes;   //v.fast
    QVector<int> MustInclude_Processes;   //slow
    QSet<QString> MustNotInclude_Volumes; //fast
    QVector<QString> MustInclude_Volumes; //v.slow


private:
    TH1I* WaveSpectrum;
    TH1D* TimeSpectrum;
    TH1I* CosAngleSpectrum;
    TH1I* TransitionSpectrum;

    int numBins;
    TString NameID;

    double WaveFrom, WaveTo;
    int WaveNodes;

    long countPhotons();
};

#endif // ASIMULATIONSTATISTICS_H
