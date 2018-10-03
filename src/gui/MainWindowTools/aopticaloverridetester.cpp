#include "aopticaloverridetester.h"
#include "ui_aopticaloverridetester.h"
#include "amessage.h"
#include "amaterialparticlecolection.h"
#include "aopticaloverride.h"
#include "aopticaloverridescriptinterface.h"
#include "aphoton.h"
#include "atracerstateful.h"
#include "graphwindowclass.h"
#include "asimulationstatistics.h"

#include <QDoubleValidator>
#include <QLineEdit>
#include <QSpinBox>
#include <QDebug>

#include "TVector3.h"
#include "TRandom2.h"
#include "TGraph.h"
#include "TLegend.h"
#include "TMath.h"

AOpticalOverrideTester::AOpticalOverrideTester(GraphWindowClass* GraphWindow, AMaterialParticleCollection* MPcollection, int matFrom, int matTo, QWidget *parent) :
    QMainWindow(parent), ui(new Ui::AOpticalOverrideTester),
    MPcollection(MPcollection), MatFrom(matFrom), MatTo(matTo),
    GraphWindow(GraphWindow)
{
    ui->setupUi(this);

    ov = (*MPcollection)[matFrom]->OpticalOverrides.at(matTo);

    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);

    RandGen = new TRandom2();
    Resources = new ATracerStateful(RandGen);
    Resources->generateScriptInfrastructure(MPcollection);
}

AOpticalOverrideTester::~AOpticalOverrideTester()
{
    delete Resources;
    delete RandGen;
    delete ui;
}

void AOpticalOverrideTester::on_pbDirectionHelp_clicked()
{
    QString s = "Vectors are not necessary normalized to unity (automatic when used)\n\n"
            "Note that in overrides caluclations"
            "normal of the surface is always oriented"
            "in the direction away from the incoming photon";
}

void AOpticalOverrideTester::on_pbST_RvsAngle_clicked()
{
    int numPhotons = ui->sbST_number->value();
    QVector<double> Spike(91, 0), Lobe(91, 0), Diffuse(91, 0), Total(91, 0), Angle;
    double N[3], K[3];
    N[0] = 0;
    N[1] = 0;
    N[2] = 1.0;

    APhoton ph;
    ph.SimStat = new ASimulationStatistics();

    for (int iA=0; iA<91; iA++) //cycle by angle of incidence
      {
        double angle = iA;
        if (angle == 90) angle = 89.9;
        Angle.append(angle);
        //angle->photon direction
        double cosA = cos(TMath::Pi() * angle / 180.0);
        double sinA = sin(TMath::Pi() * angle / 180.0);
        for (int i=0; i<numPhotons; i++) //cycle by photons
        {
            //have to reset since K is modified by the override object
            K[0] = sinA;
            K[1] = 0;
            K[2] = cosA;

            ph.v[0] = K[0];
            ph.v[1] = K[1];
            ph.v[2] = K[2];
            ph.waveIndex = -1;  //TODO wave res?
            ov->calculate(*Resources, &ph, N);

            switch (ov->Status)
            {
              case AOpticalOverride::Absorption: continue; break;
              case AOpticalOverride::SpikeReflection: Spike[iA]++; break;
              case AOpticalOverride::LobeReflection: Lobe[iA]++; break;
              case AOpticalOverride::LambertianReflection: Diffuse[iA]++; break; //TODO what about scrip override?
              default: ;
            }
        }
        Spike[iA] /= numPhotons;
        Lobe[iA] /= numPhotons;
        Diffuse[iA] /= numPhotons;
        Total[iA] = Spike[iA]+Lobe[iA]+Diffuse[iA];
      }

    TGraph *gS, *gL, *gD, *gT;
    gT = GraphWindow->MakeGraph(&Angle, &Total,   2, "Angle", "", 0, 1, 1, 2, "", true);
    gT->SetMinimum(0);
    gT->SetTitle("Total");
    gS = GraphWindow->MakeGraph(&Angle, &Spike,   1, "Angle", "", 0, 1, 1, 1, "", true);
    gS->SetTitle("Spike");
    gL = GraphWindow->MakeGraph(&Angle, &Lobe,    3, "Angle", "", 0, 1, 1, 1, "", true);
    gL->SetTitle("Lobe");
    gD = GraphWindow->MakeGraph(&Angle, &Diffuse, 4, "Angle", "", 0, 1, 1, 1, "", true);
    gD->SetTitle("Diffuse");

    GraphWindow->Draw(gT, "AL");
    GraphWindow->Draw(gS, "Lsame");
    GraphWindow->Draw(gL, "Lsame");
    GraphWindow->Draw(gD, "Lsame");

    TLegend *leg = new TLegend(0.1, 0.7, 0.3, 0.9);
    leg->SetFillColor(0);
    leg->AddEntry(gT, "Total", "lp");
    leg->AddEntry(gS, "Spike", "lp");
    leg->AddEntry(gL, "Lobe", "lp");
    leg->AddEntry(gD, "Diffuse", "lp");
    GraphWindow->Draw(leg, "same");

    delete ph.SimStat;
}

