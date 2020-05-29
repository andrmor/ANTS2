#include "amath_si.h"

#include "TRandom2.h"

AMath_SI::AMath_SI(TRandom2* RandGen)
{
  Description = "Expanded math module; implemented using std and CERN ROOT functions";

  this->RandGen = RandGen;

  H["random"] = "Returns a random number between 0 and 1.\nGenerator respects the seed set by SetSeed method of the sim module!";
  H["gauss"] = "Returns a random value sampled from Gaussian distribution with mean and sigma given by the user";
  H["poisson"] = "Returns a random value sampled from Poisson distribution with mean given by the user";
  H["maxwell"] = "Returns a random value sampled from maxwell distribution with Sqrt(kT/M) given by the user";
  H["exponential"] = "Returns a random value sampled from exponential decay with decay time given by the user";
  H["fit1D"] = "Fits the array of [x,y] points using the provided TFormula of Cern ROOT. Retuns array of parameter values";
}

void AMath_SI::setRandomGen(TRandom2 *RandGen)
{
  this->RandGen = RandGen;
}

double AMath_SI::abs(double val)
{
  return std::abs(val);
}

double AMath_SI::acos(double val)
{
  return std::acos(val);
}

double AMath_SI::asin(double val)
{
  return std::asin(val);
}

double AMath_SI::atan(double val)
{
  return std::atan(val);
}

double AMath_SI::atan2(double y, double x)
{
  return std::atan2(y, x);
}

double AMath_SI::ceil(double val)
{
  return std::ceil(val);
}

double AMath_SI::cos(double val)
{
  return std::cos(val);
}

double AMath_SI::exp(double val)
{
  return std::exp(val);
}

double AMath_SI::floor(double val)
{
  return std::floor(val);
}

double AMath_SI::log(double val)
{
  return std::log(val);
}

double AMath_SI::max(double val1, double val2)
{
  return std::max(val1, val2);
}

double AMath_SI::min(double val1, double val2)
{
  return std::min(val1, val2);
}

double AMath_SI::pow(double val, double power)
{
  return std::pow(val, power);
}

double AMath_SI::sin(double val)
{
  return std::sin(val);
}

double AMath_SI::sqrt(double val)
{
  return std::sqrt(val);
}

double AMath_SI::tan(double val)
{
    return std::tan(val);
}

double AMath_SI::round(double val)
{
  int f = std::floor(val);
  if (val>0)
    {
      if (val - f < 0.5) return f;
      else return f+1;
    }
  else
    {
      if (val - f < 0.5 ) return f;
      else return f+1;
    }
}

double AMath_SI::random()
{
  if (!RandGen) return 0;
  return RandGen->Rndm();
}

double AMath_SI::gauss(double mean, double sigma)
{
  if (!RandGen) return 0;
  return RandGen->Gaus(mean, sigma);
}

double AMath_SI::poisson(double mean)
{
  if (!RandGen) return 0;
  return RandGen->Poisson(mean);
}

double AMath_SI::maxwell(double a)
{
  if (!RandGen) return 0;

  double v2 = 0;
  for (int i=0; i<3; i++)
    {
      double v = RandGen->Gaus(0, a);
      v *= v;
      v2 += v;
    }
  return std::sqrt(v2);
}

double AMath_SI::exponential(double tau)
{
    if (!RandGen) return 0;
    return RandGen->Exp(tau);
}

#include "TFormula.h"
#include "TF1.h"
#include "TGraph.h"
#include "QVariant"
#include "QVariantList"
#include "TFitResult.h"
#include "TFitResultPtr.h"
QVariantList AMath_SI::fit1D(QVariantList array, QString tformula, QVariantList range, QVariantList startParValues, bool extendedOutput)
{
    QVariantList res;
    TFormula * f = new TFormula("", tformula.toLocal8Bit().data());
    if (!f || !f->IsValid())
    {
        abort("Cannot create TFormula");
        return res;
    }

    bool bParVals = false;
    int numPars = f->GetNpar();
    if (!startParValues.isEmpty())
    {
        if (startParValues.size() != numPars)
        {
            abort("Mismatch in the number of parameter for provided start parameter values");
            return res;
        }
        bParVals = true;
    }

    bool bRange = false;
    double from = 0;
    double to = 1;
    if (!range.isEmpty())
    {
        if (range.size() != 2)
        {
            abort("Range should contain start and stop values");
            return res;
        }
        bool ok1, ok2;
        bRange = true;
        from = range[0].toDouble(&ok1);
        to   = range[1].toDouble(&ok2);
        if (!ok1 || !ok2)
        {
            abort("Format error in range");
            return res;
        }
    }

    const int size = array.size();
    if (size == 0)
    {
        abort("Array is empty!");
        return res;
    }
    double xx[size];
    double yy[size];
    for (int i=0; i<array.size(); i++)
    {
        QVariantList el = array[i].toList();
        if (el.size() != 2)
        {
            abort("array argument must contain arrays of [x,val]!");
            return res;
        }
        xx[i] = el[0].toDouble();
        yy[i] = el[1].toDouble();
    }

    TGraph g(size, xx, yy);

    TF1  *f1 = new TF1("f1", tformula.toLocal8Bit().data(), from, to);

    if (bParVals)
    {
        for (int i=0; i<numPars; i++)
            f1->SetParameter(i, startParValues[i].toDouble());
    }

    TString opt = (bRange ? "SR" : "S");
    TFitResultPtr fr = g.Fit(f1, opt, "");

    //int numPars = fr->NTotalParameters();

    if (extendedOutput)
    {
        for (int i=0; i<numPars; i++)
        {
            QVariantList el;
            el << fr->Value(i) << fr->ParError(i);
            res.push_back(el);
        }
        res.push_back(fr->Chi2());
    }
    else
    {
        for (int i=0; i<numPars; i++) res << fr->GetParams()[i];
    }

    return res;
}
