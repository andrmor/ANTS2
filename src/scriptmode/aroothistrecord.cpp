#include "aroothistrecord.h"
#include "apeakfinder.h"

#include <QDebug>

#include "TH1D.h"
#include "TH2D.h"
#include "TF1.h"
#include "TAxis.h"

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

void ARootHistRecord::SetFillColor(int Color)
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (h) h->SetFillColor(Color);
}

void ARootHistRecord::SetXLabels(const QVector<QString> &labels)
{
    int numLabels = labels.size();

    TH1* h = dynamic_cast<TH1*>(Object);
    if (h)
    {
        TAxis * axis = h->GetXaxis();
        if (!axis) return;

        int numBins = axis->GetNbins();
        for (int i=1; i <= numLabels && i <= numBins ; i++)
            axis->SetBinLabel(i, labels.at(i-1).toLatin1().data());
    }
}

void ARootHistRecord::SetXDivisions(int primary, int secondary, int tertiary, bool canOptimize)
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (h)
    {
        TAxis * axis = h->GetXaxis();
        if (!axis) return;

        axis->SetNdivisions(primary, secondary, tertiary, canOptimize);
    }
}

void ARootHistRecord::SetYDivisions(int primary, int secondary, int tertiary, bool canOptimize)
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (h)
    {
        TAxis * axis = h->GetYaxis();
        if (!axis) return;

        axis->SetNdivisions(primary, secondary, tertiary, canOptimize);
    }
}

void ARootHistRecord::SetXLabelProperties(double size, double offset)
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (h)
    {
        TAxis * axis = h->GetXaxis();
        if (!axis) return;

        axis->SetLabelSize(size);
        axis->SetLabelOffset(offset);
    }
}

void ARootHistRecord::SetYLabelProperties(double size, double offset)
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (h)
    {
        TAxis * axis = h->GetYaxis();
        if (!axis) return;

        axis->SetLabelSize(size);
        axis->SetLabelOffset(offset);
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

void ARootHistRecord::SetMax(double max)
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (h) h->SetMaximum(max);
}

void ARootHistRecord::SetMin(double min)
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (h) h->SetMinimum(min);
}

void ARootHistRecord::Save(const QString &fileName) const
{
    QMutexLocker locker(&Mutex);

    if (Type == "TH1D")
    {
        TH1D* h = static_cast<TH1D*>(Object);
        TH1D* hc = new TH1D(*h);
        locker.unlock();
        hc->SaveAs(fileName.toLatin1());
        delete hc;
    }
    else //if (Type == "TH2D")
    {
        TH2D* h = static_cast<TH2D*>(Object);
        TH2D* hc = new TH2D(*h);
        locker.unlock();
        hc->SaveAs(fileName.toLatin1());
        delete hc;
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

bool ARootHistRecord::MedianFilter(int span, int spanRight)
{
    QMutexLocker locker(&Mutex);

    int deltaLeft = span;
    int deltaRight = spanRight;

    if (deltaRight == -1)
    {
        deltaLeft  = ( span % 2 == 0 ? span / 2 - 1 : span / 2 );
        deltaRight = span / 2;
    }
    //qDebug() << "Delta left:"<< deltaLeft <<" Delta right:"<< deltaRight;

    TH1* h = dynamic_cast<TH1*>(Object);
    if (!h) return false;

    QVector<double> Filtered;
    int num = h->GetNbinsX();
    for (int iThisBin = 1; iThisBin <= num; iThisBin++)  // 0-> underflow; num+1 -> overflow
    {
        QVector<double> content;
        for (int i = iThisBin - deltaLeft; i <= iThisBin + deltaRight; i++)
        {
            if (i < 1 || i > num) continue;
            content << h->GetBinContent(i);
        }

        std::sort(content.begin(), content.end());
        int size = content.size();
        double val;
        if (size == 0) val = 0;
        else val = ( size % 2 == 0 ? (content[size / 2 - 1] + content[size / 2]) / 2 : content[size / 2] );

        Filtered.append(val);
    }
    //qDebug() << "Result:" << Filtered;

    for (int iThisBin = 1; iThisBin <= num; iThisBin++)
        h->SetBinContent(iThisBin, Filtered.at(iThisBin-1));

    return true;
}

double ARootHistRecord::GetIntegral(bool bMultipliedByBinWidth)
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (!h) return 1.0;
    return ( bMultipliedByBinWidth ? h->Integral("width") : h->Integral() );
}

int ARootHistRecord::GetEntries()
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (!h) return 0;

    return h->GetEntries();
}

void ARootHistRecord::SetEntries(int num)
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (!h) return;

    h->SetEntries(num);
}

double ARootHistRecord::GetMaximum()
{
    TH1* h = dynamic_cast<TH1*>(Object);
    if (!h) return 1.0;

    return h->GetMaximum();
}

bool ARootHistRecord::GetContent(QVector<double> &x, QVector<double> &y) const
{
    QMutexLocker locker(&Mutex);

    if (!Type.startsWith("TH1")) return false;
    TH1* h = dynamic_cast<TH1*>(Object);
    if (!h) return false;

    int num = h->GetNbinsX();
    for (int i = 1; i <= num; i++)
    {
        x.append(h->GetBinCenter(i));
        y.append(h->GetBinContent(i));
    }
    return true;
}

bool ARootHistRecord::GetContent2D(QVector<double> & x, QVector<double> & y, QVector<double> & z) const
{
    QMutexLocker locker(&Mutex);

    if (!Type.startsWith("TH2")) return false;
    TH2* h = dynamic_cast<TH2*>(Object);
    if (!h) return false;

    int numX = h->GetNbinsX();
    int numY = h->GetNbinsY();
    for (int iy = 1; iy <= numY; iy++)
        for (int ix = 1; ix <= numX; ix++)
        {
            x.append(h->GetXaxis()->GetBinCenter(ix));
            y.append(h->GetYaxis()->GetBinCenter(iy));
            z.append(h->GetBinContent(ix, iy));
        }
    return true;
}

bool ARootHistRecord::GetUnderflow(double & undeflow) const
{
    if (!Type.startsWith("TH1")) return false;
    TH1* h = dynamic_cast<TH1*>(Object);
    if (!h) return false;

    undeflow = h->GetBinContent(0);
    return true;
}

bool ARootHistRecord::GetOverflow(double & overflow) const
{
    if (!Type.startsWith("TH1")) return false;
    TH1* h = dynamic_cast<TH1*>(Object);
    if (!h) return false;

    int num = h->GetNbinsX();
    overflow = h->GetBinContent(num+1);
    return true;
}

double ARootHistRecord::GetRandom()
{
    TH1 * h = dynamic_cast<TH1*>(Object);
    if (!h) return 0;

    return h->GetRandom();
}

QVector<double> ARootHistRecord::GetRandomMultiple(int numRandoms)
{
    QVector<double> res;
    TH1 * h = dynamic_cast<TH1*>(Object);
    if (!h) return res;

    res.reserve(numRandoms);
    for (int i=0; i<numRandoms; i++)
        res << h->GetRandom();

    return res;
}

bool ARootHistRecord::is1D() const
{
    return Type.startsWith("TH1");
}

bool ARootHistRecord::is2D() const
{
    return Type.startsWith("TH2");
}

QVector<double> ARootHistRecord::FitGaussWithInit(const QVector<double> &InitialParValues, const QString options)
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

QVector<double> ARootHistRecord::FindPeaks(double sigma, double threshold)
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

QVector<double> ARootHistRecord::FitGauss(const QString &options)
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
