#ifndef AMONITOR_H
#define AMONITOR_H

class TH1D;
class TH2D;
class AGeoObject;

class AMonitorPhotonStat
{
public:
    AMonitorPhotonStat() : time(0), wave(0), xy(0) {}
    ~AMonitorPhotonStat();

    void clear();
    void configure(int timeBins, double timeFrom, double timeTo,
                   int waveBins, int waveFrom, int waveTo,
                   int xBins, double xFrom, double xTo,
                   int yBins, double yFrom, double yTo);

    void fill(double x, double y, double Time, int waveIndex);

    TH1D* getTime() const {return time;}
    TH1D* getWave() const {return wave;}
    TH2D* getXY()   const {return xy;}

    void appendFrom(AMonitorPhotonStat* from);

private:
    TH1D* time;
    TH1D* wave;
    TH2D* xy;
};

class AMonitorData
{
public:
    AMonitorData();
    ~AMonitorData();

    void clear() {PhotonStat.clear();}
    void appendFrom(AMonitorData* from);

    bool readFrom(const AGeoObject* MonitorRecord);

    AMonitorPhotonStat PhotonStat;

};

class AMonitor
{
public:
    AMonitor() {}
    AMonitor(const AGeoObject* MonitorRecord);

    void clearData() {FrontData.clear();}
    void appendFrom(AMonitor* from);

    AMonitorData FrontData;
    //AMonitorData BackData;
};

#endif // AMONITOR_H
