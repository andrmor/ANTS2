#include "apthistory_si.h"
#include "asimulationmanager.h"
#include "aeventtrackingrecord.h"
#include "atrackinghistorycrawler.h"

#include <QDebug>

#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoVolume.h"
#include "TGeoMaterial.h"
#include "TH1.h"
#include "TH1D.h"
#include "TH2.h"

APTHistory_SI::APTHistory_SI(ASimulationManager & SimulationManager) :
    AScriptInterface(), SM(SimulationManager), TH(SimulationManager.TrackingHistory)
{
    Crawler = new ATrackingHistoryCrawler(SM.TrackingHistory);
    Criteria = new AFindRecordSelector();
}

APTHistory_SI::~APTHistory_SI()
{
    delete Criteria;
    delete Crawler;
}

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

int APTHistory_SI::cd_countSecondaries()
{
    if (Rec)
    {
        return Rec->getSecondaries().size();
    }
    else
    {
        abort("record not set: use cd_set command");
        return 0;
    }
}

void APTHistory_SI::cd_in(int indexOfSecondary)
{
    if (Rec)
    {
        if (indexOfSecondary > -1 && indexOfSecondary < Rec->getSecondaries().size())
        {
            Rec = Rec->getSecondaries().at(indexOfSecondary);
            Step = 0;
        }
        else abort("bad index of secondary");
    }
    else abort("record not set: use cd_set command");
}

bool APTHistory_SI::cd_out()
{
    if (Rec)
    {
        if (Rec->getSecondaryOf() == nullptr) return false;
        Rec = Rec->getSecondaryOf();
        Step = 0;
        return true;
    }
    abort("record not set: use cd_set command");
    return false;
}

void APTHistory_SI::clearCriteria()
{
    delete Criteria;
    Criteria = new AFindRecordSelector();
}

void APTHistory_SI::setParticle(QString particleName)
{
    Criteria->bParticle = true;
    Criteria->Particle = particleName;
}

void APTHistory_SI::setOnlyPrimary()
{
    Criteria->bPrimary = true;
}

void APTHistory_SI::setOnlySecondary()
{
    Criteria->bSecondary = true;
}

void APTHistory_SI::setMaterial(int matIndex)
{
    Criteria->bMaterial = true;
    Criteria->Material = matIndex;
}

void APTHistory_SI::setVolume(QString volumeName)
{
    Criteria->bVolume = true;
    Criteria->Volume = volumeName.toLocal8Bit().data();
}

void APTHistory_SI::setFromMaterial(int matIndex)
{
    Criteria->bFromMat = true;
    Criteria->FromMat = matIndex;
}

void APTHistory_SI::setToMaterial(int matIndex)
{
    Criteria->bToMat = true;
    Criteria->ToMat = matIndex;
}

void APTHistory_SI::setFromVolume(QString volumeName)
{
    Criteria->bFromVolume = true;
    Criteria->FromVolume = volumeName.toLocal8Bit().data();
}

void APTHistory_SI::setToVolume(QString volumeName)
{
    Criteria->bToVolume = true;
    Criteria->ToVolume = volumeName.toLocal8Bit().data();
}

void APTHistory_SI::setFromIndex(int volumeIndex)
{
    Criteria->bFromVolIndex = true;
    Criteria->FromVolIndex = volumeIndex;
}

void APTHistory_SI::setToIndex(int volumeIndex)
{
    Criteria->bToVolIndex = true;
    Criteria->ToVolIndex = volumeIndex;
}

QVariantList APTHistory_SI::findParticles()
{
    AHistorySearchProcessor_findParticles p;
    Crawler->find(*Criteria, p);

    QVariantList vl;
    QMap<QString, int>::const_iterator it = p.FoundParticles.constBegin();
    while (it != p.FoundParticles.constEnd())
    {
        QVariantList el;
        el << it.key() << it.value();
        vl.push_back(el);
        ++it;
    }
    return vl;
}

QVariantList APTHistory_SI::findProcesses()
{
    AHistorySearchProcessor_findProcesses p;
    Crawler->find(*Criteria, p);

    QVariantList vl;
    QMap<QString, int>::const_iterator it = p.FoundProcesses.constBegin();
    while (it != p.FoundProcesses.constEnd())
    {
        QVariantList el;
        el << it.key() << it.value();
        vl.push_back(el);
        ++it;
    }
    return vl;
}

QVariantList APTHistory_SI::findDepositedEnergies(int bins, double from, double to)
{
    AHistorySearchProcessor_findDepositedEnergy p(AHistorySearchProcessor_findDepositedEnergy::Individual, bins, from, to);
    Crawler->find(*Criteria, p);

    QVariantList vl;
    int numBins = p.Hist->GetXaxis()->GetNbins();
    for (int iBin=1; iBin<numBins+1; iBin++)
    {
        QVariantList el;
        el << p.Hist->GetBinCenter(iBin) << p.Hist->GetBinContent(iBin);
        vl.push_back(el);
    }
    return vl;
}

QVariantList APTHistory_SI::findTravelledDistances(int bins, double from, double to)
{
    AHistorySearchProcessor_findTravelledDistances p(bins, from, to);

    AFindRecordSelector tmp = *Criteria;

    //updating criteria to have independent entrance/exit checks
    Criteria->bInOutSeparately = true;
    //copy good volume parameters to both from and in
    Criteria->bFromMat = Criteria->bMaterial;
    Criteria->bToMat   = Criteria->bMaterial;
    Criteria->FromMat = Criteria->Material;
    Criteria->ToMat   = Criteria->Material;

    Criteria->bFromVolume = Criteria->bVolume;
    Criteria->bToVolume   = Criteria->bVolume;
    Criteria->FromVolume = Criteria->Volume;
    Criteria->ToVolume   = Criteria->Volume;

    Criteria->bFromVolIndex = Criteria->bVolumeIndex;
    Criteria->bToVolIndex   = Criteria->bVolumeIndex;
    Criteria->FromVolIndex = Criteria->VolumeIndex;
    Criteria->ToVolIndex   = Criteria->VolumeIndex;

    Crawler->find(*Criteria, p);

    QVariantList vl;
    int numBins = p.Hist->GetXaxis()->GetNbins();
    for (int iBin=1; iBin<numBins+1; iBin++)
    {
        QVariantList el;
        el << p.Hist->GetBinCenter(iBin) << p.Hist->GetBinContent(iBin);
        vl.push_back(el);
    }

    *Criteria = tmp;
    return vl;
}

QVariantList APTHistory_SI::findOnBorder(QString what, QString cuts, int bins, double from, double to)
{
    QVariantList vl;

    AHistorySearchProcessor_Border2 p(what, cuts, bins, from, to);
    if (!p.ErrorString.isEmpty())
        abort(p.ErrorString);
    else
    {
        Crawler->find(*Criteria, p);

        int numBins = p.Hist1D->GetXaxis()->GetNbins();
        for (int iBin=1; iBin<numBins+1; iBin++)
        {
            QVariantList el;
            el << p.Hist1D->GetBinCenter(iBin) << p.Hist1D->GetBinContent(iBin);
            vl.push_back(el);
        }
    }
    return vl;
}

QVariantList APTHistory_SI::findOnBorder(QString what, QString vsWhat, QString cuts, int bins1, double from1, double to1, int bins2, double from2, double to2)
{
    QVariantList vl;

    AHistorySearchProcessor_Border2 p(what, vsWhat, cuts, bins1, from1, to1, bins2, from2, to2);
    if (!p.ErrorString.isEmpty())
        abort(p.ErrorString);
    else
    {
        Crawler->find(*Criteria, p);

        int numX = p.Hist2D->GetXaxis()->GetNbins();
        int numY = p.Hist2D->GetYaxis()->GetNbins();
        for (int iX=1; iX<numX+1; iX++)
        {
            double x = p.Hist2D->GetXaxis()->GetBinCenter(iX);
            for (int iY=1; iY<numY+1; iY++)
            {
                QVariantList el;
                el << x
                   << p.Hist2D->GetYaxis()->GetBinCenter(iY)
                   << p.Hist2D->GetBinContent(iX, iY);
                vl.push_back(el);
            }
        }
    }
    return vl;
}
