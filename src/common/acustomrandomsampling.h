#ifndef ACUSTOMRANDOMSAMPLING_H
#define ACUSTOMRANDOMSAMPLING_H

#include <QVector>

class TRandom2;

class ACustomRandomSampling
{
public:
  ACustomRandomSampling(TRandom2* RandGen, const QVector<double> *Probabilities);

  int sample() const;  //always returns 0 if fUndefined
  void reportSettings() const;

private:
  TRandom2* RandGen;
  const QVector<double> *Probs;

  double Sum;
  bool fUndefined;

  int Size; //Probabilities without trailing zeros
};

#endif // ACUSTOMRANDOMSAMPLING_H
