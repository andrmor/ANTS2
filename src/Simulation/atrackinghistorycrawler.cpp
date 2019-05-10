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
}

void ATrackingHistoryCrawler::findRecursive(const AParticleTrackingRecord & pr, const AFindRecordSelector & opt, AHistorySearchProcessor & processor) const
{
    //int iSec = 0;
    const std::vector<AParticleTrackingRecord *> & secondaries = pr.getSecondaries();
    for (AParticleTrackingRecord * sec : secondaries)
    {
        //qDebug() << "Sec #"<<iSec;
        findRecursive(*sec, opt, processor);
        //qDebug() << "Sec done"<<iSec++;
    }

    //qDebug() << pr.ParticleName;

    if (opt.bParticle && opt.Particle != pr.ParticleName) return;
    if (opt.bPrimary && pr.getSecondaryOf() ) return;
    if (opt.bSecondary && !pr.getSecondaryOf() ) return;

    processor.onNewTrack(pr);

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
                    processor.onTransitionOut(*thisStep);
                if (bEntranceValidated )
                    processor.onTransitionIn(*thisStep);
            }
            else
            {
                const ATrackingStepData * prevStep = (iStep == 0 ? nullptr : steps[iStep-1]); // to safe-guard against handling "Creation" here

                if (bExitValidated && bEntranceValidated && prevStep)
                    processor.onTransition(*prevStep, *thisStep); // not the "next" step here! this is just to extract direction information
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

            processor.onLocalStep(*thisStep);
        }
    }

    processor.onTrackEnd();
}

void AHistorySearchProcessor_findParticles::onNewTrack(const AParticleTrackingRecord &pr)
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

AHistorySearchProcessor_findDepositedEnergy::AHistorySearchProcessor_findDepositedEnergy(CollectionMode mode, int bins, double from, double to)
{
    //qDebug() << mode;
    Mode = mode;
    Hist = new TH1D("", "Deposited energy", bins, from, to);
    Hist->GetXaxis()->SetTitle("Energy, keV");
}

AHistorySearchProcessor_findDepositedEnergy::~AHistorySearchProcessor_findDepositedEnergy()
{
    delete Hist;
}

void AHistorySearchProcessor_findDepositedEnergy::onNewEvent()
{
    if (Mode == OverEvent) Depo = 0;
}

void AHistorySearchProcessor_findDepositedEnergy::onNewTrack(const AParticleTrackingRecord & )
{
    switch (Mode)
    {
    case Individual:
        Depo = 0;
        break;
    case WithSecondaries:

        break;
    case OverEvent:

        break;
    }
}

void AHistorySearchProcessor_findDepositedEnergy::onLocalStep(const ATrackingStepData & tr)
{
    Depo += tr.DepositedEnergy;
}

void AHistorySearchProcessor_findDepositedEnergy::onTrackEnd()
{
    switch (Mode)
    {
    case Individual:
        if (Depo > 0) Hist->Fill(Depo);
        Depo = 0;
        break;
    case WithSecondaries:

        break;
    case OverEvent:

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

void AHistorySearchProcessor_findTravelledDistances::onNewTrack(const AParticleTrackingRecord &)
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

void AHistorySearchProcessor_findTravelledDistances::onTrackEnd()
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

void AHistorySearchProcessor_Border::calculateAverage()
{
    if (Hist1D)
    {
        //1D case
        if (formulaWhat2)
        {
            *Hist1D = *Hist1D / *Hist1Dnum;
        }
    }
    else
    {
        if (formulaWhat3)
        {
            *Hist2D = *Hist2D / *Hist2Dnum;
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
