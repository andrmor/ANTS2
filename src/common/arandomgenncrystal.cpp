#include "arandomgenncrystal.h"

#include "TRandom2.h"

ARandomGenNCrystal::ARandomGenNCrystal(TRandom2 &RandGen) :
    RandomBase(), RandGen(RandGen) {}

double ARandomGenNCrystal::generate()
{
    return RandGen.Rndm();
}
