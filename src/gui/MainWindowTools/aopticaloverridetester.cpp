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

    TVector3 photon(ui->ledST_i->text().toDouble(), ui->ledST_j->text().toDouble(), ui->ledST_k->text().toDouble());
    photon = photon.Unit();
    //vector surface normal
    TVector3 normal(ui->ledST_si->text().toDouble(), ui->ledST_sj->text().toDouble(), ui->ledST_sk->text().toDouble());
    normal = normal.Unit();
    double cos = photon * normal;
    double ang = 180.0 / TMath::Pi() * acos(cos);
    ui->ledAngle->setText( QString::number(ang, 'g', 4) );
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

void AOpticalOverrideTester::on_pbDirectionHelp_clicked()
{
    QString s = "Vectors are not necessary normalized to unity (automatically scaled)\n\n"
            "Note that in override caluclations the normal vector of the surface\n"
            "is provided in such a way that the dot product of the normal and\n"
            "the incoming photon direction is positive.";
    message(s, this);
}

void AOpticalOverrideTester::on_pbST_RvsAngle_clicked()
{
    if ( !testOverride() ) return;

    //TODO wavelength shifted

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
    }

    if ( ui->cobPrVsAngle_WhatToCollect->currentIndex() == 1 )
    {
        TGraph *gS, *gL, *gD, *gT;
        gT = GraphWindow->MakeGraph(&Angle, &Back,   2, "Angle", "", 0, 1, 1, 2, "", true);
        gT->SetMinimum(0);
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
    }
    else
    {
        TGraph *gN, *gA, *gB, *gF;
        gN = GraphWindow->MakeGraph(&Angle, &NotTrigger,   2, "Angle", "", 0, 1, 1, 2, "", true);
        gN->SetMinimum(0);
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
    }

    delete ph.SimStat;
}

void AOpticalOverrideTester::on_pbCSMtestmany_clicked()
{
    if ( !testOverride() ) return;

    //surface normal and photon direction
    TVector3 SurfNorm(ui->ledST_si->text().toDouble(),
                      ui->ledST_sj->text().toDouble(),
                      ui->ledST_sk->text().toDouble());
    SurfNorm = SurfNorm.Unit();

    double N[3]; //needs to calculate override
    N[0] = SurfNorm.X();
    N[1] = SurfNorm.Y();
    N[2] = SurfNorm.Z();

    TVector3 PhotDir(ui->ledST_i->text().toDouble(),
                     ui->ledST_j->text().toDouble(),
                     ui->ledST_k->text().toDouble());
    PhotDir = PhotDir.Unit();

    tracks.clear();
    double d = 0.5; //offset - for drawing only

    //preparing and running cycle with photons
    double abs, back, forw, notTrigger, spike, lobe, lamb;
    abs = back = forw = notTrigger = spike = lobe = lamb = 0;
    TH1D* hist1 = new TH1D("", "", 100, 0, 0);
    hist1->GetXaxis()->SetTitle("Backscattering angle, degrees");

    APhoton ph;
    ph.SimStat = new ASimulationStatistics();

    const int numPhot = ui->sbST_number->value();
    for (int i = 0; i < numPhot; i++)
    {
        ph.v[0] = PhotDir.X(); //old has output direction after full cycle!
        ph.v[1] = PhotDir.Y();
        ph.v[2] = PhotDir.Z();
        ph.waveIndex = getWaveIndex();
        AOpticalOverride::OpticalOverrideResultEnum result = (*pOV)->calculate(*Resources, &ph, N);

        switch (result)
        {
        case AOpticalOverride::NotTriggered: notTrigger++; break;
        case AOpticalOverride::Absorbed: abs++; break;
        case AOpticalOverride::Forward: forw++; break;
        case AOpticalOverride::Back: back++; break;
        default:;
        }

        Color_t col;
        Int_t type;
        if ((*pOV)->Status == AOpticalOverride::SpikeReflection)
        {
            spike++;
            type = 0;
            col = 6; //0,magenta for Spike
        }
        else if ((*pOV)->Status == AOpticalOverride::LobeReflection)
        {
            lobe++;
            type = 1;
            col = 7; //1,teal for Lobe
        }
        else if ((*pOV)->Status == AOpticalOverride::LambertianReflection)
        {
            lamb++;
            type = 2;
            col = 3; //2,grean for lambert
        }
        else
        {
            type = 666;
            col = kBlue; //blue for error
        }

        //TODO if Fresnel, change direction!

        tracks.append(TrackHolderClass(type, col));
        tracks.last().Nodes.append(TrackNodeStruct(d, d, d, 0));
        tracks.last().Nodes.append(TrackNodeStruct(d + ph.v[0], d + ph.v[1], d + ph.v[2], 0));

        double costr = - SurfNorm[0] * ph.v[0] - SurfNorm[1] * ph.v[1] - SurfNorm[2] * ph.v[2];

        hist1->Fill(180.0 / TMath::Pi() * acos(costr));
    }

    GraphWindow->Draw(hist1);
    on_pbST_showTracks_clicked();

    ui->pte->clear();
    QString t = "Process fractions:\n";
    t += QString("  Absorption: %1\n").arg(abs/numPhot);
    t += QString("  Back: %1\n").arg(back/numPhot);
    t += QString("  Forward: %1\n").arg(forw/numPhot);
    t += QString("  Not triggered: %1\n").arg(notTrigger/numPhot);
    t += "\n";

    //show stat of processes
    t += "Backscattering info:\n";
    t += QString("  Specular spike: %1\n").arg(spike/numPhot);
    t += QString("  Diffuse lobe: %1\n").arg(lobe/numPhot);
    t += QString("  Lambertian: %1\n").arg(lamb/numPhot);

    ui->pte->appendPlainText(t);
    ui->pte->moveCursor(QTextCursor::Start);
    ui->pte->ensureCursorVisible();

    delete ph.SimStat;
}

void AOpticalOverrideTester::showGeometry()
{
    gGeoManager->ClearTracks();
    GeometryWindow->ClearRootCanvas();

    double d = 0.5;
    double f = 0.5;

    //surface normal
    TVector3 SurfNorm(ui->ledST_si->text().toDouble(),
                      ui->ledST_sj->text().toDouble(),
                      ui->ledST_sk->text().toDouble());
    SurfNorm = SurfNorm.Unit();
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
    TVector3 PhotDir(ui->ledST_i->text().toDouble(),
                     ui->ledST_j->text().toDouble(),
                     ui->ledST_k->text().toDouble());
    PhotDir = PhotDir.Unit();
    track->AddPoint(d-PhotDir.X(), d-PhotDir.Y(), d-PhotDir.Z(), 0);
    track->SetLineColor(kRed);
    track->SetLineWidth(3);

    GeometryWindow->show();
    GeometryWindow->DrawTracks();
}

void AOpticalOverrideTester::on_pbST_showTracks_clicked()
{
    showGeometry();

    if (tracks.isEmpty()) return;
    int selector = ui->cobST_trackType->currentIndex() - 1;
    if (selector == 3) return; //do not show any tracks

    int numTracks = 0;
    for(int i = 1; i<tracks.count() && numTracks<10000; i++)
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

void AOpticalOverrideTester::on_pbST_uniform_clicked()
{
    if ( !testOverride() ) return;

    TVector3 SurfNorm(ui->ledST_si->text().toDouble(),
                      ui->ledST_sj->text().toDouble(),
                      ui->ledST_sk->text().toDouble());
    SurfNorm = SurfNorm.Unit();
    double N[3];
    N[0] = SurfNorm.X();
    N[1] = SurfNorm.Y();
    N[2] = SurfNorm.Z();

    double K[3];

    int abs, spike, lobe, lamb;
    abs = spike = lobe = lamb = 0;
    TH1D* hist1;
    hist1 = new TH1D("", "", 100, 0, 0);
    hist1->GetXaxis()->SetTitle("Backscattering angle, degrees");

    int num = ui->sbST_number->value();

    APhoton ph;
    ph.SimStat = new ASimulationStatistics();

    for (int i=0; i<num; i++)
      {
        //diffuse illumination - lambertian is used
        double sin2angle = RandGen->Rndm();
        double angle = asin(sqrt(sin2angle));
        double yOff = cos(angle), zOff = -sin(angle);
        N[0] = 0; N[1] = 0; N[2] = -1;   // convention of photon tracer - normal is med1->med2
        K[0] = 0; K[1] = yOff; K[2] = zOff;  // -z direction on xy plane (incidence angle from 90 to 0)

        ph.v[0] = K[0];
        ph.v[1] = K[1];
        ph.v[2] = K[2];
        ph.waveIndex = -1;
        (*pOV)->calculate(*Resources, &ph, N);

        if ((*pOV)->Status == AOpticalOverride::Absorption)
          {
            abs++;
            continue;
          }
        else if ((*pOV)->Status == AOpticalOverride::SpikeReflection)
            spike++;
        else if ((*pOV)->Status == AOpticalOverride::LobeReflection)
            lobe++;
        else if ((*pOV)->Status == AOpticalOverride::LambertianReflection)
            lamb++;
        else
          {
            qCritical()<<"Unknown process!";
            exit(666);
          }

        double costr = -N[0]*K[0] -N[1]*K[1] -N[2]*K[2];  // after scatter, K will be in positive Z direction
        hist1->Fill(180.0 / TMath::Pi() * acos(costr));
      }

    //show cos angle hist
    GraphWindow->Draw(hist1);

    //show stat of processes
    int sum = abs + spike + lobe + lamb;
    QString str = "Scat probability: "+QString::number(1.0*(sum-abs)/num, 'g', 4) + "  Processes:"+
        QString::number(abs) + "/" +
        QString::number(spike) + "/" +
        QString::number(lobe) + "/" +
        QString::number(lamb);
    if (sum>0)
      {
        str += "   (" + QString::number(1.0*abs/sum, 'g', 3) + "/" +
              QString::number(1.0*spike/sum, 'g', 3) + "/" +
              QString::number(1.0*lobe/sum, 'g', 3) + "/" +
              QString::number(1.0*lamb/sum, 'g', 3) + ")";
      }
    //ui->leST_out->setText(str);
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

void AOpticalOverrideTester::on_ledST_i_editingFinished()
{
    updateGUI();
    showGeometry();
}

void AOpticalOverrideTester::on_ledST_j_editingFinished()
{
    updateGUI();
    showGeometry();
}

void AOpticalOverrideTester::on_ledST_k_editingFinished()
{
    if (ui->ledST_k->text().toDouble() >= 0)
    {
        ui->ledST_k->setText("-1");
        message("The photon direction vector along Z axis should be neagtive!", this);
    }
    updateGUI();
    showGeometry();
}

void AOpticalOverrideTester::on_ledAngle_editingFinished()
{
    double angle = ui->ledAngle->text().toDouble();
    if (angle <= -90.0 || angle >= 90.0 )
    {
        ui->ledAngle->setText("45");
        message("Angle should be within (-90, 90) degrees!", this);
        angle = 45.0;
    }

    TVector3 photon(0, 0, -1.0);
    TVector3 perp(0, 1.0, 0);
    angle *= -TMath::Pi() / 180.0;
    photon.Rotate(angle, perp);
    ui->ledST_i->setText( QString::number( photon.x(), 'g', 6) );
    ui->ledST_j->setText( QString::number( photon.y(), 'g', 6) );
    ui->ledST_k->setText( QString::number( photon.z(), 'g', 6) );

    showGeometry();
}
