#include "acalibratorsignalperphel.h"
#include "eventsdataclass.h"
#include "apeakfinder.h"
#include "detectorclass.h"
#include "alrfmoduleselector.h"
#include "apmgroupsmanager.h"
#include "apmhub.h"
#include "apositionenergyrecords.h"

#include <QDebug>
#include <QApplication>

#include "TH1D.h"
#include "TFitResultPtr.h"
#include "TFitResult.h"
#include "TGraph.h"

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

double ACalibratorSignalPerPhEl::GetSignalPerPhEl(int ipm) const
{
    if (ipm < 0 || ipm >= SignalPerPhEl.size()) return -1;
    return SignalPerPhEl.at(ipm);
}

const QString &ACalibratorSignalPerPhEl::GetLastError() const
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
    if (Detector.PMgroups->countPMgroups() > 1)
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
    const int numEvents = EventsDataHub.Events.size();

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
    for (int iev = 0; iev < numEvents; iev++)
    {
        if (iev % 1000 == 0)
        {
            int ipr = 100.0 * iev / numEvents;
            emit progressChanged(ipr);
            qApp->processEvents();
        }
        const AReconRecord* rec = EventsDataHub.ReconstructionData.at(0).at(iev);

        if (!rec->GoodEvent) continue;          //respecting the filters
        if (rec->Points.size() > 1) continue;   //ignoring multiple events in reconstruction

        const double& x = rec->Points[0].r[0];
        const double& y = rec->Points[0].r[1];
        const double& z = rec->Points[0].r[2];

        double energy   = rec->Points[0].energy;
        if (energy < 1e-10) energy = 1.0;

        for (int ipm = 0; ipm < numPMs; ipm++)
        {
            double x0 = Detector.PMs->X(ipm);
            double y0 = Detector.PMs->Y(ipm);
            double r2 = (x-x0)*(x-x0) + (y-y0)*(y-y0);
            if ( r2 < minRange2  ||  r2 > maxRange2 ) continue;

            double AvSig = Detector.LRFs->getLRF(ipm, x, y, z);
            if (AvSig <= 0) continue;
            double sig = EventsDataHub.Events.at(iev).at(ipm);
            double delta2 = sig/energy - AvSig;
            delta2 *= delta2;

            DataHists[ipm]->Fill(AvSig, delta2);
            numberHists[ipm]->Fill(AvSig, 1.0);
        }
    }

    //calculating final sigma2
    for (int ipm=0; ipm<numPMs; ipm++)
    {
        const int Bins = numberHists.at(ipm)->GetXaxis()->GetNbins();
        for (int i = 1; i < Bins+1; i++) //ignoring underflow (#0) and overflow (#Bins+1) bins
        {
            const double numberEventsInBin = numberHists.at(ipm)->GetBinContent(i);  //it is double in ROOT :)
            double sigma2 = DataHists.at(ipm)->GetBinContent(i);

            if (numberEventsInBin < sigmaCalculationThreshold)
                sigma2 = 0;
            else
                sigma2 /= numberEventsInBin;

            DataHists[ipm]->SetBinContent(i, sigma2);
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

    //double cutoff = fit->Value(0);
    double ChannelsPerPhEl = fit->Value(1);
    if (enf < 1.0e-10)
    {
        LastError = "ENF has to be positive!";
        SignalPerPhEl[ipm] = -1;
        return false;
    }

    ChannelsPerPhEl /= enf;
    //  qDebug() << ipm << "> ChPerPhEl:"<<ChannelsPerPhEl<<" const:"<<cutoff;

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

// ------------ from photoelectron peaks --------------------

ACalibratorSignalPerPhEl_Peaks::ACalibratorSignalPerPhEl_Peaks(const EventsDataClass &EventsDataHub, QVector<TH1D *> &DataHists, QVector<double> &SignalPerPhEl, QVector< QVector<double>>& foundPeaks) :
    ACalibratorSignalPerPhEl(EventsDataHub, DataHists, SignalPerPhEl), FoundPeaks(foundPeaks) {}

bool ACalibratorSignalPerPhEl_Peaks::PrepareData()
{
    if (EventsDataHub.isEmpty())
      {
        LastError = "There are no signal data!";
        return false;
      }

    ClearData();
    const int numPMs = EventsDataHub.getNumPMs();
    const int numEvents = EventsDataHub.Events.size();

    for (int ipm = 0; ipm < numPMs; ipm++)
    {
        TH1D* h = new TH1D("", "", numBins, rangeFrom, rangeTo);
        h->SetXTitle("Signal");

        for (int iev = 0; iev < numEvents; iev++)
        {
            if (iev % 10000 == 0)
            {
                int ipr = 100.0 *  (ipm * numEvents + iev) / (numPMs * numEvents);
                emit progressChanged(ipr);
                qApp->processEvents();
            }

            h->Fill(EventsDataHub.getEvent(iev)->at(ipm));
        }
        DataHists << h;
    }

    progressChanged(100);
    return true;
}

bool ACalibratorSignalPerPhEl_Peaks::Extract(int ipm)
{
    if (DataHists.size()     <= ipm || !DataHists.at(ipm)) return false;

    if (FoundPeaks.size()    <= ipm) FoundPeaks.resize(ipm+1);
    if (SignalPerPhEl.size() <= ipm) SignalPerPhEl.resize(ipm+1);

    APeakFinder f(DataHists.at(ipm));
    QVector<double> peaks = f.findPeaks(sigma, threshold, maxNumPeaks, true);
    std::sort(peaks.begin(), peaks.end());
    FoundPeaks[ipm] = peaks;

    double constant, slope;
    int ifail;

    bool bOK = (peaks.size() > 1);
    if (bOK)
    {
        TGraph g;
        for (int i=0; i<peaks.size(); i++) g.SetPoint(i, i, peaks.at(i));

        g.LeastSquareLinearFit(peaks.size(), constant, slope, ifail);

        if (ifail != 0) bOK = false;
    }

    if (bOK) SignalPerPhEl[ipm] = slope;
    else     SignalPerPhEl[ipm] = -1;

    return bOK;
}

bool ACalibratorSignalPerPhEl_Peaks::ExtractSignalPerPhEl(int ipm)
{
    const int numPMs = EventsDataHub.getNumPMs();
    if (ipm < 0 || ipm >= numPMs)
    {
        LastError = "Bad PM index";
        return false;
    }
    if (DataHists.size() != numPMs)
    {
        LastError = "Signal data not ready!";
        return false;
    }

    if (FoundPeaks.size() != numPMs) FoundPeaks.resize(numPMs);
    if (SignalPerPhEl.size() != numPMs) SignalPerPhEl.resize(numPMs);

    bool bOK = Extract(ipm);

    if (bOK) LastError.clear();
    else     LastError = "Peak finding failed for PM #" + QString::number(ipm);

    return bOK;
}

bool ACalibratorSignalPerPhEl_Peaks::ExtractSignalPerPhEl()
{
    const int numPMs = EventsDataHub.getNumPMs();
    if (DataHists.size() != numPMs)
    {
        LastError = "Signal data not ready!";
        return false;
    }

    FoundPeaks.resize(numPMs);
    SignalPerPhEl.resize(numPMs);

    QVector<int> failedPMs;
    for (int ipm = 0; ipm < numPMs; ipm++)
    {
        bool bOK = Extract(ipm);
        if (!bOK) failedPMs << ipm;
    }

    if (failedPMs.isEmpty())
    {
        LastError.clear();
        return true;
    }
    else
    {
        LastError = "Peaks < 2 or failed fit for PM#:";
        for (int i : failedPMs) LastError += " " + QString::number(i);
        return false;
    }
}

void ACalibratorSignalPerPhEl_Peaks::SetNumBins(int bins)
{
    numBins = bins;
}

void ACalibratorSignalPerPhEl_Peaks::SetRange(double from, double to)
{
    rangeFrom = from;
    rangeTo = to;
}

void ACalibratorSignalPerPhEl_Peaks::SetSigma(double sigmaPeakfinder)
{
    sigma = sigmaPeakfinder;
}

void ACalibratorSignalPerPhEl_Peaks::SetThreshold(double thresholdPeakfinder)
{
    threshold = thresholdPeakfinder;
}

void ACalibratorSignalPerPhEl_Peaks::SetMaximumPeaks(int number)
{
    maxNumPeaks = number;
}
