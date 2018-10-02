#include "amathscriptinterface.h"

#include "TRandom2.h"

AMathScriptInterface::AMathScriptInterface(TRandom2* RandGen)
{
  Description = "Expanded math module; implemented using std and CERN ROOT functions";

  this->RandGen = RandGen;

  H["random"] = "Returns a random number between 0 and 1.\nGenerator respects the seed set by SetSeed method of the sim module!";
  H["gauss"] = "Returns a random value sampled from Gaussian distribution with mean and sigma given by the user";
  H["poisson"] = "Returns a random value sampled from Poisson distribution with mean given by the user";
  H["maxwell"] = "Returns a random value sampled from maxwell distribution with Sqrt(kT/M) given by the user";
  H["exponential"] = "Returns a random value sampled from exponential decay with decay time given by the user";
}

void AMathScriptInterface::setRandomGen(TRandom2 *RandGen)
{
  this->RandGen = RandGen;
}

double AMathScriptInterface::abs(double val)
{
  return std::abs(val);
}

double AMathScriptInterface::acos(double val)
{
  return std::acos(val);
}

double AMathScriptInterface::asin(double val)
{
  return std::asin(val);
}

double AMathScriptInterface::atan(double val)
{
  return std::atan(val);
}

double AMathScriptInterface::atan2(double y, double x)
{
  return std::atan2(y, x);
}

double AMathScriptInterface::ceil(double val)
{
  return std::ceil(val);
}

double AMathScriptInterface::cos(double val)
{
  return std::cos(val);
}

double AMathScriptInterface::exp(double val)
{
  return std::exp(val);
}

double AMathScriptInterface::floor(double val)
{
  return std::floor(val);
}

double AMathScriptInterface::log(double val)
{
  return std::log(val);
}

double AMathScriptInterface::max(double val1, double val2)
{
  return std::max(val1, val2);
}

double AMathScriptInterface::min(double val1, double val2)
{
  return std::min(val1, val2);
}

double AMathScriptInterface::pow(double val, double power)
{
  return std::pow(val, power);
}

double AMathScriptInterface::sin(double val)
{
  return std::sin(val);
}

double AMathScriptInterface::sqrt(double val)
{
  return std::sqrt(val);
}

double AMathScriptInterface::tan(double val)
{
    return std::tan(val);
}

double AMathScriptInterface::round(double val)
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

double AMathScriptInterface::random()
{
  if (!RandGen) return 0;
  return RandGen->Rndm();
}

double AMathScriptInterface::gauss(double mean, double sigma)
{
  if (!RandGen) return 0;
  return RandGen->Gaus(mean, sigma);
}

double AMathScriptInterface::poisson(double mean)
{
  if (!RandGen) return 0;
  return RandGen->Poisson(mean);
}

double AMathScriptInterface::maxwell(double a)
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

double AMathScriptInterface::exponential(double tau)
{
    if (!RandGen) return 0;
    return RandGen->Exp(tau);
}
