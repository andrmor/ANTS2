#ifndef ACUSTOMRANDOMSAMPLING_H
#define ACUSTOMRANDOMSAMPLING_H

#include <QVector>

class TRandom2;

class ACustomRandomSampling
{
public:
  ACustomRandomSampling(const QVector<double> *Probabilities);

  int sample(TRandom2* RandGen) const;  //always returns 0 if fUndefined
  void reportSettings() const;

private:
  const QVector<double> *Probs;

  double Sum;
  bool fUndefined;

  int Size; //Probabilities without trailing zeros
};

#endif // ACUSTOMRANDOMSAMPLING_H
