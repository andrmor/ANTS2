#include "atrackinghistorycrawler.h"

#include <QDebug>

#include "TGeoNode.h"
#include "TH1D.h"

ATrackingHistoryCrawler::ATrackingHistoryCrawler(const std::vector<AEventTrackingRecord *> &History) :
    History(History) {}

void ATrackingHistoryCrawler::find(const AFindRecordSelector & criteria, std::vector<AHistorySearchProcessor*> & processors) const
{
    //int iEv = 0;
    for (const AEventTrackingRecord * e : History)
    {
        //qDebug() << "Event #"<<iEv++;
        const std::vector<AParticleTrackingRecord *> & prim = e->getPrimaryParticleRecords();
        for (const AParticleTrackingRecord * p : prim)
            findRecursive(*p, criteria, processors);
    }
}

void ATrackingHistoryCrawler::findRecursive(const AParticleTrackingRecord & pr, const AFindRecordSelector & opt, std::vector<AHistorySearchProcessor *> &processors) const
{
    //int iSec = 0;
    const std::vector<AParticleTrackingRecord *> & secondaries = pr.getSecondaries();
    for (AParticleTrackingRecord * sec : secondaries)
    {
        //qDebug() << "Sec #"<<iSec;
        findRecursive(*sec, opt, processors);
        //qDebug() << "Sec done"<<iSec++;
    }

    //qDebug() << pr.ParticleName;

    if (opt.bParticle && opt.Particle != pr.ParticleName) return;
    if (opt.bPrimary && pr.getSecondaryOf() ) return;
    if (opt.bSecondary && !pr.getSecondaryOf() ) return;

    for (AHistorySearchProcessor * hp : processors) hp->onParticle(pr);

    const std::vector<ATrackingStepData *> & steps = pr.getSteps();
    for (size_t iStep = 0; iStep < steps.size(); iStep++)
    {
        const ATrackingStepData * thisStep = steps[iStep];

        //different handling of Transportation ("T", "O") and other processes
        // Creation ("C") is checked as both "Transportation" and "Other" type process

        ProcessType ProcType;
        if      (thisStep->Process == "C") ProcType = Creation;
        else if (thisStep->Process == "T") ProcType = NormalTransportation;
        else if (thisStep->Process == "O") ProcType = ExitingWorld;
        else                               ProcType = Local;

        if (ProcType == Creation || ProcType == NormalTransportation || ProcType == ExitingWorld)
        {
            bool bExitValidated;
            if (ProcType != Creation)
            {
                const bool bCheckingExit = (opt.bFromMat || opt.bFromVolume || opt.bFromVolIndex);
                if (bCheckingExit)
                {
                    if (thisStep->GeoNode)
                    {
                        const bool bRejectedByMaterial = (opt.bFromMat      && opt.FromMat      != thisStep->GeoNode->GetVolume()->GetMaterial()->GetIndex());
                        const bool bRejectedByVolName  = (opt.bFromVolume   && opt.FromVolume   != thisStep->GeoNode->GetVolume()->GetName());
                        const bool bRejectedByVolIndex = (opt.bFromVolIndex && opt.FromVolIndex != thisStep->GeoNode->GetIndex());
                        bExitValidated = !(bRejectedByMaterial || bRejectedByVolName || bRejectedByVolIndex);
                    }
                    else bExitValidated = false;
                }
                else bExitValidated = true;
            }
            else bExitValidated = false;

            bool bEntranceValidated;
            const bool bCheckingEnter = (opt.bToMat || opt.bToVolume || opt.bToVolIndex);
            if (bCheckingEnter)
            {
                //const ATrackingStepData * nextStep = (ProcType == ExitingWorld ? nullptr : steps[iStep+1]);
                const ATrackingStepData * nextStep = (iStep == steps.size()-1 ? nullptr : steps[iStep+1]); // there could be "T" as the last step!
                if (nextStep && thisStep->GeoNode)
                {
                    const bool bRejectedByMaterial = (opt.bToMat      && opt.ToMat      != nextStep->GeoNode->GetVolume()->GetMaterial()->GetIndex());
                    const bool bRejectedByVolName  = (opt.bToVolume   && opt.ToVolume   != nextStep->GeoNode->GetVolume()->GetName());
                    const bool bRejectedByVolIndex = (opt.bToVolIndex && opt.ToVolIndex != nextStep->GeoNode->GetIndex());
                    bEntranceValidated = !(bRejectedByMaterial || bRejectedByVolName || bRejectedByVolIndex);
                }
                else bEntranceValidated = false;
            }
            else bEntranceValidated = true;

            if (opt.bInOutSeparately)
            {
                if (bExitValidated)
                    for (AHistorySearchProcessor * hp : processors) hp->onTransition(*thisStep, AHistorySearchProcessor::OUT);
                if (bEntranceValidated )
                    for (AHistorySearchProcessor * hp : processors) hp->onTransition(*thisStep, AHistorySearchProcessor::IN);
            }
            else
            {
                if (bExitValidated && bEntranceValidated)
                    for (AHistorySearchProcessor * hp : processors) hp->onTransition(*thisStep, AHistorySearchProcessor::OUT_IN);
            }
        }

        if (ProcType != NormalTransportation && ProcType != ExitingWorld)
        {
            if (opt.bMaterial)
            {
                if (!thisStep->GeoNode) continue;
                if (opt.Material != thisStep->GeoNode->GetVolume()->GetMaterial()->GetIndex()) continue;
            }

            if (opt.bVolume)
            {
                if (!thisStep->GeoNode) continue;
                if (opt.Volume != thisStep->GeoNode->GetVolume()->GetName()) continue;
            }

            if (opt.bVolumeIndex)
            {
                if (!thisStep->GeoNode) continue;
                if (opt.VolumeIndex != thisStep->GeoNode->GetIndex()) continue;
            }

            for (AHistorySearchProcessor * hp : processors)
                hp->onLocalStep(*thisStep);
        }
    }

    //here integral collector-based criteria can be checked -> next line activated only if "pass"

    for (AHistorySearchProcessor * hp : processors) hp->onTrackEnd();
}

void AHistorySearchProcessor_findParticles::onParticle(const AParticleTrackingRecord &pr)
{
    Candidate = pr.ParticleName;
    bConfirmed = false;
}

void AHistorySearchProcessor_findParticles::onLocalStep(const ATrackingStepData & )
{
    bConfirmed = true;
}

void AHistorySearchProcessor_findParticles::onTrackEnd()
{
    if (bConfirmed && !Candidate.isEmpty())
    {
        QMap<QString, int>::iterator it = FoundParticles.find(Candidate);
        if (it == FoundParticles.end())
            FoundParticles.insert(Candidate, 1);
        else it.value()++;
        Candidate.clear();
    }
}

void AHistorySearchProcessor_findMaterials::onLocalStep(const ATrackingStepData &tr)
{
    if (tr.GeoNode)
        FoundMaterials.insert(tr.GeoNode->GetVolume()->GetMaterial()->GetIndex());
}

AHistorySearchProcessor_findDepositedEnergy::AHistorySearchProcessor_findDepositedEnergy(int bins, double from, double to)
{
    Hist = new TH1D("", "Deposited energy", bins, from, to);
    Hist->GetXaxis()->SetTitle("Energy, keV");
}

AHistorySearchProcessor_findDepositedEnergy::~AHistorySearchProcessor_findDepositedEnergy()
{
    delete Hist;
}

void AHistorySearchProcessor_findDepositedEnergy::onParticle(const AParticleTrackingRecord & )
{
    Depo = 0;
}

void AHistorySearchProcessor_findDepositedEnergy::onLocalStep(const ATrackingStepData & tr)
{
    Depo += tr.DepositedEnergy;
}

void AHistorySearchProcessor_findDepositedEnergy::onTrackEnd()
{
    if (Depo > 0) Hist->Fill(Depo);
    Depo = 0;
}

AHistorySearchProcessor_findTravelledDistances::AHistorySearchProcessor_findTravelledDistances(int bins, double from, double to)
{
    Hist = new TH1D("", "Travelled distance", bins, from, to);
    Hist->GetXaxis()->SetTitle("Distance, mm");
}

AHistorySearchProcessor_findTravelledDistances::~AHistorySearchProcessor_findTravelledDistances()
{
    delete Hist;
}

void AHistorySearchProcessor_findTravelledDistances::onParticle(const AParticleTrackingRecord &pr)
{
    Distance = 0;
}

void AHistorySearchProcessor_findTravelledDistances::onLocalStep(const ATrackingStepData &tr)
{
    if (bStarted)
    {
        float d2 = 0;
        for (int i=0; i<3; i++)
        {
            const float delta = LastPosition[i] - tr.Position[i];
            d2 += delta * delta;
            LastPosition[i] = tr.Position[i];
        }
        Distance += sqrt(d2);
    }
}

void AHistorySearchProcessor_findTravelledDistances::onTransition(const ATrackingStepData &tr, AHistorySearchProcessor::Direction direction)
{
    if (direction == IN)
    {
        bStarted = true;
        for (int i=0; i<3; i++)
            LastPosition[i] = tr.Position[i];
    }
    else if (direction == OUT)
    {
        bStarted = false;
        float d2 = 0;
        for (int i=0; i<3; i++)
        {
            const float delta = LastPosition[i] - tr.Position[i];
            d2 += delta * delta;
        }
        Distance += sqrt(d2);
    }
}

void AHistorySearchProcessor_findTravelledDistances::onTrackEnd()
{
    if (Distance > 0) Hist->Fill(Distance);
    Distance = 0;
}
