#include "apm.h"

#include <QTextStream>

#include "TH1D.h"

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
