#include "asimulationstatistics.h"
#include "ageoobject.h"
#include "amonitor.h"
#include "aroothistappenders.h"

#include <QDebug>

#include "TH1I.h"
#include "TH1D.h"

ASimulationStatistics::ASimulationStatistics(const TString nameID)
{
    WaveSpectrum = 0;
    TimeSpectrum = 0;
    AngularDistr = 0;
    TransitionSpectrum = 0;
    WaveNodes = 0;
    numBins = 100;

    NameID = nameID;
    //initialize();
}

ASimulationStatistics::~ASimulationStatistics()
{
    clearAll();
}

void ASimulationStatistics::clearAll()
{
    if (WaveSpectrum) delete WaveSpectrum; WaveSpectrum = 0;
    if (TimeSpectrum) delete TimeSpectrum; TimeSpectrum = 0;
    if (AngularDistr) delete AngularDistr; AngularDistr = 0;
    if (TransitionSpectrum) delete TransitionSpectrum; TransitionSpectrum = 0;
    clearMonitors();
}

void ASimulationStatistics::initialize(QVector<const AGeoObject*> monitorRecords, int nBins, int waveNodes)
{    
    if (nBins != 0) numBins = nBins;
    if (waveNodes != 0) WaveNodes = waveNodes;

    if (WaveSpectrum) delete WaveSpectrum;
    if (WaveNodes != 0)
       WaveSpectrum = new TH1D("iWaveSpectrum"+NameID,"WaveIndex spectrum", WaveNodes, 0, WaveNodes);
    else
       WaveSpectrum = new TH1D("iWaveSpectrum"+NameID,"WaveIndex spectrum",numBins,0,-1);

    if (TimeSpectrum) delete TimeSpectrum;
    TimeSpectrum = new TH1D("TimeSpectrum"+NameID, "Time spectrum",numBins,0,-1);

    if (AngularDistr) delete AngularDistr;
    AngularDistr = new TH1D("AngularDistr"+NameID, "cosAngle spectrum", numBins, 0, 90.0);

    if (TransitionSpectrum) delete TransitionSpectrum;
    TransitionSpectrum = new TH1D("TransitionsSpectrum"+NameID, "Transitions", numBins, 0,-1);

    Absorbed = OverrideLoss = HitPM = HitDummy = Escaped = LossOnGrid = TracingSkipped = MaxCyclesReached = GeneratedOutsideGeometry = KilledByMonitor = 0;

    FresnelTransmitted = FresnelReflected = BulkAbsorption = Rayleigh = Reemission = 0;
    OverrideForward = OverrideBack = 0;

    PhotonHistoryLog.clear();
    PhotonHistoryLog.squeeze();

    clearMonitors();
    if (!monitorRecords.isEmpty())
    {
        for (int i=0; i<monitorRecords.size(); i++)
            Monitors.append(new AMonitor(monitorRecords.at(i)));
    }
}

bool ASimulationStatistics::isEmpty()
{    
  return (countPhotons() == 0);
}

void ASimulationStatistics::registerWave(int iWave)
{
    WaveSpectrum->Fill(iWave);
}

void ASimulationStatistics::registerTime(double Time)
{
    TimeSpectrum->Fill(Time);
}

void ASimulationStatistics::registerAngle(double angle)
{
    AngularDistr->Fill(angle);
}

void ASimulationStatistics::registerNumTrans(int NumTransitions)
{
  TransitionSpectrum->Fill(NumTransitions);
}

//static void addTH1(TH1 *first, const TH1 *second)
//{
//    if (!first || !second) return;
//    int bins = second->GetNbinsX();
//    for(int i = 0; i < bins; i++)
//        first->Fill(second->GetBinCenter(i), second->GetBinContent(i));
//}

void ASimulationStatistics::AppendSimulationStatistics(ASimulationStatistics* from)
{
    appendTH1D(AngularDistr, from->getAngularDistr());
    appendTH1D(TimeSpectrum, from->getTimeSpectrum());
    appendTH1D(WaveSpectrum, from->getWaveSpectrum());
    appendTH1D(TransitionSpectrum, from->getTransitionSpectrum());

    Absorbed += from->Absorbed;
    OverrideLoss += from->OverrideLoss;
    HitPM += from->HitPM;
    HitDummy += from->HitDummy;
    Escaped += from->Escaped;
    LossOnGrid += from->LossOnGrid;
    TracingSkipped += from->TracingSkipped;
    MaxCyclesReached += from->MaxCyclesReached;
    GeneratedOutsideGeometry += from->GeneratedOutsideGeometry;
    KilledByMonitor += from->KilledByMonitor;

    FresnelTransmitted += from->FresnelTransmitted;
    FresnelReflected += from->FresnelReflected;
    BulkAbsorption += from->BulkAbsorption;
    Rayleigh += from->Rayleigh;
    Reemission += from->Reemission;

    OverrideBack += from->OverrideBack;
    OverrideForward += from->OverrideForward;

    if (Monitors.size() != from->Monitors.size())
    {
        qWarning() << "Cannot append monitor data - size mismatch:\n" <<
                      "Monitors here and in 'from':" << Monitors.size() << from->Monitors.size();
    }
    else
    {
        for (int i=0; i<Monitors.size(); i++)
            Monitors[i]->appendDataFromAnotherMonitor(from->Monitors[i]);
    }
}

long ASimulationStatistics::countPhotons()
{
    return Absorbed + OverrideLoss + HitPM + HitDummy + Escaped + LossOnGrid + TracingSkipped + MaxCyclesReached + GeneratedOutsideGeometry + KilledByMonitor;
}

void ASimulationStatistics::clearMonitors()
{
    for (AMonitor* mon : Monitors)
        delete mon;
    Monitors.clear();
}

