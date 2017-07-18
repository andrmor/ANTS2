#include "asimulationstatistics.h"
#include "ageoobject.h"
#include "amonitor.h"

#include <QDebug>

#include "TH1I.h"
#include "TH1D.h"

ASimulationStatistics::ASimulationStatistics(const TString nameID)
{
    WaveSpectrum = 0;
    TimeSpectrum = 0;
    CosAngleSpectrum = 0;
    TransitionSpectrum = 0;
    WaveNodes = 0;
    numBins = 100;

    NameID = nameID;
    initialize(numBins);
}

ASimulationStatistics::~ASimulationStatistics()
{
    //clearAll();
}

void ASimulationStatistics::clearAll()
{
    if (WaveSpectrum) delete WaveSpectrum; WaveSpectrum = 0;
    if (TimeSpectrum) delete TimeSpectrum; TimeSpectrum = 0;
    if (CosAngleSpectrum) delete CosAngleSpectrum; CosAngleSpectrum = 0;
    if (TransitionSpectrum) delete TransitionSpectrum; TransitionSpectrum = 0;
    clearMonitors();
}

void ASimulationStatistics::initialize(int nBins, int waveNodes)
{    
    if (nBins != 0) numBins = nBins;
    if (waveNodes != 0) WaveNodes = waveNodes;

    if (WaveSpectrum) delete WaveSpectrum;
    if (WaveNodes != 0)
       WaveSpectrum = new TH1I("iWaveSpectrum"+NameID,"WaveIndex spectrum", WaveNodes, 0, WaveNodes-1);
    else
       WaveSpectrum = new TH1I("iWaveSpectrum"+NameID,"WaveIndex spectrum",numBins,0,-1);

    if (TimeSpectrum) delete TimeSpectrum;
    TimeSpectrum = new TH1D("TimeSpectrum"+NameID, "Time spectrum",numBins,0,-1);

    if (CosAngleSpectrum) delete CosAngleSpectrum;
    CosAngleSpectrum = new TH1I("CosAngleSpectrum"+NameID, "cosAngle spectrum", numBins, 0,-1);

    if (TransitionSpectrum) delete TransitionSpectrum;
    TransitionSpectrum = new TH1I("TransitionsSpectrum"+NameID, "Transitions", numBins, 0,-1);

    Absorbed = OverrideLoss = HitPM = HitDummy = Escaped = LossOnGrid = TracingSkipped = MaxCyclesReached = GeneratedOutsideGeometry = 0;

    FresnelTransmitted = FresnelReflected = BulkAbsorption = Rayleigh = Reemission = 0;
    OverrideForward = OverrideBack = 0;
    OverrideSimplisticAbsorption = OverrideSimplisticReflection = OverrideSimplisticScatter = 0;
    OverrideFSNPabs = OverrideFSNlambert = OverrideFSNPspecular = 0;
    OverrideMetalAbs = OverrideMetalReflection = 0;
    OverrideClaudioAbs = OverrideClaudioSpec = OverrideClaudioLamb = 0;
    OverrideWLSabs = OverrideWLSshift = 0;

    PhotonHistoryLog.clear();
    PhotonHistoryLog.squeeze();

    clearMonitors();
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

void ASimulationStatistics::registerAngle(double CosAngle)
{
    CosAngleSpectrum->Fill(CosAngle);
}

void ASimulationStatistics::registerNumTrans(int NumTransitions)
{
  TransitionSpectrum->Fill(NumTransitions);
}

void ASimulationStatistics::AppendSimulationStatistics(const ASimulationStatistics* from)
{
  Absorbed += from->Absorbed;
  OverrideLoss += from->OverrideLoss;
  HitPM += from->HitPM;
  HitDummy += from->HitDummy;
  Escaped += from->Escaped;
  LossOnGrid += from->LossOnGrid;
  TracingSkipped += from->TracingSkipped;
  MaxCyclesReached += from->MaxCyclesReached;
  GeneratedOutsideGeometry += from->GeneratedOutsideGeometry;

  FresnelTransmitted += from->FresnelTransmitted;
  FresnelReflected += from->FresnelReflected;
  BulkAbsorption += from->BulkAbsorption;
  Rayleigh += from->Rayleigh;
  Reemission += from->Reemission;

  OverrideBack += from->OverrideBack;
  OverrideForward += from->OverrideForward;

  OverrideSimplisticAbsorption += from->OverrideSimplisticAbsorption;
  OverrideSimplisticReflection += from->OverrideSimplisticReflection;
  OverrideSimplisticScatter += from->OverrideSimplisticScatter;

  OverrideFSNPabs += from->OverrideFSNPabs;
  OverrideFSNlambert += from->OverrideFSNlambert;
  OverrideFSNPspecular += from->OverrideFSNPspecular;

  OverrideMetalAbs += from->OverrideMetalAbs;
  OverrideMetalReflection += from->OverrideMetalReflection;

  OverrideClaudioAbs += from->OverrideClaudioAbs;
  OverrideClaudioSpec += from->OverrideClaudioSpec;
  OverrideClaudioLamb += from->OverrideClaudioLamb;

  OverrideWLSabs += from->OverrideWLSabs;
  OverrideWLSshift += from->OverrideWLSshift;
}

long ASimulationStatistics::countPhotons()
{
    return Absorbed + OverrideLoss + HitPM + HitDummy + Escaped + LossOnGrid + TracingSkipped + MaxCyclesReached + GeneratedOutsideGeometry;
}

void ASimulationStatistics::clearMonitors()
{
    for (AMonitor* mon : Monitors)
        delete mon;
    Monitors.clear();
}

