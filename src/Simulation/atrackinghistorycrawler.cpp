#include "atrackinghistorycrawler.h"

#include "TGeoNode.h"

#include <QDebug>

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

    for (AHistorySearchProcessor * hp : processors) hp->confirm();
}

void AHistorySearchProcessor_findParticles::onParticle(const AParticleTrackingRecord &pr)
{
    candidate = pr.ParticleName;
}

void AHistorySearchProcessor_findParticles::confirm()
{
    FoundParticles.insert(candidate);
}

void AHistorySearchProcessor_findParticles::report()
{
    qDebug() << FoundParticles;
}

void AHistorySearchProcessor_findMaterials::onStep(const ATrackingStepData &tr)
{
    if (tr.GeoNode)
        FoundMaterials.insert(tr.GeoNode->GetVolume()->GetMaterial()->GetIndex());
}

void AHistorySearchProcessor_findMaterials::report()
{
    qDebug() << FoundMaterials;
}
