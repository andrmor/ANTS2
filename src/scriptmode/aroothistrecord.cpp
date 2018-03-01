#include "aroothistrecord.h"

#include "TH1D.h"
#include "TH2D.h"
#include "TF1.h"

ARootHistRecord::ARootHistRecord(TObject *hist, const QString &name, QString type) :
    Hist(hist), Name(name), Type(type) {}

TObject *ARootHistRecord::GetHistForDrawing()
{
    return Hist;
}

void ARootHistRecord::SetTitles(QString X_Title, QString Y_Title, QString Z_Title)
{
    if (!Hist) return;
    QMutexLocker locker(&Mutex);

    if (Type == "TH1D")
      {
        TH1D* h = static_cast<TH1D*>(Hist);
        h->SetXTitle(X_Title.toLatin1().data());
        h->SetYTitle(Y_Title.toLatin1().data());
      }
    else if (r.type == "TH2D")
      {
        TH2D* h = static_cast<TH2D*>(r.Obj);
        h->SetXTitle(X_Title.toLatin1().data());
        h->SetYTitle(Y_Title.toLatin1().data());
        h->SetZTitle(Z_Title.toLatin1().data());
    }
}

void ARootHistRecord::SetLineProperties(int LineColor, int LineStyle, int LineWidth)
{
    if (!Hist) return;
    QMutexLocker locker(&Mutex);

    if (Type == "TH1D")
      {
        TH1D* h = static_cast<TH1D*>(Hist);
        h->SetLineColor(LineColor);
        h->SetLineWidth(LineWidth);
        h->SetLineStyle(LineStyle);
      }
    else if (r.type == "TH2D")
      {
        TH2D* h = static_cast<TH2D*>(r.Obj);
        h->SetLineColor(LineColor);
        h->SetLineWidth(LineWidth);
        h->SetLineStyle(LineStyle);
    }
}

void ARootHistRecord::Fill(double val, double weight)
{
    if (!Hist) return;
    QMutexLocker locker(&Mutex);

    if (Type == "TH1D")
      {
        TH1D* h = static_cast<TH1D*>(Hist);
        h->Fill(val, weight);
    }
}

void ARootHistRecord::Fill2D(double x, double y, double weight)
{
    if (!Hist) return;
    QMutexLocker locker(&Mutex);

    if (Type == "TH2D")
      {
        TH2D* h = static_cast<TH2D*>(Hist);
        h->Fill(x, y, weight);
    }
}

void ARootHistRecord::Smooth(int times)
{
    if (!Hist) return;
    QMutexLocker locker(&Mutex);

    if (Type == "TH1D")
      {
        TH1D* h = static_cast<TH1D*>(Hist);
        h->Smooth(times);
      }
    else if (Type == "TH2D")
      {
        TH2D* h = static_cast<TH2D*>(Hist);
        h->Smooth(times);
    }
}

const QVector<double> ARootHistRecord::FitGaussWithInit(const QVector<double> &InitialParValues, const QString options)
{
    if (!Hist) return;
    QMutexLocker locker(&Mutex);

    QVector<double> res;

    if (InitialParValues.size() != 3) return res;
    if (Type.startsWith("TH1"))
      {
        TH1* h = static_cast<TH1*>(Hist);

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
    if (!Hist) return;
    QMutexLocker locker(&Mutex);

    QVector<double> res;
    if (Type.startsWith("TH1"))
      {
        TH1* h = static_cast<TH1*>(Hist);
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
