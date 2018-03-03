#include "aroothistrecord.h"

#include "TH1D.h"
#include "TH2D.h"
#include "TF1.h"

ARootHistRecord::ARootHistRecord(TObject *hist, const QString &title, const QString &type) :
     ARootObjBase(hist, title, type) {}

TObject* ARootHistRecord::GetObject()
{
    return Object;
}

void ARootHistRecord::SetTitles(const QString X_Title, const QString Y_Title, const QString Z_Title)
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

    if (Type == "TH1D")
      {
        TH1D* h = static_cast<TH1D*>(Object);
        h->SetLineColor(LineColor);
        h->SetLineWidth(LineWidth);
        h->SetLineStyle(LineStyle);
      }
    else if (Type == "TH2D")
      {
        TH2D* h = static_cast<TH2D*>(Object);
        h->SetLineColor(LineColor);
        h->SetLineWidth(LineWidth);
        h->SetLineStyle(LineStyle);
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
