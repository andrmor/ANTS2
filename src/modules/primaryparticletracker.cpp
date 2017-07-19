#include "primaryparticletracker.h"
#include "amaterialparticlecolection.h"
#include "generalsimsettings.h"
#include "acompton.h"
#include "atrackrecords.h"
#include "aenergydepositioncell.h"
#include "aparticleonstack.h"
#include "ahistoryrecords.h"
#include "acommonfunctions.h"
#include "asimulationstatistics.h"
#include "amonitor.h"

#include <QDebug>
#include <QMessageBox>
#include <QtAlgorithms>

#include "TMath.h"
#include "TGeoManager.h"
#include "TRandom2.h"
#include "TGeoTrack.h"

PrimaryParticleTracker::PrimaryParticleTracker(TGeoManager *geoManager,
                                               TRandom2 *RandomGenerator,
                                               AMaterialParticleCollection* MpCollection,
                                               QVector<AParticleOnStack*>* particleStack,
                                               QVector<AEnergyDepositionCell*>* energyVector,
                                               QVector<EventHistoryStructure*>* eventHistory,
                                               ASimulationStatistics *simStat,
                                               QObject *parent) :
  QObject(parent), GeoManager(geoManager), RandGen (RandomGenerator),
  MpCollection(MpCollection), ParticleStack(particleStack), EnergyVector(energyVector), EventHistory(eventHistory), SimStat(simStat)
{
  BuildTracks = true;
  RemoveTracksIfNoEnergyDepo = true;
  AddColorIndex = 0;
  counter = -1;
}

void PrimaryParticleTracker::configure(const GeneralSimSettings* simSet, bool fbuildTracks, QVector<TrackHolderClass*> *tracks, bool fremoveEmptyTracks)
{
  SimSet = simSet;
  BuildTracks = fbuildTracks;
  RemoveTracksIfNoEnergyDepo = fremoveEmptyTracks;
  AddColorIndex = simSet->TrackColorAdd;
  Tracks = tracks;
}

bool PrimaryParticleTracker::TrackParticlesInStack(int eventId)
{
  while (ParticleStack->size())
    {//-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/ beginning cycle over the elements of the stack
      //reading data for the top particle at the stack
      counter ++; //new particle
      Id = ParticleStack->at(0)->Id; //id of the particle
//      qDebug()<<"Tracking new particle "<<counter;
//      qDebug()<<"Particle Id:"<<Id<<" (Collection size = "<<MpCollection->countParticles()<<")";
//      qDebug()<<"Part name, charge, mass"<<MpCollection->getParticleName(Id)<<MpCollection->getParticleCharge(Id)<<MpCollection->getParticleMass(Id);
//      qDebug()<<"Part type:"<<MpCollection->getParticleType(Id);
//      qDebug()<<"Secondary of:"<<ParticleStack->at(0)->secondaryOf;

      EventHistoryStructure* History;
      if (SimSet->fLogsStat)
        {
          //initializing history record - at the end it will be appended to EventHistory QVector
          History = new EventHistoryStructure(Id, counter, ParticleStack->at(0)->secondaryOf, ParticleStack->at(0)->v) ;
        }
      //setting current position, direction vector, energy and time
      for (int i=0; i<3; i++)
        {
          r[i] = ParticleStack->at(0)->r[i];
          v[i] = ParticleStack->at(0)->v[i];
        }
      NormalizeVector(v); //normalization of the staring vector
      energy = ParticleStack->at(0)->energy;
      time = ParticleStack->at(0)->time;

      //initializing GeoManager navigator
      TGeoNavigator *navigator = GeoManager->GetCurrentNavigator();
      navigator->SetCurrentPoint(r);
      navigator->SetCurrentDirection(v);
      navigator->FindNode();

      //is it created outside of Top?
      if (navigator->IsOutside())
        {
          if (SimSet->fLogsStat)
            {
              History->Termination = EventHistoryStructure::CreatedOutside;
              EventHistory->append(History);
            }
          //delete the particle record and remove from the stack
          delete ParticleStack->at(0);
          ParticleStack->remove(0);
          return true;
        }

      //What is the starting material?
      int MatId = navigator->GetCurrentVolume()->GetMaterial()->GetIndex();
      //       qDebug()<<"First material id: "<<MatId;

      EventHistoryStructure::TerminationTypes terminationStatus = EventHistoryStructure::NotFinished; //NotFinished (0) - continue

      TrackHolderClass *track;
      if (BuildTracks)
        {
          if (Tracks->size()<1000)   //***!!! absolute number
            {
              track = new TrackHolderClass();
              track->UserIndex = 22;
              track->Color = Id+1+ AddColorIndex;
              track->Width = 2;
              track->Nodes.append(TrackNodeStruct(r, time));
            }
          else BuildTracks = false;
        }

      //--------------------- loop over tracking-allowed materials on the path --------------------------
      do {
          //           qDebug()<<"cycle starts for the new material: "<<GeoManager->GetPath();
          double distanceHistory = 0; //for diagnostics - travelled distance and deposited energy in THIS material
          double energyHistory = 0;

          const Double_t *global = navigator->GetCurrentPoint();
          for (int j=0; j<3; j++) r[j] = global[j]; //current position

          if (navigator->IsOutside())
            {
              //              qDebug()<<"Escaped from the defined geometry!";
              terminationStatus = EventHistoryStructure::Escaped;//1
              break; //do-break
            }

          //monitors
          if (!SimStat->Monitors.isEmpty())
            {
              if (navigator->GetCurrentVolume()->GetTitle()[0] == 'M')
                {
                  const int iMon = navigator->GetCurrentNode()->GetNumber();
                  //qDebug() << "Monitor #:"<< iMon << "Total monitors:"<< SimStat->Monitors.size();
                  if (SimStat->Monitors.at(iMon)->ParticleStat.isActive())
                    {
                      Double_t local[3];
                      navigator->MasterToLocal(global, local);
                      //qDebug()<<local[0]<<local[1];
                      SimStat->Monitors[iMon]->ParticleStat.fill(local[0], local[1], time);
                    }
                }
            }

          if ( !(*MpCollection)[MatId]->MatParticle[Id].TrackingAllowed )
            {
              //              qDebug()<<"Found medium where tracking is not allowed!";
              terminationStatus = EventHistoryStructure::FoundUntrackableMaterial;//8
              break; //do-break
            }

          //Maximum allowed range?
          navigator->FindNextBoundary();
          const Double_t MaxLength  = navigator->GetStep();

          if ( !(*MpCollection)[MatId]->MatParticle[Id].MaterialIsTransparent && (*MpCollection)[MatId]->MatParticle[Id].InteractionDataX.size() > 1 )
            {
              // if no interaction data are defined, pass this medium (one point is also not enough)
              //           qDebug()<<"Distance to the next border: "<<MaxLength;
              //checking are we in range of available interaction data
              const int &DataPoints = (*MpCollection)[MatId]->MatParticle[Id].InteractionDataX.size();
              if (energy >  (*MpCollection)[MatId]->MatParticle[Id].InteractionDataX[DataPoints-1]) //largest value is at the end - sorted data!
                {  //outside!
                  //             qDebug()<<"energy: "<<energy;
                  qCritical() << "Energy is outside of the supplied data range!";
                  return false; //fail
                }

              const double &Density = (*MpCollection)[MatId]->density;
              const double &AtomicDensity = (*MpCollection)[MatId]->atomicDensity;

              //Next processing depends on the interaction type!
              if (MpCollection->getParticleType(Id) == AParticle::_charged_)
                {
                  //================ Charged particle - continuous deposition =================
                  //historyDistance - travelled inside this material; energy - current energy
                  bool flagDone = false;
                  do
                    {
                      //dE/dx [keV/mm] = Density[g/cm3] * [cm2/g*keV] * 0.1  //0.1 since local units are mm, not cm
                      const double dEdX = 0.1*Density * InteractionValue(energy, &(*MpCollection)[MatId]->MatParticle[Id].InteractionDataX,
                                                                   &(*MpCollection)[MatId]->MatParticle[Id].InteractionDataF,
                                                                   MpCollection->fLogLogInterpolation);

                      //recommended step: (RecFraction - suggested decrease of energy per step)
                      double RecStep = SimSet->dE * energy/dEdX; //*** TO DO zero control
                      //                   qDebug()<<dEdX<<RecStep;
                      //vs absolute limits?
                      if (RecStep > SimSet->MaxStep) RecStep = SimSet->MaxStep;
                      if (RecStep < SimSet->MinStep) RecStep = SimSet->MinStep;

                      //will pass to another volume?
                      if (distanceHistory + RecStep > MaxLength - SimSet->Safety)
                        {
                          //                       qDebug()<<"boundary reached!";
                          RecStep = MaxLength - distanceHistory - SimSet->Safety;
                          flagDone = true;
                          terminationStatus = EventHistoryStructure::NotFinished;//0 //should enter the next material after "do" is over
                        }
                      //doing the step
                      navigator->SetStep(RecStep);
                      navigator->Step(kTRUE, kFALSE);
                      //                  qDebug() << "Asked for:"<<RecStep<<"Did:"<<navigator->GetStep();  //actual step is less by a fixed value (safity?)
                      RecStep = navigator->GetStep();
                      //energy loss?
                      double dE = dEdX*RecStep;
                      //                   qDebug()<<"     "<<RecStep<<dE;
                      if (dE > energy) dE = energy;
                      energy -= dE;
                      //all energy dissipated?
                      if (energy < SimSet->MinEnergy)
                        {
                          flagDone = true;
                          terminationStatus = EventHistoryStructure::AllEnergyDisspated;//2;
                          dE += energy;
                        }

                      for (int j=0; j<3; j++) r[j] = navigator->GetCurrentPoint()[j];  //new current point
                      //                   qDebug()<<"Step"<<RecStep<<"dE"<<dE<<"New energy"<<energy;
                      AEnergyDepositionCell* tc = new AEnergyDepositionCell(r, RecStep, time, dE, Id, MatId, counter, eventId);
                      EnergyVector->append(tc);

                      distanceHistory += RecStep;
                      energyHistory += dE;
                    }
                  while (!flagDone);
                }
              //================ END: charged particle =================
              else
              //================ Neutral particle - photoeffect/compton/capture =================
                {
                  //how many processes?
                  const int processes = (*MpCollection)[MatId]->MatParticle[Id].Terminators.size();
                  //               qDebug()<<"defined processes:"<<processes;
                  if (processes != 0)
                    {
                      double SoFarShortestId = 0;
                      double SoFarShortest = 1.0e10;
                      for (int i=0; i<processes; i++)
                        {
                             //qDebug()<<"---Process #:"<<i;
                          //calculating (random) how much this particle would travel before this kind of interaction happens
                          const double InteractionCoefficient = InteractionValue(energy, &(*MpCollection)[MatId]->MatParticle[Id].Terminators[i].PartialCrossSectionEnergy,
                                                                           &(*MpCollection)[MatId]->MatParticle[Id].Terminators[i].PartialCrossSection,
                                                                           MpCollection->fLogLogInterpolation);
                             //qDebug()<<"energy and cross-section:"<<energy<<InteractionCoefficient;
                          double MeanFreePath;
                          if (MpCollection->getParticleType(Id) == AParticle::_neutron_)
                            { //neutron! - using atomic density and cross-section in barns
                                //qDebug()<<"isotope density:"<<AtomicDensity;
                              MeanFreePath = 10.0/InteractionCoefficient/AtomicDensity;  //1/(cm2)/(1/cm3) - need in mm (so that 10.)
                            }
                          else MeanFreePath = 10.0/InteractionCoefficient/Density;  //1/(cm2/g)/(g/cm3) - need in mm (so that 10.)
                             //qDebug()<<"MeanFreePath="<<MeanFreePath;
                          double Step = -MeanFreePath * log(RandGen->Rndm());
                             //qDebug()<< "Generated length:"<<Step;
                          if (Step < SoFarShortest)
                            {
                              SoFarShortest = Step;
                              SoFarShortestId = i;
                            }
                        }
                                         //qDebug()<<SoFarShortest<<SoFarShortestId;
                      //shortest vs MaxLength?
                      if (SoFarShortest >= MaxLength)
                        {
                          //nothing was triggered in this medium
                          //                       qDebug()<<"Passed the volume - no interaction";
                          distanceHistory = MaxLength;
                          energyHistory = 0;
                          terminationStatus = EventHistoryStructure::NotFinished;//0
                        }
                      else
                        {
                          //termination scenario Id was triggered at the distance SoFarShortest
                          //doing the proper step
                          navigator->SetStep(SoFarShortest);
                          navigator->Step(kTRUE, kFALSE);
                          for (int j=0; j<3; j++) r[j] = navigator->GetCurrentPoint()[j];  //new current point

                          //triggering the process
                          switch ( (*MpCollection)[MatId]->MatParticle[Id].Terminators[SoFarShortestId].Type)
                            {
                            case (NeutralTerminatorStructure::Photoelectric): //0 - Photoelectric
                              {
                                //                           qDebug()<<"Photoelectric";
                                AEnergyDepositionCell* tc = new AEnergyDepositionCell(r, 0, time, energy, Id, MatId, counter, eventId);
                                EnergyVector->append(tc);

                                terminationStatus = EventHistoryStructure::Photoelectric;//3
                                distanceHistory = SoFarShortest;
                                energyHistory = energy;
                                break; //switch-break
                              }
                            case (NeutralTerminatorStructure::ComptonScattering): //1 - Compton
                              {
                                //                           qDebug()<<"Compton";
                                GammaStructure G0;
                                G0.energy = energy;
                                TVector3 dir(v);
                                G0.direction = dir;
                                //qDebug()<<dir[0]<<dir[1]<<dir[2];

                                GammaStructure G1 = Compton(&G0, RandGen); // new gamma
                                //qDebug()<<"energy"<<G1.energy<<" "<<G1.direction[0]<<G1.direction[1]<<G1.direction[2];
                                AEnergyDepositionCell* tc = new AEnergyDepositionCell(r, 0, time, G0.energy - G1.energy, Id, MatId, counter, eventId);
                                EnergyVector->append(tc);

                                //creating gamma and putting it on stack
                                AParticleOnStack *tmp = new AParticleOnStack(Id, r[0],r[1],r[2], G1.direction[0], G1.direction[1], G1.direction[2], time, G1.energy, counter);
                                ParticleStack->append(tmp);
                                  //creating electron
                                  //int IdElectron =
                                  //double ElectronEnergy = G0.energy - G1.energy;
                                  //TVector3 eDirection = G0.energy*G0.direction - G1.energy*G1.direction;
                                  //eDirection = eDirection.Unit();
                                  //tmp = new ParticleOnStack(IdElectron, r[0],r[1],r[2], eDirection[0], eDirection[1], eDirection[2], time, ElectronEnergy, 1);
                                  //ParticleStack->append(tmp);

                                terminationStatus = EventHistoryStructure::ComptonScattering;//4
                                distanceHistory = SoFarShortest;
                                energyHistory = tc->dE;
                                break;//switch-break
                              }                            
                            case (NeutralTerminatorStructure::Capture): //2 - capture
                              {
                                //                           qDebug()<<"capture";
                                //nothing is added to the EnergyVector, the result of capture is generation of secondary particles!
                                int numGenParticles = (*MpCollection)[MatId]->MatParticle[Id].Terminators[SoFarShortestId].GeneratedParticles.size();

                                if (numGenParticles>0)
                                  {
                                    //adding first particle to the stack
                                    double vv[3]; //random direction
                                    GenerateRandomDirection(vv);
                                    const int  &Particle1 = (*MpCollection)[MatId]->MatParticle[Id].Terminators[SoFarShortestId].GeneratedParticles[0];
                                    const double &energy1 = (*MpCollection)[MatId]->MatParticle[Id].Terminators[SoFarShortestId].GeneratedParticleEnergies[0];
                                    //                         qDebug()<<"Adding to stack particle with id: "<<Particle1<<" energy:"<<energy1;
                                    AParticleOnStack* pp = new AParticleOnStack(Particle1, r[0], r[1], r[2], vv[0], vv[1], vv[2], time, energy1, counter);
                                    ParticleStack->append(pp);

                                    // adding second particle
                                    //  for the moment, the creation rule is: opposite direction to the first particle    <- POSSIBLE UPGRADE ***
                                    if (numGenParticles > 1)
                                      {
                                        const int  &Particle2 = (*MpCollection)[MatId]->MatParticle[Id].Terminators[SoFarShortestId].GeneratedParticles[1];
                                        const double &energy2 = (*MpCollection)[MatId]->MatParticle[Id].Terminators[SoFarShortestId].GeneratedParticleEnergies[1];
                                        //                                 qDebug()<<"Adding to stack particle with id: "<<Particle2<<" energy:"<<energy2;
                                        pp = new AParticleOnStack(Particle2, r[0], r[1], r[2], -vv[0], -vv[1], -vv[2], time, energy2, counter);
                                        ParticleStack->append(pp);
                                      }
                                    if (numGenParticles > 2)
                                      for (int ipart=2; ipart<numGenParticles; ipart++)
                                        {
                                          //                                   //all particle after 2nd - random direction
                                          GenerateRandomDirection(vv);
                                          const int  &Particle = (*MpCollection)[MatId]->MatParticle[Id].Terminators[SoFarShortestId].GeneratedParticles[ipart];
                                          const double &energy = (*MpCollection)[MatId]->MatParticle[Id].Terminators[SoFarShortestId].GeneratedParticleEnergies[ipart];
                                          //                             qDebug()<<"Adding to stack particle with id: "<<Particle<<" energy:"<<energy;
                                          AParticleOnStack* pp = new AParticleOnStack(Particle, r[0], r[1], r[2], vv[0], vv[1], vv[2], time, energy, counter);
                                          ParticleStack->append(pp);
                                        }
                                  }

                                terminationStatus = EventHistoryStructure::Capture;//5
                                distanceHistory = SoFarShortest;
                                energyHistory = 0;
                                break; //switch-break
                              }
                            case (NeutralTerminatorStructure::PairProduction): //3 - PairProduction
                              {
                                //                           qDebug()<<"pair";
                                const double depo = energy - 1022.0; //directly deposited energy (kinetic energies), assuming electron and positron do not travel far
                                AEnergyDepositionCell* tc = new AEnergyDepositionCell(r, 0, time, depo, Id, MatId, counter, eventId);
                                EnergyVector->append(tc);

                                //creating two gammas from positron anihilation and putting it on stack
                                double vv[3];
                                GenerateRandomDirection(vv);
                                AParticleOnStack* tmp = new AParticleOnStack(Id, r[0],r[1],r[2], vv[0], vv[1], vv[2], time, 511, counter);
                                ParticleStack->append(tmp);
                                tmp = new AParticleOnStack(Id, r[0],r[1],r[2], -vv[0], -vv[1], -vv[2], time, 511, counter);
                                ParticleStack->append(tmp);

                                terminationStatus = EventHistoryStructure::PairProduction;//9
                                distanceHistory = SoFarShortest;
                                energyHistory = depo;
                                break; //switch-break
                              }
                            case (NeutralTerminatorStructure::EllasticScattering): //4
                              {
                                double m = 27; //mass of atom in atomic units
                                //vn[3], va[] - velocitis of neutron and atom in lab frame in m/s
                                //vn[i] = 2200.0*v[i];
                                //va[] is randomly generated from Gauss(0, sqrt(kT/m)
                                double va[3];
                                double a = sqrt(1.38065e-23*300/m/1.6605e-27);
                                for (int i=0; i<3; i++) va[i] = RandGen->Gaus(0, a); //maxwell!
                                qDebug() << "Speed of atom in lab, m/s"<<va[0]<<va[1]<<va[2];
                                const double sumM = m + 1.0;
                                double vcm[3]; //center of mass velocity in lab frame: (1*vn + m*va)/(1+m)
                                for (int i=0; i<3; i++) vcm[i] = (2200.0*v[i]+m*va[i])/sumM;
                                double V[3]; //neutron velocity in the center of mass frame
                                for (int i=0; i<3; i++) V[i] = 2200.0*v[i] - vcm[i];
                                double Vmod = sqrt(V[0]*V[0] + V[1]*V[1] + V[2]*V[2]); //abs value of the velocity
                                double Vnew[3]; //neutron velocity after scatter in thecenter of mass frame
                                GenerateRandomDirection(Vnew);
                                for (int i=0; i<3; i++) Vnew[i] *= Vmod;
                                double vnew[3]; //neutrn velocity in the lab frame
                                for (int i=0; i<3; i++) vnew[i] = Vnew[i] + vcm[i];
                                double vnewMod = sqrt(vnew[0]*vnew[0] + vnew[1]*vnew[1] + vnew[2]*vnew[2]); //abs value of the neutron velocity
                                qDebug() << "new neutron velocity:"<<vnewMod;
                                AParticleOnStack *tmp = new AParticleOnStack(Id, r[0],r[1], r[2], vnew[0]/vnewMod, vnew[1]/vnewMod, vnew[2]/vnewMod, time, energy, counter);
                                ParticleStack->append(tmp);

                                terminationStatus = EventHistoryStructure::EllasticScattering;
                                distanceHistory = SoFarShortest;
                                energyHistory = 0;
                                break;//switch-break
                              }
                            }
                        }
                    }
                  else
                    {
                      //no processes - error!
                      qDebug()<<"Error - no processes are defined for this particle in this material!" ;
                      return false;
                    }
                }
                //================ END: neutral particle ===================
            } //end block - interaction data defined
          else
            {  //interaction data were not defined
              distanceHistory = MaxLength;
            }

          if (SimSet->fLogsStat) History->Deposition.append(MaterialHistoryStructure(MatId,energyHistory,distanceHistory)); //if continue to the next volume

          //was particle killed in this material?
          if (terminationStatus != EventHistoryStructure::NotFinished) break;  //0, do-break

          //stepping to check the next medium
          navigator->FindNextBoundaryAndStep();
          MatId = navigator->GetCurrentVolume()->GetMaterial()->GetIndex();
          //            qDebug()<<"Next material id: "<<MatId;
        } while (terminationStatus == EventHistoryStructure::NotFinished); //0, repeat if tracking is OK
      //--------------- END of loop over tracking-allowed materials on the path-------------------

      //particle escaped, stopped or captured     do-breaks are collected here!
      if (BuildTracks)
        {
          track->Nodes.append(TrackNodeStruct(r, time));
          TrackCandidates.append(track);
        }
      //finalizing diagnostics
      if (SimSet->fLogsStat)
        {
          History->Termination = terminationStatus;
          EventHistory->append(History);
        }

      //delete the particle record and remove from the stack
      delete ParticleStack->at(0);
      ParticleStack->remove(0);
    }//-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/ end while cycle over the elements of the stack

  //stack is empty, no errors found

  if (TrackCandidates.size()>0)
    {
      if (RemoveTracksIfNoEnergyDepo && EnergyVector->isEmpty() )
        {
          //clear all particle tracks - there were no energy deposition - nothing will be added to Geomanager later
          for (int i=0; i<TrackCandidates.size(); i++)
            delete TrackCandidates[i];
        }
      else
        {
          for (int i=0; i<TrackCandidates.size(); i++)
            Tracks->append(TrackCandidates.at(i)); // delete later when create GeoManager tracks
        }
      TrackCandidates.clear();
    }

  return true; //success
}


void PrimaryParticleTracker::GenerateRandomDirection(double *vv)
{
  //Sphere function of Root:
  double a=0,b=0,r2=1;
  while (r2 > 0.25)
    {
      a  = RandGen->Rndm() - 0.5;
      b  = RandGen->Rndm() - 0.5;
      r2 =  a*a + b*b;
    }
  vv[2] = ( -1.0 + 8.0 * r2 );
  double scale = 8.0 * TMath::Sqrt(0.25 - r2);
  vv[0] = a*scale;
  vv[1] = b*scale;
}
