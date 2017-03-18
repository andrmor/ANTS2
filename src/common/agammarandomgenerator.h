#ifndef AGAMMARANDOMGENERATOR_H
#define AGAMMARANDOMGENERATOR_H

#include "Math/Random.h"
#include "Math/GSLRndmEngines.h"

//class AGammaRandomGenerator
//{
//public:
//    AGammaRandomGenerator(){}
//    double getGamma(double k, double theta){ return rnd.Gamma(k, theta);}
//private:
//    ROOT::Math::Random<ROOT::Math::GSLRandomEngine> rnd;
//};

class AGammaRandomGenerator
{
public:
    AGammaRandomGenerator(){rnd.Initialize();}
    double getGamma(double k, double theta){ return rnd.Gamma(k, theta);}
private:
    ROOT::Math::GSLRandomEngine rnd;
};

#endif // AGAMMARANDOMGENERATOR_H
