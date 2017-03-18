#include "acompton.h"

#include "TRandom2.h"

GammaStructure Compton(GammaStructure *InGamma, TRandom2 *RandGen)
{
  double gamEnergy0 = InGamma->energy;
  double electron_mass_c2 = 510.998928; //keV
  double E0_m = gamEnergy0 / electron_mass_c2 ;
  TVector3 gamDirection0 = InGamma->direction;
  double epsilon, epsilonsq, onecost, sint2, greject;
  double eps0   = 1./(1. + 2.*E0_m);
  double eps0sq = eps0*eps0;
  double alpha1 = - log(eps0);
  double alpha2 = 0.5*(1.- eps0sq);

  do
    {
      if ( alpha1/(alpha1+alpha2) > RandGen->Rndm() )
        {
          epsilon   = exp(-alpha1*RandGen->Rndm());
          epsilonsq = epsilon*epsilon;
        }
      else
        {
          epsilonsq = eps0sq + (1.- eps0sq)*RandGen->Rndm();
          epsilon   = sqrt(epsilonsq);
        };
      onecost = (1.- epsilon)/(epsilon*E0_m);
      sint2   = onecost*(2.-onecost);
      greject = 1. - epsilon*sint2/(1.+ epsilonsq);
    } while (greject < RandGen->Rndm());

  double cosTeta = 1. - onecost;
  double sinTeta = sqrt(sint2);
  double Phi     = 2.*3.14159 * RandGen->Rndm();
  double dirx = sinTeta*cos(Phi), diry = sinTeta*sin(Phi), dirz = cosTeta;
  double gamEnergy1 = epsilon*gamEnergy0;

  TVector3 gamDirection1 ( dirx,diry,dirz );
  gamDirection1.RotateUz(gamDirection0);

  GammaStructure G1;
  G1.energy = gamEnergy1;
  G1.direction = gamDirection1;
  return G1;
}
