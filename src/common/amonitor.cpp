#include "amonitor.h"

#include "TH1D.h"
#include "TH2D.h"

AMonitorData::AMonitorData()
{

}

AMonitorData::~AMonitorData()
{

}

AMonitor::AMonitor()
{

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

void AMonitorPhotonStat::fill(double Time, int waveIndex, double x, double y)
{
    time->Fill(Time);
    wave->Fill(waveIndex);
    xy->Fill(x,y);
}
