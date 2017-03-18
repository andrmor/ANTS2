#include "acustomrandomsampling.h"

#include <QDebug>

#include "TRandom2.h"

ACustomRandomSampling::ACustomRandomSampling(TRandom2* RandGen, const QVector<double>* Probabilities)
  : RandGen(RandGen), Probs(Probabilities)
{
  fUndefined = true;
  if (Probs->isEmpty() || Probs->size() == 1) return;

  Sum = 0;
  for (int i=0; i<Probs->size(); i++)
    {
      double v = Probs->at(i);
      if (v < 0)
        {
          qWarning() << "ACustomRandomSampler: negative value in the probabilities!";
          return;
        }
      Sum += v;
      if (v!=0) Size = i+1;
    }

  if (Size==1) return;
  if (Sum == 0) return;

  fUndefined = false;
}

void ACustomRandomSampling::reportSettings() const
{
  qDebug() << "Data:" << *Probs;
  qDebug() << "Undefined flag =" << fUndefined;
  if (!fUndefined)
    {
      qDebug() << "Sum ="<<Sum;
      qDebug() << "True size = "<<Size;
    }
}

int ACustomRandomSampling::sample() const
{
   if (fUndefined) return 0;

   double rnd = RandGen->Rndm()*Sum;

   double cum = 0;
   for (int i=0; i<Size; i++)
     {
       cum += Probs->at(i);
       if (rnd < cum) return i;
     }

   return Size-1; //paranoic protection
}
