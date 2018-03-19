#include "acalibratorsignalperphel.h"
#include "eventsdataclass.h"

#include "detectorclass.h"
#include "alrfmoduleselector.h"
#include "apmgroupsmanager.h"
#include "pms.h"
#include "apositionenergyrecords.h"

#include <QDebug>

#include "TH1D.h"
#include "TFitResultPtr.h"
#include "TFitResult.h"

// ----------------- base ----------------------

ACalibratorSignalPerPhEl::ACalibratorSignalPerPhEl(const EventsDataClass &EventsDataHub, QVector<TH1D *> &DataHists, QVector<double> &SignalPerPhEl) :
    QObject(),
    EventsDataHub(EventsDataHub), DataHists(DataHists), SignalPerPhEl(SignalPerPhEl) {}

ACalibratorSignalPerPhEl::~ACalibratorSignalPerPhEl()
{
    ClearData();
}

void ACalibratorSignalPerPhEl::ClearData()
{
    for (TH1D* h : DataHists) delete h;
    DataHists.clear();

    SignalPerPhEl.clear();
}

TH1D *ACalibratorSignalPerPhEl::GetHistogram(int ipm)
{
    if (ipm < 0 || ipm >= DataHists.size()) return 0;
    return DataHists[ipm];
}

double ACalibratorSignalPerPhEl::GetSignalPerPhEl(int ipm)
{
    if (ipm < 0 || ipm >= SignalPerPhEl.size()) return 0;
    return SignalPerPhEl[ipm];
}

const QString &ACalibratorSignalPerPhEl::GetLastError()
{
    return LastError;
}



// ----------------- from statistics ------------------

ACalibratorSignalPerPhEl_Stat::ACalibratorSignalPerPhEl_Stat(const EventsDataClass &EventsDataHub, QVector<TH1D *> &DataHists, QVector<double> &SignalPerPhEl, DetectorClass& Detector) :
    ACalibratorSignalPerPhEl(EventsDataHub, DataHists, SignalPerPhEl), Detector(Detector) {}

bool ACalibratorSignalPerPhEl_Stat::PrepareData()
{
    if (EventsDataHub.isEmpty())
    {
        LastError = "There are no events data";
        return false;
    }
    if (Detector.PMgroups->countPMgroups()>1)
    {
        LastError = "This procedure is implemented only for the case of one PM group";
        return false;
    }
    if (!EventsDataHub.isReconstructionReady())
    {
        LastError = "Reconstruction is not ready!";
        return false;
    }
    if (!Detector.LRFs->isAllLRFsDefined())
    {
        LastError = "LRFs are not ready!";
        return false;
    }

    LastError.clear();
    ClearData();
    QVector<TH1D*> numberHists; //will be used to calculate sigma vs distance to PM center
    const int numPMs = Detector.PMs->count();

    emit progressChanged(0);

    for (int ipm = 0; ipm < numPMs; ipm++)
    {
        TString s = "pm #";
        s += ipm;

        double SigForMin = calculateSignalLimit(ipm, minRange);
        double SigForMax = calculateSignalLimit(ipm, maxRange);

        //  qDebug() << "Signal range for PM#" << ipm << "is set to min:"<<SigForMax <<"max:"<< SigForMin;
        if (SigForMin <= SigForMax)
        {
            LastError = "Defined spatial range results in expected signal at lower bound smaller than for the upper one";
            return false;
        }
        TH1D* tmp1 = new TH1D("", s, numBins, SigForMax,SigForMin);

        DataHists.append(tmp1);
        tmp1->GetXaxis()->SetTitle("Average signal");
        tmp1->GetYaxis()->SetTitle("Sigma square");
        TH1D* tmp2 = new TH1D("", s, numBins, SigForMax,SigForMin);
        numberHists.append(tmp2);
    }

    //filling hists - go through all events, each will populate data for all PMs
    for (int iev=0; iev<EventsDataHub.Events.size(); iev++)
    {
        if (iev % 1000 == 0)
        {
            int ipr = 100.0 * iev / EventsDataHub.Events.size();
            emit progressChanged(ipr);
        }
        const AReconRecord* rec = EventsDataHub.ReconstructionData.at(0).at(iev);

        if (!rec->GoodEvent) continue;          //respecting the filters
        if (rec->Points.size() > 1) continue;   //ignoring multiple events in reconstruction

        const double& x = rec->Points[0].r[0];
        const double& y = rec->Points[0].r[1];
        const double& z = rec->Points[0].r[2];

        double energy   = rec->Points[0].energy;
        if (energy < 1e-10) energy = 1.0;

        for (int ipm=0; ipm<numPMs; ipm++)
        {
            double x0 = Detector.PMs->X(ipm);
            double y0 = Detector.PMs->Y(ipm);
            double r2 = (x-x0)*(x-x0) + (y-y0)*(y-y0);
            if ( r2 < minRange2  ||  r2 > maxRange2 ) continue;

            double AvSig = Detector.LRFs->getLRF(ipm, x, y, z);
            double sig = EventsDataHub.Events.at(iev).at(ipm);
            double delta2 = sig/energy - AvSig;
            delta2 *= delta2;

            DataHists[ipm]->Fill(AvSig, delta2);
            numberHists[ipm]->Fill(AvSig, 1.0);
        }
    }

    //calculating sigmas
    for (int ipm=0; ipm<numPMs; ipm++)
    {
        for (int i=1; i<numberHists[ipm]->GetXaxis()->GetNbins()+1; i++) //ignoring under and over bins
        {
            const double numberEventsInBin = numberHists[ipm]->GetBinContent(i);
            //  qDebug() << ipm << i << numberEventsInBin << sigmaCalculationThreshold;
            if (numberEventsInBin < sigmaCalculationThreshold)
                DataHists[ipm]->SetBinContent(i, 0);
            else
                DataHists[ipm]->SetBinContent(i, DataHists.at(ipm)->GetBinContent(i) / numberEventsInBin);
        }
    }

    for (int ipm=0; ipm<numberHists.size(); ipm++) delete numberHists[ipm];
    numberHists.clear();

    progressChanged(100);
    return true;
}

bool ACalibratorSignalPerPhEl_Stat::ExtractSignalPerPhEl(int ipm)
{
    int numPMs = Detector.PMs->count();
    if (ipm <0 || ipm >= numPMs)
    {
        LastError = "Wrong PM number";
        return false;
    }
    if (DataHists.size() != numPMs)
    {
        LastError = "PrepareData method has to be invoked first!";
        return false;
    }

    LastError.clear();
    if (SignalPerPhEl.size() != numPMs) SignalPerPhEl.resize(numPMs);

    TFitResultPtr fit = DataHists[ipm]->Fit("pol1", "S", "", lowerLimit, upperLimit);

    if (!fit.Get())
    {
        LastError = "Fit failed for PM #" + QString::number(ipm);
        SignalPerPhEl[ipm] = -1;
        return false;
    }

    double cutoff = fit->Value(0);
    double ChannelsPerPhEl = fit->Value(1);
    if (enf < 1.0e-10)
    {
        LastError = "ENF has to be positive!";
        SignalPerPhEl[ipm] = -1;
        return false;
    }

    ChannelsPerPhEl /= enf;
    qDebug() << ipm << "> ChPerPhEl:"<<ChannelsPerPhEl<<" const:"<<cutoff;

    SignalPerPhEl[ipm] = ChannelsPerPhEl;
    return true;
}

bool ACalibratorSignalPerPhEl_Stat::ExtractSignalPerPhEl()
{
    int numPMs = Detector.PMs->count();
    if (DataHists.size() != numPMs)
    {
        LastError = "PrepareData method has to be invoked first!";
        return false;
    }

    QString FailedPMs;
    for (int ipm = 0; ipm < numPMs; ipm++)
    {
        bool bOK = ExtractSignalPerPhEl(ipm);
        if (!bOK) FailedPMs += " " + QString::number(ipm);
    }

    if (FailedPMs.isEmpty()) return true;

    LastError = "Fit failed for PM#:" + FailedPMs;
    return false;
}

void ACalibratorSignalPerPhEl_Stat::SetNumBins(int bins)
{
    numBins = bins;
}

void ACalibratorSignalPerPhEl_Stat::SetRange(double min, double max)
{
    minRange = min;
    minRange2 = minRange * minRange;

    maxRange = max;
    maxRange2 = maxRange * maxRange;
}

void ACalibratorSignalPerPhEl_Stat::SetThresholdSigmaCalc(int threshold)
{
    sigmaCalculationThreshold = threshold;
}

void ACalibratorSignalPerPhEl_Stat::SetENF(int ENF)
{
    enf = ENF;
}

void ACalibratorSignalPerPhEl_Stat::SetSignalLimits(double lower, double upper)
{
    lowerLimit = lower;
    upperLimit = upper;
}

double ACalibratorSignalPerPhEl_Stat::calculateSignalLimit(int ipm, double range)
{
    double x0 = Detector.PMs->X(ipm);
    double y0 = Detector.PMs->Y(ipm);

    double sig = 0;
    do
    {
        sig = Detector.LRFs->getLRF(ipm, x0+range, y0, 0);
        range -= 0.1;
        if (range <= 0) return 0;
    }
    while (sig <= 0);
    return sig;
}
