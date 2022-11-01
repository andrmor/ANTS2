***
ANTS2 development is stopped now to focus on ANTS3: https://github.com/andrmor/ANTS3bundle
***


ANTS2 is a simulation and experimental data processing package for Anger camera-type detectors. The package is intended for:

+ Optimization of detector design and geometry using Monte Carlo simulations
+ Optimization and development of event reconstruction techniques
+ Adaptive (iterative) reconstruction of response of light sensors using flood field illumination data
+ Reconstruction and filtering of events for experimental/simulation data using 
  * Statistical methods(including GPU-based implementations)
  * Artificial Neural Networks
  * kNN-based methods

Highlights:

+ Tools for semi-supervised optimization
+ GUI + JavaScript / Pythin scripting 
+ Particle simulations can be delegated to Geant4 (automatic interface)
+ Implementation of NCrystal library for coherent neutron scattering
+ Distributed simulation/reconstruction is supported
+ Docker version (inlcudes Geant4 installation and X-win GUI) is available

It is an open source package developed using CERN ROOT and Qt framework (C++). Currently we are working on implementation of gemetry visualization using JSROOT library.

For installation guides see:
https://github.com/andrmor/ANTS2/wiki/Ants2Install
