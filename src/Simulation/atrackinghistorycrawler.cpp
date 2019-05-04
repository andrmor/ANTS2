#include "atrackinghistorycrawler.h"

#include <QDebug>

#include "TGeoNode.h"
#include "TH1D.h"

ATrackingHistoryCrawler::ATrackingHistoryCrawler(const std::vector<AEventTrackingRecord *> &History) :
    History(History) {}

void ATrackingHistoryCrawler::find(const AFindRecordSelector & criteria, std::vector<AHistorySearchProcessor*> & processors) const
{
    for (const AEventTrackingRecord * e : History)
    {
        const std::vector<AParticleTrackingRecord *> & prim = e->getPrimaryParticleRecords();
        for (const AParticleTrackingRecord * p : prim)
            findRecursive(*p, criteria, processors);
    }
}

void ATrackingHistoryCrawler::findRecursive(const AParticleTrackingRecord & pr, const AFindRecordSelector & opt, std::vector<AHistorySearchProcessor *> &processors) const
{
    const std::vector<AParticleTrackingRecord *> & secondaries = pr.getSecondaries();
    for (AParticleTrackingRecord * sec : secondaries)
        findRecursive(*sec, opt, processors);

    if (opt.bParticle && opt.Particle != pr.ParticleName) return;
    if (opt.bPrimary && pr.getSecondaryOf() ) return;
    if (opt.bSecondary && !pr.getSecondaryOf() ) return;

    for (AHistorySearchProcessor * hp : processors) hp->onParticle(pr);

    const std::vector<ATrackingStepData *> & steps = pr.getSteps();
    for (size_t iStep = 0; iStep < steps.size(); iStep++)
    {
        const ATrackingStepData * thisStep = steps[iStep];

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
        {
            //if (thisStep->Process != "T")
            hp->onStep(*thisStep);
            //else
            //{
            //    const ATrackingStepData * nextStep = steps[iStep+1];
            //    hp->onBorder(thisStep, nextStep);
            //}
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

void AHistorySearchProcessor_findParticles::onStep(const ATrackingStepData & )
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

void AHistorySearchProcessor_findMaterials::onStep(const ATrackingStepData &tr)
{
    if (tr.GeoNode)
        FoundMaterials.insert(tr.GeoNode->GetVolume()->GetMaterial()->GetIndex());
}

AHistorySearchProcessor_findDepositedEnergy::AHistorySearchProcessor_findDepositedEnergy(int bins, double from, double to)
{
    Hist = new TH1D("", "Travelled distance", bins, from, to);
    Hist->GetXaxis()->SetTitle("Distance, mm");
}

AHistorySearchProcessor_findDepositedEnergy::~AHistorySearchProcessor_findDepositedEnergy()
{
    delete Hist;
}

void AHistorySearchProcessor_findDepositedEnergy::onParticle(const AParticleTrackingRecord & pr)
{
    Depo = 0;
}

void AHistorySearchProcessor_findDepositedEnergy::onStep(const ATrackingStepData & tr)
{
    Depo += tr.DepositedEnergy;
}

void AHistorySearchProcessor_findDepositedEnergy::onTrackEnd()
{
    if (Depo > 0) Hist->Fill(Depo);
    Depo = 0;
}
