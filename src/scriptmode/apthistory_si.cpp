#include "apthistory_si.h"
#include "asimulationmanager.h"
#include "aeventtrackingrecord.h"
#include "atrackinghistorycrawler.h"

#include <QDebug>

#include "TH1.h"
#include "TH1D.h"
#include "TH2.h"

APTHistory_SI::APTHistory_SI(ASimulationManager & SimulationManager) :
    AScriptInterface(), SM(SimulationManager), TH(SimulationManager.TrackingHistory)
{
    Crawler = new ATrackingHistoryCrawler(SM.TrackingHistory);
    Criteria = new AFindRecordSelector();

    H["cd_getTrackRecord"] = "returns [string_ParticleName, bool_IsSecondary, int_NumberOfSecondaries, int_NumberOfTrackingSteps]";
    H["cd_getStepRecord"] = "returns [ [X,Y,Z], Time, [MatIndex, VolumeName, VolumeIndex], Energy, DepositedEnergy, ProcessName, indexesOfSecondaries[] ]\n"
            "XYZ in mm, Time in ns, Energies in keV, [MatVolIndex] array is empty if node does not exist, indexesOfSec is array with ints";
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
    if (iEvent < 0 || iEvent >= (int)TH.size())
    {
        abort("Bad event number");
        return 0;
    }
    return TH.at(iEvent)->countPrimaries();
}

QString APTHistory_SI::recordToString(int iEvent, int iPrimary, bool includeSecondaries)
{
    if (iEvent < 0 || iEvent >= (int)TH.size())
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

void APTHistory_SI::clearHistory()
{
    for (auto * r : TH) delete r;
    TH.clear();
}

void APTHistory_SI::cd_set(int iEvent, int iPrimary)
{
    if (iEvent < 0 || iEvent >= (int)TH.size())
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
           << (int)Rec->getSecondaries().size()
           << (int)Rec->getSteps().size();
    }
    else abort("Record not set: use cd_set command");

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

    abort("Record not set: use cd_set command");
    return false;
}

bool APTHistory_SI::cd_step(int iStep)
{
    if (Rec)
    {
        if (iStep<0 || iStep>=(int)Rec->getSteps().size())
            abort("bad step number");
        else
        {
            Step = iStep;
            return true;
        }
    }
    else
        abort("Record not set: use cd_set command");

    return false;
}

bool APTHistory_SI::cd_stepToProcess(QString processName)
{
    if (Rec)
    {
        while ( Step < (int)Rec->getSteps().size() - 1)
        {
            Step++;
            if (Rec->getSteps().at(Step)->Process == processName) return true;
        }
    }
    else abort("Record not set: use cd_set command");

    return false;
}

int APTHistory_SI::cd_getCurrentStep()
{
    if (Rec) return Step;

    abort("Record not set: use cd_set command");
    return 0;
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
    else abort("Record not set: use cd_set command");
}

bool APTHistory_SI::cd_firstNonTransportationStep()
{
    if (Rec)
    {
        if (Rec->getSteps().empty())
        {
            abort("Error: container with steps is empty!");
            return false;
        }
        Step = 0;

        const int last = (int)Rec->getSteps().size() - 1;
        while (Step < last)
        {
            Step++;
            if (Step == last) return false;
            if (Rec->getSteps().at(Step)->Process != "T") return true;
        }
    }
    else abort("Record not set: use cd_set command");
    return false;
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
    else abort("Record not set: use cd_set command");
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
            for (int iStep = Step; iStep > -2; iStep--)
            {
                if (iStep < 0)
                {
                    abort("Corrupted tracking history!");
                    return vl;
                }
                ATransportationStepData * trans = dynamic_cast<ATransportationStepData*>(Rec->getSteps().at(iStep));
                if (!trans) continue;

                vnode << trans->iMaterial;
                vnode << trans->VolName;
                vnode << trans->VolIndex;
                break;
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
    else abort("Record not set: use cd_set command");

    return vl;
}

int APTHistory_SI::cd_countSteps()
{
    if (Rec) return Rec->getSteps().size();
    else     abort("Record not set: use cd_set command");
    return 0;
}

int APTHistory_SI::cd_countSecondaries()
{
    if (Rec)
    {
        return Rec->getSecondaries().size();
    }
    else
    {
        abort("Record not set: use cd_set command");
        return 0;
    }
}

bool APTHistory_SI::cd_hadPriorInteraction()
{
    if (Rec)
    {
        while ( Step > 2 && Step < (int)Rec->getSteps().size() ) // note Step-- below: no need to test Step=0 and the last step
        {
            Step--;
            const QString & Proc = Rec->getSteps().at(Step)->Process;
            if (Proc != "T") return true;
        }
    }
    else abort("Record not set: use cd_set command");
    return false;
}

void APTHistory_SI::cd_in(int indexOfSecondary)
{
    if (Rec)
    {
        if (indexOfSecondary > -1 && indexOfSecondary < (int)Rec->getSecondaries().size())
        {
            Rec = Rec->getSecondaries().at(indexOfSecondary);
            Step = 0;
        }
        else abort("bad index of secondary");
    }
    else abort("Record not set: use cd_set command");
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
    abort("Record not set: use cd_set command");
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

void APTHistory_SI::setLimitToFirstInteractionOfPrimary()
{
    Criteria->bLimitToFirstInteractionOfPrimary = true;
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

void APTHistory_SI::setVolumeIndex(int volumeIndex)
{
    Criteria->bVolumeIndex = true;
    Criteria->VolumeIndex = volumeIndex;
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

QVariantList APTHistory_SI::findDepE(AHistorySearchProcessor_findDepositedEnergy & p)
{
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

QVariantList APTHistory_SI::findDepositedEnergies(int bins, double from, double to)
{
    AHistorySearchProcessor_findDepositedEnergy p(AHistorySearchProcessor_findDepositedEnergy::Individual, bins, from, to);
    return findDepE(p);
}

QVariantList APTHistory_SI::findDepositedEnergiesWithSecondaries(int bins, double from, double to)
{
    AHistorySearchProcessor_findDepositedEnergy p(AHistorySearchProcessor_findDepositedEnergy::WithSecondaries, bins, from, to);
    return findDepE(p);
}

QVariantList APTHistory_SI::findDepositedEnergiesOverEvent(int bins, double from, double to)
{
    AHistorySearchProcessor_findDepositedEnergy p(AHistorySearchProcessor_findDepositedEnergy::OverEvent, bins, from, to);
    return findDepE(p);
}

QVariantList APTHistory_SI::findDepositedEnergyStats()
{
    AHistorySearchProcessor_getDepositionStats p;
    Crawler->find(*Criteria, p);

    QVariantList vl;
    QMap<QString, AParticleDepoStat>::const_iterator it = p.DepoData.constBegin();
    while (it != p.DepoData.constEnd())
    {
        QVariantList el;
        const AParticleDepoStat & rec = it.value();
        const double mean = rec.sum / rec.num;
        const double sigma = sqrt( (rec.sumOfSquares - 2.0*mean*rec.sum)/rec.num + mean*mean );
        el << it.key() << rec.num << mean << sigma;
        vl.push_back(el);
        ++it;
    }
    return vl;
}

QVariantList APTHistory_SI::findDepositedEnergyStats(double timeFrom, double timeTo)
{
    AHistorySearchProcessor_getDepositionStatsTimeAware p(timeFrom, timeTo);
    Crawler->find(*Criteria, p);

    QVariantList vl;
    QMap<QString, AParticleDepoStat>::const_iterator it = p.DepoData.constBegin();
    while (it != p.DepoData.constEnd())
    {
        QVariantList el;
        const AParticleDepoStat & rec = it.value();
        const double mean = rec.sum / rec.num;
        const double sigma = sqrt( (rec.sumOfSquares - 2.0*mean*rec.sum)/rec.num + mean*mean );
        el << it.key() << rec.num << mean << sigma;
        vl.push_back(el);
        ++it;
    }
    return vl;
}

QVariantList APTHistory_SI::findTravelledDistances(int bins, double from, double to)
{
    AHistorySearchProcessor_findTravelledDistances p(bins, from, to);
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

const QVariantList APTHistory_SI::findOB_1D(AHistorySearchProcessor_Border & p)
{
    QVariantList vl;

    Crawler->find(*Criteria, p);

    int numBins = p.Hist1D->GetXaxis()->GetNbins();
    for (int iBin=1; iBin<numBins+1; iBin++)
    {
        QVariantList el;
        el << p.Hist1D->GetBinCenter(iBin) << p.Hist1D->GetBinContent(iBin);
        vl.push_back(el);
    }
    return vl;
}

QVariantList APTHistory_SI::findOnBorder(QString what, QString cuts, int bins, double from, double to)
{
    AHistorySearchProcessor_Border p(what, cuts, bins, from, to);
    if (!p.ErrorString.isEmpty())
    {
        abort(p.ErrorString);
        return QVariantList();
    }
    else
        return findOB_1D(p);
}

QVariantList APTHistory_SI::findOnBorder(QString what, QString vsWhat, QString cuts, int bins, double from, double to)
{
    AHistorySearchProcessor_Border p(what, vsWhat, cuts, bins, from, to);
    if (!p.ErrorString.isEmpty())
    {
        abort(p.ErrorString);
        return QVariantList();
    }
    else
        return findOB_1D(p);
}

const QVariantList APTHistory_SI::findOB_2D(AHistorySearchProcessor_Border & p)
{
    QVariantList vl;

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
    return vl;
}

QVariantList APTHistory_SI::findOnBorder(QString what, QString vsWhat, QString cuts, int bins1, double from1, double to1, int bins2, double from2, double to2)
{
    AHistorySearchProcessor_Border p(what, vsWhat, cuts, bins1, from1, to1, bins2, from2, to2);
    if (!p.ErrorString.isEmpty())
    {
        abort(p.ErrorString);
        return QVariantList();
    }
    else
        return findOB_2D(p);
}

QVariantList APTHistory_SI::findOnBorder(QString what, QString vsWhat, QString andVsWhat, QString cuts, int bins1, double from1, double to1, int bins2, double from2, double to2)
{
    AHistorySearchProcessor_Border p(what, vsWhat, andVsWhat, cuts, bins1, from1, to1, bins2, from2, to2);
    if (!p.ErrorString.isEmpty())
    {
        abort(p.ErrorString);
        return QVariantList();
    }
    else
        return findOB_2D(p);
}
