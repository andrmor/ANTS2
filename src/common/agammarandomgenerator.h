#ifndef AGAMMARANDOMGENERATOR_H
#define AGAMMARANDOMGENERATOR_H

/*
class AGammaRandomGenerator
{
public:
    AGammaRandomGenerator(){rnd.Initialize();}
    double getGamma(double k, double theta){ return rnd.Gamma(k, theta);}
private:
    ROOT::Math::GSLRandomEngine rnd;
};
*/

class TRandom2;

class AGammaRandomGenerator
{
public:
    AGammaRandomGenerator(TRandom2* RandGen) : RandGen(RandGen) {}

    double getPosUniRand() const;

    double getGamma(const double a, const double b) const;

private:
    TRandom2* RandGen;
};

#endif // AGAMMARANDOMGENERATOR_H
