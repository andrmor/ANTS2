#include "amonitor.h"
#include "ageoobject.h"
#include "aroothistappenders.h"

#include <QDebug>

#include "TH1D.h"
#include "TH2D.h"
#include "TString.h"

AMonitor::AMonitor() : name("Undefined"), time(0), xy(0), angle(0), wave(0), energy(0) {}

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

int AMonitor::getHits() const
{
    return xy->GetEntries();
}

void AMonitor::fillForParticle(double x, double y, double Time, double Angle, double Energy)
{
    xy->Fill(x,y);
    time->Fill(Time);
    angle->Fill(Angle);

    switch (config.energyUnitsInHist)
    {
    case 0: Energy *= 1.0e6; break;
    case 1: Energy *= 1.0e3; break;
    case 2: break;
    case 3: Energy *= 1.0e-3;break;
    }
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

    name = MonitorRecord->Name;

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

void AMonitor::configureWave(int waveBins, double waveFrom, double waveTo)
{
    config.waveBins = waveBins;
    config.waveFrom = waveFrom;
    config.waveTo = waveTo;
    initWaveHist();
}

void AMonitor::configureAngle(int angleBins, double angleFrom, double angleTo)
{
    config.angleBins = angleBins;
    config.angleFrom = angleFrom;
    config.angleTo = angleTo;
    initAngleHist();
}

void AMonitor::configureEnergy(int energyBins, double energyFrom, double energyTo)
{
    config.energyBins = energyBins;
    config.energyFrom = energyFrom;
    config.energyTo = energyTo;
    initEnergyHist();
}

#include "ahistogram.h"
#include <QJsonObject>
#include <QJsonArray>
void AMonitor::overrideDataFromJson(const QJsonObject &json)
{
    QJsonObject jEnergy = json["Energy"].toObject();
    delete energy; energy = create1D(jEnergy, true);
   /*
    {
        int bins =    jEnergy["bins"].toInt();
        double from = jEnergy["from"].toDouble();
        double to =   jEnergy["to"].toDouble();
        QJsonArray data = jEnergy["data"].toArray();
        std::vector<double> vec;
        double multiplier = 1.0;
        switch (config.energyUnitsInHist)
        {
        case 0:  multiplier = 1.0e6;  break;// keV -> meV
        case 1:  multiplier = 1.0e3;  break;// keV -> eV
        default: multiplier = 1.0;    break;// keV -> keV
        case 3:  multiplier = 1.0e-3; break;// keV -> MeV
        }
        for (int i=0; i<data.size(); i++)
            vec.push_back(data[i].toDouble());
        ATH1D * hist = new ATH1D("", "", 100, 0, 1.0);
        hist->Import(from * multiplier, to * multiplier, vec, {100,100,100,100,123});
        delete energy;
        energy = hist;
    }
    */

    QJsonObject jAngle = json["Angle"].toObject();
    delete angle; angle = create1D(jAngle, false);
    /*
    {
        int bins =    jAngle["bins"].toInt();
        double from = jAngle["from"].toDouble();
        double to =   jAngle["to"].toDouble();
        QJsonArray data = jAngle["data"].toArray();
        QVector<double> vec;
        for (int i=0; i<data.size(); i++)
            vec << data[i].toDouble();
        mon->configureAngle(bins, from, to);
        mon->overrideAngleData(vec);
    }
    */

    QJsonObject jTime = json["Time"].toObject();
    delete time; time = create1D(jTime, false);
    /*
    {
        int bins =    jTime["bins"].toInt();
        double from = jTime["from"].toDouble();
        double to =   jTime["to"].toDouble();
        QJsonArray data = jTime["data"].toArray();
        QVector<double> vec;
        for (int i=0; i<data.size(); i++)
            vec << data[i].toDouble();
        mon->configureTime(bins, from, to);
        mon->overrideTimeData(vec);
    }
    */

    QJsonObject jSpatial = json["Spatial"].toObject();
    {
        //int xbins =    jSpatial["xbins"].toInt();
        //int ybins =    jSpatial["ybins"].toInt();
        double xfrom = jSpatial["xfrom"].toDouble();
        double xto   = jSpatial["xto"].toDouble();
        double yfrom = jSpatial["yfrom"].toDouble();
        double yto   = jSpatial["yto"].toDouble();

        QJsonArray dataAr = jSpatial["data"].toArray();
        int ybins = dataAr.size();
        std::vector<std::vector<double>> dataVec;
        dataVec.resize(ybins);
        for (int iy=0; iy<ybins; iy++)
        {
            QJsonArray row = dataAr[iy].toArray();
            int xbins = row.size();
            dataVec[iy].resize(xbins);
            for (int ix=0; ix<xbins; ix++)
                dataVec[iy][ix] = row[ix].toDouble();
        }

        QJsonArray statAr = jSpatial["stat"].toArray();
        std::vector<double> statVec;
        for (int i=0; i<statAr.size(); i++)
            statVec.push_back(statAr[i].toDouble());

        //mon->configureXY(xbins, ybins);
        ATH2D * hist = new ATH2D("", "", 100, 0, 1.0, 100, 0, 1.0);
        hist->Import(xfrom, xto, yfrom, yto, dataVec, statVec);
        delete xy; xy = hist;
        //mon->overrideXYData(dataVec);
    }
}

TH1D * AMonitor::create1D(const QJsonObject & json, bool bEnergy)
{
    double from = json["from"].toDouble();
    double to =   json["to"].toDouble();

    QJsonArray dataAr = json["data"].toArray();
    std::vector<double> dataVec;
    for (int i=0; i<dataAr.size(); i++)
        dataVec.push_back(dataAr[i].toDouble());

    QJsonArray statAr = json["stat"].toArray();
    std::vector<double> statVec;
    for (int i=0; i<statAr.size(); i++)
        statVec.push_back(statAr[i].toDouble());

    double multiplier = 1.0;
    if (bEnergy)
    {
        switch (config.energyUnitsInHist)
        {
        case 0:  multiplier = 1.0e6;  break;// keV -> meV
        case 1:  multiplier = 1.0e3;  break;// keV -> eV
        default: multiplier = 1.0;    break;// keV -> keV
        case 3:  multiplier = 1.0e-3; break;// keV -> MeV
        }
    }

    ATH1D * hist = new ATH1D("", "", 100, 0, 1.0);
    hist->Import(from * multiplier, to * multiplier, dataVec, statVec);
    return hist;
}


void AMonitor::overrideEnergyData(double from, double to, const std::vector<double> & binContent, const std::vector<double> & stats)
{

    ATH1D * hist = new ATH1D("", "", 100, 0, 1.0);
    hist->Import(from, to, binContent, stats);
    /*
    int size = energy->GetNbinsX();
    int entries = 0;
    for (int i=0; i<vec.size(); i++)
    {
        if (i > size+1) break;
        const double val = vec.at(i);
        energy->SetBinContent(i, val);
        if (i>0 && i<size+1) entries += val;
    }
    energy->SetEntries(entries);
    */
    delete energy;
    energy = hist;
}

void AMonitor::overrideAngleData(const QVector<double> &vec)
{
    /*
    ATH1D mycopy(*angle);
    int size = mycopy.GetNbinsX();
    int entries = 0;
    for (int i=0; i<vec.size(); i++)
    {
        if (i > size+1) break;
        const double val = vec.at(i);
        mycopy.SetBinContent(i, val);
        if (i>0 && i<size+1) entries += val;
    }
    mycopy.SetEntries(entries);
    mycopy.SetSumW(100);
    mycopy.SetSumWX(4500);
    delete angle; angle = new TH1D(mycopy);// or just mycopy as pointer to angle
    */

    int size = angle->GetNbinsX();
    int entries = 0;
    for (int i=0; i<vec.size(); i++)
    {
        if (i > size+1) break;
        const double val = vec.at(i);
        angle->SetBinContent(i, val);
        if (i>0 && i<size+1) entries += val;
    }
    angle->SetEntries(entries);
}

void AMonitor::overrideTimeData(const QVector<double> &vec)
{
    int size = time->GetNbinsX();
    int entries = 0;
    for (int i=0; i<vec.size(); i++)
    {
        if (i > size+1) break;
        const double val = vec.at(i);
        time->SetBinContent(i, val);
        if (i>0 && i<size+1) entries += val;
    }
    time->SetEntries(entries);
}

void AMonitor::overrideXYData(const QVector<QVector<double> > &vec)
{
    int sizeX = xy->GetNbinsX();
    int sizeY = xy->GetNbinsY();
    int entries = 0;
    for (int iy=0; iy<sizeY; iy++)
    {
        for (int ix=0; ix<sizeX; ix++)
        {
            const double val = vec.at(iy).at(ix);
            xy->SetBinContent(ix, iy, val);
            if (ix>0 && ix<sizeX+1 && iy>0 && iy<sizeY+1 ) entries += val;
        }
    }
    xy->SetEntries(entries);
}

void AMonitor::initXYHist()
{
    delete xy;
    const double limit2 = ( config.shape == 0 ? config.size2 : config.size1 ); // 0 - rectangular, 1 - round
    xy = new TH2D("", "", config.xbins, -config.size1, config.size1, config.ybins, -limit2, limit2);
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

    double from = config.energyFrom;
    double to = config.energyTo;
    TString title = "";
    switch (config.energyUnitsInHist)
    {
    case 0: title = "Energy, meV"; break;
    case 1: title = "Energy, eV"; break;
    case 2: title = "Energy, keV"; break;
    case 3: title = "Energy, MeV"; break;
    }

    energy = new TH1D("", "", config.energyBins, from, to);
    energy->SetXTitle(title);
}
