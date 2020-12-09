#include "amath_si.h"

#include <QDebug>
#include <QVector>

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
  H["fit1D"] = "Fits the array of [x,y] points using the provided TFormula of Cern ROOT.\n"
          "Optional startParValues arguments can hold array of initial parameter values.\n"
          "Returned value depends on the extendedOutput argument (false by default),\n"
          "false: array of parameter values; true: array of [value, error] for each parameter";
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

double AMath_SI::log10(double val)
{
    return std::log10(val);
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
//QVariantList AMath_SI::fit1D(QVariantList array, QString tformula, QVariantList range, QVariantList startParValues, bool extendedOutput)
QVariantList AMath_SI::fit1D(QVariantList array, QString tformula, QVariantList startParValues, bool extendedOutput)
{
    QVariantList res;
    TFormula * f = new TFormula("", tformula.toLocal8Bit().data());
    if (!f || !f->IsValid())
    {
        delete f;
        abort("Cannot create TFormula");
        return res;
    }
    int numPars = f->GetNpar();
    //qDebug() << "TFormula accepted, #par = " << numPars;

    bool bParVals = false;
    QVector<double> ParValues;
    bool ok1, ok2;
    if (!startParValues.isEmpty())
    {
        if (startParValues.size() != numPars)
        {
            delete f;
            abort("Mismatch in the number of parameters for provided initial values");
            return res;
        }
        for (int i=0; i<startParValues.size(); i++)
        {
            double v = startParValues[i].toDouble(&ok1);
            if (!ok1)
            {
                delete f;
                abort("Format error in range");
                return res;
            }
            ParValues << v;
        }
        bParVals = true;
        //qDebug() << "Initial values:" << ParValues;
    }

    //bool bRange = false;
    double from = 0;
    double to = 1.0;
    /*
    if (!range.isEmpty())
    {
        if (range.size() != 2)
        {
            delete f;
            abort("Range should contain start and stop values");
            return res;
        }
        from = range[0].toDouble(&ok1);
        to   = range[1].toDouble(&ok2);
        if (!ok1 || !ok2)
        {
            delete f;
            abort("Format error in range");
            return res;
        }
        bRange = true;
        //qDebug() << "Fixed range:" << from << to;
    }
    */

    const int arSize = array.size();
    //qDebug() << "Data size:"<< arSize;
    if (arSize == 0)
    {
        delete f;
        abort("Array is empty!");
        return res;
    }
    QVector<double> xx; xx.reserve(arSize);
    QVector<double> yy; yy.reserve(arSize);
    //qDebug() << "Vectors are initialized";

    for (int i=0; i<arSize; i++)
    {
        QVariantList el = array[i].toList();
        if (el.size() != 2)
        {
            delete f;
            abort("array argument must contain arrays of [x,val]!");
            return res;
        }

        xx << el[0].toDouble(&ok1);
        yy << el[1].toDouble(&ok2);
        if (!ok1 || !ok2)
        {
            delete f;
            abort("Format error in data");
            return res;
        }
    }

    TGraph g(arSize, xx.data(), yy.data());
    //qDebug() << "Graph created";

    TF1  * f1 = new TF1("f1", tformula.toLocal8Bit().data(), from, to);
    //qDebug() << "TF1 created" << f1;

    if (bParVals)
    {
        for (int i=0; i<numPars; i++) f1->SetParameter(i, ParValues[i]);
        //qDebug() << "Init par values are set!";
    }

    TString opt = "SQN"; //(bRange ? "SR" : "S"); //https://root.cern.ch/root/htmldoc/guides/users-guide/FittingHistograms.html
    TFitResultPtr fr = g.Fit(f1, opt, "");

    //qDebug() << "Fit done!";

    if ((int)fr->NTotalParameters() != numPars)
    {
        delete f1;
        delete f;
        abort("Bad number of parameters in fit result");
        return res;
    }

    if (extendedOutput)
    {
        for (int i=0; i<numPars; i++)
        {
            QVariantList el;
            el << fr->Value(i) << fr->ParError(i);
            res.push_back(el);
        }
        //res.push_back(fr->Chi2());
    }
    else
    {
        for (int i=0; i<numPars; i++) res << fr->Value(i);
    }

    delete f1;
    delete f;
    return res;
}
