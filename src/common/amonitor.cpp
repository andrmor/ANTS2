#include "amonitor.h"
#include "ageoobject.h"
#include "aroothistappenders.h"

#include <QDebug>

#include "TH1D.h"
#include "TH2D.h"

AMonitor::AMonitor() : time(0), xy(0), angle(0), wave(0), energy(0) {}

AMonitor::AMonitor(const AGeoObject *MonitorGeoObject) : time(0), xy(0), angle(0), wave(0), energy(0)
{
    readFromGeoObject(MonitorGeoObject);
}

AMonitor::~AMonitor()
{
    clearData();
}

void AMonitor::clearData()
{
    delete time; time = 0;
    delete xy;   xy = 0;
    delete angle; angle = 0;
    delete wave; wave = 0;
    delete energy; energy = 0;
}

void AMonitor::fillForParticle(double x, double y, double Time, double Angle, double Energy)
{
    xy->Fill(x,y);
    time->Fill(Time);
    angle->Fill(Angle);
    energy->Fill(Energy);
}

void AMonitor::fillForPhoton(double x, double y, double Time, double Angle, int waveIndex)
{
    xy->Fill(x,y);
    time->Fill(Time);
    angle->Fill(Angle);
    wave->Fill(waveIndex);
}

bool AMonitor::readFromGeoObject(const AGeoObject *MonitorRecord)
{
    const ATypeMonitorObject* mon = dynamic_cast<const ATypeMonitorObject*>(MonitorRecord->ObjectType);
    if (!mon)
      {
        qWarning() << "This is not a monitor type AGeoObject!";
        return false;
      }

    config = mon->config;

    initXYHist();
    initTimeHist();
    initAngleHist();
    if (config.PhotonOrParticle == 0) initWaveHist();
    else initEnergyHist();

    return true;
}

void AMonitor::appendDataFromAnotherMonitor(AMonitor *from)
{
    appendTH1D(time, from->getTime());
    appendTH2D(xy, from->getXY());
    appendTH1D(angle, from->getAngle());
    appendTH1D(wave, from->getWave());
    appendTH1D(energy, from->getEnergy());
}

void AMonitor::configureTime(int timeBins, double timeFrom, double timeTo)
{
    config.timeBins = timeBins;
    config.timeFrom = timeFrom;
    config.timeTo = timeTo;
    initTimeHist();
}

void AMonitor::configureXY(int xBins, int yBins)
{
    config.xbins = xBins;
    config.ybins = yBins;
    initXYHist();
}

void AMonitor::configureWave(int waveBins, int waveFrom, int waveTo)
{
    config.waveBins = waveBins;
    config.waveFrom = waveFrom;
    config.waveTo = waveTo;
    initWaveHist();
}

void AMonitor::configureAngle(int angleBins, int angleFrom, int angleTo)
{
    config.angleBins = angleBins;
    config.angleFrom = angleFrom;
    config.angleTo = angleTo;
    initAngleHist();
}

void AMonitor::configureEnergy(int energyBins, int energyFrom, int energyTo)
{
    config.energyBins = energyBins;
    config.energyFrom = energyFrom;
    config.energyTo = energyTo;
    initEnergyHist();
}

void AMonitor::initXYHist()
{
    delete xy;
    const double limit2 = ( config.shape == 0 ? config.size2 : config.size1 ); // 0 - rectangular, 1 - round
    xy = new TH2D("", "", config.xbins, -config.size1, +config.size1, config.ybins, -limit2, limit2);
    xy->SetXTitle("X, mm");
    xy->SetYTitle("Y, mm");
}

void AMonitor::initTimeHist()
{
    delete time;
    time = new TH1D("", "", config.timeBins, config.timeFrom, config.timeTo);
    time->SetXTitle("Time, ns");
}

void AMonitor::initWaveHist()
{
    delete wave;
    wave = new TH1D("", "", config.waveBins, config.waveFrom, config.waveTo);
    wave->SetXTitle("Wave index");
}

void AMonitor::initAngleHist()
{
    delete angle;
    angle = new TH1D("", "", config.angleBins, config.angleFrom, config.angleTo);
    angle->SetXTitle("Angle, degrees");
}

void AMonitor::initEnergyHist()
{
    delete energy;
    energy = new TH1D("", "", config.energyBins, config.energyFrom, config.energyTo);
    energy->SetXTitle("Energy, keV");
}
