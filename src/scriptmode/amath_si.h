#ifndef AMATHSCRIPTINTERFACE_H
#define AMATHSCRIPTINTERFACE_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVariantList>

class TRandom2;

class AMath_SI : public AScriptInterface
{
  Q_OBJECT
  Q_PROPERTY(double pi READ pi)
  double pi() const { return 3.141592653589793238462643383279502884; }

public:
  AMath_SI(TRandom2* RandGen);
  void setRandomGen(TRandom2* RandGen);

  virtual bool IsMultithreadCapable() const override {return true;}

public slots:
  double abs(double val);
  double acos(double val);
  double asin(double val);
  double atan(double val);
  double atan2(double y, double x);
  double ceil(double val);
  double cos(double val);
  double exp(double val);
  double floor(double val);
  double log(double val);
  double max(double val1, double val2);
  double min(double val1, double val2);
  double pow(double val, double power);
  double sin(double val);
  double sqrt(double val);
  double tan(double val);
  double round(double val);
  double random();
  double gauss(double mean, double sigma);
  double poisson(double mean);
  double maxwell(double a);  // a is sqrt(kT/m)
  double exponential(double tau);

  //QVariantList fit1D(QVariantList array, QString tformula, QVariantList range = QVariantList(), QVariantList startParValues = QVariantList(), bool extendedOutput = false);
  QVariantList fit1D(QVariantList array, QString tformula, QVariantList startParValues = QVariantList(), bool extendedOutput = false);

private:
  TRandom2* RandGen;
};

#endif // AMATHSCRIPTINTERFACE_H
