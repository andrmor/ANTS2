#include "atrackinghistorycrawler.h"

#include <QDebug>

#include "TGeoNode.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH2.h"
#include "TTree.h"
#include "TFormula.h"

ATrackingHistoryCrawler::ATrackingHistoryCrawler(const std::vector<AEventTrackingRecord *> &History) :
    History(History) {}

void ATrackingHistoryCrawler::find(const AFindRecordSelector & criteria, AHistorySearchProcessor & processor) const
{
    processor.beforeSearch();

    //int iEv = 0;
    for (const AEventTrackingRecord * e : History)
    {
        //qDebug() << "Event #"<<iEv++;
        processor.onNewEvent();

        const std::vector<AParticleTrackingRecord *> & prim = e->getPrimaryParticleRecords();
        for (const AParticleTrackingRecord * p : prim)
            findRecursive(*p, criteria, processor);

        processor.onEventEnd();
    }

    processor.afterSearch();
}

void ATrackingHistoryCrawler::findRecursive(const AParticleTrackingRecord & pr, const AFindRecordSelector & opt, AHistorySearchProcessor & processor) const
{
    bool bDoTrack = true;

    if (!processor.isIgnoreParticleSelectors())
    {
        if      (opt.bParticle  && opt.Particle != pr.ParticleName) bDoTrack = false;
        else if (opt.bPrimary   && pr.getSecondaryOf() ) bDoTrack = false;
        else if (opt.bSecondary && !pr.getSecondaryOf() ) bDoTrack = false;
    }

    bool bInlineTrackingOfSecondaries = processor.isInlineSecondaryProcessing();
    bool bSkipTrackingOfSecondaries = false;
    AParticleTrackingRecord * lastSecondaryToTrack = nullptr;

    if (bDoTrack)
    {
        bool bMaster = processor.onNewTrack(pr);
        bInlineTrackingOfSecondaries = processor.isInlineSecondaryProcessing(); // give a possibility to change the mode

        const std::vector<ATrackingStepData *> & steps = pr.getSteps();
        for (size_t iStep = 0; iStep < steps.size(); iStep++)
        {
            const ATrackingStepData * thisStep = steps[iStep];

            // different handling of Transportation ("T", "O") and all other processes
            // Creation ("C") is checked as both "Transportation" and all other type process

            ProcessType ProcType;
            if      (thisStep->Process == "C") ProcType = Creation;
            else if (thisStep->Process == "T") ProcType = NormalTransportation;
            else if (thisStep->Process == "O") ProcType = ExitingWorld;
            else                               ProcType = Local;

            if (ProcType == Creation || ProcType == NormalTransportation || ProcType == ExitingWorld)
            {
                // two different checks:
                // 1. for specific transitions from - to
                // 2. for enter / exit of the defined volume/mat/index

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
                    const ATrackingStepData * nextStep = (iStep == steps.size()-1 ? nullptr : steps[iStep+1]); // there could be "T" as the last step or exit from world
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

                // if transition validated, calling onTransition (+paranoic test on existence of the prevStep - for Creation exit is always not validated
                const ATrackingStepData * prevStep = (iStep == 0 ? nullptr : steps[iStep-1]);
                if (bExitValidated && bEntranceValidated && prevStep)
                    processor.onTransition(*prevStep, *thisStep); // not the "next" step here! this is just to extract direction information


                //checking for specific material/volume/index for enter/exit
                //out
                if (ProcType != Creation)
                {
                    const bool bCheckingExit = (opt.bMaterial || opt.bVolume || opt.bVolumeIndex);
                    if (bCheckingExit)
                    {
                        if (thisStep->GeoNode)
                        {
                            const bool bRejectedByMaterial = (opt.bMaterial    && opt.Material    != thisStep->GeoNode->GetVolume()->GetMaterial()->GetIndex());
                            const bool bRejectedByVolName  = (opt.bVolume      && opt.Volume      != thisStep->GeoNode->GetVolume()->GetName());
                            const bool bRejectedByVolIndex = (opt.bVolumeIndex && opt.VolumeIndex != thisStep->GeoNode->GetIndex());
                            bExitValidated = !(bRejectedByMaterial || bRejectedByVolName || bRejectedByVolIndex);
                        }
                        else bExitValidated = false;
                    }
                    else bExitValidated = true;

                    if (bExitValidated) processor.onTransitionOut(*thisStep);
                }
                //in
                if (ProcType != ExitingWorld)
                {
                    bool bEntranceValidated;
                    const bool bCheckingEnter = (opt.bMaterial || opt.bVolume || opt.bVolumeIndex);
                    if (bCheckingEnter)
                    {
                        //const ATrackingStepData * nextStep = (ProcType == ExitingWorld ? nullptr : steps[iStep+1]);
                        const ATrackingStepData * nextStep = (iStep == steps.size()-1 ? nullptr : steps[iStep+1]); // there could be "T" as the last step!
                        if (nextStep && thisStep->GeoNode)
                        {
                            const bool bRejectedByMaterial = (opt.bMaterial    && opt.Material    != nextStep->GeoNode->GetVolume()->GetMaterial()->GetIndex());
                            const bool bRejectedByVolName  = (opt.bVolume      && opt.Volume      != nextStep->GeoNode->GetVolume()->GetName());
                            const bool bRejectedByVolIndex = (opt.bVolumeIndex && opt.VolumeIndex != nextStep->GeoNode->GetIndex());
                            bEntranceValidated = !(bRejectedByMaterial || bRejectedByVolName || bRejectedByVolIndex);
                        }
                        else bEntranceValidated = false;
                    }
                    else bEntranceValidated = true;

                    if (bEntranceValidated ) processor.onTransitionIn(*thisStep);
                }
            }

            // Local step or Creation (Creation is treated again -> this time as it would be a local step)
            if (ProcType == Local || ProcType == Creation)
            {
                bool bSkipThisStep = false;
                if (opt.bMaterial || opt.bVolume || opt.bVolumeIndex)
                {
                    if (!thisStep->GeoNode) bSkipThisStep = true;
                    else if (opt.bMaterial    && opt.Material != thisStep->GeoNode->GetVolume()->GetMaterial()->GetIndex()) bSkipThisStep = true;
                    else if (opt.bVolumeIndex && opt.VolumeIndex != thisStep->GeoNode->GetIndex()) bSkipThisStep = true;
                    else if (opt.bVolume      && opt.Volume != thisStep->GeoNode->GetVolume()->GetName()) bSkipThisStep = true;
                }

                if (!bSkipThisStep) processor.onLocalStep(*thisStep);

                if (bInlineTrackingOfSecondaries)
                {
                    for (int iSec : thisStep->Secondaries)
                        findRecursive(*pr.getSecondaries().at(iSec), opt, processor);

                    if (!pr.getSecondaryOf() && opt.bLimitToFirstInteractionOfPrimary && ProcType != Creation)
                        break;
                }
                else
                {
                    if (!pr.getSecondaryOf() && opt.bLimitToFirstInteractionOfPrimary && ProcType != Creation)
                    {
                        if (thisStep->Secondaries.empty()) bSkipTrackingOfSecondaries = true;
                        else lastSecondaryToTrack = pr.getSecondaries().at( thisStep->Secondaries.back() );
                        break;
                    }
                }

            }
        }

        processor.onTrackEnd(bMaster);
    }

    if (!bInlineTrackingOfSecondaries)
    {
        if (!bSkipTrackingOfSecondaries)
        {
            const std::vector<AParticleTrackingRecord *> & secondaries = pr.getSecondaries();
            for (AParticleTrackingRecord * sec : secondaries)
            {
                findRecursive(*sec, opt, processor);
                if (sec == lastSecondaryToTrack) break;
            }
        }
    }
}

bool AHistorySearchProcessor_findParticles::onNewTrack(const AParticleTrackingRecord &pr)
{
    Candidate = pr.ParticleName;
    bConfirmed = false;
    return false;
}

void AHistorySearchProcessor_findParticles::onLocalStep(const ATrackingStepData & )
{
    bConfirmed = true;
}

void AHistorySearchProcessor_findParticles::onTrackEnd(bool)
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

AHistorySearchProcessor_findDepositedEnergy::AHistorySearchProcessor_findDepositedEnergy(CollectionMode mode, int bins, double from, double to)
{
    Mode = mode;
    Hist = new TH1D("", "Deposited energy", bins, from, to);
    Hist->GetXaxis()->SetTitle("Energy, keV");

    bInlineSecondaryProcessing = false; // on start it is default non-inline mode even in WithSecondaries mode
}

AHistorySearchProcessor_findDepositedEnergy::~AHistorySearchProcessor_findDepositedEnergy()
{
    delete Hist;
}

void AHistorySearchProcessor_findDepositedEnergy::onNewEvent()
{
    if (Mode == OverEvent) Depo = 0;
}

bool AHistorySearchProcessor_findDepositedEnergy::onNewTrack(const AParticleTrackingRecord & )
{
    switch (Mode)
    {
    case Individual:
        Depo = 0;
        return false;
    case WithSecondaries:
        if (bSecondaryTrackingStarted)
            return false;
        else
        {
            Depo = 0;
            bSecondaryTrackingStarted  = true;
            bInlineSecondaryProcessing = true; // change to inline secondaries mode
            bIgnoreParticleSelectors   = true; // to allow secondaries even not allowed by the selection
            return true;
        }
    case OverEvent:
        // no need to do anything - Depo is reset on new event
        return false;
    }

    return false;
}

void AHistorySearchProcessor_findDepositedEnergy::onLocalStep(const ATrackingStepData & tr)
{
    Depo += tr.DepositedEnergy;
}

void AHistorySearchProcessor_findDepositedEnergy::onTrackEnd(bool bMaster)
{
    switch (Mode)
    {
    case Individual:
        if (Depo > 0) Hist->Fill(Depo);
        Depo = 0;
        break;
    case WithSecondaries:
        if (bMaster)
        {
            if (Depo > 0) Hist->Fill(Depo);
            Depo = 0;
            bSecondaryTrackingStarted  = false;
            bInlineSecondaryProcessing = false; // back to default non-inline mode!
            bIgnoreParticleSelectors   = false; // back to normal mode
        }
        break;
    case OverEvent:
        // nothing to do - waiting for the end of the event
        break;
    }
}

void AHistorySearchProcessor_findDepositedEnergy::onEventEnd()
{
    if (Mode == OverEvent)
    {
        if (Depo > 0) Hist->Fill(Depo);
        Depo = 0;
    }
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

bool AHistorySearchProcessor_findTravelledDistances::onNewTrack(const AParticleTrackingRecord &)
{
    Distance = 0;
    return false;
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

void AHistorySearchProcessor_findTravelledDistances::onTransitionOut(const ATrackingStepData &tr)
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

void AHistorySearchProcessor_findTravelledDistances::onTransitionIn(const ATrackingStepData &tr)
{
    bStarted = true;
    for (int i=0; i<3; i++)
        LastPosition[i] = tr.Position[i];
}

void AHistorySearchProcessor_findTravelledDistances::onTrackEnd(bool)
{
    if (Distance > 0) Hist->Fill(Distance);
    Distance = 0;
}

void AHistorySearchProcessor_findProcesses::onLocalStep(const ATrackingStepData &tr)
{
    const QString & Proc = tr.Process;
    QMap<QString, int>::iterator it = FoundProcesses.find(Proc);
    if (it == FoundProcesses.end())
        FoundProcesses.insert(Proc, 1);
    else it.value()++;
}

void AHistorySearchProcessor_findProcesses::onTransitionOut(const ATrackingStepData &)
{
    QMap<QString, int>::iterator it = FoundProcesses.find("Out");
    if (it == FoundProcesses.end())
        FoundProcesses.insert("Out", 1);
    else it.value()++;
}

void AHistorySearchProcessor_findProcesses::onTransitionIn(const ATrackingStepData &)
{
    QMap<QString, int>::iterator it = FoundProcesses.find("In");
    if (it == FoundProcesses.end())
        FoundProcesses.insert("In", 1);
    else it.value()++;
}

AHistorySearchProcessor_Border::AHistorySearchProcessor_Border(const QString &what,
                                                               const QString &cuts,
                                                               int bins, double from, double to)
{
    QString s = what;
    formulaWhat1 = parse(s);
    if (!formulaWhat1->IsValid())
        ErrorString = "Invalid formula for 'what'";
    else
    {
        if (!cuts.isEmpty())
        {
            QString s = cuts;
            formulaCuts = parse(s);
            if (!formulaCuts)
                ErrorString = "Invalid formula for cuts";
        }

        if (formulaCuts || cuts.isEmpty())
        {
            Hist1D = new TH1D("", "", bins, from, to);
            TString title = what.toLocal8Bit().data();
            Hist1D->GetXaxis()->SetTitle(title);
            title += ", with ";
            title += cuts.toLocal8Bit().data();
            Hist1D->SetTitle(title);
        }
    }
}

AHistorySearchProcessor_Border::AHistorySearchProcessor_Border(const QString &what, const QString &vsWhat,
                                                               const QString &cuts,
                                                               int bins, double from, double to)
{
    QString s = what;
    formulaWhat1 = parse(s);
    if (!formulaWhat1->IsValid())
        ErrorString = "Invalid formula for 'what'";
    else
    {
        s = vsWhat;
        formulaWhat2 = parse(s);
        if (!formulaWhat2)
            ErrorString = "Invalid formula for 'vsWhat'";
        else
        {
            if (!cuts.isEmpty())
            {
                QString s = cuts;
                formulaCuts = parse(s);
                if (!formulaCuts)
                    ErrorString = "Invalid formula for cuts";
            }

            if (formulaCuts || cuts.isEmpty())
            {
                Hist1D = new TH1D("", "", bins, from, to);
                TString titleY = what.toLocal8Bit().data();
                Hist1D->GetYaxis()->SetTitle(titleY);
                TString titleX = vsWhat.toLocal8Bit().data();
                Hist1D->GetXaxis()->SetTitle(titleX);
                TString title = titleY + " vs " + titleX;
                title += ", with ";
                title += cuts.toLocal8Bit().data();
                Hist1D->SetTitle(title);

                Hist1Dnum = new TH1D("", "", bins, from, to);
            }
        }
    }
}

AHistorySearchProcessor_Border::AHistorySearchProcessor_Border(const QString &what, const QString &vsWhat,
                                                               const QString &cuts,
                                                               int bins1, double from1, double to1,
                                                               int bins2, double from2, double to2)
{
    QString s = what;
    formulaWhat1 = parse(s);
    if (!formulaWhat1)
        ErrorString = "Invalid formula for 'what'";
    else
    {
        s = vsWhat;
        formulaWhat2 = parse(s);
        if (!formulaWhat2)
            ErrorString = "Invalid formula for 'vsWhat'";
        else
        {
            if (!cuts.isEmpty())
            {
                s = cuts;
                formulaCuts = parse(s);
                if (!formulaCuts)
                    ErrorString = "Invalid formula for cuts";
            }

            if (formulaCuts || cuts.isEmpty())
            {
                Hist2D = new TH2D("", "", bins1, from1, to1, bins2, from2, to2);
                TString titleY = what.toLocal8Bit().data();
                Hist2D->GetYaxis()->SetTitle(titleY);
                TString titleX = vsWhat.toLocal8Bit().data();
                Hist2D->GetXaxis()->SetTitle(titleX);
                TString title = titleY + " vs " + titleX;
                if (!cuts.isEmpty())
                {
                    title += ", with ";
                    title += cuts.toLocal8Bit().data();
                }
                Hist2D->SetTitle(title);
            }
        }
    }
}

AHistorySearchProcessor_Border::AHistorySearchProcessor_Border(const QString &what, const QString &vsWhat, const QString &andVsWhat,
                                                               const QString &cuts,
                                                               int bins1, double from1, double to1,
                                                               int bins2, double from2, double to2)
{
    QString s = what;
    formulaWhat1 = parse(s);
    if (!formulaWhat1)
        ErrorString = "Invalid formula for 'what'";
    else
    {
        s = vsWhat;
        formulaWhat2 = parse(s);
        if (!formulaWhat2)
            ErrorString = "Invalid formula for 'vsWhat'";
        else
        {
            s = andVsWhat;
            formulaWhat3 = parse(s);
            if (!formulaWhat3)
                ErrorString = "Invalid formula for 'andVsWhat'";
            else
            {
                if (!cuts.isEmpty())
                {
                    s = cuts;
                    formulaCuts = parse(s);
                    if (!formulaCuts)
                        ErrorString = "Invalid formula for cuts";
                }

                if (formulaCuts || cuts.isEmpty())
                {
                    Hist2D = new TH2D("", "", bins1, from1, to1, bins2, from2, to2);
                    TString titleZ = what.toLocal8Bit().data();
                    Hist2D->GetZaxis()->SetTitle(titleZ);
                    TString titleX = vsWhat.toLocal8Bit().data();
                    Hist2D->GetXaxis()->SetTitle(titleX);
                    TString titleY = andVsWhat.toLocal8Bit().data();
                    Hist2D->GetYaxis()->SetTitle(titleY);
                    TString title = titleZ + " vs " + titleX + " and " + titleY;
                    if (!cuts.isEmpty())
                    {
                        title += ", with ";
                        title += cuts.toLocal8Bit().data();
                    }
                    Hist2D->SetTitle(title);

                    Hist2Dnum = new TH2D("", "", bins1, from1, to1, bins2, from2, to2);
                }
            }
        }
    }
}

AHistorySearchProcessor_Border::~AHistorySearchProcessor_Border()
{
    delete formulaWhat1;
    delete formulaWhat2;
    delete formulaCuts;
    delete Hist1D;
    delete Hist2D;
}

void AHistorySearchProcessor_Border::afterSearch()
{
    //calculating average of the bins for two cases

    if (Hist1D)
    {
        //1D case
        if (formulaWhat2)
        {
            int numEntr = Hist1D->GetEntries();
            *Hist1D = *Hist1D / *Hist1Dnum;
            Hist1D->SetEntries(numEntr);
        }
    }
    else
    {
        if (formulaWhat3)
        {
            int numEntr = Hist2D->GetEntries();
            *Hist2D = *Hist2D / *Hist2Dnum;
            Hist2D->SetEntries(numEntr);
        }
    }
}

void AHistorySearchProcessor_Border::onTransition(const ATrackingStepData &fromfromTr, const ATrackingStepData &fromTr)
{
    //double  x, y, z, time, energy, vx, vy, vz
    //        0  1  2    3     4      5   6   7

    par[0] = fromTr.Position[0];
    par[1] = fromTr.Position[1];
    par[2] = fromTr.Position[2];
    par[3] = fromTr.Time;
    par[4] = fromTr.Energy;

    if (bRequiresDirections)
    {
        double sum2 = 0;
        for (int i=0; i<3; i++)
        {
            const double delta = fromTr.Position[i] - fromfromTr.Position[i];
            sum2 += delta * delta;
        }
        double length = sqrt(sum2);

        if (length == 0)
        {
            par[5] = 0;
            par[6] = 0;
            par[7] = 0;
        }
        else
        {
            for (int i=0; i<3; i++)
                par[5+i] = (fromTr.Position[i] - fromfromTr.Position[i]) / length;
        }
    }

    if (formulaCuts)
    {
        bool bPass = formulaCuts->EvalPar(nullptr, par);
        if (!bPass) return;
    }

    if (Hist1D)
    {
        //1D case
        double res = formulaWhat1->EvalPar(nullptr, par);
        if (formulaWhat2)
        {
            double resX = formulaWhat2->EvalPar(nullptr, par);
            Hist1D->Fill(resX, res);
            Hist1Dnum->Fill(resX, 1.0);
        }
        else Hist1D->Fill(res);
    }
    else
    {
        if (formulaWhat3)
        {
            //3D case
            double res1 = formulaWhat1->EvalPar(nullptr, par);
            double res2 = formulaWhat2->EvalPar(nullptr, par);
            double res3 = formulaWhat3->EvalPar(nullptr, par);
            Hist2D->Fill(res2, res3, res1);
            Hist2Dnum->Fill(res2, res3, 1.0);
        }
        else
        {
            //2D case
            double res1 = formulaWhat1->EvalPar(nullptr, par);
            double res2 = formulaWhat2->EvalPar(nullptr, par);
            Hist2D->Fill(res2, res1);
        }
    }
}

TFormula *AHistorySearchProcessor_Border::parse(QString & expr)
{
    //double  x, y, z, time, energy, vx, vy, vz
    //        0  1  2    3     4      5   6   7

    expr.replace("X", "[0]");
    expr.replace("Y", "[1]");
    expr.replace("Z", "[2]");

    expr.replace("Time", "[3]");
    expr.replace("time", "[3]");

    expr.replace("Energy", "[4]");
    expr.replace("energy", "[4]");

    bRequiresDirections = false;
    if (expr.contains("VX") || expr.contains("VY") || expr.contains("VZ")) bRequiresDirections = true;
    if (expr.contains("Vx") || expr.contains("Vy") || expr.contains("Vz")) bRequiresDirections = true;
    expr.replace("Vx", "[5]");
    expr.replace("VX", "[5]");
    expr.replace("Vy", "[6]");
    expr.replace("VY", "[6]");
    expr.replace("Vz", "[7]");
    expr.replace("VZ", "[7]");

    TFormula * f = new TFormula("", expr.toLocal8Bit().data());
    if (f && f->IsValid()) return f;

    delete f;
    return nullptr;
}
