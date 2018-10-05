#include "awaveshifteroverride.h"
#include "aphoton.h"
#include "amaterial.h"
#include "amaterialparticlecolection.h"
#include "atracerstateful.h"
#include "asimulationstatistics.h"
#include "acommonfunctions.h"
#include "ajsontools.h"

#include <QJsonObject>

#include "TMath.h"
#include "TRandom2.h"
#include "TH1D.h"

#ifdef GUI
#include "afiletools.h"
#include "graphwindowclass.h"
#include "amessage.h"
#include "aglobalsettings.h"
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QComboBox>
#include <QPushButton>
#include "TGraph.h"
#endif

AWaveshifterOverride::AWaveshifterOverride(AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo) {}

AWaveshifterOverride::~AWaveshifterOverride()
{
    if (Spectrum) delete Spectrum;
}

void AWaveshifterOverride::initializeWaveResolved()
{
    bool bWavelengthResolved;
    double waveTo;
    MatCollection->GetWave(bWavelengthResolved, WaveFrom, waveTo, WaveStep, WaveNodes);

    if (bWavelengthResolved)
    {
        ConvertToStandardWavelengthes(&ReemissionProbability_lambda, &ReemissionProbability, WaveFrom, WaveStep, WaveNodes, &ReemissionProbabilityBinned);

        QVector<double> y;
        ConvertToStandardWavelengthes(&EmissionSpectrum_lambda, &EmissionSpectrum, WaveFrom, WaveStep, WaveNodes, &y);
        TString name = "WLSEmSpec";
        name += MatFrom;
        name += "to";
        name += MatTo;
        if (Spectrum) delete Spectrum;
        Spectrum = new TH1D(name,"", WaveNodes, WaveFrom, WaveFrom+WaveStep*WaveNodes);
        for (int j = 1; j<WaveNodes+1; j++)  Spectrum->SetBinContent(j, y[j-1]);
        Spectrum->GetIntegral(); //to make thread safe
    }
    else
    {
        ReemissionProbabilityBinned.clear();
        delete Spectrum; Spectrum = 0;
    }
}

AOpticalOverride::OpticalOverrideResultEnum AWaveshifterOverride::calculate(ATracerStateful &Resources, APhoton *Photon, const double *NormalVector)
{
    //currently assuming there is no scattering on original wavelength - only reemission or absorption

    if ( !Spectrum ||                               //emission spectrum not defined
         Photon->waveIndex == -1 ||                 //or photon without wavelength
         ReemissionProbabilityBinned.isEmpty() )    //or probability not defined
    {
        Status = Absorption;
        Photon->SimStat->OverrideWLSabs++;
        return Absorbed;
    }

    double prob = ReemissionProbabilityBinned.at(Photon->waveIndex); // probability of reemission
    if (Resources.RandGen->Rndm() < prob)
    {
        //triggered!

        //generating new wavelength and waveindex
        double wavelength;
        int waveIndex;
        int attempts = -1;
        do
        {
            attempts++;
            if (attempts > 9)
              {
                Status = Absorption;
                Photon->SimStat->OverrideWLSabs++;
                return Absorbed;
              }
            wavelength = Spectrum->GetRandom();
            waveIndex = (wavelength - WaveFrom)/WaveStep;
        }
        while (waveIndex < Photon->waveIndex); //conserving energy

        Photon->waveIndex = waveIndex;
        Photon->SimStat->OverrideWLSshift++;

        if (ReemissionModel == 0)
        {
            Photon->RandomDir(Resources.RandGen);
            //enering new volume or backscattering?
            //normal is in the positive direction in respect to the original direction!
            if (Photon->v[0]*NormalVector[0] + Photon->v[1]*NormalVector[1] + Photon->v[2]*NormalVector[2] < 0)
              {
                // qDebug()<<"   scattering back";
                Status = LambertianReflection;
                return Back;
              }
            // qDebug()<<"   continuing to the next volume";
            Status = Transmission;
            return Forward;
        }

        double norm2 = 0;
        if (ReemissionModel == 1)
        {
            // qDebug()<<"2Pi lambertian scattering backward";
            do
            {
                Photon->RandomDir(Resources.RandGen);
                Photon->v[0] -= NormalVector[0]; Photon->v[1] -= NormalVector[1]; Photon->v[2] -= NormalVector[2];
                norm2 = Photon->v[0]*Photon->v[0] + Photon->v[1]*Photon->v[1] + Photon->v[2]*Photon->v[2];
            }
            while (norm2 < 0.000001);

            double normInverted = 1.0/TMath::Sqrt(norm2);
            Photon->v[0] *= normInverted; Photon->v[1] *= normInverted; Photon->v[2] *= normInverted;
            Status = LambertianReflection;

            return Back;
        }

        // qDebug()<<"2Pi lambertian scattering forward";
        do
          {
            Photon->RandomDir(Resources.RandGen);
            Photon->v[0] += NormalVector[0]; Photon->v[1] += NormalVector[1]; Photon->v[2] += NormalVector[2];
            norm2 = Photon->v[0]*Photon->v[0] + Photon->v[1]*Photon->v[1] + Photon->v[2]*Photon->v[2];
          }
        while (norm2 < 0.000001);

        double normInverted = 1.0/TMath::Sqrt(norm2);
        Photon->v[0] *= normInverted; Photon->v[1] *= normInverted; Photon->v[2] *= normInverted;
        Status = Transmission;
        return Forward;
    }

    // else absorption
    Status = Absorption;
    Photon->SimStat->OverrideWLSabs++;
    return Absorbed;
}

const QString AWaveshifterOverride::getReportLine() const
{
    QString s = QString("CProb %1 pts; Spectr %2 pts; Mod: ").arg(ReemissionProbability_lambda.size()).arg(EmissionSpectrum_lambda.size());
    switch( ReemissionModel )
    {
    case 0:
        s += "Iso";
        break;
    case 1:
        s += "Lamb_B";
        break;
    case 2:
        s += "Lamb_F";
        break;
    }
    return s;
}

const QString AWaveshifterOverride::getLongReportLine() const
{
    QString s = "--> Wavelength shifter <--\n";
    s += QString("Reemission probaility from %1 to %2 nm\n").arg(ReemissionProbability_lambda.first()).arg(ReemissionProbability_lambda.last());
    s += QString("Number of points: %1\n").arg(ReemissionProbability_lambda.size());
    s += QString("Emission spectrum from %1 to %2 nm\n").arg(EmissionSpectrum_lambda.first()).arg(EmissionSpectrum_lambda.last());
    s += QString("Number of points: %1\n").arg(EmissionSpectrum_lambda.size());
    s += "Reemission model: ";
    switch( ReemissionModel )
    {
    case 0:
        s += "isotropic";
        break;
    case 1:
        s += "Lambertian, back";
        break;
    case 2:
        s += "Lambertian, forward";
        break;
    }
    return s;
}

void AWaveshifterOverride::writeToJson(QJsonObject &json) const
{
    AOpticalOverride::writeToJson(json);

    QJsonArray arRP;
    writeTwoQVectorsToJArray(ReemissionProbability_lambda, ReemissionProbability, arRP);
    json["ReemissionProbability"] = arRP;
    QJsonArray arEm;
    writeTwoQVectorsToJArray(EmissionSpectrum_lambda, EmissionSpectrum, arEm);
    json["EmissionSpectrum"] = arEm;
    json["ReemissionModel"] = ReemissionModel;
}

bool AWaveshifterOverride::readFromJson(const QJsonObject &json)
{
    if ( !parseJson(json, "ReemissionModel", ReemissionModel) ) return false;

    QJsonArray arRP;
    if ( !parseJson(json, "ReemissionProbability", arRP) ) return false;
    if (arRP.isEmpty()) return false;
    if ( !readTwoQVectorsFromJArray(arRP, ReemissionProbability_lambda, ReemissionProbability) ) return false;

    QJsonArray arES;
    if ( !parseJson(json, "EmissionSpectrum", arES) ) return false;
    if (arES.isEmpty()) return false;
    if ( !readTwoQVectorsFromJArray(arES, EmissionSpectrum_lambda, EmissionSpectrum) ) return false;

    return true;
}

#ifdef GUI
QWidget *AWaveshifterOverride::getEditWidget(QWidget *caller, GraphWindowClass *GraphWindow)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QVBoxLayout* vl = new QVBoxLayout(f);
        QHBoxLayout* l = new QHBoxLayout();
            QLabel* lab = new QLabel("Reemission generation:");
        l->addWidget(lab);
            QComboBox* com = new QComboBox();
            com->addItem("Isotropic (4Pi)"); com->addItem("Lambertian, 2Pi back"); com->addItem("Lambertian, 2Pi forward");
            com->setCurrentIndex(ReemissionModel);
            QObject::connect(com, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this](int index) { this->ReemissionModel = index; } );
        l->addWidget(com);
    vl->addLayout(l);
        l = new QHBoxLayout();
            QVBoxLayout* vv = new QVBoxLayout();
                lab = new QLabel("Reemission probability:");
            vv->addWidget(lab);
                lab = new QLabel("Emission spectrum:");
            vv->addWidget(lab);
        l->addLayout(vv);
            vv = new QVBoxLayout();
                QPushButton* pb = new QPushButton("Load");
                pb->setToolTip("Every line of the file should contain 2 numbers:\nwavelength[nm] reemission_probability[0..1]");
                QObject::connect(pb, &QPushButton::clicked, [caller, this] {loadReemissionProbability(caller);});
            vv->addWidget(pb);
                pb = new QPushButton("Load");
                pb->setToolTip("Every line of the file should contain 2 numbers:\nwavelength[nm] relative_intencity[>=0]");
                QObject::connect(pb, &QPushButton::clicked, [caller, this] {loadEmissionSpectrum(caller);});
            vv->addWidget(pb);
        l->addLayout(vv);
            vv = new QVBoxLayout();
                pb = new QPushButton("Show");
                QObject::connect(pb, &QPushButton::clicked, [GraphWindow, caller, this] {showReemissionProbability(GraphWindow, caller);});
            vv->addWidget(pb);
                pb = new QPushButton("Show");
                QObject::connect(pb, &QPushButton::clicked, [GraphWindow, caller, this] {showEmissionSpectrum(GraphWindow, caller);});
            vv->addWidget(pb);
        l->addLayout(vv);
            vv = new QVBoxLayout();
                pb = new QPushButton("Binned");
                QObject::connect(pb, &QPushButton::clicked, [GraphWindow, caller, this] {showBinnedReemissionProbability(GraphWindow, caller);});
            vv->addWidget(pb);
                pb = new QPushButton("Binned");
                QObject::connect(pb, &QPushButton::clicked, [GraphWindow, caller, this] {showBinnedEmissionSpectrum(GraphWindow, caller);});
            vv->addWidget(pb);
        l->addLayout(vv);
    vl->addLayout(l);
        lab = new QLabel("If simulation is NOT wavelength-resolved, this override does nothing!");
        lab->setAlignment(Qt::AlignCenter);
    vl->addWidget(lab);

    return f;
}
#endif

#ifdef GUI
void AWaveshifterOverride::loadReemissionProbability(QWidget* caller)
{
    AGlobalSettings& GlobSet = AGlobalSettings::getInstance();
    QString fileName = QFileDialog::getOpenFileName(caller, "Load reemission probability", GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
    QVector<double> X, Y;
    int ret = LoadDoubleVectorsFromFile(fileName, &X, &Y);
    if (ret == 0)
    {
        ReemissionProbability_lambda = X;
        ReemissionProbability = Y;
    }
}

void AWaveshifterOverride::loadEmissionSpectrum(QWidget *caller)
{
    AGlobalSettings& GlobSet = AGlobalSettings::getInstance();
    QString fileName = QFileDialog::getOpenFileName(caller, "Load emission spectrum", GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
    QVector<double> X, Y;
    int ret = LoadDoubleVectorsFromFile(fileName, &X, &Y);
    if (ret == 0)
    {
        EmissionSpectrum_lambda = X;
        EmissionSpectrum = Y;
    }
}

void AWaveshifterOverride::showReemissionProbability(GraphWindowClass *GraphWindow, QWidget* caller)
{
    if (ReemissionProbability_lambda.isEmpty())
    {
        message("No data were loaded", caller);
        return;
    }
    TGraph* gr = GraphWindow->ConstructTGraph(ReemissionProbability_lambda, ReemissionProbability, "Reemission probability", "Wavelength, nm", "Reemission probability, a.u.", 2, 20, 1, 2, 2);
    gr->SetMinimum(0);
    GraphWindow->Draw(gr, "apl");
}

void AWaveshifterOverride::showEmissionSpectrum(GraphWindowClass *GraphWindow, QWidget *caller)
{
    if (EmissionSpectrum_lambda.isEmpty())
    {
        message("No data were loaded", caller);
        return;
    }
    TGraph* gr = GraphWindow->ConstructTGraph(EmissionSpectrum_lambda, EmissionSpectrum,
                                              "Emission spectrum", "Wavelength, nm", "Relative intensity, a.u.",
                                              4, 20, 1,
                                              4, 2);
    gr->SetMinimum(0);
    GraphWindow->Draw(gr, "apl");
}

void AWaveshifterOverride::showBinnedReemissionProbability(GraphWindowClass *GraphWindow, QWidget *caller)
{
    initializeWaveResolved();

    //TODO run checker

    if ( !MatCollection->IsWaveResolved() )
    {
        message("Simulation is NOT wavelength resolved, override is inactive!", caller);
        return;
    }

    QVector<double> waveIndex;
    for (int i=0; i<WaveNodes; i++) waveIndex << i;
    TGraph* gr = GraphWindow->ConstructTGraph(waveIndex, ReemissionProbabilityBinned,
                                              "Reemission probability (binned)", "Wave index", "Reemission probability, a.u.",
                                              2, 20, 1,
                                              2, 2);
    gr->SetMinimum(0);
    GraphWindow->Draw(gr, "apl");
}

void AWaveshifterOverride::showBinnedEmissionSpectrum(GraphWindowClass *GraphWindow, QWidget *caller)
{
    initializeWaveResolved();

    //TODO run checker

    if ( !MatCollection->IsWaveResolved() )
    {
        message("Simulation is NOT wavelength resolved, override is inactive!", caller);
        return;
    }

    double integral = Spectrum->ComputeIntegral();
    if (integral <= 0)
    {
        message("Binned emission spectrum: integral <=0, override will report an error!", caller);
        return;
    }

    TH1D* SpectrumCopy = new TH1D(*Spectrum);
    SpectrumCopy->SetTitle("Binned emission spectrum");
    SpectrumCopy->GetXaxis()->SetTitle("Wavelength, nm");
    SpectrumCopy->GetYaxis()->SetTitle("Relative intensity, a.u.");
    GraphWindow->Draw(SpectrumCopy, "hist"); //gets ownership of the copy
}
#endif

const QString AWaveshifterOverride::checkOverrideData()
{
    if (ReemissionProbability_lambda.isEmpty())
        return "Reemission probability not loaded";
    if (ReemissionProbability_lambda.size() != ReemissionProbability.size())
        return "Mismatch in reemission probability data";

    if (EmissionSpectrum_lambda.isEmpty())
        return "Emission spectrum not loaded";
    if (EmissionSpectrum_lambda.size() != EmissionSpectrum.size())
        return "Mismatch in emission spectrum data";

    initializeWaveResolved();

    if (MatCollection->IsWaveResolved() && Spectrum->ComputeIntegral() <= 0)
            return "Binned emission spectrum: integral should be > 0";
    return "";
}
