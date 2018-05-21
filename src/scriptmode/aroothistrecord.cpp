#include "aroothistrecord.h"
#include "apeakfinder.h"

#include "TH1D.h"
#include "TH2D.h"
#include "TF1.h"

ARootHistRecord::ARootHistRecord(TObject *hist, const QString &title, const QString &type) :
     ARootObjBase(hist, title, type) {}

TObject* ARootHistRecord::GetObject()
{
    return Object;
}

void ARootHistRecord::SetTitle(const QString Title)
{
    QMutexLocker locker(&Mutex);
    TH1* h = dynamic_cast<TH1*>(Object);
    h->SetTitle(Title.toLatin1().data());
}

void ARootHistRecord::SetAxisTitles(const QString X_Title, const QString Y_Title, const QString Z_Title)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TH1D")
      {
        TH1D* h = static_cast<TH1D*>(Object);
        h->SetXTitle(X_Title.toLatin1().data());
        h->SetYTitle(Y_Title.toLatin1().data());
      }
    else if (Type == "TH2D")
      {
        TH2D* h = static_cast<TH2D*>(Object);
        h->SetXTitle(X_Title.toLatin1().data());
        h->SetYTitle(Y_Title.toLatin1().data());
        h->SetZTitle(Z_Title.toLatin1().data());
    }
}

void ARootHistRecord::SetLineProperties(int LineColor, int LineStyle, int LineWidth)
{
    QMutexLocker locker(&Mutex);

    TH1* h = dynamic_cast<TH1*>(Object);
    if (h)
    {
        h->SetLineColor(LineColor);
        h->SetLineWidth(LineWidth);
        h->SetLineStyle(LineStyle);
    }
}

void ARootHistRecord::SetMarkerProperties(int MarkerColor, int MarkerStyle, double MarkerSize)
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (h)
    {
        h->SetMarkerColor(MarkerColor);
        h->SetMarkerStyle(MarkerStyle);
        h->SetMarkerSize(MarkerSize);
    }
}

void ARootHistRecord::Fill(double val, double weight)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TH1D")
      {
        TH1D* h = static_cast<TH1D*>(Object);
        h->Fill(val, weight);
    }
}

void ARootHistRecord::Fill2D(double x, double y, double weight)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TH2D")
      {
        TH2D* h = static_cast<TH2D*>(Object);
        h->Fill(x, y, weight);
    }
}

void ARootHistRecord::FillArr(const QVector<double>& val, const QVector<double>& weight)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TH1D")
      {
        TH1D* h = static_cast<TH1D*>(Object);

        for (int i=0; i<val.size(); i++)
            h->Fill(val.at(i), weight.at(i));
    }
}

void ARootHistRecord::Fill2DArr(const QVector<double> &x, const QVector<double> &y, const QVector<double> &weight)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TH2D")
      {
        TH2D* h = static_cast<TH2D*>(Object);

        for (int i=0; i<x.size(); i++)
            h->Fill(x.at(i), y.at(i), weight.at(i));
    }
}

bool ARootHistRecord::Divide(ARootHistRecord *other)
{
    other->externalLock();
    QMutexLocker locker(&Mutex);

    TH1* h1 = dynamic_cast<TH1*>(Object);
    TH1* h2 = dynamic_cast<TH1*>(other->GetObject());

    bool bOK = false;
    if (h1 && h2)
         bOK = h1->Divide(h2);

    other->externalUnlock();
    return bOK;
}

void ARootHistRecord::Smooth(int times)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TH1D")
      {
        TH1D* h = static_cast<TH1D*>(Object);
        h->Smooth(times);
      }
    else if (Type == "TH2D")
      {
        TH2D* h = static_cast<TH2D*>(Object);
        h->Smooth(times);
    }
}

void ARootHistRecord::Scale(double ScaleIntegralTo, bool bDividedByBinWidth)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TH1D")
      {
        TH1* h = dynamic_cast<TH1*>(Object);
        h->Scale( ScaleIntegralTo, ( bDividedByBinWidth ? "width" : "" ) );
    }
}

double ARootHistRecord::GetIntegral(bool bMultipliedByBinWidth)
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (!h) return 1.0;
    return ( bMultipliedByBinWidth ? h->Integral("width") : h->Integral() );
}

double ARootHistRecord::GetMaximum()
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (!h) return 1.0;
    return h->GetMaximum();
}

const QVector<double> ARootHistRecord::FitGaussWithInit(const QVector<double> &InitialParValues, const QString options)
{
    QMutexLocker locker(&Mutex);

    QVector<double> res;

    if (InitialParValues.size() != 3) return res;
    if (Type.startsWith("TH1"))
      {
        TH1* h = static_cast<TH1*>(Object);

        TF1 *f1 = new TF1("f1","[0]*exp(-0.5*((x-[1])/[2])^2)");
        f1->SetParameters(InitialParValues.at(0), InitialParValues.at(1), InitialParValues.at(2));

        int status = h->Fit(f1, options.toLatin1());
        if (status == 0)
        {
            for (int i=0; i<3; i++) res << f1->GetParameter(i);
            for (int i=0; i<3; i++) res << f1->GetParError(i);
        }
      }
    return res;
}

const QVector<double> ARootHistRecord::FindPeaks(double sigma, double threshold)
{
    QMutexLocker locker(&Mutex);

    QVector<double> res;

    TH1* h = dynamic_cast<TH1*>(Object);
    if (h)
    {
        APeakFinder pf(h);
        res = pf.findPeaks(sigma, threshold);
    }
    return res;
}

const QVector<double> ARootHistRecord::FitGauss(const QString &options)
{
    QMutexLocker locker(&Mutex);

    QVector<double> res;
    if (Type.startsWith("TH1"))
      {
        TH1* h = static_cast<TH1*>(Object);
        TF1 *f1 = new TF1("f1", "gaus");
        int status = h->Fit(f1, options.toLatin1().data());
        if (status == 0)
        {
            for (int i=0; i<3; i++) res << f1->GetParameter(i);
            for (int i=0; i<3; i++) res << f1->GetParError(i);
        }
      }
    return res;
}
