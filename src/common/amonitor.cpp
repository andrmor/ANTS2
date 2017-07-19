#include "amonitor.h"
#include "ageoobject.h"
#include "aroothistappenders.h"

#include <QDebug>

#include "TH1D.h"
#include "TH2D.h"

AMonitorData::AMonitorData()
{

}

AMonitorData::~AMonitorData()
{

}

void AMonitorData::appendFrom(AMonitorData *from)
{
    PhotonStat.appendFrom(&from->PhotonStat);
}

bool AMonitorData::readFrom(const AGeoObject *MonitorRecord)
{
    const ATypeMonitorObject* mon = dynamic_cast<const ATypeMonitorObject*>(MonitorRecord->ObjectType);
    if (!mon)
    {
        qWarning() << "This is not a monitor type AGeoObject!";
        return false;
    }
    PhotonStat.configure(111, 000, 000,
                         111, 000, 000,
                         111, -mon->size1, mon->size1,
                         111, -mon->size2, mon->size2);

    return true;
}

AMonitor::AMonitor(const AGeoObject *MonitorRecord)
{
    FrontData.readFrom(MonitorRecord);
}

void AMonitor::appendFrom(AMonitor *from)
{
    FrontData.appendFrom(&from->FrontData);
}

AMonitorPhotonStat::~AMonitorPhotonStat()
{
    clear();
}

void AMonitorPhotonStat::clear()
{
    delete time; time = 0;
    delete wave; wave = 0;
    delete xy;   xy = 0;
}

void AMonitorPhotonStat::configure(int timeBins, double timeFrom, double timeTo, int waveBins, int waveFrom, int waveTo, int xBins, double xFrom, double xTo, int yBins, double yFrom, double yTo)
{
    clear();

    time = new TH1D("", "", timeBins, timeFrom, timeTo);
    wave = new TH1D("", "", waveBins, waveFrom, waveTo);
    xy =   new TH2D("", "", xBins, xFrom, xTo, yBins, yFrom, yTo);
}

void AMonitorPhotonStat::fill(double x, double y, double Time, int waveIndex)
{
    time->Fill(Time);
    wave->Fill(waveIndex);
    xy->Fill(x,y);
}

void AMonitorPhotonStat::appendFrom(AMonitorPhotonStat *from)
{
    appendTH1D(time, from->getTime());
    appendTH1D(wave, from->getWave());
    appendTH2D(xy, from->getXY());
}
