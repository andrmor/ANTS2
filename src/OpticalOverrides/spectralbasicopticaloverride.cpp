#include "spectralbasicopticaloverride.h"

#include "aphoton.h"
#include "amaterial.h"
#include "amaterialparticlecolection.h"
#include "atracerstateful.h"
#include "asimulationstatistics.h"
#include "acommonfunctions.h"
#include "ajsontools.h"

#include <QJsonObject>
#include <QDebug>

#include "TMath.h"
#include "TRandom2.h"
#include "TH1D.h"

#ifdef GUI
#include "afiletools.h"
#include "graphwindowclass.h"
#include "amessage.h"
#include "TGraph.h"
#include "TMultiGraph.h"
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
#endif

SpectralBasicOpticalOverride::SpectralBasicOpticalOverride(AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo)
    : ABasicOpticalOverride(MatCollection, MatFrom, MatTo)
{
    Wave << 500;
    ProbLoss << 0;
    ProbRef << 0;
    ProbDiff << 0;
}

AOpticalOverride::OpticalOverrideResultEnum SpectralBasicOpticalOverride::calculate(ATracerStateful &Resources, APhoton *Photon, const double *NormalVector)
{
    int waveIndex = Photon->waveIndex;
    if (!bWaveResolved || waveIndex == -1) waveIndex = effectiveWaveIndex; //guard: if not resolved, script ovberride can in principle assign index != -1

    probLoss = ProbLossBinned.at(waveIndex);
    probDiff = ProbDiffBinned.at(waveIndex);
    probRef  = ProbRefBinned.at(waveIndex);

    return ABasicOpticalOverride::calculate(Resources, Photon, NormalVector);
}

const QString SpectralBasicOpticalOverride::getReportLine() const
{
    return QString("Spectral data with %1 points").arg(Wave.size());
}

const QString SpectralBasicOpticalOverride::getLongReportLine() const
{
    QString s = "--> Simplistic spectral <--\n";
    s += QString("Spectral data from %1 to %2 nm\n").arg(Wave.first()).arg(Wave.last());
    s += QString("Number of points: %1\n").arg(Wave.size());
    s += QString("Effective wavelength: %1").arg(effectiveWavelength);
    return s;
}

void SpectralBasicOpticalOverride::writeToJson(QJsonObject &json) const
{
    AOpticalOverride::writeToJson(json);

    json["ScatMode"] = scatterModel;
    json["EffWavelength"] = effectiveWavelength;

    if (Wave.size() != ProbLoss.size() || Wave.size() != ProbRef.size() || Wave.size() != ProbDiff.size())
    {
        qWarning() << "Mismatch in data size for SpectralBasicOverride! skipping data!";
        return;
    }
    QJsonArray sp;
    for (int i=0; i<Wave.size(); i++)
    {
        QJsonArray ar;
        ar << Wave.at(i) << ProbLoss.at(i) << ProbRef.at(i) << ProbDiff.at(i);
        sp << ar;
    }
    json["Data"] = sp;
}

bool SpectralBasicOpticalOverride::readFromJson(const QJsonObject &json)
{
    if ( !parseJson(json, "ScatMode", scatterModel) ) return false;
    if ( !parseJson(json, "EffWavelength", effectiveWavelength) ) return false;

    //after constructor vectors are not empty!
    Wave.clear();
    ProbLoss.clear();
    ProbRef.clear();
    ProbDiff.clear();

    QJsonArray sp;
    if ( !parseJson(json, "Data", sp) ) return false;
    if (sp.isEmpty()) return false;
    for (int i=0; i<sp.size(); i++)
    {
        QJsonArray ar = sp.at(i).toArray();
        if (ar.size() != 4) return false;
        Wave     << ar.at(0).toDouble();
        ProbLoss << ar.at(1).toDouble();
        ProbRef  << ar.at(2).toDouble();
        ProbDiff << ar.at(3).toDouble();
    }
    return true;
}

void SpectralBasicOpticalOverride::initializeWaveResolved()
{
    double waveFrom, waveTo, waveStep;
    int waveNodes;
    MatCollection->GetWave(bWaveResolved, waveFrom, waveTo, waveStep, waveNodes);

    if (bWaveResolved)
    {
        ConvertToStandardWavelengthes(&Wave, &ProbLoss, waveFrom, waveStep, waveNodes, &ProbLossBinned);
        ConvertToStandardWavelengthes(&Wave, &ProbRef, waveFrom, waveStep, waveNodes, &ProbRefBinned);
        ConvertToStandardWavelengthes(&Wave, &ProbDiff, waveFrom, waveStep, waveNodes, &ProbDiffBinned);

        effectiveWaveIndex = (effectiveWavelength - waveFrom) / waveStep;
        if (effectiveWaveIndex < 0) effectiveWaveIndex = 0;
        else if (effectiveWaveIndex >= waveNodes ) effectiveWaveIndex = waveNodes - 1;
        //qDebug() << "Eff wave"<<effectiveWavelength << "assigned index:"<<effectiveWaveIndex;
    }
    else
    {
        int isize = Wave.size();
        int i = 0;
        if (isize != 1)  //esle use i = 0
        {
            for (; i < isize; i++)
                if (Wave.at(i) > effectiveWavelength) break;

            //closest is i-1 or i
            if (i != 0)
                if ( fabs(Wave.at(i-1) - effectiveWavelength) < fabs(Wave.at(i) - effectiveWavelength) ) i--;
        }

        //qDebug() << "Selected i = "<< i << "with wave"<<Wave.at(i) << Wave;
        effectiveWaveIndex = 0;
        ProbLossBinned = QVector<double>(1, ProbLoss.at(i));
        ProbRefBinned = QVector<double>(1, ProbRef.at(i));
        ProbDiffBinned = QVector<double>(1, ProbDiff.at(i));
        //qDebug() << "LossRefDiff"<<ProbLossBinned.at(effectiveWaveIndex)<<ProbRefBinned.at(effectiveWaveIndex)<<ProbDiffBinned.at(effectiveWaveIndex);
    }
}

const QString SpectralBasicOpticalOverride::loadData(const QString &fileName)
{
    QVector< QVector<double>* > vec;
    vec << &Wave << &ProbLoss << &ProbRef << &ProbDiff;
    QString err = LoadDoubleVectorsFromFile(fileName, vec);
    if (!err.isEmpty()) return err;

    for (int i=0; i<Wave.size(); i++)
    {
        double sum = ProbLoss.at(i) + ProbRef.at(i) + ProbDiff.at(i);
        if (sum > 1.0) return QString("Sum of probabilities is larger than 1.0 for wavelength of ") + QString::number(Wave.at(i)) + " nm";
    }

    if (Wave.isEmpty())
        return "No data were read from the file!";

    return "";
}

#ifdef GUI
void SpectralBasicOpticalOverride::loadSpectralData(QWidget* caller)
{
    AGlobalSettings& GlobSet = AGlobalSettings::getInstance();
    QString fileName = QFileDialog::getOpenFileName(caller, "Load spectral data (Wavelength, Absorption, Reflection, Scattering)", GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    QVector< QVector<double>* > vec;
    vec << &Wave << &ProbLoss << &ProbRef << &ProbDiff;
    QString err = LoadDoubleVectorsFromFile(fileName, vec);
    if (!err.isEmpty()) message(err, caller);
}

void SpectralBasicOpticalOverride::showLoaded(GraphWindowClass* GraphWindow)
{
    QVector<double> Fr;
    for (int i=0; i<Wave.size(); i++)
        Fr << (1.0 - ProbLoss.at(i) - ProbRef.at(i) - ProbDiff.at(i));

    TMultiGraph* mg = new TMultiGraph();
    TGraph* gLoss = GraphWindow->ConstructTGraph(Wave, ProbLoss, "Absorption", "Wavelength, nm", "", 2, 20, 1, 2);
    mg->Add(gLoss, "LP");
    TGraph* gRef = GraphWindow->ConstructTGraph(Wave, ProbRef, "Specular reflection", "Wavelength, nm", "", 4, 21, 1, 4);
    mg->Add(gRef, "LP");
    TGraph* gDiff = GraphWindow->ConstructTGraph(Wave, ProbDiff, "Diffuse scattering", "Wavelength, nm", "", 7, 22, 1, 7);
    mg->Add(gDiff, "LP");
    TGraph* gFr = GraphWindow->ConstructTGraph(Wave, Fr, "Fresnel", "Wavelength, nm", "", 1, 24, 1, 1, 1, 1);
    mg->Add(gFr, "LP");

    mg->SetMinimum(0);
    GraphWindow->Draw(mg, "apl");
    mg->GetXaxis()->SetTitle("Wavelength, nm");
    mg->GetYaxis()->SetTitle("Probability");
    GraphWindow->AddLegend(0.7,0.8, 0.95,0.95, "");
}

void SpectralBasicOpticalOverride::showBinned(QWidget *widget, GraphWindowClass *GraphWindow)
{
    bool bWR;
    double WaveFrom, WaveTo, WaveStep;
    int WaveNodes;
    MatCollection->GetWave(bWR, WaveFrom, WaveTo, WaveStep, WaveNodes);

    initializeWaveResolved();

    //TODO run checker

    if (!bWR)
    {
        QString s = "Simulation is configured as not wavelength-resolved\n";
        s +=        "All photons will have the same properties:\n";
        s +=QString("Absorption: %1").arg(probLoss) + "\n";
        s +=QString("Specular reflection: %1").arg(probRef) + "\n";
        s +=QString("Scattering: %1").arg(probDiff);
        message(s, widget);
        return;
    }

    QVector<double> waveIndex;
    for (int i=0; i<WaveNodes; i++) waveIndex << i;

    QVector<double> Fr;
    for (int i=0; i<waveIndex.size(); i++)
        Fr << (1.0 - ProbLossBinned.at(i) - ProbRefBinned.at(i) - ProbDiffBinned.at(i));

    TMultiGraph* mg = new TMultiGraph();
    TGraph* gLoss = GraphWindow->ConstructTGraph(waveIndex, ProbLossBinned, "Loss", "Wave index", "Loss", 2, 20, 1, 2);
    mg->Add(gLoss, "LP");
    TGraph* gRef = GraphWindow->ConstructTGraph(waveIndex, ProbRefBinned, "Specular reflection", "Wave index", "Reflection", 4, 21, 1, 4);
    mg->Add(gRef, "LP");
    TGraph* gDiff = GraphWindow->ConstructTGraph(waveIndex, ProbDiffBinned, "Diffuse scattering", "Wave index", "Scatter", 7, 22, 1, 7);
    mg->Add(gDiff, "LP");
    TGraph* gFr = GraphWindow->ConstructTGraph(waveIndex, Fr, "Fresnel", "Wave index", "", 1, 24, 1, 1, 1, 1);
    mg->Add(gFr, "LP");

    mg->SetMinimum(0);
    GraphWindow->Draw(mg, "apl");
    mg->GetXaxis()->SetTitle("Wave index");
    mg->GetYaxis()->SetTitle("Probability");
    GraphWindow->AddLegend(0.7,0.8, 0.95,0.95, "");
}

void SpectralBasicOpticalOverride::updateButtons()
{
    pbShow->setDisabled(Wave.isEmpty());
    bool bWR = MatCollection->IsWaveResolved();
    pbShowBinned->setDisabled(!bWR || Wave.isEmpty());
}
#endif

#ifdef GUI
QWidget *SpectralBasicOpticalOverride::getEditWidget(QWidget *caller, GraphWindowClass *GraphWindow)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QVBoxLayout* vl = new QVBoxLayout(f);
        QHBoxLayout* l = new QHBoxLayout();
            QLabel* lab = new QLabel("Absorption, reflection and scattering:");
        l->addWidget(lab);
            QPushButton* pb = new QPushButton("Load");
            pb->setToolTip("Every line of the file should contain 4 numbers:\nwavelength[nm] absorption_prob[0..1] reflection_prob[0..1] scattering_prob[0..1]");
            QObject::connect(pb, &QPushButton::clicked, [caller, this] {loadSpectralData(caller);});
        l->addWidget(pb);
            pbShow = new QPushButton("Show");
            QObject::connect(pbShow, &QPushButton::clicked, [GraphWindow, this] {showLoaded(GraphWindow);});
        l->addWidget(pbShow);
            pbShowBinned = new QPushButton("Binned");
            QObject::connect(pbShowBinned, &QPushButton::clicked, [caller, GraphWindow, this] {showBinned(caller, GraphWindow);});
        l->addWidget(pbShowBinned);
    vl->addLayout(l);
        l = new QHBoxLayout();
            lab = new QLabel("Scattering model:");
        l->addWidget(lab);
            QComboBox* com = new QComboBox();
            com->addItem("Isotropic (4Pi)"); com->addItem("Lambertian, 2Pi back"); com->addItem("Lambertian, 2Pi forward");
            com->setCurrentIndex(scatterModel);
            QObject::connect(com, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this](int index) { this->scatterModel = index; } );
        l->addWidget(com);
    vl->addLayout(l);
        l = new QHBoxLayout();
            lab = new QLabel("For photons with WaveIndex=-1, assume wavelength of:");
        l->addWidget(lab);
            QLineEdit* le = new QLineEdit(QString::number(effectiveWavelength));
            QDoubleValidator* val = new QDoubleValidator(f);
            val->setNotation(QDoubleValidator::StandardNotation);
            val->setBottom(0);
            val->setDecimals(6);
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->effectiveWavelength = le->text().toDouble(); } );
        l->addWidget(le);
            lab = new QLabel("nm");
        l->addWidget(lab);
    vl->addLayout(l);
    updateButtons();

    return f;
}
#endif

const QString SpectralBasicOpticalOverride::checkOverrideData()
{
    //checking spectrum
    if (Wave.size() == 0) return "Spectral data are not defined";
    if (Wave.size() != ProbLoss.size() || Wave.size() != ProbRef.size() || Wave.size() != ProbDiff.size()) return "Spectral data do not match in size";
    for (int i=0; i<Wave.size(); i++)
    {
        if (Wave.at(i) < 0) return "negative wavelength are not allowed";
        if (ProbLoss.at(i) < 0 || ProbLoss.at(i) > 1.0) return "absorption probability has to be in the range of [0, 1.0]";
        if (ProbDiff.at(i) < 0 || ProbDiff.at(i) > 1.0) return "scattering probability has to be in the range of [0, 1.0]";
        if (ProbRef.at(i) < 0 || ProbRef.at(i) > 1.0) return "scattering probability has to be in the range of [0, 1.0]";
        double sum = ProbLoss.at(i) + ProbRef.at(i) + ProbDiff.at(i);
        if (sum > 1.0) return QString("Sum of probabilities is larger than 1.0 for wavelength of %1 nm").arg(Wave.at(i));
    }
    if (scatterModel < 0 || scatterModel > 2) return "unknown scattering model";

    return "";
}
