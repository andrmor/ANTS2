#include "apm.h"

#include <QTextStream>

#include "TH1D.h"
#include "TMath.h"

APm::~APm()
{
   clearSPePHSCustomDist();
}

void APm::saveCoords(QTextStream &file) const
{
    file << x << " " << y << " " << z << "\r\n";
}

void APm::saveCoords2D(QTextStream &file) const
{
    file << x << " " << y << "\r\n";
}

void APm::saveAngles(QTextStream &file) const
{
    file << phi << " " << theta << " " << psi << "\r\n";
}

void APm::preparePHS()
{
  if (SPePHShist)
    {
      delete SPePHShist;
      SPePHShist = 0;
    }

  const int size = SPePHS_x.size();
  if (size < 1) return;

  //SPePHShist = new TH1D("SPePHS"+ipm,"SPePHS", size, SPePHS_x.at(0), SPePHS_x.at(size-1));
  SPePHShist = new TH1D("", "", size, SPePHS_x.at(0), SPePHS_x.at(size-1));
  for (int j = 1; j<size+1; j++) SPePHShist->SetBinContent(j, SPePHS.at(j-1));
  SPePHShist->GetIntegral(); //to make thread safe
}

void APm::clearSPePHSCustomDist()
{
    SPePHS_x.clear();
    SPePHS.clear();
    if (SPePHShist)
    {
        delete SPePHShist;
        SPePHShist = 0;
    }
}

void APm::setADC(double max, int bits)
{
    ADCmax = max;
    ADCbits = bits;
    ADClevels = TMath::Power(2, bits) - 1;
    ADCstep = max / ADClevels;
}

void APm::setElChanSPePHS(const QVector<double>& x, const QVector<double>& y)
{
    SPePHS_x = x;
    SPePHS = y;
}

void APm::scaleSPePHS(double gain)
{
    if (gain == AverageSigPerPhE) return; //nothing to change

    if (fabs(gain) > 1e-20) gain /= AverageSigPerPhE;
    else gain = 0;

    if (SPePHSmode < 3) AverageSigPerPhE = AverageSigPerPhE * gain;
    else if (SPePHSmode == 3)
    {
        //custom SPePHS - have to adjust the distribution
        //QVector<double> x = SPePHS_x;
        //QVector<double> y = SPePHS;
        //for (int ix=0; ix<x.size(); ix++) x[ix] *= gain;
        //setElChanSPePHS(x, y);

        for (double& d : SPePHS_x) d *= gain;
    }
}
