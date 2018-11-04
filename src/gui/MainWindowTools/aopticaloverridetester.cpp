#include "aopticaloverridetester.h"
#include "ui_aopticaloverridetester.h"
#include "mainwindow.h"
#include "amessage.h"
#include "amaterialparticlecolection.h"
#include "aopticaloverride.h"
#include "aopticaloverridescriptinterface.h"
#include "aphoton.h"
#include "atracerstateful.h"
#include "graphwindowclass.h"
#include "geometrywindowclass.h"
#include "asimulationstatistics.h"
#include "atrackrecords.h"
#include "ajsontools.h"
#include "aglobalsettings.h"

#include <QDoubleValidator>
#include <QLineEdit>
#include <QSpinBox>
#include <QDebug>

#include "TVector3.h"
#include "TRandom2.h"
#include "TGraph.h"
#include "TLegend.h"
#include "TMath.h"
#include "TH1D.h"
#include "TGeoManager.h"
#include "TVirtualGeoTrack.h"

static QVector<TrackHolderClass> tracks;

AOpticalOverrideTester::AOpticalOverrideTester(AOpticalOverride ** ovLocal, MainWindow *MW, int matFrom, int matTo, QWidget *parent) :
    QMainWindow(parent), ui(new Ui::AOpticalOverrideTester),
    pOV(ovLocal), MatFrom(matFrom), MatTo(matTo), MW(MW),
    MPcollection(MW->MpCollection), GraphWindow(MW->GraphWindow), GeometryWindow(MW->GeometryWindow)
{
    ui->setupUi(this);
    setWindowTitle("Override tester");

    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);

    RandGen = new TRandom2();
    Resources = new ATracerStateful(RandGen);
    Resources->generateScriptInfrastructure(MW->MpCollection);

    QStringList matNames = MW->MpCollection->getListOfMaterialNames();
    ui->labMaterials->setText( QString("(%1 -> %2)").arg(matNames.at(matFrom)).arg(matNames.at(matTo)) );

    updateGUI();
}

void AOpticalOverrideTester::updateGUI()
{
    bool bWR = MPcollection->IsWaveResolved();
    if (!bWR)
    {
        ui->cbWavelength->setEnabled(false);
        ui->cbWavelength->setChecked(false);
    }
    else
        ui->cbWavelength->setEnabled(true);

    int waveIndex = getWaveIndex();
    ui->ledST_Ref1->setText( QString::number( (*MPcollection)[MatFrom]->getRefractiveIndex(waveIndex) ) );
    ui->ledST_Ref2->setText( QString::number( (*MPcollection)[MatTo]  ->getRefractiveIndex(waveIndex) ) );
}

AOpticalOverrideTester::~AOpticalOverrideTester()
{
    delete Resources;
    delete RandGen;
    delete ui;
}

void AOpticalOverrideTester::writeToJson(QJsonObject &json) const
{
    json["PositionX"] = x();
    json["PositionY"] = y();
}

void AOpticalOverrideTester::readFromJson(const QJsonObject &json)
{
    if (json.isEmpty()) return;

    int x, y;
    parseJson(json, "PositionX", x);
    parseJson(json, "PositionY", y);
    if (x>0 && y>0) move(x, y);
}

void AOpticalOverrideTester::on_pbST_RvsAngle_clicked()
{
    if ( !testOverride() ) return;

    int numPhotons = ui->sbST_number->value();
    QVector<double> Back(91, 0), Forward(91, 0), Absorb(91, 0), NotTrigger(91, 0);
    QVector<double> Spike(91, 0), BackLobe(91, 0), BackLambert(91, 0), WaveShifted(91, 0);
    QVector<double> Angle;
    double N[3], K[3];
    N[0] = 0;
    N[1] = 0;
    N[2] = 1.0;

    APhoton ph;
    ph.SimStat = new ASimulationStatistics();

    for (int iAngle = 0; iAngle < 91; iAngle++) //cycle by angle of incidence
    {
        double angle = iAngle;
        if (angle == 90) angle = 89.99;
        Angle.append(angle);

        //angle->photon direction
        double cosA = cos(TMath::Pi() * angle / 180.0);
        double sinA = sin(TMath::Pi() * angle / 180.0);
        for (int iPhot=0; iPhot<numPhotons; iPhot++) //cycle by photons
        {
            //have to reset since K is modified by the override object
            K[0] = sinA;
            K[1] = 0;
            K[2] = cosA;

            ph.v[0] = K[0];
            ph.v[1] = K[1];
            ph.v[2] = K[2];
            ph.waveIndex = getWaveIndex();
            AOpticalOverride::OpticalOverrideResultEnum result = (*pOV)->calculate(*Resources, &ph, N);

            switch (result)
            {
            case AOpticalOverride::NotTriggered: NotTrigger[iAngle]++; break;
            case AOpticalOverride::Absorbed: Absorb[iAngle]++; break;
            case AOpticalOverride::Forward: Forward[iAngle]++; break;
            case AOpticalOverride::Back: Back[iAngle]++; break;
            default:;
            }

            switch ((*pOV)->Status)
            {
              case AOpticalOverride::SpikeReflection: Spike[iAngle]++; break;
              case AOpticalOverride::LobeReflection: BackLobe[iAngle]++; break;
              case AOpticalOverride::LambertianReflection: BackLambert[iAngle]++; break;
              default: ;
            }
        }
        NotTrigger[iAngle] /= numPhotons;
        Absorb[iAngle] /= numPhotons;
        Forward[iAngle] /= numPhotons;
        Back[iAngle] /= numPhotons;

        Spike[iAngle] /= numPhotons;
        BackLobe[iAngle] /= numPhotons;
        BackLambert[iAngle] /= numPhotons;

        WaveShifted[iAngle] = ph.SimStat->wavelengthChanged / numPhotons;
        ph.SimStat->wavelengthChanged = 0;
    }

    int what = ui->cobPrVsAngle_WhatToCollect->currentIndex();
    switch (what)
    {
      case 0:
      {
        TGraph *gN, *gA, *gB, *gF = 0;
        gN = GraphWindow->MakeGraph(&Angle, &NotTrigger,   2, "Angle, deg", "Fraction", 0, 1, 1, 2, "", true);
        gN->SetMinimum(0);
        gN->SetMaximum(1.05);
        gN->SetTitle("Not triggered");
        gA = GraphWindow->MakeGraph(&Angle, &Absorb,   1, "Angle", "", 0, 1, 1, 2, "", true);
        gA->SetTitle("Absorption");
        gB = GraphWindow->MakeGraph(&Angle, &Back,    3, "Angle", "", 0, 1, 1, 2, "", true);
        gB->SetTitle("Back");
        gF = GraphWindow->MakeGraph(&Angle, &Forward, 4, "Angle", "", 0, 1, 1, 2, "", true);
        gF->SetTitle("Forward");

        GraphWindow->Draw(gN, "AL");
        GraphWindow->Draw(gA, "Lsame");
        GraphWindow->Draw(gB, "Lsame");
        GraphWindow->Draw(gF, "Lsame");

        GraphWindow->on_pbAddLegend_clicked();
        break;
      }
      case 1:
      {
        TGraph *gS, *gL, *gD, *gT;
        gT = GraphWindow->MakeGraph(&Angle, &Back,   2, "Angle, deg", "Fraction", 0, 1, 1, 2, "", true);
        gT->SetMinimum(0);
        gT->SetMaximum(1.05);
        gT->SetTitle("All reflections");
        gS = GraphWindow->MakeGraph(&Angle, &Spike,   1, "Angle", "", 0, 1, 1, 2, "", true);
        gS->SetTitle("Spike");
        gL = GraphWindow->MakeGraph(&Angle, &BackLobe,    3, "Angle", "", 0, 1, 1, 2, "", true);
        gL->SetTitle("Lobe");
        gD = GraphWindow->MakeGraph(&Angle, &BackLambert, 4, "Angle", "", 0, 1, 1, 2, "", true);
        gD->SetTitle("Diffuse");

        GraphWindow->Draw(gT, "AL");
        GraphWindow->Draw(gS, "Lsame");
        GraphWindow->Draw(gL, "Lsame");
        GraphWindow->Draw(gD, "Lsame");

        GraphWindow->on_pbAddLegend_clicked();
        break;
      }
      case 2:
      {
        TGraph *gW = GraphWindow->ConstructTGraph(Angle, WaveShifted, "Wavelength shifted", "Angle, deg", "Fraction", 4, 0, 1, 4, 1, 2);
        gW->SetMaximum(1.05);
        gW->SetMinimum(0);
        GraphWindow->Draw(gW, "AL");
        GraphWindow->on_pbAddLegend_clicked();
        break;
      }
    }

    delete ph.SimStat;
}

void AOpticalOverrideTester::on_pbCSMtestmany_clicked()
{
    if ( !testOverride() ) return;

    //surface normal and photon direction
    TVector3 SurfNorm(0, 0, -1.0);
    double N[3]; //needs to calculate override
    N[0] = SurfNorm.X();
    N[1] = SurfNorm.Y();
    N[2] = SurfNorm.Z();
    TVector3 PhotDir = getPhotonVector();

    tracks.clear();
    double d = 0.5; //offset - for drawing only

    //preparing and running cycle with photons

    TH1D* hist1 = new TH1D("", "", 100, 0, 0);
    hist1->GetXaxis()->SetTitle("Backscattering angle, degrees");

    APhoton ph;
    ph.SimStat = new ASimulationStatistics();
    AReportForOverride rep;

    const int waveIndex = getWaveIndex();
    const int numPhot = ui->sbST_number->value();    
    for (int i = 0; i < numPhot; i++)
    {
        ph.v[0] = PhotDir.X(); //old has output direction after full cycle!
        ph.v[1] = PhotDir.Y();
        ph.v[2] = PhotDir.Z();
        ph.time = 0;
        ph.waveIndex = waveIndex;
        AOpticalOverride::OpticalOverrideResultEnum result = (*pOV)->calculate(*Resources, &ph, N);

        //in case of absorption or not triggered override, do not build tracks!
        switch (result)
        {
        case AOpticalOverride::Absorbed:     rep.abs++; continue;           // ! ->
        case AOpticalOverride::NotTriggered: rep.notTrigger++; continue;    // ! ->
        case AOpticalOverride::Forward:      rep.forw++; break;
        case AOpticalOverride::Back:         rep.back++; break;
        default:                             rep.error++; continue;         // ! ->
        }

        Color_t col;
        Int_t type;
        if ((*pOV)->Status == AOpticalOverride::SpikeReflection)
        {
            rep.Bspike++;
            type = 0;
            col = 6; //0,magenta for Spike
        }
        else if ((*pOV)->Status == AOpticalOverride::LobeReflection)
        {
            rep.Blobe++;
            type = 1;
            col = 7; //1,teal for Lobe
        }
        else if ((*pOV)->Status == AOpticalOverride::LambertianReflection)
        {
            rep.Blamb++;
            type = 2;
            col = 3; //2,grean for lambert
        }
        else
        {
            type = 666;
            col = kBlue; //blue for error
        }

        tracks.append(TrackHolderClass(type, col));
        tracks.last().Nodes.append(TrackNodeStruct(d, d, d, 0));
        tracks.last().Nodes.append(TrackNodeStruct(d + ph.v[0], d + ph.v[1], d + ph.v[2], 0));

        double costr = - SurfNorm[0] * ph.v[0] - SurfNorm[1] * ph.v[1] - SurfNorm[2] * ph.v[2];

        hist1->Fill(180.0 / TMath::Pi() * acos(costr));
    }

    GraphWindow->Draw(hist1);
    on_pbST_showTracks_clicked();

    rep.waveChanged = ph.SimStat->wavelengthChanged;
    rep.timeChanged = ph.SimStat->timeChanged;
    reportStatistics(rep, numPhot);

    delete ph.SimStat;
}

void AOpticalOverrideTester::showGeometry()
{
    gGeoManager->ClearTracks();
    GeometryWindow->ClearRootCanvas();

    double d = 0.5;
    double f = 0.5;

    //surface normal
    TVector3 SurfNorm(0, 0, -1.0);
    int track_index = gGeoManager->AddTrack(1,22);
    TVirtualGeoTrack* track = gGeoManager->GetTrack(track_index);
    track->AddPoint(d, d, d, 0);
    track->AddPoint(d + SurfNorm.X(), d + SurfNorm.Y(), d + SurfNorm.Z(), 0);
    track->SetLineWidth(3);
    track->SetLineStyle(2);
    track->SetLineColor(1);

    //surface
    track_index = gGeoManager->AddTrack(1, 22);
    track = gGeoManager->GetTrack(track_index);
    TVector3 perp = SurfNorm.Orthogonal();
    perp.Rotate(0.25 * TMath::Pi(), SurfNorm);
    for (int i=0; i<5; i++)
      {
        track->AddPoint(d + f * perp.X(), d + f * perp.Y(), d + f * perp.Z(), 0);
        perp.Rotate(0.5 * TMath::Pi(), SurfNorm);
      }
    track->SetLineWidth(3);
    track->SetLineColor(1);

    //intitial photon track
    track_index = gGeoManager->AddTrack(1, 10);
    track = gGeoManager->GetTrack(track_index);
    track->AddPoint(d, d, d, 0);

    TVector3 PhotDir = getPhotonVector();
    track->AddPoint(d - PhotDir.X(), d - PhotDir.Y(), d - PhotDir.Z(), 0);
    track->SetLineColor(kRed);
    track->SetLineWidth(3);

    GeometryWindow->show();
    GeometryWindow->DrawTracks();
}

void AOpticalOverrideTester::on_pbST_showTracks_clicked()
{
    GeometryWindow->ShowAndFocus();
    showGeometry();

    if (tracks.isEmpty()) return;
    int selector = ui->cobST_trackType->currentIndex() - 1;
    if (selector == 3) return; //do not show any tracks

    int numTracks = 0;
    for(int i = 1; i<tracks.count() && numTracks < maxNumTracks; i++)
    {
        const TrackHolderClass* th = &tracks.at(i);
        //filter
        if (selector>-1)  //-1 - show all
          if (selector != th->UserIndex) continue;

        int track_index = gGeoManager->AddTrack(1,22);
        TVirtualGeoTrack* track = gGeoManager->GetTrack(track_index);
        track->SetLineColor(th->Color);
        track->SetLineWidth(1);
        for (int iNode=0; iNode<th->Nodes.size(); iNode++)
          track->AddPoint(th->Nodes[iNode].R[0], th->Nodes[iNode].R[1], th->Nodes[iNode].R[2], th->Nodes[iNode].Time);
    }

    GeometryWindow->show();
    GeometryWindow->DrawTracks();
}

bool AOpticalOverrideTester::testOverride()
{
    if ( !(*pOV) )
    {
        message("Override not defined!", this);
        return false;
    }

    QString err = (*pOV)->checkOverrideData();
    if (!err.isEmpty())
    {
        message("Override reports an error:\n" + err, this);
        return false;
    }
    return true;
}

int AOpticalOverrideTester::getWaveIndex()
{
    if (ui->cbWavelength->isChecked())
        return MPcollection->WaveToIndex( ui->ledST_wave->text().toDouble() ); // always in [0, WaveNodes-1]
    else return -1;
}

const TVector3 AOpticalOverrideTester::getPhotonVector()
{
    TVector3 PhotDir(0, 0, -1.0);
    TVector3 perp(0, 1.0, 0);
    double angle = ui->ledAngle->text().toDouble();
    angle *= -TMath::Pi() / 180.0;
    PhotDir.Rotate(angle, perp);
    return PhotDir;
}

void AOpticalOverrideTester::on_pbST_uniform_clicked()
{
    if ( !testOverride() ) return;

    double N[3]; //normal
    N[0] = 0;
    N[1] = 0;
    N[2] = -1.0;

    double K[3]; //photon direction - new for every photon!

    TH1D* hist1 = new TH1D("", "", 100, 0, 0);
    hist1->GetXaxis()->SetTitle("Backscattering angle, degrees");

    const int waveIndex = getWaveIndex();
    const int numPhot = ui->sbST_number->value();

    APhoton ph;
    ph.SimStat = new ASimulationStatistics();
    AReportForOverride rep;

    for (int i = 0; i < numPhot; i++)
    {
        //diffuse illumination - lambertian is used
        double sin2angle = RandGen->Rndm();
        double angle = asin(sqrt(sin2angle));
        double yOff = cos(angle), zOff = -sin(angle);
        K[0] = 0; K[1] = yOff; K[2] = zOff;  // -z direction on xy plane (incidence angle from 90 to 0)

        ph.v[0] = K[0];
        ph.v[1] = K[1];
        ph.v[2] = K[2];
        ph.time = 0;
        ph.waveIndex = waveIndex;

        AOpticalOverride::OpticalOverrideResultEnum result = (*pOV)->calculate(*Resources, &ph, N);

        switch (result)
        {
        case AOpticalOverride::Absorbed: rep.abs++; break;
        case AOpticalOverride::NotTriggered: rep.notTrigger++; break;
        case AOpticalOverride::Forward: rep.forw++; break;
        case AOpticalOverride::Back: rep.back++; break;
        default: rep.error++;
        }

        if ((*pOV)->Status == AOpticalOverride::SpikeReflection) rep.Bspike++;
        else if ((*pOV)->Status == AOpticalOverride::LobeReflection) rep.Blobe++;
        else if ((*pOV)->Status == AOpticalOverride::LambertianReflection) rep.Blamb++;

        double costr = N[0]*K[0] + N[1]*K[1] + N[2]*K[2];
        hist1->Fill(180.0 / TMath::Pi() * acos(costr));
    }

    GraphWindow->Draw(hist1);

    rep.waveChanged = ph.SimStat->wavelengthChanged;
    rep.timeChanged = ph.SimStat->timeChanged;
    reportStatistics(rep, numPhot);

    showGeometry(); //to clear track vis

    delete ph.SimStat;
}

void AOpticalOverrideTester::on_cbWavelength_toggled(bool)
{
    updateGUI();
}

void AOpticalOverrideTester::on_ledST_wave_editingFinished()
{
    updateGUI();
}

void AOpticalOverrideTester::on_ledAngle_editingFinished()
{
    double angle = ui->ledAngle->text().toDouble();
    if (angle < 0 || angle >= 90.0 )
    {
        ui->ledAngle->setText("45");
        message("Angle should be within [0, 90) degrees!", this);
        angle = 45.0;
    }

    showGeometry();
}

void AOpticalOverrideTester::reportStatistics(const AReportForOverride &rep, int numPhot)
{
    ui->pte->clear();

    QString t;
    if (rep.error > 0)  t += QString("Error detected: %1\n\n").arg(rep.error);

    t += "Processes:\n";
    if (rep.abs > 0)    t += QString("  Absorption: %1%  (%2)\n").arg(rep.abs/numPhot*100.0).arg(rep.abs);
    if (rep.back > 0)   t += QString("  Back: %1%  (%2)\n").arg(rep.back/numPhot*100.0).arg(rep.back);
    if (rep.forw)       t += QString("  Forward: %1%  (%2)\n").arg(rep.forw/numPhot*100.0).arg(rep.forw);
    if (rep.notTrigger) t += QString("  Not triggered: %1%  (%2)\n").arg(rep.notTrigger/numPhot*100.0).arg(rep.notTrigger);
    t += "\n";

    if (rep.back > 0)
    {
        //show stat of processes
        t += "Backscattering composition:\n";
        if (rep.Bspike > 0) t += QString("  Specular spike: %1%  (%2)\n").arg(rep.Bspike/rep.back*100.0).arg(rep.Bspike);
        if (rep.Blobe > 0)  t += QString("  Diffuse lobe: %1%  (%2)\n").arg(rep.Blobe/rep.back*100.0).arg(rep.Blobe);
        if (rep.Blamb > 0)  t += QString("  Lambertian: %1%  (%2)\n").arg(rep.Blamb/rep.back*100.0).arg(rep.Blamb);
    }

    if (rep.waveChanged > 0)
        t += QString("\nWavelength changed: %1  (%2)\n").arg(rep.waveChanged/numPhot*100.0).arg(rep.waveChanged);

    if (rep.timeChanged > 0)
        t += QString("\nTime changed: %1  (%2)\n").arg(rep.timeChanged/numPhot*100.0).arg(rep.timeChanged);


    ui->pte->appendPlainText(t);
    ui->pte->moveCursor(QTextCursor::Start);
    ui->pte->ensureCursorVisible();
}
