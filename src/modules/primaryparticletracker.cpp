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
//#include <QMessageBox>
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
                                               ASimulationStatistics *simStat, int threadIndex,
                                               QObject *parent) :
  QObject(parent), GeoManager(geoManager), RandGen (RandomGenerator),
  MpCollection(MpCollection), ParticleStack(particleStack), EnergyVector(energyVector), EventHistory(eventHistory), SimStat(simStat),
  threadIndex(threadIndex)
{
  BuildTracks = true;
  RemoveTracksIfNoEnergyDepo = true;
  counter = -1;
}

void PrimaryParticleTracker::configure(const GeneralSimSettings* simSet, bool fbuildTracks, QVector<TrackHolderClass*> *tracks, bool fremoveEmptyTracks)
{
  SimSet = simSet;
  BuildTracks = fbuildTracks;
  RemoveTracksIfNoEnergyDepo = fremoveEmptyTracks;
  Tracks = tracks;
}

bool PrimaryParticleTracker::TrackParticlesOnStack(int eventId)
{
  while (ParticleStack->size())
  {//-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/ beginning cycle over the particles on the stack
      //reading data for the top particle at the stack
      counter ++; //new particle
      const int ParticleId = ParticleStack->at(0)->Id; //id of the particle
      const AParticle::ParticleType ParticleType = MpCollection->getParticleType(ParticleId);
      //qDebug()<<"Tracking new particle "<<counter;
      //qDebug()<<"Particle Id:"<<ParticleId<<" (Collection size = "<<MpCollection->countParticles()<<")";
      //qDebug()<<"Part name, charge, mass"<<MpCollection->getParticleName(ParticleId)<<MpCollection->getParticleCharge(ParticleId)<<MpCollection->getParticleMass(ParticleId);
      //qDebug()<<"Part type:"<<MpCollection->getParticleType(ParticleId);
      //qDebug()<<"Secondary of:"<<ParticleStack->at(0)->secondaryOf;

      EventHistoryStructure* History;
      if (SimSet->fLogsStat)
        {
          //initializing history record - at the end it will be appended to EventHistory QVector
          History = new EventHistoryStructure(ParticleId, counter, ParticleStack->at(0)->secondaryOf, ParticleStack->at(0)->v) ;
        }
      //setting current position, direction vector, energy and time
      double r[3];          //position
      double v[3];          //direction vector
      for (int i=0; i<3; i++)
        {
          r[i] = ParticleStack->at(0)->r[i];
          v[i] = ParticleStack->at(0)->v[i];
        }
      NormalizeVector(v); //normalization of the staring vector
      double energy = ParticleStack->at(0)->energy;     //particle energy
      double time = ParticleStack->at(0)->time;         //particle creation time - not modified!

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
      bool bBuildThisTrack = BuildTracks;
      if (bBuildThisTrack)
      {
          if (Tracks->size() < SimSet->TrackBuildOptions.MaxParticleTracks)
          {
              if (SimSet->TrackBuildOptions.bSkipPrimaries && ParticleStack->at(0)->secondaryOf == -1)
                  bBuildThisTrack = false;
              else if (SimSet->TrackBuildOptions.bSkipSecondaries && ParticleStack->at(0)->secondaryOf != -1)
                  bBuildThisTrack = false;
              else
              {
                  track = new TrackHolderClass();
                  track->UserIndex = 22;
                  SimSet->TrackBuildOptions.applyToParticleTrack(track, ParticleId);
                  track->Nodes.append(TrackNodeStruct(r, time));
              }
          }
          else
          {
              bBuildThisTrack = false;
              BuildTracks = false;
          }
      }

      //--------------------- loop over tracking-allowed materials on the path --------------------------
      do
      {
          //           qDebug()<<"cycle starts for the new material: "<<GeoManager->GetPath();
          double distanceHistory = 0; //for diagnostics - travelled distance and deposited energy in THIS material
          double energyHistory = 0;

          const Double_t *global = navigator->GetCurrentPoint();
          for (int j=0; j<3; j++) r[j] = global[j]; //current position

          if (navigator->IsOutside())
            {
              //              qDebug()<<"Escaped from the defined geometry!";
              terminationStatus = EventHistoryStructure::Escaped;//1
              if (bBuildThisTrack && (ParticleStack->at(0)->secondaryOf == -1) && SimSet->TrackBuildOptions.bSkipPrimariesNoInteraction)
              {
                  delete track;
                  bBuildThisTrack = false;
              }
              break; //do-break
            }

          //monitors
          if (!SimStat->Monitors.isEmpty())
            {
              if (navigator->GetCurrentVolume()->GetTitle()[0] == 'M')
                {
                  const int iMon = navigator->GetCurrentNode()->GetNumber();
                  //qDebug() << "Monitor #:"<< iMon << "Total monitors:"<< SimStat->Monitors.size();
                  if (SimStat->Monitors.at(iMon)->isForParticles() && SimStat->Monitors.at(iMon)->getParticleIndex()==ParticleId)
                    {
                      const bool bPrimary = (ParticleStack->at(0)->secondaryOf == -1);
                      if (bPrimary && SimStat->Monitors.at(iMon)->isPrimary() ||
                          !bPrimary && SimStat->Monitors.at(iMon)->isSecondary())
                      {
                          Double_t local[3];
                          navigator->MasterToLocal(global, local);
                          //qDebug()<<local[0]<<local[1];
                          if ( (local[2]>0 && SimStat->Monitors.at(iMon)->isUpperSensitive()) || (local[2]<0 && SimStat->Monitors.at(iMon)->isLowerSensitive()) )
                          {
                              const double* N = navigator->FindNormal(kFALSE);
                              double cosAngle = 0;
                              for (int i=0; i<3; i++) cosAngle += N[i] * v[i];  //assuming v never changes!
                              SimStat->Monitors[iMon]->fillForParticle(local[0], local[1], time, 180.0/3.1415926535*TMath::ACos(cosAngle), energy);
                              if (SimStat->Monitors.at(iMon)->isStopsTracking())
                              {
                                  terminationStatus = EventHistoryStructure::StoppedOnMonitor;//11
                                  break; //do-break
                              }
                          }
                      }
                    }
                }
            }

          if ( !(*MpCollection)[MatId]->MatParticle[ParticleId].TrackingAllowed )
            {
              //              qDebug()<<"Found medium where tracking is not allowed!";
              terminationStatus = EventHistoryStructure::FoundUntrackableMaterial;//8
              break; //do-break
            }

          //Maximum allowed range?
          navigator->FindNextBoundary();
          const Double_t MaxLength  = navigator->GetStep();

          if ( (*MpCollection)[MatId]->MatParticle[ParticleId].MaterialIsTransparent )
            {
              // pass this medium
              distanceHistory = MaxLength;
            }
          else
            {
              const double &Density = (*MpCollection)[MatId]->density;              

              //Next processing depends on the interaction type!
              if (ParticleType == AParticle::_charged_)
                {
                  //================ Charged particle - continuous deposition =================
                  //historyDistance - travelled inside this material; energy - current energy
                  bool flagDone = false;
                  do
                    {
                      //    qDebug() << "-->On step start energy:"<<energy;
                      //dE/dx [keV/mm] = Density[g/cm3] * [cm2/g*keV] * 0.1  //0.1 since local units are mm, not cm
                      const double dEdX = 0.1*Density * GetInterpolatedValue(energy, &(*MpCollection)[MatId]->MatParticle[ParticleId].InteractionDataX,
                                                                   &(*MpCollection)[MatId]->MatParticle[ParticleId].InteractionDataF,
                                                                   MpCollection->fLogLogInterpolation);

                      //recommended step: (RecFraction - suggested decrease of energy per step)
                      double RecStep = SimSet->dE * energy/dEdX; //*** TO DO zero control
                      //    qDebug()<<dEdX<<RecStep;
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
                      //                   qDebug()<<"     Step:"<<RecStep<<" would result in dE of"<<dE;
                      if (dE > energy) dE = energy;
                      energy -= dE;
                      //qDebug() << "     New energy:"<<energy<<"  (min energy:"<<SimSet->MinEnergy<<")";
                      //all energy dissipated?
                      if (energy < SimSet->MinEnergy)
                        {
                          //qDebug() << "  Dissipated below low limit!";
                          flagDone = true;
                          terminationStatus = EventHistoryStructure::AllEnergyDisspated;//2;
                          dE += energy;
                        }

                      for (int j=0; j<3; j++) r[j] = navigator->GetCurrentPoint()[j];  //new current point
                      //                   qDebug()<<"Step"<<RecStep<<"dE"<<dE<<"New energy"<<energy;
                      AEnergyDepositionCell* tc = new AEnergyDepositionCell(r, RecStep, time, dE, ParticleId, MatId, counter, eventId);
                      EnergyVector->append(tc);

                      distanceHistory += RecStep;
                      energyHistory += dE;
                    }
                  while (!flagDone);
                }
              //================ END: charged particle =================
              else
              //================ Neutral particle =================
                {
                  //how many processes?
                  const int numProcesses = (*MpCollection)[MatId]->MatParticle[ParticleId].Terminators.size();
                  //        qDebug()<<"defined processes:"<<numProcesses;
                  if (numProcesses != 0)
                    {
                      double SoFarShortestId = 0;
                      double SoFarShortest = 1.0e10;
                      double TotalCrossSection; //used by neutrons - elastic scattering or absorption
                      for (int iProcess=0; iProcess<numProcesses; iProcess++)
                        {
                          //        qDebug()<<"---Process #:"<<iProcess;
                          //calculating (random) how much this particle would travel before this kind of interaction happens

                          bool bUseNCrystal = false;
                          //for neutrons have additional flags to suppress capture and elastic and if it NCrystal lib which handles scattering
                          if (ParticleType == AParticle::_neutron_)
                          {
                              if ((*MpCollection)[MatId]->MatParticle[ParticleId].Terminators[iProcess].Type == NeutralTerminatorStructure::Absorption)
                                  if ( !(*MpCollection)[MatId]->MatParticle[ParticleId].bCaptureEnabled )
                                  {
                                      //        qDebug() << "Skipping capture - it is disabled";
                                      continue;
                                  }
                              if ((*MpCollection)[MatId]->MatParticle[ParticleId].Terminators[iProcess].Type == NeutralTerminatorStructure::ElasticScattering)
                              {
                                  if ( !(*MpCollection)[MatId]->MatParticle[ParticleId].bElasticEnabled )
                                  {
                                      //        qDebug() << "Skipping elastic - it is disabled";
                                      continue;
                                  }
                                  bUseNCrystal = (*MpCollection)[MatId]->MatParticle[ParticleId].bUseNCrystal;
                              }
                          }

                          double InteractionCoefficient;
                          if (!bUseNCrystal)
                            InteractionCoefficient = GetInterpolatedValue(energy,
                                                                                 &(*MpCollection)[MatId]->MatParticle[ParticleId].Terminators[iProcess].PartialCrossSectionEnergy,
                                                                                 &(*MpCollection)[MatId]->MatParticle[ParticleId].Terminators[iProcess].PartialCrossSection,
                                                                                 MpCollection->fLogLogInterpolation);
                          else
                            InteractionCoefficient = (*MpCollection)[MatId]->MatParticle[ParticleId].Terminators[iProcess].getNCrystalCrossSectionBarns(energy, threadIndex) * 1.0e-24; //in cm2

                          //  qDebug()<<"energy and cross-section:"<<energy<<InteractionCoefficient;

                          double MeanFreePath;
                          if (ParticleType == AParticle::_neutron_)
                            { //for capture using atomic density and cross-section in barns
                              const double AtDens = Density / (*MpCollection)[MatId]->ChemicalComposition.getMeanAtomMass() / 1.66054e-24;
                              MeanFreePath = 10.0/InteractionCoefficient/AtDens;  //1/(cm2)/(1/cm3) - need in mm (so that 10.)
                              //    qDebug() <<"Density:"<<Density <<"MAM:"<<(*MpCollection)[MatId]->ChemicalComposition.getMeanAtomMass()<<"Effective atomic density:"<<AtDens;
                              //    qDebug() <<"Energy:"<<energy<<"CS:"<< InteractionCoefficient<< "MFP:"<<MeanFreePath;
                            }
                          else
                            {
                               //qDebug()<<"Mat density:"<<Density;
                              MeanFreePath = 10.0/InteractionCoefficient/Density;  //1/(cm2/g)/(g/cm3) - need in mm (so that 10.)
                            }
                            //qDebug()<<"MeanFreePath="<<MeanFreePath;
                          double Step = -MeanFreePath * log(RandGen->Rndm());
                             //qDebug()<< "Generated length:"<<Step;
                          if (Step < SoFarShortest)
                            {
                              SoFarShortest = Step;
                              SoFarShortestId = iProcess;
                              TotalCrossSection = InteractionCoefficient;
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
                          const NeutralTerminatorStructure& term = (*MpCollection)[MatId]->MatParticle[ParticleId].Terminators.at(SoFarShortestId);
                          switch ( term.Type)
                            {
                            case (NeutralTerminatorStructure::Photoelectric): //0
                              {
                                //                           qDebug()<<"Photoelectric";
                                AEnergyDepositionCell* tc = new AEnergyDepositionCell(r, 0, time, energy, ParticleId, MatId, counter, eventId);
                                EnergyVector->append(tc);

                                terminationStatus = EventHistoryStructure::Photoelectric;//3
                                distanceHistory = SoFarShortest;
                                energyHistory = energy;
                                break; //switch-break
                              }
                            case (NeutralTerminatorStructure::ComptonScattering): //1
                              {
                                //                           qDebug()<<"Compton";
                                GammaStructure G0;
                                G0.energy = energy;
                                TVector3 dir(v);
                                G0.direction = dir;
                                //qDebug()<<dir[0]<<dir[1]<<dir[2];

                                GammaStructure G1 = Compton(&G0, RandGen); // new gamma
                                //qDebug()<<"energy"<<G1.energy<<" "<<G1.direction[0]<<G1.direction[1]<<G1.direction[2];
                                AEnergyDepositionCell* tc = new AEnergyDepositionCell(r, 0, time, G0.energy - G1.energy, ParticleId, MatId, counter, eventId);
                                EnergyVector->append(tc);

                                //creating gamma and putting it on stack
                                AParticleOnStack *tmp = new AParticleOnStack(ParticleId, r[0],r[1],r[2], G1.direction[0], G1.direction[1], G1.direction[2], time, G1.energy, counter);
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
                            case (NeutralTerminatorStructure::Absorption): //2
                              {
                                //      qDebug()<<"------ neutron absorption triggered!";
                                //nothing is added to the EnergyVector, but the result of capture can be generation of secondary particles
                                //first select which of the isotopes captured neutron
                                const QVector<ANeutronInteractionElement> &elements = term.IsotopeRecords;
                                if (!elements.isEmpty())
                                {
                                    int iselected = 0;
                                    if (elements.size() > 1)
                                    {
                                      double rnd = RandGen->Rndm();

                                      //this procedure is vulnerable to rounding errors - sum of individual crossections vs interpolated sum crossection
//                                      const int max = elements.size()-1;  //if before-last failed, the last is selected automatically
//                                      for (; iselected<max; iselected++)
//                                        {
//                                          const AAbsorptionElement& el = elements.at(iselected);
//                                          double crossSection = 0;  // will be zero if this element has no cross-section ("Can leave empty option is activated")
//                                          if (!elements.at(iselected).Energy.isEmpty())
//                                              crossSection = GetInterpolatedValue(energy,
//                                                                                  &elements.at(iselected).Energy, &elements.at(iselected).CrossSection,
//                                                                                  MpCollection->fLogLogInterpolation);
//                                          // fraction of this element in the total cross section:
//                                          const double thisOne = (crossSection * el.MolarFraction) / TotalCrossSection;
//                                          //        qDebug() << "--Absorption->checking element"<<el.Name<<"-"<<el.Mass<<"  molar fraction:"<<el.MolarFraction;
//                                          //        qDebug() << "  cs:"<<crossSection<<"total:"<<TotalCrossSection<<"-> fraction in total cross-section:"<<thisOne;
//                                          if (rnd <= thisOne) break;
//                                          rnd -= thisOne;
//                                        }

                                      // slow but more accurate
                                      QVector<double> mfcs(elements.size(), 0);
                                      double trueSum = 0;
                                      for (int i=0; i<elements.size(); i++)
                                      {
                                          if (!elements.at(i).Energy.isEmpty())
                                          {
                                              mfcs[i] = elements.at(i).MolarFraction * GetInterpolatedValue(energy, &elements.at(i).Energy, &elements.at(i).CrossSection, MpCollection->fLogLogInterpolation);
                                              trueSum += mfcs.at(i);
                                          }
                                      }
                                      const int max = elements.size()-1;  //if before-last failed, the last is selected automatically
                                      for (; iselected<max; iselected++)
                                        {
                                          const double thisOne = mfcs.at(iselected) / trueSum; // fraction of this element in the total effective cross section
                                          if (rnd <= thisOne) break;
                                          rnd -= thisOne;
                                        }
                                    }
                                    const ANeutronInteractionElement& el = elements.at(iselected);
                                    //      qDebug() << "Absorption triggered for"<<el.Name<<"-"<<el.Mass;

                                    // post-capture effect
                                    if (el.DecayScenarios.isEmpty())
                                    {
                                        //      qDebug() << "No decay scenarios following neutron capture are defined";
                                    }
                                    else
                                    {
                                        // choose which of the decay scenarios was triggered
                                        int iScenario = 0;
                                        if (el.DecayScenarios.size() > 1)
                                        {
                                          double rnd = RandGen->Rndm();
                                          for (; iScenario<el.DecayScenarios.size(); iScenario++)
                                            {
                                              const double& thisBranching = el.DecayScenarios.at(iScenario).Branching;
                                              if (rnd < thisBranching) break;
                                              rnd -= thisBranching;
                                            }
                                        }
                                        //      qDebug() << "Decay scenario #" << iScenario << "was triggered";
                                        const ADecayScenario& reaction = el.DecayScenarios.at(iScenario);

                                        //generating particles if defined
                                        if (reaction.GeneratedParticles.isEmpty())
                                        {
                                            //      qDebug() << "In this scenario there is no emission of secondary particles";
                                        }
                                        else
                                        {
                                            double vv[3]; //generated direction of the particle
                                            for (int igp=0; igp<reaction.GeneratedParticles.size(); igp++)
                                            {
                                                const int&   ParticleId = reaction.GeneratedParticles.at(igp).ParticleId;
                                                const double& energy    = reaction.GeneratedParticles.at(igp).Energy;
                                                //      qDebug() << "  generating particle with Id"<<ParticleId << "and energy"<<energy;
                                                if (igp>0 && reaction.GeneratedParticles.at(igp).bOpositeDirectionWithPrevious)
                                                {
                                                    vv[0] = -vv[0]; vv[1] = -vv[1]; vv[2] = -vv[2];
                                                    //      qDebug() << "   opposite direction of the previous";
                                                }                                               
                                                else
                                                {
                                                    GenerateRandomDirection(vv);
                                                    //      qDebug() << "   in random direction";
                                                }
                                                AParticleOnStack* pp = new AParticleOnStack(ParticleId, r[0], r[1], r[2], vv[0], vv[1], vv[2], time, energy, counter);
                                                ParticleStack->append(pp);
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    qWarning() << "||| No absorption elements are defined for"<<(*MpCollection)[MatId]->name;
                                }

                                terminationStatus = EventHistoryStructure::NeutronAbsorption;//5
                                distanceHistory = SoFarShortest;
                                energyHistory = 0;
                                break; //switch-break
                              }
                            case (NeutralTerminatorStructure::PairProduction): //3
                              {
                                //                           qDebug()<<"pair";
                                const double depo = energy - 1022.0; //directly deposited energy (kinetic energies), assuming electron and positron do not travel far
                                AEnergyDepositionCell* tc = new AEnergyDepositionCell(r, 0, time, depo, ParticleId, MatId, counter, eventId);
                                EnergyVector->append(tc);

                                //creating two gammas from positron anihilation and putting it on stack
                                double vv[3];
                                GenerateRandomDirection(vv);
                                AParticleOnStack* tmp = new AParticleOnStack(ParticleId, r[0],r[1],r[2], vv[0], vv[1], vv[2], time, 511, counter);
                                ParticleStack->append(tmp);
                                tmp = new AParticleOnStack(ParticleId, r[0],r[1],r[2], -vv[0], -vv[1], -vv[2], time, 511, counter);
                                ParticleStack->append(tmp);

                                terminationStatus = EventHistoryStructure::PairProduction;//9
                                distanceHistory = SoFarShortest;
                                energyHistory = depo;
                                break; //switch-break
                              }
                            case (NeutralTerminatorStructure::ElasticScattering): //4
                              {
                                //  qDebug()<<"------ elastic scattering triggered!";

                                double newEnergy; // scattered neutron energy
                                double vnew[3];   // scattered neutron velocity in the lab frame
                                double vnewMod;   // magnitude of the scattered neutron velocity

                                if ( !(*MpCollection)[MatId]->MatParticle[ParticleId].bUseNCrystal )
                                {
                                    const QVector<ANeutronInteractionElement> &elements = term.IsotopeRecords;

                                    //selecting element according to its contribution to the total cross-section
                                    //      qDebug() << "Def elements:" << elements.size();
                                    int iselected = 0;
                                    if (elements.size() > 1)
                                    {
                                        double rnd = RandGen->Rndm();
                                        QVector<double> mfcs(elements.size(), 0);
                                        double trueSum = 0;
                                        for (int i=0; i<elements.size(); i++)
                                        {
                                            if (!elements.at(i).Energy.isEmpty())
                                            {
                                                mfcs[i] = elements.at(i).MolarFraction * GetInterpolatedValue(energy, &elements.at(i).Energy, &elements.at(i).CrossSection, MpCollection->fLogLogInterpolation);
                                                trueSum += mfcs[i];
                                            }
                                        }
                                        const int max = elements.size()-1;  //if before-last failed, the last is selected automatically
                                        for (; iselected<max; iselected++)
                                        {
                                            const double thisOne = mfcs.at(iselected) / trueSum; // fraction of this element in the total effective cross section
                                            if (rnd <= thisOne) break;
                                            rnd -= thisOne;
                                        }
                                    }
                                    //selected element iselected
                                    //        qDebug() << "Elastic scattering triggered for"<< elements.at(iselected).Name <<"-"<<elements.at(iselected).Mass;

                                    //performing elastic scattering in this element
                                    // "energy" is the neutron energy in keV
                                    // vn[3], va[] - velocitis of neutron and atom in lab frame in m/s
                                    double vnMod = sqrt(energy*1.9131e11); //vnMod = sqrt(2*energy*e*1000/Mn) = sqrt(energy*1.9131e11)  //for thermal: ~2200.0
                                    // vn[i] = vnMod * v[i]
                                    //        qDebug() << "Neutron energy: "<<energy << "keV   Velocity:" << vnMod << " m/s";
                                    // va[] is randomly generated from Gauss(0, sqrt(kT/m)
                                    double va[3];
                                    const double m = elements.at(iselected).Mass; //mass of atom in atomic units
                                    //        qDebug() << "atom - mass:"<<m;
                                    //double a = sqrt(1.38065e-23*300.0/m/1.6605e-27);  //assuming temperature of 300K
                                    const double& Temperature = (*MpCollection)[MatId]->temperature;
                                    double a = sqrt(1.38065e-23*Temperature/m/1.6605e-27);
                                    bool bCannotCollide;
                                    do
                                    {
                                        for (int i=0; i<3; i++) va[i] = RandGen->Gaus(0, a); //maxwell!
                                        //        qDebug() << "  Speed of atom in lab frame, m/s"<<va[0]<<va[1]<<va[2];
                                        //calculating projection on the neutron direction
                                        double proj = 0;
                                        for (int i=0; i<3; i++) proj += v[i] * va[i];  //v[i] is the direction vector of neutron (unitary length)
                                        //        qDebug() << "  Projection:"<<proj<<"m/s";
                                        // proj has "+" if atom moves in the same direction as the neutron, "-" if opposite
                                        if (proj > vnMod)
                                        {
                                            bCannotCollide = true;
                                            //        qDebug() << "  Rejected - generating atom velocity again";
                                        }
                                        else bCannotCollide = false;
                                    }
                                    while (bCannotCollide);
                                    //transition to the center of mass frame
                                    const double sumM = m + 1.0;
                                    double vcm[3]; //center of mass velocity in lab frame: (1*vn + m*va)/(1+m)
                                    for (int i=0; i<3; i++)
                                        vcm[i] = (vnMod*v[i]+m*va[i])/sumM;
                                    double V[3]; //neutron velocity in the center of mass frame
                                    for (int i=0; i<3; i++)
                                        V[i] = vnMod*v[i] - vcm[i];
                                    double Vmod = sqrt(V[0]*V[0] + V[1]*V[1] + V[2]*V[2]); //abs value of the velocity
                                    double Vnew[3]; //neutron velocity after scatter in thecenter of mass frame
                                    GenerateRandomDirection(Vnew);
                                    for (int i=0; i<3; i++) Vnew[i] *= Vmod;

                                    //double vnew[3]; //neutrn velocity in the lab frame
                                    for (int i=0; i<3; i++)
                                        vnew[i] = Vnew[i] + vcm[i];
                                    vnewMod = sqrt(vnew[0]*vnew[0] + vnew[1]*vnew[1] + vnew[2]*vnew[2]); //abs value of the neutron velocity
                                    newEnergy = 0.52270e-11 * vnewMod * vnewMod;   // Mn*V*V/2/e/1000 [keV]
                                    //        qDebug() << "new neutron velocity and energy:"<<vnewMod<<newEnergy;
                                }
                                else
                                {
                                    //NCRYSTAL handles scattering
                                    double angle;
                                    double deltaE;
                                    term.generateScatteringNonOriented(energy, angle, deltaE, threadIndex);

                                    TVector3 original(v);
                                    TVector3 orthogonal_to_original = original.Orthogonal();

                                    TVector3 scattered(original);
                                    scattered.Rotate(angle, orthogonal_to_original);

                                    double phi = 6.283185307 * RandGen->Rndm(); //0..2Pi
                                    scattered.Rotate(phi, original);

                                    // need only direction, magnitude of velosity is not needed, just to reuse in the scattered particle constructor below
                                    TVector3 unit_scat = scattered.Unit();
                                    for (int i=0; i<3; i++)
                                        vnew[i] = unit_scat(i);
                                    vnewMod = 1.0;

                                    newEnergy = energy + deltaE;
                                }

                                if (newEnergy > SimSet->MinEnergyNeutrons * 1.0e-6) // meV -> keV to compare
                                {
                                    AParticleOnStack *tmp = new AParticleOnStack(ParticleId, r[0],r[1], r[2], vnew[0]/vnewMod, vnew[1]/vnewMod, vnew[2]/vnewMod, time, newEnergy, counter);
                                    ParticleStack->append(tmp);
                                    energyHistory = energy - newEnergy;
                                }
                                else
                                {
                                    qDebug() << "Elastic scattering: Neutron energy" << newEnergy * 1.0e6 <<
                                                "is below the lower limit (" << SimSet->MinEnergyNeutrons<<
                                                "meV) - tracking skipped for this neutron";
                                    energyHistory = energy;
                                }

                                terminationStatus = EventHistoryStructure::ElasticScattering;
                                distanceHistory = SoFarShortest;

                                break; //switch-break
                              }
                            }
                        }
                    }
                  else
                    {
                      //no processes - error!
                      qWarning() << "No processes are defined for neutron in" << MpCollection->getMaterialName(MatId);
                      distanceHistory = MaxLength;
                      //return false;
                    }
                }
                //================ END: neutral particle ===================
            } //end block - interaction data defined          

          if (SimSet->fLogsStat) History->Deposition.append(MaterialHistoryStructure(MatId,energyHistory,distanceHistory)); //if continue to the next volume

          //was particle killed in this material?
          if (terminationStatus != EventHistoryStructure::NotFinished) break;  //0, do-break

          //stepping to check the next medium
          navigator->FindNextBoundaryAndStep();
          MatId = navigator->GetCurrentVolume()->GetMaterial()->GetIndex();
          //            qDebug()<<"Next material id: "<<MatId;
      }
      while (terminationStatus == EventHistoryStructure::NotFinished); //0, repeat if tracking is OK
      //--------------- END of loop over tracking-allowed materials on the path-------------------

      //particle escaped, stopped or captured     do-breaks are collected here!
      if (bBuildThisTrack)
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
  }//-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/ end while cycle over the particles on the stack

  //stack is empty, no errors found

  if (TrackCandidates.size() > 0)
  {
      if (RemoveTracksIfNoEnergyDepo && EnergyVector->isEmpty() )
      {
          //clear all particle tracks - there were no energy deposition - nothing will be added to Geomanager later
          for (int i=0; i<TrackCandidates.size(); i++)
              delete TrackCandidates.at(i);
      }
      else
      {
          for (int i = 0; i < TrackCandidates.size(); i++)
              Tracks->append(TrackCandidates.at(i));
      }
      TrackCandidates.clear();
  }// delete later when creating GeoManager tracks

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
