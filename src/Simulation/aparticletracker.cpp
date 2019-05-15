#include "aparticletracker.h"
#include "aeventtrackingrecord.h"
#include "amaterialparticlecolection.h"
#include "generalsimsettings.h"
#include "acompton.h"
#include "atrackrecords.h"
#include "aenergydepositioncell.h"
#include "aparticlerecord.h"
#include "acommonfunctions.h"
#include "asimulationstatistics.h"
#include "amonitor.h"

#include <QDebug>
#include <QtAlgorithms>

#include "TMath.h"
#include "TGeoManager.h"
#include "TRandom2.h"
#include "TGeoTrack.h"

AParticleTracker::AParticleTracker(TRandom2 & RandomGenerator,
                                   AMaterialParticleCollection & MpCollection,
                                   QVector<AParticleRecord *> & particleStack,
                                   QVector<AEnergyDepositionCell*> & energyVector,
                                   std::vector<AEventTrackingRecord *> & TrackingHistory,
                                   ASimulationStatistics & simStat,
                                   int ThreadIndex) :
    RandGen(RandomGenerator), MpCollection(MpCollection),
    ParticleStack(particleStack), EnergyVector(energyVector),
    TrackingHistory(TrackingHistory), SimStat(simStat), ThreadIndex(ThreadIndex) {}

void AParticleTracker::configure(const GeneralSimSettings* simSet, bool fbuildTracks, std::vector<TrackHolderClass *> * tracks, bool fRemoveEmptyTracks)
{
    SimSet = simSet;
    BuildTracks = fbuildTracks;
    MaxTracks = simSet->TrackBuildOptions.MaxParticleTracks; // in multithread it is overriden using setMaxTracks()
    RemoveTracksIfNoEnergyDepo = fRemoveEmptyTracks;
    Tracks = tracks;
    ParticleTracksAdded = 0;
}

bool AParticleTracker::TrackParticlesOnStack(int eventId)
{
    if (SimSet->fLogsStat) EventRecord = AEventTrackingRecord::create();
    const bool bCheckMonitors = !SimStat.Monitors.isEmpty();
    navigator = gGeoManager->GetCurrentNavigator();
    if (!navigator)
    {
        qDebug() << "Tracking: Current navigator does not exist, creating new";
        navigator = gGeoManager->AddNavigator();
    }
    EventId = eventId;

    while ( !ParticleStack.empty() )
    {
        //new particle
        p = ParticleStack.last();
        ParticleStack.removeLast();
        counter++;

        if (SimSet->fLogsStat) initLog();
        NormalizeVector(p->v); //normalization of the starting vector
        const AParticle::ParticleType ParticleType = MpCollection.getParticleType(p->Id);

        navigator->SetCurrentPoint(p->r);
        navigator->SetCurrentDirection(p->v);
        navigator->FindNode();

        //was this particle created outside the defined World?
        if (navigator->IsOutside())
        {
            if (SimSet->fLogsStat)
            {
                ATrackingStepData * step = new ATrackingStepData(p->r, p->time, p->energy, 0, "O");
                thisParticleRecord->addStep(step);
            }
            delete p; p = nullptr;
            continue;
        }

        initTrack();

        //--------------------- loop over volumes on the path --------------------------
        do
        {
            if (bCheckMonitors && navigator->GetCurrentVolume()->GetTitle()[0] == 'M')
            {
                if ( checkMonitors_isKilled() ) break;
            }

            thisMatId = navigator->GetCurrentVolume()->GetMaterial()->GetIndex();
            thisMaterial = MpCollection[thisMatId];
            thisMatParticle = &thisMaterial->MatParticle[p->Id];

            if ( !thisMatParticle->TrackingAllowed )
            {
                //qDebug()<<"Found medium where tracking is not allowed!";
                if (SimSet->fLogsStat)
                {
                    ATrackingStepData * step = new ATrackingStepData(p->r, p->time, p->energy, 0, "TrackingForbidden");
                    thisParticleRecord->addStep(step);
                }
                break;
            }

            //Maximum allowed range?
            navigator->FindNextBoundary();

            if ( !thisMatParticle->MaterialIsTransparent )
            {
                if (ParticleType == AParticle::_charged_)
                {
                    if (trackCharged_isKilled()) break;
                }
                else
                {
                    if (trackNeutral_isKilled()) break;
                }
            }

            //stepping to the next volume
            navigator->FindNextBoundaryAndStep();

            const double * global = navigator->GetCurrentPoint();
            for (int j=0; j<3; j++) p->r[j] = global[j];

            if (navigator->IsOutside())
            {
                if (SimSet->fLogsStat)
                {
                    ATrackingStepData * step = new ATrackingStepData(p->r, p->time, p->energy, 0, "O");
                    thisParticleRecord->addStep(step);
                }
                break;
            }
            else
            {
                if (SimSet->fLogsStat)
                {
                    ATrackingStepData * step = new ATrackingStepData(p->r, p->time, p->energy, 0, "T");
                    thisParticleRecord->addStep(step);
                }
            }
        }
        while (true);

        //--------------- END of loop over volumes on the path ---------------

        //particle escaped, stopped or captured     do-breaks are collected here!
        if (bBuildThisTrack)
        {
            track->Nodes.append(TrackNodeStruct(p->r, p->time));
            TrackCandidates.push_back(track);
            ParticleTracksAdded++;
        }

        //delete the current particle
        delete p;
    }

    //stack is empty, no errors found

    if (TrackCandidates.size() > 0)
    {
        if (RemoveTracksIfNoEnergyDepo && EnergyVector.empty() )
        {
            for (size_t i = 0; i < TrackCandidates.size(); i++)
                delete TrackCandidates.at(i);
            ParticleTracksAdded -= TrackCandidates.size();
        }
        else
        {
            for (size_t i = 0; i < TrackCandidates.size(); i++)
                Tracks->push_back(TrackCandidates.at(i));
        }
        TrackCandidates.clear();
    }

    if (SimSet->fLogsStat)
    {
        if (RemoveTracksIfNoEnergyDepo && EnergyVector.empty() ) delete EventRecord;
        else TrackingHistory.push_back(EventRecord);
        EventRecord = nullptr;
    }

    // TODO same RemoveTracksIfNoEnergyDepo control for Monitors! ***!!!

    return true; //success
}

void AParticleTracker::generateRandomDirection(double *vv)
{
    //Sphere function of Root:
    double a = 0, b = 0, r2 = 1.0;
    while (r2 > 0.25)
    {
        a  = RandGen.Rndm() - 0.5;
        b  = RandGen.Rndm() - 0.5;
        r2 =  a*a + b*b;
    }
    vv[2] = ( -1.0 + 8.0 * r2 );
    double scale = 8.0 * TMath::Sqrt(0.25 - r2);
    vv[0] = a * scale;
    vv[1] = b * scale;
}

void AParticleTracker::initLog()
{
    if (p->ParticleRecord)
    {
        //this is secondary locally created
        thisParticleRecord = p->ParticleRecord;
    }
    else
    {
        //new primary
        thisParticleRecord = AParticleTrackingRecord::create( MpCollection.getParticleName( p->Id ) );
        EventRecord->addPrimaryRecord(thisParticleRecord);
    }

    ATrackingStepData * step = new ATrackingStepData(p->r, p->time, p->energy, 0, "C");
    thisParticleRecord->addStep(step);
}

void AParticleTracker::initTrack()
{
    if (BuildTracks)
    {
        if (ParticleTracksAdded < MaxTracks)
        {
            if (SimSet->TrackBuildOptions.bSkipPrimaries && p->secondaryOf == -1)        bBuildThisTrack = false;
            else if (SimSet->TrackBuildOptions.bSkipSecondaries && p->secondaryOf != -1) bBuildThisTrack = false;
            else
            {
                track = new TrackHolderClass();
                track->UserIndex = 22;
                SimSet->TrackBuildOptions.applyToParticleTrack(track, p->Id);
                track->Nodes.append(TrackNodeStruct(p->r, p->time));
                bBuildThisTrack = true;
            }
        }
        else
        {
            bBuildThisTrack = false;
            BuildTracks = false;
        }
    }
    else bBuildThisTrack = false;
}

bool AParticleTracker::checkMonitors_isKilled()
{
    const int iMon = navigator->GetCurrentNode()->GetNumber();
    //qDebug() << "Monitor #:"<< iMon << "Total monitors:"<< SimStat->Monitors.size();
    if (SimStat.Monitors.at(iMon)->isForParticles() && SimStat.Monitors.at(iMon)->getParticleIndex() == p->Id)
    {
        const bool bPrimary = (p->secondaryOf == -1);
        if (  ( bPrimary && SimStat.Monitors.at(iMon)->isPrimary() ) ||
              (!bPrimary && SimStat.Monitors.at(iMon)->isSecondary()) )
        {
            double local[3];
            navigator->MasterToLocal(p->r, local);
            //qDebug()<<local[0]<<local[1];
            if ( (local[2] > 0  &&  SimStat.Monitors.at(iMon)->isUpperSensitive()) ||
                 (local[2] < 0  &&  SimStat.Monitors.at(iMon)->isLowerSensitive()) )
            {
                const double * N = navigator->FindNormal(false);
                double cosAngle = 0;
                for (int i=0; i<3; i++) cosAngle += N[i] * p->v[i];  //assuming v never changes!
                SimStat.Monitors[iMon]->fillForParticle(local[0], local[1], p->time, 180.0/3.1415926535*TMath::ACos(cosAngle), p->energy);
                if (SimStat.Monitors.at(iMon)->isStopsTracking())
                {
                    if (SimSet->fLogsStat)
                    {
                        ATrackingStepData * step = new ATrackingStepData(p->r, p->time, p->energy, 0, "MonitorStop");
                        thisParticleRecord->addStep(step);
                    }
                    return true; // particle is stopped
                }
            }
        }
    }
    return false; //particle is not stopped
}

bool AParticleTracker::trackCharged_isKilled()
{
    double distanceHistory = 0;
    const double MaxLength = navigator->GetStep();

    bool bKilled;
    bool flagDone = false;
    do
    {
        //    qDebug() << "-->On step start energy:"<<energy;
        //dE/dx [keV/mm] = Density[g/cm3] * [cm2/g*keV] * 0.1  //0.1 since local units are mm, not cm
        const double dEdX = 0.1 * thisMaterial->density * GetInterpolatedValue(p->energy,
                                                                               &thisMatParticle->InteractionDataX,
                                                                               &thisMatParticle->InteractionDataF,
                                                                               MpCollection.fLogLogInterpolation);

        //recommended step: (RecFraction - suggested decrease of energy per step)
        double RecStep = (dEdX < 1.0e-25 ? 1.0e25 : SimSet->dE * p->energy/dEdX);
        //    qDebug()<<dEdX<<RecStep;

        if (RecStep > SimSet->MaxStep) RecStep = SimSet->MaxStep;
        if (RecStep < SimSet->MinStep) RecStep = SimSet->MinStep;

        //will pass to another volume?
        if (distanceHistory + RecStep > MaxLength - SimSet->Safety)
        {
            //                       qDebug()<<"boundary reached!";
            RecStep = MaxLength - distanceHistory - SimSet->Safety;
            flagDone = true;
            //should enter the next volume after "do" is over
            bKilled = false;
        }
        //doing the step
        navigator->SetStep(RecStep);
        navigator->Step(true, false);
        //                  qDebug() << "Asked for:"<<RecStep<<"Did:"<<navigator->GetStep();  //actual step is less by a fixed value (safity?)
        RecStep = navigator->GetStep();
        //energy loss?
        double dE = dEdX*RecStep;
        //                   qDebug()<<"     Step:"<<RecStep<<" would result in dE of"<<dE;
        if (dE > p->energy) dE = p->energy;
        p->energy -= dE;
        //qDebug() << "     New energy:"<<energy<<"  (min energy:"<<SimSet->MinEnergy<<")";
        //all energy dissipated?
        if (p->energy < SimSet->MinEnergy)
        {
            //qDebug() << "  Dissipated below low limit!";
            flagDone = true;
            //terminationStatus = EventHistoryStructure::AllEnergyDisspated;//2;
            bKilled = true;
            dE += p->energy;
        }

        for (int j=0; j<3; j++) p->r[j] = navigator->GetCurrentPoint()[j];  //new current point
        //                   qDebug()<<"Step"<<RecStep<<"dE"<<dE<<"New energy"<<energy;
        AEnergyDepositionCell* tc = new AEnergyDepositionCell(p->r, RecStep, p->time, dE, p->Id, thisMatId, counter, EventId);
        EnergyVector.push_back(tc);

        if (SimSet->fLogsStat)
        {
            ATrackingStepData * step = new ATrackingStepData(p->r, p->time, p->energy, dE, "hIoni");
            thisParticleRecord->addStep(step);
        }

        distanceHistory += RecStep;
    }
    while (!flagDone);

    return bKilled;
}

bool AParticleTracker::trackNeutral_isKilled()
{
    const double MaxLength  = navigator->GetStep();
    const double & Density = thisMaterial->density;
    const AParticle::ParticleType ParticleType = MpCollection.getParticleType(p->Id);
    QVector<NeutralTerminatorStructure> & Terminators = thisMatParticle->Terminators; //const?

    //how many processes?
    const int numProcesses = Terminators.size();
    if (numProcesses != 0)
    {
        double SoFarShortestId = 0;
        double SoFarShortest = 1.0e10;
        //double TotalCrossSection; //used by neutrons - elastic scattering or absorption  //obsolete? ***!!!
        for (int iProcess=0; iProcess<numProcesses; iProcess++)
        {
            //        qDebug()<<"---Process #:"<<iProcess;
            //calculating (random) how much this particle would travel before this kind of interaction happens

            bool bUseNCrystal = false;
            //for neutrons there are additional flags to suppress capture and elastic
            //and for the case of the NCrystal lib which handles scattering
            if (ParticleType == AParticle::_neutron_)
            {
                if (Terminators[iProcess].Type == NeutralTerminatorStructure::Absorption)
                    if ( !thisMatParticle->bCaptureEnabled )
                    {
                        //        qDebug() << "Skipping capture - it is disabled";
                        continue;
                    }
                if (Terminators[iProcess].Type == NeutralTerminatorStructure::ElasticScattering)
                {
                    if ( !thisMatParticle->bElasticEnabled )
                    {
                        //        qDebug() << "Skipping elastic - it is disabled";
                        continue;
                    }
                    bUseNCrystal = thisMatParticle->bUseNCrystal;
                }
            }

            double InteractionCoefficient;
            if (!bUseNCrystal)
                InteractionCoefficient = GetInterpolatedValue(p->energy,
                                                              &Terminators[iProcess].PartialCrossSectionEnergy,
                                                              &Terminators[iProcess].PartialCrossSection,
                                                              MpCollection.fLogLogInterpolation);
            else
                InteractionCoefficient = Terminators[iProcess].getNCrystalCrossSectionBarns(p->energy, ThreadIndex) * 1.0e-24; //in cm2

            //  qDebug()<<"energy and cross-section:"<<energy<<InteractionCoefficient;

            double MeanFreePath;
            if (ParticleType == AParticle::_neutron_)
            {
                //for capture using atomic density and cross-section in barns
                const double AtDens = Density / thisMaterial->ChemicalComposition.getMeanAtomMass() / 1.66054e-24;
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

            double Step = -MeanFreePath * log(RandGen.Rndm());
            //qDebug()<< "Generated length:"<<Step;
            if (Step < SoFarShortest)
            {
                SoFarShortest = Step;
                SoFarShortestId = iProcess;
                //TotalCrossSection = InteractionCoefficient;
            }
        }
        //qDebug()<<SoFarShortest<<SoFarShortestId;

        if (SoFarShortest >= MaxLength)
        {
            //nothing was triggered in this medium
            //qDebug()<<"Passed the volume - no interaction";
            return false;
        }
        else
        {
            //termination scenario Id was triggered at the distance SoFarShortest
            //doing the proper step
            navigator->SetStep(SoFarShortest);
            navigator->Step(true, false);
            for (int j=0; j<3; j++)
                p->r[j] = navigator->GetCurrentPoint()[j];  //new current point

            //triggering the process
            const NeutralTerminatorStructure & term = Terminators.at(SoFarShortestId);
            switch ( term.Type)
            {
            case (NeutralTerminatorStructure::Photoelectric):
                return processPhotoelectric_isKilled();
            case (NeutralTerminatorStructure::ComptonScattering):
                return processCompton_isKilled();
            case (NeutralTerminatorStructure::Absorption):
                return processNeutronAbsorption_isKilled(term);
            case (NeutralTerminatorStructure::PairProduction):
                return processPairProduction_isKilled();
            case (NeutralTerminatorStructure::ElasticScattering):
                return processNeutronElastic_isKilled(term);
            default:
                qWarning() << "||| Unknown terminator type for"<<MpCollection.getParticleName(p->Id) << "in" << MpCollection.getMaterialName(thisMatId);
            }
        }
    }
    else
    {
        //no processes - error!
        qWarning() << "||| No processes are defined for" << MpCollection.getParticleName(p->Id) << "in" << MpCollection.getMaterialName(thisMatId);
    }
    return false;
}

bool AParticleTracker::processPhotoelectric_isKilled()
{
    // qDebug()<<"Photoelectric";
    AEnergyDepositionCell* tc = new AEnergyDepositionCell(p->r, 0, p->time, p->energy, p->Id, thisMatId, counter, EventId);
    EnergyVector.push_back(tc);

    if (SimSet->fLogsStat)
    {
        ATrackingStepData * step = new ATrackingStepData(p->r, p->time, 0, p->energy, "phot");
        thisParticleRecord->addStep(step);
    }
    return true;
}

bool AParticleTracker::processCompton_isKilled()
{
    // qDebug() << "Compton";
    GammaStructure G0;
    G0.energy = p->energy;
    TVector3 dir(p->v);
    G0.direction = dir;
    // qDebug()<<dir[0]<<dir[1]<<dir[2];

    GammaStructure G1 = Compton(&G0, &RandGen); // new gamma
    // qDebug()<<"energy"<<G1.energy<<" "<<G1.direction[0]<<G1.direction[1]<<G1.direction[2];
    AEnergyDepositionCell* tc = new AEnergyDepositionCell(p->r, 0, p->time, G0.energy - G1.energy, p->Id, thisMatId, counter, EventId);
    EnergyVector.push_back(tc);

    // ***!!! make the same way as in Geant4 and continue with gamma?

    //creating gamma and putting it on stack
    AParticleRecord * tmp = new AParticleRecord(p->Id, p->r[0],p->r[1],p->r[2], G1.direction[0], G1.direction[1], G1.direction[2], p->time, G1.energy, counter);
    ParticleStack.append(tmp);

    if (SimSet->fLogsStat)
    {
        ATrackingStepData * step = new ATrackingStepData(p->r, p->time, 0, G0.energy - G1.energy, "compt");
        thisParticleRecord->addStep(step);

        AParticleTrackingRecord * secTR = AParticleTrackingRecord::create( "gamma" );
        tmp->ParticleRecord = secTR;
        thisParticleRecord->addSecondary(secTR);
        step->Secondaries.push_back( thisParticleRecord->countSecondaries()-1 );
    }

    //creating electron
    //int IdElectron =
    //double ElectronEnergy = G0.energy - G1.energy;
    //TVector3 eDirection = G0.energy*G0.direction - G1.energy*G1.direction;
    //eDirection = eDirection.Unit();
    //tmp = new ParticleOnStack(IdElectron, r[0],r[1],r[2], eDirection[0], eDirection[1], eDirection[2], time, ElectronEnergy, 1);
    //ParticleStack->append(tmp);

    return true;
}

bool AParticleTracker::processNeutronAbsorption_isKilled(const NeutralTerminatorStructure & term)
{
    // qDebug() << "neutron absorption";

    // Unless direct energy depsoition model is triggered, nothing is added to the EnergyVector

    //select which of the isotopes interacted with the neutron
    const QVector<ANeutronInteractionElement> & elements = term.IsotopeRecords;
    if (!elements.isEmpty())
    {
        int iselected = 0;
        if (elements.size() > 1)
        {
            double rnd = RandGen.Rndm();

            QVector<double> mfcs(elements.size(), 0);
            double trueSum = 0;
            for (int i=0; i<elements.size(); i++)
            {
                if (!elements.at(i).Energy.isEmpty())
                {
                    mfcs[i] = elements.at(i).MolarFraction * GetInterpolatedValue(p->energy, &elements.at(i).Energy, &elements.at(i).CrossSection, MpCollection.fLogLogInterpolation);
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
        const ANeutronInteractionElement & el = elements.at(iselected);
        //      qDebug() << "Absorption triggered for"<<el.Name<<"-"<<el.Mass;

        // post-capture effect
        if (el.DecayScenarios.isEmpty())
        {
            // qDebug() << "No decay scenarios following neutron capture are defined";
            if (SimSet->fLogsStat)
            {
                ATrackingStepData * step = new ATrackingStepData(p->r, p->time, 0, 0, "nCapture");
                thisParticleRecord->addStep(step);
            }
        }
        else
        {
            // choose one of the decay scenarios
            int iScenario = 0;
            if (el.DecayScenarios.size() > 1)
            {
                double rnd = RandGen.Rndm();
                for (; iScenario < el.DecayScenarios.size(); iScenario++)
                {
                    const double & thisBranching = el.DecayScenarios.at(iScenario).Branching;
                    if (rnd < thisBranching) break;
                    rnd -= thisBranching;
                }
            }
            // qDebug() << "Decay scenario #" << iScenario << "was triggered";
            const ADecayScenario & reaction = el.DecayScenarios.at(iScenario);

            //simplistic model?
            if (reaction.eDirectDepo != ADecayScenario::NotActive)
            {
                // TODO ***!!!
                qDebug() << "Direct neutron depo model!";
                if (SimSet->fLogsStat)
                {
                    double depoE = 0; // ***!!! generate energy depo here
                    ATrackingStepData * step = new ATrackingStepData(p->r, p->time, 0, depoE, "nDirect");
                    thisParticleRecord->addStep(step);
                }
            }
            else if (reaction.GeneratedParticles.isEmpty())
            {
                //      qDebug() << "In this scenario there is no emission of secondary particles";
                if (SimSet->fLogsStat)
                {
                    ATrackingStepData * step = new ATrackingStepData(p->r, p->time, 0, 0, "nCapture");
                    thisParticleRecord->addStep(step);
                }
            }
            else //generating particles if they are defined
            {
                ATrackingStepData * step;
                if (SimSet->fLogsStat)
                {
                    step = new ATrackingStepData(p->r, p->time, 0, 0, "neutronInelastic");
                    thisParticleRecord->addStep(step);
                }

                double vv[3]; //generated direction of the particle
                for (int igp = 0; igp < reaction.GeneratedParticles.size(); igp++)
                {
                    const int    & ParticleId = reaction.GeneratedParticles.at(igp).ParticleId;
                    const double & energy     = reaction.GeneratedParticles.at(igp).Energy;
                    // qDebug() << "  generating particle with Id"<<ParticleId << "and energy"<<energy;
                    if (igp > 0 && reaction.GeneratedParticles.at(igp).bOpositeDirectionWithPrevious)
                    {
                        vv[0] = -vv[0]; vv[1] = -vv[1]; vv[2] = -vv[2];
                        // qDebug() << "   opposite direction of the previous";
                    }
                    else
                    {
                        generateRandomDirection(vv);
                        // qDebug() << "   in random direction";
                    }
                    AParticleRecord * pp = new AParticleRecord(ParticleId, p->r[0], p->r[1], p->r[2], vv[0], vv[1], vv[2], p->time, energy, counter);
                    ParticleStack.append(pp);

                    if (SimSet->fLogsStat)
                    {
                        AParticleTrackingRecord * secTR = AParticleTrackingRecord::create( MpCollection.getParticleName(ParticleId) );
                        pp->ParticleRecord = secTR;
                        thisParticleRecord->addSecondary(secTR);
                        step->Secondaries.push_back( thisParticleRecord->countSecondaries()-1 );
                    }
                }
            }
        }
    }
    else
    {
        qWarning() << "||| Error: No isotopes are defined for" << thisMaterial->name;
        if (SimSet->fLogsStat)
        {
            ATrackingStepData * step = new ATrackingStepData(p->r, p->time, 0, 0, "nCapture");
            thisParticleRecord->addStep(step);
        }
    }
    return true;
}

bool AParticleTracker::processPairProduction_isKilled()
{
    // qDebug()<<"pair production";
    const double depo = p->energy - 1022.0; //directly deposited energy (kinetic energies), assuming electron and positron do not travel far
    AEnergyDepositionCell* tc = new AEnergyDepositionCell(p->r, 0, p->time, depo, p->Id, thisMatId, counter, EventId);
    EnergyVector.push_back(tc);

    //creating two gammas from positron anihilation and putting it on stack
    double vv[3];
    generateRandomDirection(vv);
    AParticleRecord * tmp1 = new AParticleRecord(p->Id, p->r[0],p->r[1],p->r[2], vv[0], vv[1], vv[2], p->time, 511, counter);
    ParticleStack.append(tmp1);
    AParticleRecord * tmp2 = new AParticleRecord(p->Id, p->r[0],p->r[1],p->r[2], -vv[0], -vv[1], -vv[2], p->time, 511, counter);
    ParticleStack.append(tmp2);

    if (SimSet->fLogsStat)
    {
        ATrackingStepData * step = new ATrackingStepData(p->r, p->time, 0, depo, "pair");
        thisParticleRecord->addStep(step);

        AParticleTrackingRecord * secTR = AParticleTrackingRecord::create( "gamma" );
        tmp1->ParticleRecord = secTR;
        thisParticleRecord->addSecondary(secTR);
        step->Secondaries.push_back( thisParticleRecord->countSecondaries()-1 );

        secTR = AParticleTrackingRecord::create( "gamma" );
        tmp2->ParticleRecord = secTR;
        thisParticleRecord->addSecondary(secTR);
        step->Secondaries.push_back( thisParticleRecord->countSecondaries()-1 );
    }
    return true;
}

bool AParticleTracker::processNeutronElastic_isKilled(const NeutralTerminatorStructure & term)
{
    // qDebug() << "neutron elastic scattering";

    double newEnergy; // scattered neutron energy
    double vnew[3];   // scattered neutron velocity in the lab frame
    double vnewMod;   // magnitude of the scattered neutron velocity

    bool bCoherent;
    if ( !thisMatParticle->bUseNCrystal )
    {
        bCoherent = false;
        const QVector<ANeutronInteractionElement> & elements = term.IsotopeRecords;

        //selecting element according to its contribution to the total cross-section
        // qDebug() << "Def elements:" << elements.size();
        int iselected = 0;
        if (elements.size() > 1)
        {
            double rnd = RandGen.Rndm();
            QVector<double> mfcs(elements.size(), 0);
            double trueSum = 0;
            for (int i=0; i<elements.size(); i++)
            {
                if (!elements.at(i).Energy.isEmpty())
                {
                    mfcs[i] = elements.at(i).MolarFraction * GetInterpolatedValue(p->energy, &elements.at(i).Energy, &elements.at(i).CrossSection, MpCollection.fLogLogInterpolation);
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
        // qDebug() << "Elastic scattering triggered for"<< elements.at(iselected).Name <<"-"<<elements.at(iselected).Mass;

        //performing elastic scattering using this element
        // energy is the neutron energy in keV
        // vn[3], va[] - velocitis of neutron and atom in lab frame in m/s
        double vnMod = sqrt(p->energy * 1.9131e11); //vnMod = sqrt(2*energy*e*1000/Mn) = sqrt(energy*1.9131e11)  //for thermal: ~2200.0
        // vn[i] = vnMod * v[i]
        //        qDebug() << "Neutron energy: "<<energy << "keV   Velocity:" << vnMod << " m/s";
        // va[] is randomly generated from Gauss(0, sqrt(kT/m)
        double va[3];
        const double m = elements.at(iselected).Mass; //mass of atom in atomic units
        //        qDebug() << "atom - mass:"<<m;
        //double a = sqrt(1.38065e-23*300.0/m/1.6605e-27);  //assuming temperature of 300K
        const double & Temperature = thisMaterial->temperature;
        double a = sqrt(1.38065e-23 * Temperature / m / 1.6605e-27);
        bool bCannotCollide;
        do
        {
            for (int i=0; i<3; i++) va[i] = RandGen.Gaus(0, a); //maxwell!
            //        qDebug() << "  Speed of atom in lab frame, m/s"<<va[0]<<va[1]<<va[2];
            //calculating projection on the neutron direction
            double proj = 0;
            for (int i=0; i<3; i++) proj += p->v[i] * va[i];  //v[i] is the direction vector of neutron (unitary length)
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
            vcm[i] = (vnMod * p->v[i] + m * va[i]) / sumM;
        double V[3]; //neutron velocity in the center of mass frame
        for (int i=0; i<3; i++)
            V[i] = vnMod * p->v[i] - vcm[i];
        double Vmod = sqrt(V[0]*V[0] + V[1]*V[1] + V[2]*V[2]); //abs value of the velocity
        double Vnew[3]; //neutron velocity after scatter in thecenter of mass frame
        generateRandomDirection(Vnew);
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
        term.generateScatteringNonOriented(p->energy, angle, deltaE, ThreadIndex);

        TVector3 original(p->v);
        TVector3 orthogonal_to_original = original.Orthogonal();

        TVector3 scattered(original);
        scattered.Rotate(angle, orthogonal_to_original);

        double phi = 6.283185307 * RandGen.Rndm(); //0..2Pi
        scattered.Rotate(phi, original);

        // need only direction, magnitude of velosity is not needed, just to reuse in the scattered particle constructor below
        TVector3 unit_scat = scattered.Unit();
        for (int i=0; i<3; i++)
            vnew[i] = unit_scat(i);
        vnewMod = 1.0;

        newEnergy = p->energy + deltaE;
        bCoherent = (deltaE == 0);
    }

    double depE;
    AParticleRecord * tmp = nullptr;
    if (newEnergy > SimSet->MinEnergyNeutrons * 1.0e-6) // meV -> keV to compare
    {
        tmp = new AParticleRecord(p->Id, p->r[0],p->r[1], p->r[2], vnew[0]/vnewMod, vnew[1]/vnewMod, vnew[2]/vnewMod, p->time, newEnergy, counter);
        ParticleStack.append(tmp);
        depE = p->energy - newEnergy;
    }
    else
    {
        qDebug() << "Elastic scattering: Neutron energy" << newEnergy * 1.0e6 <<
                    "is below the lower limit (" << SimSet->MinEnergyNeutrons<<
                    "meV) - tracking skipped for this neutron";
        depE = p->energy;
    }

    //terminationStatus = EventHistoryStructure::ElasticScattering;
    if (SimSet->fLogsStat)
    {
        ATrackingStepData * step = new ATrackingStepData(p->r, p->time, 0, depE, (bCoherent ? "nElasticCoherent" : "nElastic") );
        thisParticleRecord->addStep(step);

        if (tmp)
        {
            AParticleTrackingRecord * secTR = AParticleTrackingRecord::create( "neutron" );
            tmp->ParticleRecord = secTR;
            thisParticleRecord->addSecondary(secTR);
            step->Secondaries.push_back( thisParticleRecord->countSecondaries()-1 );
        }
    }
    return true;
}
