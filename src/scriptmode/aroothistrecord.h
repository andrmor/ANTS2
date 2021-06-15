#ifndef AROOTHISTRECORD_H
#define AROOTHISTRECORD_H

#include "arootobjbase.h"

#include <QVector>
#include <QString>
#include <QMutex>

#include <TObject.h>

class ARootHistRecord : public ARootObjBase
{
public:
    ARootHistRecord(TObject* hist, const QString&  title, const QString& type);

    TObject* GetObject() override;  // unsave for multithread (draw on queued signal), only GUI thread can trigger draw

    void SetTitle(const QString Title);
    void SetAxisTitles(const QString X_Title, const QString Y_Title, const QString Z_Title = "");
    void SetLineProperties(int LineColor, int LineStyle, int LineWidth);
    void SetMarkerProperties(int MarkerColor, int MarkerStyle, double MarkerSize);
    void SetFillColor(int Color);
    void SetXLabels(const QVector<QString> & labels);
    void SetXDivisions(int primary, int secondary, int tertiary, bool canOptimize);
    void SetYDivisions(int primary, int secondary, int tertiary, bool canOptimize);
    void SetXLabelProperties(double size, double offset);
    void SetYLabelProperties(double size, double offset);

    void Fill(double val, double weight);
    void Fill2D(double x, double y, double weight);
    void Fill3D(double x, double y, double z, double weight);

    void FillArr(const QVector<double>& val, const QVector<double>& weight);
    void Fill2DArr(const QVector<double>& x, const QVector<double>& y, const QVector<double>& weight);

    void SetMax(double max);
    void SetMin(double min);

    void Save(const QString & fileName) const;

    bool Divide(ARootHistRecord* other);

    void   Smooth(int times);
    void   Scale(double ScaleIntegralTo, bool bDividedByBinWidth = false);
    bool   MedianFilter(int span, int spanRight = -1);

    double GetIntegral(bool bMultipliedByBinWidth = false);
    int    GetEntries();
    void   SetEntries(int num);
    double GetMaximum();
    bool   GetContent(QVector<double> & x, QVector<double> & y) const;
    bool   GetContent2D(QVector<double> & x, QVector<double> & y, QVector<double> & z) const;
    bool   GetContent3D(QVector<double> & x, QVector<double> & y, QVector<double> & z, QVector<double> & val) const;
    bool   GetUnderflow(double & undeflow) const;
    bool   GetOverflow (double & overflow) const;
    double GetRandom();
    QVector<double> GetRandomMultiple(int numRandoms);

    bool   is1D() const;
    bool   is2D() const;
    bool   is3D() const;

    QVector<double> FitGauss(const QString& options = "");
    QVector<double> FitGaussWithInit(const QVector<double>& InitialParValues, const QString options = "");
    QVector<double> FindPeaks(double sigma, double threshold);

};

#endif // AROOTHISTRECORD_H
