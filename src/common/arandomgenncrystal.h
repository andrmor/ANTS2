#ifndef ARANDOMGENNCRYSTAL_H
#define ARANDOMGENNCRYSTAL_H

#include "NCrystal/NCrystal.hh"

class TRandom2;

class ARandomGenNCrystal : public NCrystal::RandomBase
{
public:
  ARandomGenNCrystal(TRandom2& RandGen);

  virtual double generate() override;

protected:
  virtual ~ARandomGenNCrystal() {}

  TRandom2& RandGen;
};

#endif // ARANDOMGENNCRYSTAL_H
