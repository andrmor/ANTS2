#include "amonitor.h"
#include "ageoobject.h"
#include "aroothistappenders.h"

#include <QDebug>

#include "TH1D.h"
#include "TH2D.h"

AMonitor::AMonitor(const AGeoObject *MonitorRecord)
{
  readFrom(MonitorRecord);
}

void AMonitor::clearData()
{
  PhotonStat.clear();
  ParticleStat.clear();
}

bool AMonitor::readFrom(const AGeoObject *MonitorRecord)
{
  const ATypeMonitorObject* mon = dynamic_cast<const ATypeMonitorObject*>(MonitorRecord->ObjectType);
  if (!mon)
    {
      qWarning() << "This is not a monitor type AGeoObject!";
      return false;
    }
  PhotonStat.configureTime(111, 000, 000);
  PhotonStat.configureWave(111, 000, 000);
  PhotonStat.configureXY(111, -mon->size1, mon->size1,
                         111, -mon->size2, mon->size2);
  PhotonStat.setActive(true);
  PhotonStat.setLowerIsSensitive(true);
  PhotonStat.setUpperIsSensitive(true);

  ParticleStat.configureTime(111, 000, 000);
  ParticleStat.configureXY(111, -mon->size1, mon->size1,
                           111, -mon->size2, mon->size2);
  ParticleStat.setActive(true);
  ParticleStat.setLowerIsSensitive(true);
  ParticleStat.setUpperIsSensitive(true);
  ParticleStat.setParticle(0);
  ParticleStat.setPrimaryEnabled(true);
  ParticleStat.setSecondaryEnabled(true);

  return true;
}

void AMonitor::appendFrom(AMonitor *from)
{
  PhotonStat.appendFrom(&from->PhotonStat);
  ParticleStat.appendFrom(&from->ParticleStat);
}

AMonitorPhotonStat::~AMonitorPhotonStat()
{
  clear();
}

void AMonitorPhotonStat::clear()
{
  AMonitorBaseStat::clear();
  delete wave; wave = 0;
}

void AMonitorPhotonStat::configureWave(int waveBins, int waveFrom, int waveTo)
{
  delete wave;
  wave = new TH1D("", "", waveBins, waveFrom, waveTo);
}

void AMonitorPhotonStat::fill(double x, double y, double Time, int waveIndex)
{
  AMonitorBaseStat::fill(x, y, Time);
  wave->Fill(waveIndex);
}

void AMonitorPhotonStat::appendFrom(AMonitorPhotonStat *from)
{
  AMonitorBaseStat::appendFrom(from);
  appendTH1D(wave, from->getWave());
}

void AMonitorBaseStat::clear()
{
  delete time; time = 0;
  delete xy;   xy = 0;
}

void AMonitorBaseStat::configureTime(int timeBins, double timeFrom, double timeTo)
{
  delete time;
  time = new TH1D("", "", timeBins, timeFrom, timeTo);
}

void AMonitorBaseStat::configureXY(int xBins, double xFrom, double xTo, int yBins, double yFrom, double yTo)
{
  delete xy;
  xy = new TH2D("", "", xBins, xFrom, xTo, yBins, yFrom, yTo);
}

void AMonitorBaseStat::fill(double x, double y, double Time)
{
  time->Fill(Time);
  xy->Fill(x,y);
}

void AMonitorBaseStat::appendFrom(AMonitorBaseStat *from)
{
  appendTH1D(time, from->getTime());
  appendTH2D(xy, from->getXY());
}
