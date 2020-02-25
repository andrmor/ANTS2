#ifndef ACOMPTON_H
#define ACOMPTON_H

#include "TVector3.h"
class TRandom2;

struct GammaStructure
{
    double energy;
    TVector3 direction;
};

//http://geant4.web.cern.ch/geant4/UserDocumentation/Doxygen/examples_doc/html_fanoCavity/html/MyKleinNishinaCompton_8cc_source.html
GammaStructure Compton(GammaStructure *InGamma, TRandom2 *RandGen);

#endif // ACOMPTON_H
