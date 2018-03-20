#ifndef ACALIBRATORSIGNALPERPHEL_H
#define ACALIBRATORSIGNALPERPHEL_H

#include <QObject>
#include <QVector>
#include <QString>

class EventsDataClass;
class TH1D;
class DetectorClass;

class ACalibratorSignalPerPhEl : public QObject
{
    Q_OBJECT

public:
    ACalibratorSignalPerPhEl(const EventsDataClass& EventsDataHub, QVector<TH1D*>& DataHists, QVector<double>& SignalPerPhEl);
    virtual ~ACalibratorSignalPerPhEl();

    virtual bool   PrepareData() = 0;
    virtual bool   ExtractSignalPerPhEl(int ipm) = 0;
    virtual bool   ExtractSignalPerPhEl() = 0; //for all pms

    virtual void   ClearData();

    TH1D*          GetHistogram(int ipm);
    double         GetSignalPerPhEl(int ipm) const;

    const QString& GetLastError() const;

protected:
    const EventsDataClass& EventsDataHub;
    QVector<TH1D*>&        DataHists;
    QVector<double>&       SignalPerPhEl;

    QString                LastError;

signals:
    void            progressChanged(int percents);

};

class ACalibratorSignalPerPhEl_Stat : public ACalibratorSignalPerPhEl
{
public:
    ACalibratorSignalPerPhEl_Stat(const EventsDataClass& EventsDataHub, QVector<TH1D*>& DataHists, QVector<double>& SignalPerPhEl, DetectorClass& Detector);

    virtual bool PrepareData() override;
    virtual bool ExtractSignalPerPhEl(int ipm) override;
    virtual bool ExtractSignalPerPhEl() override;

    void         SetNumBins(int bins);
    void         SetRange(double min, double max);
    void         SetThresholdSigmaCalc(int threshold);
    void         SetENF(int ENF);
    void         SetSignalLimits(double lower, double upper);

protected:
    DetectorClass& Detector;

    int    numBins = 100;
    double minRange = 0;
    double minRange2 = 0;
    double maxRange = 100;
    double maxRange2 = 10000;

    int    sigmaCalculationThreshold = 5;

    double enf = 1.0;

    double lowerLimit = 0;
    double upperLimit = 1e10;

private:
    double calculateSignalLimit(int ipm, double range);
};

class ACalibratorSignalPerPhEl_Peaks : public ACalibratorSignalPerPhEl
{
public:
    ACalibratorSignalPerPhEl_Peaks(const EventsDataClass& EventsDataHub, QVector<TH1D*>& DataHists, QVector<double>& SignalPerPhEl, QVector< QVector<double>>& foundPeaks);

    virtual bool PrepareData() override;
    virtual bool ExtractSignalPerPhEl(int ipm) override;
    virtual bool ExtractSignalPerPhEl() override;

    void         SetNumBins(int bins);
    void         SetRange(double from, double to);

    void         SetSigma(double sigmaPeakfinder);
    void         SetThreshold(double thresholdPeakfinder);
    void         SetMaximumPeaks(int number);

protected:
    int          numBins = 300;
    double       rangeFrom = -50;
    double       rangeTo = 250;

    double       sigma = 8.0;
    double       threshold = 0.02;
    int          maxNumPeaks = 30;

    QVector< QVector<double>>& FoundPeaks;

private:
    bool         extract(int ipm); //no watchdogs!
};

#endif // ACALIBRATORSIGNALPERPHEL_H
