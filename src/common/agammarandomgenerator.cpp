#include "agammarandomgenerator.h"

#include "TRandom2.h"

double AGammaRandomGenerator::getPosUniRand() const // random number in (0,1) range (guaranteed u>0)
{
    double x;
    do {x = RandGen->Rndm();} while (x <= 0. || x >= 1.);
    return x;
}

double AGammaRandomGenerator::getGamma(const double a, const double b) const // implementation taken fron GNU GSL library
{
    /* New version based on Marsaglia and Tsang, "A Simple Method for
        * generating gamma variables", ACM Transactions on Mathematical
        * Software, Vol 26, No 3 (2000), p363-372.
        *
        * Implemented by J.D.Lamb@btinternet.com, minor modifications for GSL
        * by Brian Gough
        */

    if (a < 1.0) {
        double u = getPosUniRand();
        return getGamma(1.0 + a, b) * pow(u, 1.0 / a);
    }

    double x, v, u;
    double d = a - 1.0 / 3.0;
    double c = (1.0 / 3.0) / sqrt(d);

    while (1) {
        do {
            x = RandGen->Gaus(0., 1.); // normal distribution with zero mean and sigma=1.0
            v = 1.0 + c * x;
        } while (v <= 0);

        v = v * v * v;
        u = getPosUniRand();
        if (u < 1 - 0.0331 * x * x * x * x)
            break;

        if (log(u) < 0.5 * x * x + d * (1 - v + log(v)))
            break;
    }
    return b * d * v;
}
