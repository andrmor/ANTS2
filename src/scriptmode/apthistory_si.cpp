#include "apthistory_si.h"
#include "asimulationmanager.h"
#include "aeventtrackingrecord.h"

#include <QDebug>

#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoVolume.h"
#include "TGeoMaterial.h"

APTHistory_SI::APTHistory_SI(ASimulationManager & SimulationManager) :
    AScriptInterface(), SM(SimulationManager), TH(SimulationManager.TrackingHistory)
{}

int APTHistory_SI::countEvents()
{
    return TH.size();
}

int APTHistory_SI::countPrimaries(int iEvent)
{
    if (iEvent < 0 || iEvent >= TH.size())
    {
        abort("Bad event number");
        return 0;
    }
    return TH.at(iEvent)->countPrimaries();
}

QString APTHistory_SI::recordToString(int iEvent, int iPrimary, bool includeSecondaries)
{
    if (iEvent < 0 || iEvent >= TH.size())
    {
        abort("Bad event number");
        return "";
    }
    if (iPrimary < 0 || iPrimary >= TH.at(iEvent)->countPrimaries())
    {
        abort(QString("Bad primary number (%1) for event #%2").arg(iPrimary).arg(iEvent));
        return "";
    }
    QString s;
    TH.at(iEvent)->getPrimaryParticleRecords().at(iPrimary)->logToString(s, 0, includeSecondaries);
    return s;
}

void APTHistory_SI::cd_set(int iEvent, int iPrimary)
{
    if (iEvent < 0 || iEvent >= TH.size())
    {
        abort("Bad event number");
        return;
    }
    if (iPrimary < 0 || iPrimary >= TH.at(iEvent)->countPrimaries())
    {
        abort(QString("Bad primary number (%1) for event #%2").arg(iPrimary).arg(iEvent));
        return;
    }
    Rec = TH[iEvent]->getPrimaryParticleRecords().at(iPrimary);
    Step = -1;
}

QVariantList APTHistory_SI::cd_getTrackRecord()
{
    QVariantList vl;
    if (Rec)
    {
        vl << Rec->ParticleName
           << (bool)Rec->getSecondaryOf()
           << (int)Rec->getSecondaries().size();
    }
    else abort("record not set: use cd_set command");

    return vl;
}

bool APTHistory_SI::cd_step()
{
    if (Rec)
    {
        if (Step < (int)Rec->getSteps().size() - 1)
        {
            Step++;
            return true;
        }
        else return false;
    }

    abort("record not set: use cd_set command");
    return false;
}

void APTHistory_SI::cd_firstStep()
{
    if (Rec)
    {
        if (Rec->getSteps().empty())
        {
            abort("Error: container with steps is empty!");
            return;
        }
        Step = 0;
    }
    else abort("record not set: use cd_set command");
}

void APTHistory_SI::cd_lastStep()
{
    if (Rec)
    {
        if (Rec->getSteps().empty())
        {
            abort("Error: container with steps is empty!");
            return;
        }
        Step = (int)Rec->getSteps().size() - 1;
    }
    else abort("record not set: use cd_set command");
}

QVariantList APTHistory_SI::cd_getStepRecord()
{
    QVariantList vl;

    if (Rec)
    {
        if (Step < 0) Step = 0; //forced first step
        if (Step < (int)Rec->getSteps().size())
        {
            ATrackingStepData * s = Rec->getSteps().at(Step);
            qDebug() << s;
            vl.push_back( QVariantList() << s->Position[0] << s->Position[1] << s->Position[2] );
            vl << s->Time;
            QVariantList vnode;
            if (s->GeoNode)
            {
                vnode << s->GeoNode->GetVolume()->GetMaterial()->GetIndex();
                vnode << s->GeoNode->GetVolume()->GetName();
                vnode << s->GeoNode->GetIndex();
            }
            vl.push_back(vnode);
            vl << s->Energy;
            vl << s->DepositedEnergy;
            vl << s->Process;
            QVariantList svl;
            for (int & iSec : s->Secondaries) svl << iSec;
            vl.push_back(svl);
        }
        else abort("Error: bad current step!");
    }
    else abort("record not set: use cd_set command");

    return vl;
}
