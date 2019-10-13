#ifndef ASIMULATIONSTATISTICS_H
#define ASIMULATIONSTATISTICS_H

#include "aphotonhistorylog.h"

#include <QVector>
#include <QSet>

#include "TString.h"

class TH1I;
class TH1D;
class AMonitor;
class AGeoObject;

class ASimulationStatistics
{
public:
    ASimulationStatistics(const TString nameID = "");
    ~ASimulationStatistics();

    void initialize(QVector<const AGeoObject*> monitorRecords = QVector<const AGeoObject*>(), int nBins = 0, int waveNodes = 0); //0 - default (100) or previously set value will be used

    void clearAll();

    bool isEmpty();  //need update - particles added, so photons-only test is not valid

    void registerWave(int iWave);
    void registerTime(double Time);
    void registerAngle(double angle);
    void registerNumTrans(int NumTransitions);

	// WATER'S FUNCTIONS
	
	void clearPhLogEntry(int iPhoton);
	QVector<APhotonHistoryLog> takePhLogEntry(int iPhoton);
	
	void clearHistory();

    //since every thread has its own statistics container:
    void AppendSimulationStatistics(ASimulationStatistics *from);

    //read-outs
    TH1D* getWaveSpectrum() {return WaveSpectrum;}
    TH1D* getTimeSpectrum() {return TimeSpectrum;}
    TH1D* getAngularDistr() {return AngularDistr;}
    TH1D* getTransitionSpectrum() {return TransitionSpectrum;}

    //photon loss statistics
    long Absorbed, OverrideLoss, HitPM, HitDummy, Escaped, LossOnGrid, TracingSkipped, MaxCyclesReached, GeneratedOutsideGeometry, KilledByMonitor;

    //statistics for optical processes
    long FresnelTransmitted, FresnelReflected, BulkAbsorption, Rayleigh, Reemission; //general bulk
    long OverrideBack, OverrideForward; //general override. Note that OverrideLoss is already defined

    //only affects script unit "photon" tracing!
    QVector< QVector <APhotonHistoryLog> > PhotonHistoryLog;    
    QSet<int> MustNotInclude_Processes;   //v.fast
    QVector<int> MustInclude_Processes;   //slow
    QSet<QString> MustNotInclude_Volumes; //fast
    QVector<QString> MustInclude_Volumes; //v.slow

    //only for optical override tester!
    long wavelengthChanged = 0;
    long timeChanged = 0;

    QVector<AMonitor*> Monitors;

private:
    TH1D* WaveSpectrum;
    TH1D* TimeSpectrum;
    TH1D* AngularDistr;
    TH1D* TransitionSpectrum;

    int numBins;
    TString NameID;

    double WaveFrom, WaveTo;
    int WaveNodes;


    long countPhotons();
    void clearMonitors();
};

#endif // ASIMULATIONSTATISTICS_H
