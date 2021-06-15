#include "amonitor.h"
#include "ageoobject.h"
#include "ageotype.h"
#include "aroothistappenders.h"
#include "ahistogram.h"

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
    appendTH1DwithStat(time, from->getTime());
    appendTH2D(xy, from->getXY());
    appendTH1DwithStat(angle, from->getAngle());
    appendTH1DwithStat(wave, from->getWave());
    //appendTH1D(energy, from->getEnergy());
    appendTH1DwithStat(energy, from->getEnergy());
}

#include "ahistogram.h"
#include <QJsonObject>
#include <QJsonArray>
void AMonitor::overrideDataFromJson(const QJsonObject &json)
{
    QJsonObject jEnergy = json["Energy"].toObject();
    update1D(jEnergy, energy);

    QJsonObject jAngle = json["Angle"].toObject();
    update1D(jAngle, angle);

    QJsonObject jTime = json["Time"].toObject();
    update1D(jTime, time);

    QJsonObject jSpatial = json["Spatial"].toObject();
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
    ATH2D * hist = new ATH2D("", "", 100, 0, 1.0, 100, 0, 1.0);
    hist->Import(xfrom, xto, yfrom, yto, dataVec, statVec);
    delete xy; xy = hist;
}

void AMonitor::update1D(const QJsonObject & json, TH1D* & old)
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
    if (old == energy)
    {
        switch (config.energyUnitsInHist)
        {
        case 0:  multiplier = 1.0e6;  break;// keV -> meV
        case 1:  multiplier = 1.0e3;  break;// keV -> eV
        default: multiplier = 1.0;    break;// keV -> keV
        case 3:  multiplier = 1.0e-3; break;// keV -> MeV
        }
    }

    ATH1D * hist = new ATH1D(*old); //to inherit all properties, including the axis titles
    hist->Import(from * multiplier, to * multiplier, dataVec, statVec);
    delete old;
    old = hist;
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
