#include "materialinspectorwindow.h"
#include "ui_materialinspectorwindow.h"
#include "mainwindow.h"
#include "amaterialparticlecolection.h"
#include "graphwindowclass.h"
#include "windownavigatorclass.h"
#include "detectorclass.h"
#include "aglobalsettings.h"
#include "ajsontools.h"
#include "afiletools.h"
#include "amessage.h"
#include "acommonfunctions.h"
#include "guiutils.h"
#include "ainternetbrowser.h"
#include "amatparticleconfigurator.h"
#include "achemicalelement.h"
#include "aelementandisotopedelegates.h"
#include "aneutronreactionsconfigurator.h"
#include "aneutroninfodialog.h"
#include "geometrywindowclass.h"

#include <QDebug>
#include <QLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QDesktopServices>
#include <QLabel>
#include <QIcon>
#include <QGroupBox>
#include <QStringListModel>
#include <QVariantList>
#include <QDialog>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QThread>

//Root
#include "TGraph.h"
#include "TH1.h"
#include "TAxis.h"
#include "TGeoManager.h"
#include "TAttLine.h"
#include "TAttMarker.h"

#ifdef __USE_ANTS_NCRYSTAL__
#include "NCrystal/NCrystal.hh"
#endif

MaterialInspectorWindow::MaterialInspectorWindow(QWidget * parent, MainWindow * mw, DetectorClass * detector) :
    AGuiWindow("mat", parent),
    ui(new Ui::MaterialInspectorWindow),
    MW(mw), Detector(detector), MpCollection(detector->MpCollection)
{
    ui->setupUi(this);

    this->move(15,15);

    Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
    windowFlags |= Qt::WindowCloseButtonHint;
    //windowFlags |= Qt::Tool;
    this->setWindowFlags( windowFlags );

    //SetWasModified(false);
    ui->pbWasModified->setVisible(false);
    ui->pbUpdateInteractionIndication->setVisible(false);
    ui->labContextMenuHelp->setVisible(false);
    ui->pbUpdateTmpMaterial->setVisible(false);
    ui->cobStoppingPowerUnits->setCurrentIndex(1);
    ui->tabwNeutron->verticalHeader()->setVisible(false);

    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);

    QString str = "Open XCOM page by clicking the button below.\n"
                  "Select the material composition and generate the file (use cm2/g units).\n"
                  "Copy the entire page including the header to a text file.\n"
                  "Import the file by clicking \"Import from XCOM\" button.";
    ui->pbImportXCOM->setToolTip(str);

    OptionsConfigurator = new AMatParticleConfigurator(this);

    QPixmap pm(QSize(16,16));
    pm.fill(Qt::transparent);
    QPainter b(&pm);
    b.setBrush(QBrush(Qt::yellow));
    b.drawEllipse(0, 2, 10, 10);
    ui->labAssumeZeroForEmpty->setPixmap(pm);

#ifndef __USE_ANTS_NCRYSTAL__
    ui->cbUseNCrystal->setText("Use NCrystal - library not available");
    ui->cbUseNCrystal->setToolTip("ANTS2 was compiled without support for NCrystal library!\nSee ants2.pro");
    on_cbUseNCrystal_toggled(ui->cbUseNCrystal->isChecked());
#endif
}

MaterialInspectorWindow::~MaterialInspectorWindow()
{
    if (NeutronInfoDialog)
    {
        ANeutronInfoDialog * NeutronInfoDialogCopy = NeutronInfoDialog;
        NeutronInfoDialog = nullptr;
        delete NeutronInfoDialogCopy;
    }

    delete OptionsConfigurator;
    delete ui;
}

void MaterialInspectorWindow::InitWindow()
{
    UpdateActiveMaterials();
    showMaterial(0);
}

void MaterialInspectorWindow::setWasModified(bool flag)
{
    if (flagDisreguardChange) return;

    bMaterialWasModified = flag;

    QString s = ( flag ? "<html><span style=\"color:#ff0000;\">Material was modified: Click \"Update\" or \"Add\" to save changes</span></html>"
                       : " " );
    ui->labMatWasModified->setText(s);

    updateActionButtons();
}

void MaterialInspectorWindow::UpdateActiveMaterials()
{   
    if (bLockTmpMaterial) return;

    int current = ui->cobActiveMaterials->currentIndex();

    ui->cobActiveMaterials->clear();
    ui->cobActiveMaterials->addItems(MpCollection->getListOfMaterialNames());

    int matCount = ui->cobActiveMaterials->count();
    if (current < -1 || current >= matCount)
        current = matCount - 1;

    showMaterial(current);
    setWasModified(false);
}

void MaterialInspectorWindow::on_pbAddNewMaterial_clicked()
{
    on_pbAddToActive_clicked(); //processes both update and add
}

void MaterialInspectorWindow::on_pbAddToActive_clicked()
{
    if ( !parseDecayOrRaiseTime(true) )  return;  //error messaging inside
    if ( !parseDecayOrRaiseTime(false) ) return;  //error messaging inside

    MpCollection->tmpMaterial.updateRuntimeProperties(MpCollection->fLogLogInterpolation, Detector->RandGen);

    const QString error = MpCollection->CheckTmpMaterial();
    if (!error.isEmpty())
    {
        message(error, this);
        return;
    }

    QString name = MpCollection->tmpMaterial.name;
    int index = MpCollection->FindMaterial(name);
    if (index == -1)
        index = MpCollection->countMaterials(); //then it will be appended, index = current size
    else
    {
        switch( QMessageBox::information( this, "", "Update properties for material "+name+"?", "Overwrite", "Cancel", 0, 1 ) )
        {
           case 0:  break;  //overwrite
           default: return; //cancel
        }
    }

    MpCollection->CopyTmpToMaterialCollection(); //if absent, new material is created!

    MW->ReconstructDetector(true);
    MW->UpdateMaterialListEdit(); //need?

    showMaterial(index);
}

void MaterialInspectorWindow::on_pbShowTotalInteraction_clicked()
{
    showTotalInteraction();
}

void MaterialInspectorWindow::on_pbShowStoppingPower_clicked()
{
    showTotalInteraction();
}

void MaterialInspectorWindow::SetLogLog(bool flag)
{
    ui->actionUse_log_log_interpolation->setChecked(flag);
}

void MaterialInspectorWindow::showMaterial(int index)
{
    if (index == -1)
    {
        ui->cobActiveMaterials->setCurrentIndex(-1);
        LastShownMaterial = index;
        return;
    }

    if (index < 0 || index > MpCollection->countMaterials()) return;

    MpCollection->CopyMaterialToTmp(index);
    ui->cobActiveMaterials->setCurrentIndex(index);
    LastShownMaterial = index;
    UpdateGui();

    ui->pbRename->setText("Rename " + ui->cobActiveMaterials->currentText());
    setWasModified(false);
}

void MaterialInspectorWindow::on_cobActiveMaterials_activated(int index)
{
    if (bMaterialWasModified)
    {
        int res = QMessageBox::question(this, "Explore another material", "All unsaved changes will be lost. Continue?", QMessageBox::Yes | QMessageBox::Cancel);
        if (res == QMessageBox::Cancel)
        {
            ui->cobActiveMaterials->setCurrentIndex(LastShownMaterial);
            return;
        }
    }
    showMaterial(index);
}

void MaterialInspectorWindow::updateWaveButtons()
{   
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;

    bool bPrimSpec = (tmpMaterial.PrimarySpectrum_lambda.size() > 0);
    ui->pbShowPrimSpectrum->setEnabled(bPrimSpec);
    ui->pbDeletePrimSpectrum->setEnabled(bPrimSpec);

    bool bSecSpec = (tmpMaterial.SecondarySpectrum_lambda.size() > 0);
    ui->pbShowSecSpectrum->setEnabled(bSecSpec);
    ui->pbDeleteSecSpectrum->setEnabled(bSecSpec);

    bool bN = (tmpMaterial.nWave_lambda.size() > 0);
    ui->pbShowNlambda->setEnabled(bN);
    ui->pbDeleteNlambda->setEnabled(bN);

    bool bA = (tmpMaterial.absWave_lambda.size() > 0);
    ui->pbShowABSlambda->setEnabled(bA);
    ui->pbDeleteABSlambda->setEnabled(bA);

    ui->pbShowReemProbLambda->setEnabled( !tmpMaterial.reemisProbWave_lambda.isEmpty() );
    ui->pbDeleteReemisProbLambda->setEnabled( !tmpMaterial.reemisProbWave_lambda.isEmpty() );
}

void MaterialInspectorWindow::UpdateGui()
{
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;

    ui->leName->setText(tmpMaterial.name);  

    ui->ledDensity->setText( QString::number(tmpMaterial.density) );
    ui->ledT->setText( QString::number(tmpMaterial.temperature) );

    ui->leChemicalComposition->setText( tmpMaterial.ChemicalComposition.getCompositionString() );
    ui->leCompositionByWeight->setText( tmpMaterial.ChemicalComposition.getCompositionByWeightString() );
    ShowTreeWithChemicalComposition();
    tmpMaterial.updateNeutronDataOnCompositionChange(MpCollection);

    ui->cbG4Material->setChecked(tmpMaterial.bG4UseNistMaterial);
    ui->leG4Material->setText(tmpMaterial.G4NistMaterial);

    ui->ledN->setText( QString::number(tmpMaterial.n) );
    ui->ledAbs->setText( QString::number(tmpMaterial.abs) );
    ui->ledReemissionProbability->setText( QString::number(tmpMaterial.reemissionProb) );

    QString s = ( tmpMaterial.rayleighMFP > 0 ? QString::number(tmpMaterial.rayleighMFP)
                                                : "" );
    ui->ledRayleigh->setText(s);
    ui->ledRayleighWave->setText( QString::number(tmpMaterial.rayleighWave) );

    //decay time
    if ( tmpMaterial.PriScint_Decay.isEmpty() )
        s = "0";
    else if (tmpMaterial.PriScint_Decay.size() == 1)
        s = QString::number( tmpMaterial.PriScint_Decay.first().value );
    else
    {
        s.clear();
        for (const APair_ValueAndWeight & pair : tmpMaterial.PriScint_Decay)
        {
            s += QString::number(pair.value);
            s += ":";
            s += QString::number(pair.statWeight);
            s += " & ";
        }
        s.chop(3);
    }
    ui->lePriT->setText(s);

    //rise time
    if ( tmpMaterial.PriScint_Raise.isEmpty() )
        s = "0";
    else if (tmpMaterial.PriScint_Raise.size() == 1)
        s = QString::number(tmpMaterial.PriScint_Raise.first().value);
    else
    {
        s.clear();
        for (const APair_ValueAndWeight& pair : tmpMaterial.PriScint_Raise)
        {
            s += QString::number(pair.value);
            s += ":";
            s += QString::number(pair.statWeight);
            s += " & ";
        }
        s.chop(3);
    }
    ui->lePriT_raise->setText(s);

    ui->ledW->setText( QString::number(tmpMaterial.W*1000.0) );  //keV->eV
    ui->ledSecYield->setText( QString::number(tmpMaterial.SecYield) );
    ui->ledSecT->setText( QString::number(tmpMaterial.SecScintDecayTime) );
    ui->ledEDriftVelocity->setText( QString::number(tmpMaterial.e_driftVelocity) );
    ui->ledEDiffL->setText( QString::number(tmpMaterial.e_diffusion_L) );
    ui->ledEDiffT->setText( QString::number(tmpMaterial.e_diffusion_T) );

    int tmp = LastSelectedParticle;
    ui->cobParticle->clear();    
    int lastSelected_cobYieldForParticle = ui->cobYieldForParticle->currentIndex();
    ui->cobYieldForParticle->clear();

    const QStringList PartList = MpCollection->getListOfParticleNames();
    ui->cobParticle->addItems(PartList);
    ui->cobYieldForParticle->addItems(QStringList() << PartList << "Undefined");

    int numPart = MpCollection->countParticles();
    if (lastSelected_cobYieldForParticle < 0) lastSelected_cobYieldForParticle = 0;
    if (lastSelected_cobYieldForParticle > numPart) //can be "Undefined"
        lastSelected_cobYieldForParticle = 0;
    ui->cobYieldForParticle->setCurrentIndex(lastSelected_cobYieldForParticle);
    double val = tmpMaterial.getPhotonYield(lastSelected_cobYieldForParticle);
    ui->ledPrimaryYield->setText(QString::number(val));
    val        = tmpMaterial.getIntrinsicEnergyResolution(lastSelected_cobYieldForParticle);
    ui->ledIntEnergyRes->setText(QString::number(val));

    ui->pteComments->clear();
    ui->pteComments->appendPlainText(tmpMaterial.Comments);

    QString sTags;
    for (const QString & s : tmpMaterial.Tags)
        sTags.append(s.simplified() + ", ");
    if (sTags.size() > 1) sTags.chop(2);
    ui->leTags->setText(sTags);

    LastSelectedParticle = tmp;
    if (LastSelectedParticle < numPart) ui->cobParticle->setCurrentIndex(LastSelectedParticle);
    else ui->cobParticle->setCurrentIndex(0);
    updateInteractionGui();

    updateWaveButtons();
    updateWarningIcons();
}

void MaterialInspectorWindow::updateWarningIcons()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    if (tmpMaterial.ChemicalComposition.countElements() == 0)
    {
        QPixmap pm(QSize(16,16));
        pm.fill(Qt::transparent);
        QPainter b(&pm);
        b.setBrush(QBrush(Qt::yellow));
        b.drawEllipse(0, 2, 10, 10);
        ui->twProperties->setTabIcon(0, QIcon(pm));
    }
    else ui->twProperties->setTabIcon(0, QIcon());
}

void MaterialInspectorWindow::updateEnableStatus()
{
    bool TrackingAllowed = ui->cbTrackingAllowed->isChecked();
    bool MaterialIsTransparent = ui->cbTransparentMaterial->isChecked();

    ui->cbTransparentMaterial->setEnabled(TrackingAllowed);
    ui->fEnDepProps->setEnabled(TrackingAllowed && !MaterialIsTransparent);

    ui->swMainMatParticle->setEnabled(!MaterialIsTransparent); // need?

    QFont font = ui->cbTransparentMaterial->font();
    font.setBold(MaterialIsTransparent);
    ui->cbTransparentMaterial->setFont(font);
}

void MaterialInspectorWindow::on_pbUpdateInteractionIndication_clicked()
{
    updateInteractionGui();
}

void MaterialInspectorWindow::updateInteractionGui()
{
    int particleId = ui->cobParticle->currentIndex();

    if (particleId < 0 || particleId >= MpCollection->countParticles())
        particleId = MpCollection->countParticles() - 1;

    ui->tabInteraction->setEnabled(particleId != -1);
    if (particleId == -1) return;
    ui->cobParticle->setCurrentIndex(particleId);

    flagDisreguardChange = true; // -->

    const AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    const MatParticleStructure & mp = tmpMaterial.MatParticle.at(particleId);
    const AParticle::ParticleType type = MpCollection->getParticleType(particleId);

    ui->cbTrackingAllowed->setChecked(mp.TrackingAllowed);
    ui->cbTransparentMaterial->setChecked(mp.MaterialIsTransparent);
    updateEnableStatus();

    LastSelectedParticle = particleId;

    ui->pbShowTotalInteraction->setEnabled(true);

    if (type == AParticle::_charged_)
    {
        ui->swMainMatParticle->setCurrentIndex(0);
        if (mp.InteractionDataX.size()>0) ui->pbShowStoppingPower->setEnabled(true);
        else ui->pbShowStoppingPower->setEnabled(false);
    }
    else
    {
        ui->swMainMatParticle->setCurrentIndex(1);

        if (type == AParticle::_neutron_)
        {
            ui->swNeutral->setCurrentIndex(1);
            ui->cbCapture->setChecked(mp.bCaptureEnabled);
            ui->cbEnableScatter->setChecked(mp.bElasticEnabled);
            ui->cbAllowAbsentCsData->setChecked(mp.bAllowAbsentCsData);

            FillNeutronTable();

            ui->cbUseNCrystal->setChecked( mp.bUseNCrystal );
            const NeutralTerminatorStructure& t = mp.Terminators.last();
            ui->ledNCmatDcutoff->setText( QString::number( t.NCrystal_Dcutoff ) );
            ui->ledNcmatPacking->setText( QString::number( t.NCrystal_Packing ) );
            bool bHaveData = !t.NCrystal_Ncmat.isEmpty();
            ui->labNCmatNotDefined->setVisible(!bHaveData);
            ui->pbShowNcmat->setVisible(bHaveData);
        }
        else if (type == AParticle::_gamma_)
        {
            ui->swNeutral->setCurrentIndex(0);
            ui->fGamma->setEnabled( mp.Terminators.size() != 0 );
            ui->frGamma1->setEnabled( mp.Terminators.size() != 0 );
            ui->cbPairProduction->setChecked( mp.Terminators.size()>2 );
            ui->pbShowTotalInteraction->setEnabled( mp.InteractionDataX.size()>0 );
            ui->pbShowXCOMdata->setEnabled( !mp.DataSource.isEmpty() );
            on_ledGammaDiagnosticsEnergy_editingFinished();
        }
        else
        {
            message("Critical error: unknown neutral particle type", this);
        }
    }

    flagDisreguardChange = false; // <--
}

void MaterialInspectorWindow::on_pbUpdateTmpMaterial_clicked()
{  
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;

    const int ParticleId = ui->cobParticle->currentIndex();
    if (ParticleId < 0 || ParticleId >= MpCollection->countParticles()) return; //transient update

    tmpMaterial.name = ui->leName->text();
    tmpMaterial.density = ui->ledDensity->text().toDouble();    
    tmpMaterial.temperature = ui->ledT->text().toDouble();
    tmpMaterial.n = ui->ledN->text().toDouble();
    tmpMaterial.abs = ui->ledAbs->text().toDouble();
    tmpMaterial.reemissionProb = ui->ledReemissionProbability->text().toDouble();

    double prYield = ui->ledPrimaryYield->text().toDouble();
    int iP = ui->cobYieldForParticle->currentIndex();
    if (iP > -1 && iP < tmpMaterial.MatParticle.size()) tmpMaterial.MatParticle[iP].PhYield = prYield;
    else tmpMaterial.PhotonYieldDefault = prYield;

    tmpMaterial.MatParticle[ParticleId].TrackingAllowed = ui->cbTrackingAllowed->isChecked();
    tmpMaterial.MatParticle[ParticleId].MaterialIsTransparent = ui->cbTransparentMaterial->isChecked();
    tmpMaterial.MatParticle[ParticleId].bCaptureEnabled = ui->cbCapture->isChecked();
    tmpMaterial.MatParticle[ParticleId].bElasticEnabled = ui->cbEnableScatter->isChecked();

    tmpMaterial.W = ui->ledW->text().toDouble()*0.001; //eV -> keV
    tmpMaterial.SecYield = ui->ledSecYield->text().toDouble();
    tmpMaterial.SecScintDecayTime = ui->ledSecT->text().toDouble();   
    tmpMaterial.e_driftVelocity = ui->ledEDriftVelocity->text().toDouble();

    tmpMaterial.e_diffusion_L = ui->ledEDiffL->text().toDouble();
    tmpMaterial.e_diffusion_T = ui->ledEDiffT->text().toDouble();

    tmpMaterial.Comments = ui->pteComments->document()->toPlainText();

    QStringList slTags = ui->leTags->text().split(',', QString::SkipEmptyParts);
    tmpMaterial.Tags.clear();
    for (const QString & s : slTags)
        tmpMaterial.Tags << s.simplified();

    tmpMaterial.bG4UseNistMaterial = ui->cbG4Material->isChecked();
    tmpMaterial.G4NistMaterial = ui->leG4Material->text();

    on_ledGammaDiagnosticsEnergy_editingFinished(); //gamma - update MFP
}

void MaterialInspectorWindow::on_pbLoadDeDr_clicked()
{
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Load dE/dr data", MW->GlobSet.LastOpenDir, "Data files (*.dat)");

  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  AMaterial& tmpMaterial = MpCollection->tmpMaterial;

  QFile file(fileName);

  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      QMessageBox::information(0, "error", file.errorString());
      return;
    }

  QTextStream in(&file);

  int particleId = ui->cobParticle->currentIndex();
  tmpMaterial.MatParticle[particleId].InteractionDataX.resize(0);
  tmpMaterial.MatParticle[particleId].InteractionDataF.resize(0);

  double Multiplier;
  switch (ui->cobStoppingPowerUnits->currentIndex())
    {
    case (0): {Multiplier = 0.001; break;}  //eV  -> keV
    case (1): {Multiplier = 1.0; break;}    //keV -> keV
    case (2): {Multiplier = 1000.0; break;} //MeV -> keV
    default: {Multiplier = 1.0;}    //just to avoid warning
    }

  //separators
  QRegExp rx("(\\ |\\,|\\:|\\t)"); //RegEx for ' ' or ',' or ':' or '\t'

  while(!in.atEnd())
    {
      QString line = in.readLine();
      QStringList fields = line.split(rx);

      if (fields.size() != 2) continue;

      bool bOK;
      double x = fields[0].toDouble(&bOK);
      if (!bOK) continue;
      double f = fields[1].toDouble(&bOK);
      if (!bOK) continue;

      tmpMaterial.MatParticle[particleId].InteractionDataX.append(x * Multiplier);
      tmpMaterial.MatParticle[particleId].InteractionDataF.append(f);
    }
  file.close();
  ui->pbShowStoppingPower->setEnabled(true);
  updateInteractionGui();
  setWasModified(true);
}

void MaterialInspectorWindow::SetParticleSelection(int index)
{
    const QStringList PartList = MpCollection->getListOfParticleNames();

    if (ui->cobParticle->count() == 0)
        ui->cobParticle->addItems(PartList);

    if (index < 0 || index >= PartList.size() ) index = 0;
    ui->cobParticle->setCurrentIndex(index);

    updateInteractionGui();
}

void MaterialInspectorWindow::SetMaterial(int index)
{
    showMaterial(index);
}

void MaterialInspectorWindow::on_pbLoadThisScenarioCrossSection_clicked()
{
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, "Load mass interaction coefficient data.\n"
                                            "The file should contain 4 colums: energy[keV], photoelectric_data[cm2/g], compton_data[cm2/g], pair_production_data[cm2/g]", MW->GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files(*)");

    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    int particleId = ui->cobParticle->currentIndex();

    QVector<double> en, phot, compt, pair;
    QVector< QVector<double>* > V;
    V << &en << &phot << &compt << &pair;
    const QString error = LoadDoubleVectorsFromFile(fileName, V);
    if (!error.isEmpty())
    {
        message(error, this);
        return;
    }

    tmpMaterial.MatParticle[particleId].Terminators.resize(3); //phot+compt+pair

    tmpMaterial.MatParticle[particleId].Terminators[0].PartialCrossSectionEnergy = en;
    tmpMaterial.MatParticle[particleId].Terminators[0].PartialCrossSection = phot;
    tmpMaterial.MatParticle[particleId].Terminators[0].Type = NeutralTerminatorStructure::Photoelectric;
    tmpMaterial.MatParticle[particleId].Terminators[1].PartialCrossSectionEnergy = en;
    tmpMaterial.MatParticle[particleId].Terminators[1].PartialCrossSection = compt;
    tmpMaterial.MatParticle[particleId].Terminators[1].Type = NeutralTerminatorStructure::ComptonScattering;
    tmpMaterial.MatParticle[particleId].Terminators[1].PartialCrossSectionEnergy = en;
    tmpMaterial.MatParticle[particleId].Terminators[1].PartialCrossSection = pair;
    tmpMaterial.MatParticle[particleId].Terminators[1].Type = NeutralTerminatorStructure::PairProduction;

    //calculating total interaction data
    tmpMaterial.MatParticle[particleId].CalculateTotalForGamma();

    //clear XCOM data if were defined
    tmpMaterial.MatParticle[particleId].DataSource.clear();
    tmpMaterial.MatParticle[particleId].DataString.clear();

    updateInteractionGui();
    setWasModified(true);
}

void MaterialInspectorWindow::on_ledIntEnergyRes_editingFinished()
{
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    double newVal = ui->ledIntEnergyRes->text().toDouble();
    if (newVal < 0)
    {
        ui->ledIntEnergyRes->setText("0");
        message("Intrinsic energy resolution cannot be negative", this);
        return;
    }

    int iP = ui->cobYieldForParticle->currentIndex();
    if (iP > -1 && iP < tmpMaterial.MatParticle.size())
        tmpMaterial.MatParticle[iP].IntrEnergyRes = newVal;
    else
        tmpMaterial.IntrEnResDefault = newVal;

    setWasModified(true);
}

void MaterialInspectorWindow::on_pbImportStoppingPowerFromTrim_clicked()
{
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, "Import stopping power data from a Trim file", MW->GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
    //qDebug()<<fileName;
    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    int particleId = ui->cobParticle->currentIndex();
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    tmpMaterial.MatParticle[particleId].InteractionDataX.resize(0);
    tmpMaterial.MatParticle[particleId].InteractionDataF.resize(0);

    QFile file(fileName);

        if(!file.open(QIODevice::ReadOnly | QFile::Text))
        {
            QMessageBox::information(0, "error", file.errorString());
            return;
        }

        QTextStream in(&file);
        //separators
        QRegExp rx("(\\ |\\,|\\:|\\(|\\)|\\t)"); //RegEx for ' ' or ',' or ':' or '\t'
        QStringList fields;

        // looking for the line with e.g.     Target Density =  2.5000E+00 g/cm3 = 1.3623E+23 atoms/cm3
        bool flagFound = false;
        while(!in.atEnd())
        {
            QString line = in.readLine();
            fields = line.split(rx, QString::SkipEmptyParts);
            if (fields.isEmpty()) continue;
//            qDebug()<<fields[0];
            if (!fields[0].compare("Target")) {flagFound = true; break;}
        }

        if (flagFound) ;//qDebug()<<"found!";
        else
        {
            message("File format not recognized!");
            file.close();
            return;
        }

        //now have the target density
        qDebug()<<"Target density: "<<fields[3]<<fields[4]; //[3] - density  [4] - units
        double TrimDensity = fields[3].toDouble(); //*** assuming its always g/cm3

        message("Setting material density to one used in Trim!");
        tmpMaterial.density = TrimDensity;
        QString str;
        str.setNum(TrimDensity);
        ui->ledDensity->setText(str);

        // looking for the line with e.g.     Stopping Units =  keV / (mg/cm2)
        while(!in.atEnd())
        {
            QString line = in.readLine();
            fields = line.split(rx, QString::SkipEmptyParts);
            if (fields.isEmpty()) continue;
//            qDebug()<<fields[0];
            if (!fields[0].compare("Stopping")) {flagFound = true; break;}
        }

        if (flagFound) ;//qDebug()<<"found!";
        else
        {        
            message("File format not recognized!");
            file.close();
            return;
        }

        //now have the stopping power units
        qDebug()<<"Stopping power units: "<<fields[3]<<fields[5]; //[3] - keV  [5] - mg/cm2
        double StoppingMultiplier;
        if (!fields[3].compare("eV")) StoppingMultiplier = 0.001;
        else if (!fields[3].compare("keV")) StoppingMultiplier = 1.0;
        else if (!fields[3].compare("MeV")) StoppingMultiplier = 1000.0;
        else
        {        
            message("Stopping power units not recognized!");
            file.close();
            return;
        }

        //maybe we have already density-corrected?
        if (!fields[5].compare("mm") || !fields[5].compare("micron") || !fields[5].compare("Angstrom"))
        { //Linear stopping power (dE/dx)
            StoppingMultiplier = StoppingMultiplier / TrimDensity *10.0; // in the body : dE/dx = TabValue * Density  //10.0 to take care of cm->mm (cm3/g -> mm cm2/g)

            if (!fields[5].compare("mm"));
            else if (!fields[5].compare("micron")) StoppingMultiplier *= 1.0e3; //divide by!
            else if (!fields[5].compare("Angstrom")) StoppingMultiplier *= 1.0e7; //divide by!
        }
        else
        { //Mass stopping power (dE / (g/cm2))

            if (!fields[5].compare("g/cm2"));
            else if (!fields[5].compare("mg/cm2")) StoppingMultiplier *= 1.0e3; //divide by!
            else if (!fields[5].compare("ug/cm2")) StoppingMultiplier *= 1.0e6;
            else
            {             
                message("Stopping power units not recognized!");
                file.close();
                return;
            }
        }
        qDebug()<<"StoppingMultiplier="<<StoppingMultiplier;

        //skipping the rest of the header
        flagFound=false;
        while(!in.atEnd())
        {
            QString line = in.readLine();
            fields = line.split(rx, QString::SkipEmptyParts);
            if (fields.isEmpty()) continue;
//            qDebug()<<fields[0];
            if (!fields[0].compare("-----------")) {flagFound = true; break;}
        }
        if (flagFound) qDebug()<<"Found beginning of the data block!";
        else
        {         
            message("File format not recognized!");
            file.close();
            return;
        }

        //reading the data
        bool flagWrongTermination = true;
        while(!in.atEnd())
        {
            QString line = in.readLine();
//            qDebug()<<"line:"<<line;
            fields = line.split(rx, QString::SkipEmptyParts);

            if (fields.isEmpty()) break; //something is wrong

 //           qDebug()<<fields[0];
            if (!fields[0].compare("-----------------------------------------------------------")) {flagWrongTermination = false; break;} //ending OK!

            if (fields.size() < 4) break; //something is wrong

            double Multiplier;
            QString mstr = fields[1];
            if (!mstr.compare("eV")) Multiplier = 0.001;
            else if (!mstr.compare("keV")) Multiplier = 1.0;
            else if (!mstr.compare("MeV")) Multiplier = 1000.0;
            else
            {          
               message("Unknown energy unit!");
               file.close();
               return;
            }
           // qDebug()<<"multiplier: "<<Multiplier;

            double x = fields[0].toDouble()*Multiplier;
            double f = (fields[2].toDouble() + fields[3].toDouble()) * StoppingMultiplier;

//            qDebug()<<x<<f;

            //assigning the data
            tmpMaterial.MatParticle[particleId].InteractionDataX.append(x);
            tmpMaterial.MatParticle[particleId].InteractionDataF.append(f);

            setWasModified(true);
        }

        if (flagWrongTermination)
        {        
            message("Unexpected termination!");
            return;
        }

        file.close();
        ui->pbShowStoppingPower->setEnabled(true);
        updateInteractionGui();
}

void MaterialInspectorWindow::on_pbImportXCOM_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Import partial interaction coefficients from an XCOM file", MW->GlobSet.LastOpenDir, "Text and data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
        message("Cannot open "+fileName, this);
        return;
    }
    QTextStream in(&file);

    int particleId = ui->cobParticle->currentIndex();
    importXCOM(in, particleId);
    updateInteractionGui();
    setWasModified(true);
}

bool MaterialInspectorWindow::importXCOM(QTextStream &in, int particleId)
{
  if (MpCollection->getParticleType(particleId) != AParticle::_gamma_)
    {
      message("It is not gamma particle!", this);
      return false;
    }

  AMaterial& tmpMaterial = MpCollection->tmpMaterial;

  QVector<double> tmpEnergy, tmpCompton, tmpPhoto, tmpPair;
  bool flagError = false;
  QRegExp rx("(\\ |\\,|\\:|\\(|\\)|\\t)"); //RegEx for separators: ' ' or ',' or ':' or '(' or ')' or '\t'

  //reading the data
  //header blocks can be between the data block
  bool flagHeaderBlock = true;

  QStringList fields;
  while(!in.atEnd())
    {
      QString line = in.readLine();

      if (flagHeaderBlock)
        { //if header block
          // looking for the line containing "MeV"
          fields = line.split(rx, QString::SkipEmptyParts);
          if (fields.isEmpty()) continue;
          //                qDebug()<<"Assuming its header, first string:"<<fields[0];
          if (fields[0] == "MeV")
            {
              flagHeaderBlock = false;
              //                    qDebug()<<"-->Header finished!<--";
              continue; //jump to data section
            }
          //end header block
        }
      else
        { //data block
          //                qDebug()<<"->assuming it is data line:"<<line;
          if (line.isEmpty() || line == " ")
            {
              //                    qDebug()<<"Empty line detected, skip"; //could be line between data paragraphs - shells
              continue;
            }
          line.remove(0,7); //removing frst characters where shell info can be
          fields = line.split(rx, QString::SkipEmptyParts);
          if (fields.size()<8)
            {
              //expecting 8 doubles!
              //can be another header
              //                    qDebug()<<"Short str found - assuming another header starts here";
              flagHeaderBlock = true;
              continue;
            }

          bool ok1, ok2, ok3, ok4, ok5;
          double Column0 = fields[0].toDouble(&ok1);  //Energy in MeV
          double Column2 = fields[2].toDouble(&ok2);  //Incoherent scatter
          double Column3 = fields[3].toDouble(&ok3);  //Photoelectric
          double Column4 = fields[4].toDouble(&ok4);  //Pair in nuclear field
          double Column5 = fields[5].toDouble(&ok5);  //Pair in electron field
          if (!ok1 || !ok2 || !ok3 || !ok4 || !ok5)
            {
              //wrong format
              flagError = true;
              break;
            }
          tmpEnergy.append(Column0*1.0e3);  //MeV -> keV
          tmpCompton.append(Column2);
          tmpPhoto.append(Column3);
          tmpPair.append(Column4 + Column5);
        } //end data block
    }

  if (tmpEnergy.isEmpty()) flagError = true;
  if (flagError)
    {
      message("Unexpected format of the XCOM record!", this);
      return false;
    }

  //     qDebug()<<"-->check: shell lines...";
  //checking for shell lines - leave the first value, all from 2nd to nth - averaging
  //result: two point, first having the value "before line", second having averaged over all lines
  double xPrevious = 0;
  int SameValueTimes = 0;
  QVector<double> Energy, Compton, Photo, Pair;

  double AccuCompt=0, AccuPhoto=0, AccuPair=0;
  for (int i=0; i<tmpEnergy.size(); i++)
    {
      if (tmpEnergy[i] == xPrevious)
        {
          AccuCompt += tmpCompton[i];
          AccuPhoto += tmpPhoto[i];
          AccuPair  += tmpPair[i];
          SameValueTimes++;
          continue; //not copying to the output vectors, waiting for the change!
        }
      else
        {
          if (SameValueTimes == 0)
            {
              //there was no overlap, normal operation
            }
          else
            {
              //there was overlap!
              //                 qDebug()<<"->same value appeared times:"<<SameValueTimes;
              Energy.append(xPrevious);
              Compton.append(AccuCompt / (double)SameValueTimes);
              Photo.append(AccuPhoto / (double)SameValueTimes);
              Pair.append(AccuPair / (double)SameValueTimes);
              AccuCompt = 0;
              AccuPhoto = 0;
              AccuPair = 0;
              SameValueTimes = 0;
              //now appending the new value
            }
          xPrevious = tmpEnergy[i];
        }
      //normal
      Energy.append(tmpEnergy[i]);
      Compton.append(tmpCompton[i]);
      Photo.append(tmpPhoto[i]);
      Pair.append(tmpPair[i]);
    }

  //     qDebug()<<"-->shell lines check completed";
  //     for (int i=0; i<xx.size(); i++) qDebug()<<xx[i]<<f1[i]<<f2[i];
  //     qDebug()<<"\n-->populating tmpMaterial";

  //assigning data to the tmpMaterial
  tmpMaterial.MatParticle[particleId].InteractionDataF.resize(0);
  tmpMaterial.MatParticle[particleId].InteractionDataX.resize(0);
  tmpMaterial.MatParticle[particleId].Terminators.resize(0);

  QVector<NeutralTerminatorStructure> Terminators;
  //preparing termination scenarios
  NeutralTerminatorStructure terminator;
  //photoelectric
  terminator.Type = NeutralTerminatorStructure::Photoelectric; //0
  terminator.PartialCrossSectionEnergy = Energy;
  terminator.PartialCrossSection = Photo;
  Terminators.append(terminator);
  //compton
  terminator.Type = NeutralTerminatorStructure::ComptonScattering; //1
  terminator.PartialCrossSection = Compton;
  Terminators.append(terminator);
  //pair
  terminator.Type = NeutralTerminatorStructure::PairProduction; //2
  terminator.PartialCrossSection = Pair;
  Terminators.append(terminator);

  //     for (int i=0; i<xx.size(); i++) qDebug()<<xx[i]<<ff1[i]<<ff2[i];
  tmpMaterial.MatParticle[particleId].Terminators = Terminators;

  in.seek(0);
  tmpMaterial.MatParticle[particleId].DataSource = in.readAll();

  //calculating total interaction cross-section
  bool TotCalc =  tmpMaterial.MatParticle[particleId].CalculateTotalForGamma();
  if (!TotCalc) qCritical()<<"ERROR in calculation of the total interaction coefficient!";

  //      qDebug()<<"-->tmpMaterial terminators updated and total interaction calculated";
  //      qDebug()<<tmpMaterial.MatParticle[particleId].Terminators.size();
  //      qDebug()<<tmpMaterial.MatParticle[particleId].InteractionDataX.size();

  setWasModified(true);
  return true;
}

void MaterialInspectorWindow::on_pbLoadPrimSpectrum_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load primary scintillation spectrum", MW->GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*.*)");
    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    AMaterial & tmpMaterial = MpCollection->tmpMaterial;

    LoadDoubleVectorsFromFile(fileName, &tmpMaterial.PrimarySpectrum_lambda, &tmpMaterial.PrimarySpectrum);  //cleans previous data

    bool bHaveData = !tmpMaterial.PrimarySpectrum_lambda.isEmpty();
    ui->pbShowPrimSpectrum->setEnabled(bHaveData);
    ui->pbDeletePrimSpectrum->setEnabled(bHaveData);
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbShowPrimSpectrum_clicked()
{
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    TGraph * g = MW->GraphWindow->ConstructTGraph(tmpMaterial.PrimarySpectrum_lambda, tmpMaterial.PrimarySpectrum);
    MW->GraphWindow->configureGraph(g, "Emission spectrum",
                                    "Wavelength, nm", "Emission probability, a.u.",
                                    2, 20, 1,
                                    2, 1,  1);
    MW->GraphWindow->Draw(g, "APL");
}

void MaterialInspectorWindow::on_pbDeletePrimSpectrum_clicked()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    tmpMaterial.PrimarySpectrum_lambda.clear();
    tmpMaterial.PrimarySpectrum.clear();

    ui->pbShowPrimSpectrum->setEnabled(false);
    ui->pbDeletePrimSpectrum->setEnabled(false);
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbLoadSecSpectrum_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load secondary scintillation spectrum", MW->GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*.*)");
    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    LoadDoubleVectorsFromFile(fileName, &tmpMaterial.SecondarySpectrum_lambda, &tmpMaterial.SecondarySpectrum);  //cleans previous data

    bool bHaveData = !tmpMaterial.SecondarySpectrum_lambda.isEmpty();
    ui->pbShowSecSpectrum->setEnabled(bHaveData);
    ui->pbDeleteSecSpectrum->setEnabled(bHaveData);
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbShowSecSpectrum_clicked()
{
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    TGraph * g = MW->GraphWindow->ConstructTGraph(tmpMaterial.SecondarySpectrum_lambda, tmpMaterial.SecondarySpectrum);
    MW->GraphWindow->configureGraph(g, "Emission spectrum",
                                    "Wavelength, nm", "Emission probability, a.u.",
                                    2, 20, 1,
                                    2, 1,  1);
    MW->GraphWindow->Draw(g, "APL");
}

void MaterialInspectorWindow::on_pbDeleteSecSpectrum_clicked()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    tmpMaterial.SecondarySpectrum_lambda.clear();
    tmpMaterial.SecondarySpectrum.clear();

    ui->pbShowSecSpectrum->setEnabled(false);
    ui->pbDeleteSecSpectrum->setEnabled(false);
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbLoadNlambda_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load refractive index data", MW->GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*.*)");
    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;

    LoadDoubleVectorsFromFile(fileName, &tmpMaterial.nWave_lambda, &tmpMaterial.nWave);  //cleans previous data too

    bool bHaveData = !tmpMaterial.nWave_lambda.isEmpty();
    ui->pbShowNlambda->setEnabled(bHaveData);
    ui->pbDeleteNlambda->setEnabled(bHaveData);
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbShowNlambda_clicked()
{
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    TGraph * g = MW->GraphWindow->ConstructTGraph(tmpMaterial.nWave_lambda, tmpMaterial.nWave);
    MW->GraphWindow->configureGraph(g, "Refractive index",
                                    "Wavelength, nm", "Refractive index",
                                    2, 20, 1,
                                    2, 1,  1);
    MW->GraphWindow->Draw(g, "APL");
}

void MaterialInspectorWindow::on_pbDeleteNlambda_clicked()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    tmpMaterial.nWave_lambda.clear();
    tmpMaterial.nWave.clear();

    ui->pbShowNlambda->setEnabled(false);
    ui->pbDeleteNlambda->setEnabled(false);

    setWasModified(true);
}

void MaterialInspectorWindow::on_pbLoadABSlambda_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load exponential bulk absorption data", MW->GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*.*)");
    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    LoadDoubleVectorsFromFile(fileName, &tmpMaterial.absWave_lambda, &tmpMaterial.absWave);  //cleans previous data too

    bool bHaveData = !tmpMaterial.absWave_lambda.isEmpty();
    ui->pbShowABSlambda->setEnabled(bHaveData);
    ui->pbDeleteABSlambda->setEnabled(bHaveData);
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbShowABSlambda_clicked()
{
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    TGraph * g = MW->GraphWindow->ConstructTGraph(tmpMaterial.absWave_lambda, tmpMaterial.absWave);
    MW->GraphWindow->configureGraph(g, "Attenuation coefficient",
                                    "Wavelength, nm", "Attenuation coefficient, mm^{-1}",
                                    2, 20, 1,
                                    2, 1,  1);
    MW->GraphWindow->Draw(g, "APL");
}

void MaterialInspectorWindow::on_pbDeleteABSlambda_clicked()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    tmpMaterial.absWave_lambda.clear();
    tmpMaterial.absWave.clear();

    ui->pbShowABSlambda->setEnabled(false);
    ui->pbDeleteABSlambda->setEnabled(false);
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbShowReemProbLambda_clicked()
{
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    TGraph * g = MW->GraphWindow->ConstructTGraph(tmpMaterial.reemisProbWave_lambda, tmpMaterial.reemisProbWave);
    MW->GraphWindow->configureGraph(g, "Reemission probability",
                                    "Wavelength, nm", "Reemission probability",
                                    2, 20, 1,
                                    2, 1,  1);
    MW->GraphWindow->Draw(g, "APL");
}

void MaterialInspectorWindow::on_pbLoadReemisProbLambda_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load reemission probability vs wavelength", MW->GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*.*)");
    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;

    LoadDoubleVectorsFromFile(fileName, &tmpMaterial.reemisProbWave_lambda, &tmpMaterial.reemisProbWave);  //cleans previous data too

    bool bHaveData = !tmpMaterial.reemisProbWave_lambda.isEmpty();
    ui->pbShowReemProbLambda->setEnabled(bHaveData);
    ui->pbDeleteReemisProbLambda->setEnabled(bHaveData);
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbDeleteReemisProbLambda_clicked()
{
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    tmpMaterial.reemisProbWave_lambda.clear();
    tmpMaterial.reemisProbWave.clear();

    ui->pbShowReemProbLambda->setEnabled(false);
    ui->pbDeleteReemisProbLambda->setEnabled(false);
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbWasModified_clicked()
{
    setWasModified(true);
}

bool MaterialInspectorWindow::event(QEvent * e)
{
    if (e->type() == QEvent::Hide)
        if (OptionsConfigurator->isVisible()) OptionsConfigurator->hide();

    return AGuiWindow::event(e);
}

void MaterialInspectorWindow::on_pbGammaDiagnosticsPhComptRation_clicked()
{
    int particleId = ui->cobParticle->currentIndex();
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;

    if (tmpMaterial.MatParticle[particleId].Terminators.size() < 2) return;

    int elements0 = tmpMaterial.MatParticle[particleId].Terminators[0].PartialCrossSection.size();
    int elements1 = tmpMaterial.MatParticle[particleId].Terminators[1].PartialCrossSection.size();
    if (elements0 == 0 && elements1 == 0) return;

    if (elements0 != elements1)
    {
        message("Error: mismatch in number of entries!");
        return;
    }

    QVector<double> & X = tmpMaterial.MatParticle[particleId].Terminators[0].PartialCrossSectionEnergy;
    QVector<double> Y;
    for (int i = 0; i < elements0; i++)
    {
        double sum = tmpMaterial.MatParticle[particleId].Terminators[0].PartialCrossSection[i] + tmpMaterial.MatParticle[particleId].Terminators[1].PartialCrossSection[i];
        if (sum == 0) Y.append(0);
        else Y.append(tmpMaterial.MatParticle[particleId].Terminators[0].PartialCrossSection[i] / sum);
    }

    TGraph * g = MW->GraphWindow->ConstructTGraph(X, Y, "Photoelectric ratio", "Energy, keV", "Fraction photo/(photo+compton)", kBlue, 20, 1, kBlue);
    MW->GraphWindow->Draw(g, "AL");
    MW->GraphWindow->RedrawAll();
}

void MaterialInspectorWindow::on_leName_editingFinished()
{
  QString name = ui->leName->text();
  name.replace("+","_");
  name.replace(".","_");
  ui->leName->setText(name);

  if ( (name.at(0) >= QChar('a') && name.at(0) <= QChar('z')) || (name.at(0) >= QChar('A') && name.at(0) <= QChar('Z'))) ;//ok
  else
    {
      name.remove(0,1);
      name.insert(0, "A");
      ui->leName->setText(name);
      message("Name should start with an alphabetic character!", this);
      return;
    }
  on_pbUpdateTmpMaterial_clicked();
}

void MaterialInspectorWindow::on_leName_textChanged(const QString& /*name*/)
{
    //on text change - on chage this is a signal that it will be another material. These properties are recalculated anyway on
    //accepting changes/new material
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    tmpMaterial.absWaveBinned.clear();
    tmpMaterial.reemissionProbBinned.clear();
    tmpMaterial.nWaveBinned.clear();
    tmpMaterial.GeoMat = nullptr;  //do not delete! the original material has to have them
    tmpMaterial.GeoMed = nullptr;
    tmpMaterial.PrimarySpectrumHist = nullptr;
    tmpMaterial.SecondarySpectrumHist = nullptr;

    updateActionButtons();
}

void MaterialInspectorWindow::updateActionButtons()
{
    const QString name = ui->leName->text();
    int iMat = MpCollection->FindMaterial(name);
    if (iMat == -1)
    {
        // Material with this name does not exist
        ui->pbAddToActive->setEnabled(false);  //update button
        ui->pbAddNewMaterial->setEnabled(true);
        ui->pbRename->setEnabled(!bMaterialWasModified);
    }
    else
    {
        // Material with this name exists
        ui->pbAddToActive->setEnabled( ui->cobActiveMaterials->currentText() == name ); //update button
        ui->pbAddNewMaterial->setEnabled(false);
        ui->pbRename->setEnabled(false);
    }
}

void MaterialInspectorWindow::on_ledGammaDiagnosticsEnergy_editingFinished()
{
  int particleId = ui->cobParticle->currentIndex();
  if (MpCollection->getParticleType(particleId) != AParticle::_gamma_) return;

  ui->leoGammaDiagnosticsCoefficient->setText("n.a.");
  ui->leoMFPgamma->setText("n.a.");

  AMaterial& tmpMaterial = MpCollection->tmpMaterial;

  bool ok;
  double energy = ui->ledGammaDiagnosticsEnergy->text().toDouble(&ok);
  if (!ok) return;

  int DataPoints = tmpMaterial.MatParticle[particleId].InteractionDataX.size();  
  if (DataPoints == 0) return;
  if (energy<tmpMaterial.MatParticle[particleId].InteractionDataX[0] || energy>tmpMaterial.MatParticle[particleId].InteractionDataX[DataPoints-1])  return;

  QString str, str1;
  //double coefficient = InteractionValue(energy, &tmpMaterial.MatParticle[particleId].Terminators[TermScenario].PartialCrossSectionEnergy, &tmpMaterial.MatParticle[particleId].Terminators[TermScenario].PartialCrossSection, LogLogInterpolation);
  //str.setNum(coefficient, 'g', 4);

  double Density = tmpMaterial.density;
  int LogLogInterpolation = MpCollection->fLogLogInterpolation;
  double InteractionCoefficient = GetInterpolatedValue(energy, &tmpMaterial.MatParticle[particleId].InteractionDataX, &tmpMaterial.MatParticle[particleId].InteractionDataF, LogLogInterpolation);
  //qDebug()<<energy<<InteractionCoefficient;
  str.setNum(InteractionCoefficient, 'g', 4);

  double MeanFreePath = 10.0/InteractionCoefficient/Density;  //1/(cm2/g)/(g/cm3) - need in mm (so that 10.)
  str1.setNum(MeanFreePath, 'g', 4);

  ui->leoGammaDiagnosticsCoefficient->setText(str);
  ui->leoMFPgamma->setText(str1);
}

void MaterialInspectorWindow::on_ledRayleighWave_editingFinished()
{
    double wave = ui->ledRayleighWave->text().toDouble();
    if (wave <= 0)
    {
        message("Wavelength should be a positive number!", this);
        wave = 500.0;
        ui->ledRayleighWave->setText("500");
    }

    MpCollection->tmpMaterial.rayleighWave = wave;
}

void MaterialInspectorWindow::on_ledRayleigh_textChanged(const QString &arg1)
{
    if (arg1 == "") ui->ledRayleighWave->setEnabled(false);
    else ui->ledRayleighWave->setEnabled(true);
}

void MaterialInspectorWindow::on_ledRayleigh_editingFinished()
{
    double ray;
    if (ui->ledRayleigh->text() == "") ray = 0;
    else ray = ui->ledRayleigh->text().toDouble();
    MpCollection->tmpMaterial.rayleighMFP = ray;
}

void MaterialInspectorWindow::on_pbRemoveRayleigh_clicked()
{
    ui->ledRayleigh->setText("");
    MpCollection->tmpMaterial.rayleighMFP = 0;
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbShowUsage_clicked()
{
  AMaterial& tmpMaterial = MpCollection->tmpMaterial;
  QString name = tmpMaterial.name;
  int index = ui->cobActiveMaterials->currentIndex();

  bool flagFound = false;
  TObjArray* list = Detector->GeoManager->GetListOfVolumes();
  int size = list->GetEntries();
  for (int i=0; i<size; i++)
    {
      TGeoVolume* vol = (TGeoVolume*)list->At(i);
      if (!vol) break;
      if (index == vol->GetMaterial()->GetIndex())
        {
          flagFound = true;
          break;
        }
    }

  MW->Detector->GeoManager->ClearTracks();
  MW->GeometryWindow->ClearGeoMarkers();
  if (flagFound)
    {
      Detector->colorVolumes(2, index);
      MW->GeometryWindow->ShowGeometry(true, true, false);
    }
  else
    {
      MW->GeometryWindow->ShowGeometry(false);
      message("Current detector configuration does not have objects referring to material "+name, this);
    }
}

void MaterialInspectorWindow::on_pbNistPage_clicked()
{
    QDesktopServices::openUrl(QUrl("http://physics.nist.gov/PhysRefData/Xcom/html/xcom1-t.html", QUrl::TolerantMode));
}

void MaterialInspectorWindow::on_pbRename_clicked()
{
    if (bMaterialWasModified)
    {
        message("Material properties were modified!\nUpdate, add as new or cancel changes before renaming", this);
        return;
    }

    const QString newName = ui->leName->text();
    int iMat = ui->cobActiveMaterials->currentIndex();
    if (iMat < 0) return;

    const QString & oldName = (*MpCollection)[iMat]->name;
    if (newName == oldName) return;

    for (int i = 0; i < MpCollection->countMaterials(); i++)
        if (i != iMat && newName == (*MpCollection)[i]->name)
        {
            message("There is already a material with name " + newName, this);
            return;
        }

    (*MpCollection)[iMat]->name = newName;
    ui->pbRename->setText("Rename " + newName);

    MW->ReconstructDetector(true);
}

void MaterialInspectorWindow::on_pbComputeRangeCharged_clicked()
{
  int particleId = ui->cobParticle->currentIndex();
  if (MpCollection->getParticleType(particleId) != AParticle::_charged_) return;

  if (ui->ledMFPenergy_2->text().isEmpty())
  {
      ui->leMFP_2->setText("");
      return;
  }

  AMaterial& tmpMaterial = MpCollection->tmpMaterial;
  double energy = ui->ledMFPenergy_2->text().toDouble();

  if ( tmpMaterial.MatParticle[particleId].InteractionDataX.size() < 2)
  {
      ui->leMFP_2->setText("n.a.");
      return;
  }
  if (energy < tmpMaterial.MatParticle[particleId].InteractionDataX.first() || energy > tmpMaterial.MatParticle[particleId].InteractionDataX.last())
  {
      ui->leMFP_2->setText("n.a.");
      return;
  }

  int LogLogInterpolation = MpCollection->fLogLogInterpolation;
  //double InteractionCoefficient = InteractionValue(energy, &tmpMaterial.MatParticle[particleId].InteractionDataX, &tmpMaterial.MatParticle[particleId].InteractionDataF, LogLogInterpolation);
  //qDebug()<<InteractionCoefficient<<tmpMaterial.density;
  //double MeanFreePath = 10.0/InteractionCoefficient/tmpMaterial.density;  // 1/(cm2/g)/(g/cm3) - need in mm (so that 10.)

  double range = 0;
  double Step;
  int counter = 1000; //1000 steps!
  do
  {
      if (energy < tmpMaterial.MatParticle[particleId].InteractionDataX.first()) break;
      double InteractionCoefficient = GetInterpolatedValue(energy, &tmpMaterial.MatParticle[particleId].InteractionDataX, &tmpMaterial.MatParticle[particleId].InteractionDataF, LogLogInterpolation);
      //qDebug()<<InteractionCoefficient<<tmpMaterial.density;
      //dE/dx [keV/mm] = Density[g/cm3] * [cm2/g*keV] * 0.1  //0.1 since local units are mm, not cm
      double dEdX = 0.1 * tmpMaterial.density * InteractionCoefficient;
      Step = 0.03 * energy/dEdX;
      double dE = dEdX * Step;
      range += Step;

      energy -= dE;
      counter--;
  }
  while (counter>0);

  QString str;
  str.setNum(range, 'g', 4);
  ui->leMFP_2->setText(str);
}

void MaterialInspectorWindow::on_actionSave_material_triggered()
{
  //checkig this material
  const QString error = MpCollection->CheckTmpMaterial();
  if ( !error.isEmpty() )
    {
      message(error, this);
      return;
    }

  QString starter = (MW->GlobSet.LibMaterials.isEmpty()) ? MW->GlobSet.LastOpenDir : MW->GlobSet.LibMaterials;
  int imat = ui->cobActiveMaterials->currentIndex();
  if (imat == -1) starter += "/Material_";
  else starter += "/Material_"+(*MpCollection)[imat]->name;
  QString fileName = QFileDialog::getSaveFileName(this,"Save material", starter, "Material files (*.mat *.json);;All files (*.*)");
  if (fileName.isEmpty()) return;
  QFileInfo fileInfo(fileName);
  if (fileInfo.suffix().isEmpty()) fileName += ".mat";

  QJsonObject json, js;
  MpCollection->writeMaterialToJson(imat, json);
  js["Material"] = json;
  bool bOK = SaveJsonToFile(js, fileName);
  if (!bOK) message("Failed to save json to file: "+fileName, this);
}

void MaterialInspectorWindow::on_actionLoad_material_triggered()
{
  QString starter = (MW->GlobSet.LibMaterials.isEmpty()) ? MW->GlobSet.LastOpenDir : MW->GlobSet.LibMaterials;
  QString fileName = QFileDialog::getOpenFileName(this, "Load material", starter, "Material files (*mat *.json);;All files (*.*)");
  if (fileName.isEmpty()) return;

  QJsonObject json, js;
  bool bOK = LoadJsonFromFile(json, fileName);
  if (!bOK)
  {
      message("Cannot open file: "+fileName, this);
      return;
  }
  if (!json.contains("Material"))
    {
      message("File format error: Json with material settings not found", this);
      return;
    }
  js = json["Material"].toObject();
  MpCollection->tmpMaterial.readFromJson(js, MpCollection);

  setWasModified(true);
  updateWaveButtons();
  MW->ListActiveParticles();

  ui->cobActiveMaterials->setCurrentIndex(-1); //to avoid confusion (and update is disabled for -1)
  LastShownMaterial = -1;

  UpdateGui(); //refresh indication of tmpMaterial
  updateWaveButtons(); //refresh button state for Wave-resolved properties
  SetParticleSelection(0);
}

void MaterialInspectorWindow::on_actionClear_Interaction_for_this_particle_triggered()
{
  QMessageBox *msgBox = new QMessageBox( this );
  msgBox->setWindowTitle("Confirmation");
  msgBox->setText( QString("Clear ALL interaction data of material %1 for particle %2?").arg(ui->leName->text()).arg(ui->cobParticle->currentText()) );
  msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox->setDefaultButton(QMessageBox::Yes);

  if (msgBox->exec() == QMessageBox::No)
    {
      if (msgBox) delete msgBox;
      return;
    }
  if (msgBox) delete msgBox;

    int iPart = ui->cobParticle->currentIndex();
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    tmpMaterial.MatParticle[iPart].Clear();

    updateInteractionGui();
    setWasModified(true);
}

void MaterialInspectorWindow::on_actionClear_interaction_for_all_particles_triggered()
{
  QMessageBox *msgBox = new QMessageBox( this );
  msgBox->setWindowTitle("Confirmation");
  msgBox->setText( QString("Clear ALL interaction data of material %1 for ALL particles?").arg(ui->leName->text()) );
  msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox->setDefaultButton(QMessageBox::Yes);

  if (msgBox->exec() == QMessageBox::No)
    {
      if (msgBox) delete msgBox;
      return;
    }
  if (msgBox) delete msgBox;

  AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    //for (int i=0; i<Detector->ParticleCollection.size(); i++)
    for (int iPart=0; iPart<MpCollection->countParticles(); iPart++)
        tmpMaterial.MatParticle[iPart].Clear();

    updateInteractionGui();
    setWasModified(true);
}

void MaterialInspectorWindow::on_actionUse_log_log_interpolation_triggered()
{
  MpCollection->fLogLogInterpolation = ui->actionUse_log_log_interpolation->isChecked();
  MW->ReconstructDetector(true);
}

void MaterialInspectorWindow::on_pbShowPhotoelectric_clicked()
{
    showProcessIntCoefficient(0);
}

void MaterialInspectorWindow::on_pbShowCompton_clicked()
{
    showProcessIntCoefficient(1);
}

void MaterialInspectorWindow::on_pbShowPairProduction_clicked()
{
    showProcessIntCoefficient(2);
}

void MaterialInspectorWindow::showProcessIntCoefficient(int iTermScenario)
{
    AMaterial & tmpMaterial = MpCollection->tmpMaterial;

    const int particleId = ui->cobParticle->currentIndex();
    if (particleId < 0 || particleId >= tmpMaterial.MatParticle.size()) return;

    QVector<NeutralTerminatorStructure> & Terminators = tmpMaterial.MatParticle[particleId].Terminators;
    if (iTermScenario >= Terminators.size())
    {
        message("Not defined in the current configuration!", this);
        return;
    }
    NeutralTerminatorStructure & Scenario = Terminators[iTermScenario];

    if (Scenario.PartialCrossSection.size() < 1) return;

    int color;
    QString title;
    switch (iTermScenario)
    {
    case 0: color = kGreen;   title = "Photoelectric";      break;
    case 1: color = kBlue;    title = "Compton scattering"; break;
    case 2: color = kMagenta; title = "Pair production";    break;
    }

    //if draw is empty, root will "swallow" the axis titles when converting to log log
    TGraph * gr = MW->GraphWindow->ConstructTGraph(QVector<double>{1,2,3}, QVector<double>{1,2,3});
    MW->GraphWindow->Draw(gr, "AP");
    MW->GraphWindow->SetLog(true, true);

    gr = MW->GraphWindow->ConstructTGraph(Scenario.PartialCrossSectionEnergy, Scenario.PartialCrossSection);
    MW->GraphWindow->configureGraph(gr, title,
                                    "Energy, keV", "Interaction coefficient, cm^{2}/g",
                                    color,  2, 1,
                                    color,  2, 1);
    MW->GraphWindow->Draw(gr, "AP");

    gr = constructInterpolationGraph(Scenario.PartialCrossSectionEnergy, Scenario.PartialCrossSection);
    gr->SetLineColor(color);
    gr->SetLineWidth(1);
    MW->GraphWindow->Draw(gr, "LSAME");
}

#include <limits>
void MaterialInspectorWindow::on_pbShowAllForGamma_clicked()
{
    int particleId = ui->cobParticle->currentIndex();
    const AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    const MatParticleStructure & mp = tmpMaterial.MatParticle[particleId];

    if (mp.Terminators.size() < 2) return;

    double Xmin, Xmax, Ymin, Ymax;
    Xmin = Ymin = std::numeric_limits<double>::max();
    Xmax = Ymax = 0;
    TGraph * mainGr = nullptr;

    for (int i = 0; i < mp.Terminators.size(); i++)
    {
        TString opt, title;
        int color;
        int Lwidth = 1;
        switch (i)
        {
        case 0:
            color = kGreen;
            opt = "AL";
            title = "Photoelectric";
            Lwidth = 2;
            break;
        case 1:
            color = kBlue;
            opt = "Lsame";
            title = "Compton";
            break;
        case 2:
            color = kMagenta;
            opt = "Lsame";
            title = "Pair";
            break;
        default:
            qWarning() << "Bad terminator size for gamma!";
            continue;
        }

        const QVector<double> & X = mp.Terminators[i].PartialCrossSectionEnergy;
        const QVector<double> & Y = mp.Terminators[i].PartialCrossSection;

        //have to remove zeros due to log scale
        QVector<double> xVec, yVec;
        for (int iD = 0; iD < X.size(); iD++)
        {
            const double & x = X.at(iD);
            const double & y = Y.at(iD);
            if (y > 0)
            {
                xVec << x;
                yVec << y;

                Xmin = std::min(Xmin, x);
                Ymin = std::min(Ymin, y);
                Xmax = std::max(Xmax, x);
                Ymax = std::max(Ymax, y);
            }
        }

        TGraph* gr = constructInterpolationGraph(xVec, yVec);
        gr->SetTitle(title);
        gr->SetLineColor(color);
        gr->SetLineWidth(Lwidth);
        gr->GetXaxis()->SetTitle("Energy, keV");
        gr->GetYaxis()->SetTitle("Mass interaction coefficient, cm^{2}/g");
        gr->GetXaxis()->SetTitleOffset(1.1);
        gr->GetYaxis()->SetTitleOffset(1.3);
        if (i == 0) mainGr = gr;

        MW->GraphWindow->Draw(gr, opt);
    }

    if (mainGr && mainGr->GetHistogram())
    {
        mainGr->GetXaxis()->SetLimits(Xmin, Xmax);
        mainGr->GetHistogram()->SetMaximum(Ymax);
        mainGr->GetHistogram()->SetMinimum(Ymin);
    }

    MW->GraphWindow->AddLegend();

    MW->GraphWindow->SetLog(true, true);
}

void MaterialInspectorWindow::showTotalInteraction()
{
    int particleId = ui->cobParticle->currentIndex();
    int entries = MpCollection->tmpMaterial.MatParticle[particleId].InteractionDataX.size();
    if (entries < 1) return;

    //if draw is empty, root will "swallow" the axis titles when converting to log log
    TGraph * gr = MW->GraphWindow->ConstructTGraph(QVector<double>{1,2,3}, QVector<double>{1,2,3});
    MW->GraphWindow->Draw(gr, "AP");
    MW->GraphWindow->SetLog(true, true);

    const AParticle::ParticleType type = MpCollection->getParticleType(particleId);

    QVector<double> X(MpCollection->tmpMaterial.MatParticle[particleId].InteractionDataX);
    QVector<double> Y(MpCollection->tmpMaterial.MatParticle[particleId].InteractionDataF);

    TString Title = "", Xtitle, Ytitle;
    if (type == AParticle::_charged_)
    {
        Xtitle = "Energy, keV";
        Ytitle = "Stopping power, keV cm2/g";
    }
    else if (type == AParticle::_gamma_)
    {
        Xtitle = "Energy, keV";
        Ytitle = "Total mass interaction coefficient, cm2/g";
    }
    else if (type == AParticle::_neutron_)
    {
        Xtitle = "Energy, meV";
        Ytitle = "Capture cross section, barns";

        for (int i=0; i<X.size(); i++)
        {
            X[i] *= 1.0e6; // keV -> meV
            Y[i] *= 1.0e24; // cm2 to barns
        }
    }
    else
    {
        message("Unknown particle type!");
        return;
    }

    gr = MW->GraphWindow->ConstructTGraph(X, Y, Title, Xtitle, Ytitle, kRed, 2, 1, kRed, 0, 1);
    MW->GraphWindow->Draw(gr, "AP");

    TGraph* graphOver = constructInterpolationGraph(X, Y);
    graphOver->SetLineColor(kRed);
    graphOver->SetLineWidth(1);
    MW->GraphWindow->Draw(graphOver, "L same");
}

TGraph * MaterialInspectorWindow::constructInterpolationGraph(const QVector<double> & X, const QVector<double> & Y) const
{
    const int entries = X.size();

    QVector<double> xx, yy;
    for (int i = 1; i < entries; i++)
        for (int j = 1; j < 50; j++)
        {
            const double & previousOne = X[i-1];
            const double & thisOne     = X[i];
            double XX = previousOne + 0.02 * j * (thisOne - previousOne);
            xx << XX;
            double YY;
            if (XX < X.last()) YY = GetInterpolatedValue(XX, &X, &Y, MpCollection->fLogLogInterpolation);
            else YY = Y.last();
            yy << YY;
        }
    return MW->GraphWindow->ConstructTGraph(xx, yy);
}

void MaterialInspectorWindow::on_pbXCOMauto_clicked()
{
  QString str = ui->leChemicalComposition->text().simplified();
  str.replace(" ", "");
  if (str.isEmpty())
  {
      ui->twProperties->setCurrentIndex(0);
      message("Enter chemical composition of the material", this);
      return;
  }

  QStringList elList = str.split(QRegExp("\\+"));
  //    qDebug() << elList<<elList.size();

  QString compo;
  for (QString el : elList)
    {
      if (!compo.isEmpty()) compo += "\n";

      QStringList wList = el.split(":");
      if (wList.size()>1)
        compo += wList.at(0) + " " + wList.at(1);
      else
        compo += wList.at(0) + " 1";
    }
  //    qDebug() << compo;

  QString pack = "Formulae="+compo+"&Name="+str+"&Energies="+"&Output=on";
  QString Url = "https://physics.nist.gov/cgi-bin/Xcom/xcom3_3-t";   // xcom site: http -> https changed!
  QString Reply;

  AInternetBrowser b(3000); //  *** !!! absoluite value - 3s timeout
  MW->WindowNavigator->BusyOn();
  bool fOK = b.Post(Url, pack, Reply);
  //    qDebug() << "Post result:"<<fOK;
  MW->WindowNavigator->BusyOff();
  if (!fOK)
    {
      message("Operation failed:\n"+b.GetLastError(), this);
      return;
    }
  //    qDebug() << Reply;

  if (Reply.contains("Error: Unable to parse formula"))
    {
      message("Unable to parse formula!\n"
              "For example,\n"
              " H20:9 + NaCl:1 \n"
              "will configure a mixture of two compounds (H2O and NaCl), with relative fractions of 9 to 1", this);
      return;
    }

  QRegExp rx = QRegExp("Return to.*/a>.");
  rx.setMinimal(true);
  Reply.remove(rx);
  rx = QRegExp("<script.*</script>");
  Reply.remove(rx);
  Reply.remove("<hr>");

  int particleId = ui->cobParticle->currentIndex();
  QTextStream in(&Reply);

  fOK = importXCOM(in, particleId);

  if (fOK) MpCollection->tmpMaterial.MatParticle[particleId].DataString = ui->leChemicalComposition->text();

  updateInteractionGui();
  setWasModified(true);
}

void MaterialInspectorWindow::on_pbShowXCOMdata_clicked()
{
    int particleId = ui->cobParticle->currentIndex();
    const AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    QString s = tmpMaterial.MatParticle.at(particleId).DataSource;

    QDialog D(this);
    D.setWindowTitle("XCOM data");
    QVBoxLayout * l = new QVBoxLayout(&D);
    QPlainTextEdit * pte = new QPlainTextEdit();
    pte->appendHtml(s);
    l->addWidget(pte);
    QPushButton * pb = new QPushButton("Close");
    connect(pb, &QPushButton::clicked, &D, &QDialog::accept);
    l->addWidget(pb);
    D.resize(1000, 500);

    D.exec();
}

void MaterialInspectorWindow::on_cobYieldForParticle_activated(int index)
{
    const AMaterial & tmpMaterial = MpCollection->tmpMaterial;

    flagDisreguardChange = true; // -->
    ui->ledPrimaryYield->setText( QString::number(tmpMaterial.getPhotonYield(index)) );
    ui->ledIntEnergyRes->setText( QString::number(tmpMaterial.getIntrinsicEnergyResolution(index)) );
    flagDisreguardChange = false; // <--
}

bool MaterialInspectorWindow::doLoadCrossSection(ANeutronInteractionElement *element, QString fileName)
{
    QVector<double> x, y;
    QString header = OptionsConfigurator->getHeaderLineId();
    int hLines = OptionsConfigurator->getNumCommentLines();
    int res = LoadDoubleVectorsFromFile(fileName, &x, &y, &header, hLines);
    if (res == 0)
    {
        double Multiplier = 1.0;
        switch (OptionsConfigurator->getCrossSectionLoadOption())
        {
          case (0): {Multiplier = 1.0e-6; break;} //meV
          case (1): {Multiplier = 1.0e-3; break;} //eV
          case (2): {Multiplier = 1.0; break;}    //keV
          case (3): {Multiplier = 1.0e3; break;}  //MeV
        }
        for (int i=0; i<x.size(); i++)
        {
            x[i] *= Multiplier;  //to keV
            y[i] *= 1.0e-24;     //to cm2
        }

        if (OptionsConfigurator->isEnergyRangeLimited())
        {
            const double EnMin = OptionsConfigurator->getMinEnergy() * 1.0e-6; //meV -> keV
            const double EnMax = OptionsConfigurator->getMaxEnergy() * 1.0e-6; //meV -> keV
            QVector<double> xtmp, ytmp;
            xtmp = x;  ytmp = y;
            x.clear(); y.clear();
            for (int i=0; i<xtmp.size(); i++)
            {
                const double& xx = xtmp.at(i);
                if (xx<EnMin || xx>EnMax) continue;
                x << xx; y << ytmp.at(i);
            }
        }

        element->Energy = x;
        element->CrossSection = y;
        element->CSfileHeader = header;
        setWasModified(true);
        return true;
    }
    return false;
}

bool MaterialInspectorWindow::autoLoadCrossSection(ANeutronInteractionElement *element, QString target)
{
    QString Mass = QString::number(element->Mass);
    QString fileName;
    if (target == "elastic scattering")
        fileName = OptionsConfigurator->getElasticScatteringFileName(element->Name, Mass);
    else if (target == "absorption")
        fileName = OptionsConfigurator->getAbsorptionFileName(element->Name, Mass);
    else qWarning() << "Unknown selector in autoload neutron cross-section";

    if (fileName.isEmpty()) return false;
    if ( !QFileInfo(fileName).exists() ) return false;

    //qDebug() << "Autoload cross-section from file: " <<fileName;

    return doLoadCrossSection(element, fileName);
}

int MaterialInspectorWindow::autoLoadReaction(ANeutronInteractionElement& element)
{
    QString fileName = QString("%1/Neutrons/Reactions/%2-%3.json").arg(MW->GlobSet.ResourcesDir).arg(element.Name).arg(element.Mass);
    //  qDebug() << "Is there file for reaction? "<<fileName;

    int delta = 0;
    if (QFile(fileName).exists())
    {
        QJsonObject json;
        bool bOK = LoadJsonFromFile(json, fileName);
        if (bOK)
        {
            int numPartBefore = MpCollection->countParticles();
            //  qDebug() << "File found! Loading reaction data...";
            element.readScenariosFromJson(json, MpCollection);
            int numPartAfter = MpCollection->countParticles();
            delta+= numPartAfter - numPartBefore;
        }
    }
    return delta;
}

void MaterialInspectorWindow::onAddIsotope(AChemicalElement *element)
{
    element->Isotopes << AIsotope(element->Symbol, 777, 0);

    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    //tmpMaterial.ChemicalComposition.CalculateMeanAtomMass();
    tmpMaterial.ChemicalComposition.updateMassRelatedPoperties();
    tmpMaterial.updateNeutronDataOnCompositionChange(MpCollection);

    FillNeutronTable();
    UpdateGui();

    //ShowTreeWithChemicalComposition();
    setWasModified(true);
}

void MaterialInspectorWindow::onRemoveIsotope(AChemicalElement *element, int isotopeIndexInElement)
{
    if (element->Isotopes.size()<2)
    {
        message("Cannot remove the last isotope!", this);
        return;
    }
    element->Isotopes.removeAt(isotopeIndexInElement);

    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    //tmpMaterial.ChemicalComposition.CalculateMeanAtomMass();
    tmpMaterial.ChemicalComposition.updateMassRelatedPoperties();
    tmpMaterial.updateNeutronDataOnCompositionChange(MpCollection);

    //ShowTreeWithChemicalComposition();
    FillNeutronTable();
    UpdateGui();
    setWasModified(true);
}

void MaterialInspectorWindow::IsotopePropertiesChanged(const AChemicalElement * /*element*/, int /*isotopeIndexInElement*/)
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    //tmpMaterial.ChemicalComposition.CalculateMeanAtomMass();
    tmpMaterial.ChemicalComposition.updateMassRelatedPoperties();
    tmpMaterial.updateNeutronDataOnCompositionChange(MpCollection);

    //ShowTreeWithChemicalComposition();
    FillNeutronTable();
    UpdateGui();
    setWasModified(true);
}

void MaterialInspectorWindow::onRequestDraw(const QVector<double> &x, const QVector<double> &y, const QString &titleX, const QString &titleY)
{
    TGraph * g = MW->GraphWindow->ConstructTGraph(x, y, "", titleX, titleY, 4, 20, 1, 4, 1, 2);
    MW->GraphWindow->Draw(g, "APL");
    MW->GraphWindow->UpdateRootCanvas();
}

void MaterialInspectorWindow::on_pbShowStatisticsOnElastic_clicked()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    tmpMaterial.updateRuntimeProperties(MpCollection->fLogLogInterpolation, Detector->RandGen);

    NeutronInfoDialog = new ANeutronInfoDialog(&MpCollection->tmpMaterial, ui->cobParticle->currentIndex(), MpCollection->fLogLogInterpolation,
                                                    ui->cbCapture->isChecked(), ui->cbEnableScatter->isChecked(), MW->GraphWindow, this);

    NeutronInfoDialog->setWindowFlags(NeutronInfoDialog->windowFlags() | Qt::WindowStaysOnTopHint);
    NeutronInfoDialog->show();
    MW->WindowNavigator->DisableAllButGraphWindow(true);
    NeutronInfoDialog->setEnabled(true);
    do
    {
        QThread::msleep(1);
        qApp->processEvents();
        if (!NeutronInfoDialog) return;
    }
    while (NeutronInfoDialog->isVisible());

    if (NeutronInfoDialog)
    {
        delete NeutronInfoDialog;
        MW->WindowNavigator->DisableAllButGraphWindow(false);
    }
    NeutronInfoDialog = 0;
}

void MaterialInspectorWindow::on_pbModifyChemicalComposition_clicked()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    QDialog* d = new QDialog(this);
    d->setWindowTitle("Enter element composition (molar fractions!)");

    QVBoxLayout* L = new QVBoxLayout();
        QHBoxLayout* l = new QHBoxLayout();
        QLineEdit* le = new QLineEdit(tmpMaterial.ChemicalComposition.getCompositionString(), this);
        le->setMinimumSize(400,25);
        QPushButton* pb = new QPushButton("Confirm", this);
        l->addWidget(le);
        l->addWidget(pb);
        connect(pb, SIGNAL(clicked(bool)), d, SLOT(accept()));
    L->addLayout(l);
    L->addWidget(new QLabel("Format examples:\n"));
    L->addWidget(new QLabel("C2H5OH   - use only integer values!"));
    L->addWidget(new QLabel("C:0.3333 + H:0.6667  -> molar fractions of 1/3 of carbon and 2/3 of hydrogen"));
    L->addWidget(new QLabel("H2O:9.0 + NaCl:0.2 -> 9.0 parts of H2O and 0.2 parts of NaCl"));
    d->setLayout(L);

    while (d->exec() != 0)
    {
        AMaterialComposition& mc = tmpMaterial.ChemicalComposition;
        QString error = mc.setCompositionString(le->text(), true);
        if (!error.isEmpty())
        {
            message(error, d);
            continue;
        }

        UpdateGui();
        //ui->leChemicalComposition->setText(mc.getCompositionString());
        //ShowTreeWithChemicalComposition();
        break;
    }

    if (d->result() == 0) return;

    tmpMaterial.updateNeutronDataOnCompositionChange(MpCollection);

    int numNewPart = 0;
    if (OptionsConfigurator->isAutoloadEnabled())
        numNewPart += autoloadMissingCrossSectionData();

    FillNeutronTable(); //fill table if neutron is selected

    if (numNewPart > 0)
        updateTmpMatOnPartCollChange(numNewPart);

    setWasModified(true);
    updateWarningIcons();
}

void MaterialInspectorWindow::on_pbModifyByWeight_clicked()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    QDialog* d = new QDialog(this);
    d->setWindowTitle("Enter element composition (fractions by weight!)");

    QVBoxLayout* L = new QVBoxLayout();
        QHBoxLayout* l = new QHBoxLayout();
        QLineEdit* le = new QLineEdit(tmpMaterial.ChemicalComposition.getCompositionByWeightString(), this);
        le->setMinimumSize(400,25);
        QPushButton* pb = new QPushButton("Confirm", this);
        l->addWidget(le);
        l->addWidget(pb);
        connect(pb, SIGNAL(clicked(bool)), d, SLOT(accept()));
    L->addLayout(l);
    L->addWidget(new QLabel("Give weight factors for each element separately, e.g.:\n"));
    L->addWidget(new QLabel("H:0.1112 + O:0.8889"));
    L->addWidget(new QLabel("\nNote that Ants will recalculate this composition to molar one,\n"
                            "and then show re-calculated weight factors with the sum of unity!\n\n"
                            "Any subsequent changes to isotope composition of involved elements\n"
                            "will modify the composition!"));
    d->setLayout(L);

    while (d->exec() != 0)
    {
        AMaterialComposition& mc = tmpMaterial.ChemicalComposition;
        QString error = mc.setCompositionByWeightString(le->text());
        if (!error.isEmpty())
        {
            message(error, d);
            continue;
        }

        UpdateGui();
        //ui->leChemicalComposition->setText(mc.getCompositionString());
        //ShowTreeWithChemicalComposition();
        break;
    }

    if (d->result() == 0) return;

    tmpMaterial.updateNeutronDataOnCompositionChange(MpCollection);

    int numNewPart = 0;
    if (OptionsConfigurator->isAutoloadEnabled())
        numNewPart += autoloadMissingCrossSectionData();

    FillNeutronTable(); //fill table if neutron is selected

    if (numNewPart > 0)
        updateTmpMatOnPartCollChange(numNewPart);

    setWasModified(true);
    updateWarningIcons();
}

void MaterialInspectorWindow::updateTmpMatOnPartCollChange(int newPartAdded)
{
    if (newPartAdded > 0)
    {
        QString s = QString("Added %1 new particle").arg(newPartAdded);
        if (newPartAdded > 1) s += "s";
        s+= ":\n";
        for (int i=0; i<newPartAdded; i++)
        {
            int ipart = MpCollection->countParticles() - newPartAdded + i;
            QString partName = MpCollection->getParticleName(ipart);
            ui->cobParticle->addItem(partName);
            s+= QString("\n  %1").arg(partName);
        }
        s += "\n\nDo not forget to provide interaction cross-sections for the new particles!";
        message(s, this);

        //forbid the general update of MIwindow - otherwise changes in tmpmat will be lost
        bLockTmpMaterial = true;
            MW->onRequestDetectorGuiUpdate();
        bLockTmpMaterial = false;
    }
}

void MaterialInspectorWindow::ShowTreeWithChemicalComposition()
{
    bClearInProgress = true;
    ui->trwChemicalComposition->clear();
    bClearInProgress = false;

    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    bool bShowIsotopes = ui->cbShowIsotopes->isChecked();

    for (int i=0; i<tmpMaterial.ChemicalComposition.countElements(); i++)
    {
        AChemicalElement* el = tmpMaterial.ChemicalComposition.getElement(i);

        //new element
        AChemicalElementDelegate* elDel = new AChemicalElementDelegate(el, &bClearInProgress, ui->cbShowIsotopes->isChecked());
        QTreeWidgetItem* ElItem = new QTreeWidgetItem(ui->trwChemicalComposition);
        ui->trwChemicalComposition->setItemWidget(ElItem, 0, elDel);
        ElItem->setExpanded(bShowIsotopes);
        QObject::connect(elDel, &AChemicalElementDelegate::AddIsotopeActivated, this, &MaterialInspectorWindow::onAddIsotope, Qt::QueuedConnection);

        if (bShowIsotopes)
            for (int index = 0; index <el->Isotopes.size(); index++)
            {
                AIsotopeDelegate* isotopDel = new AIsotopeDelegate(el, index, &bClearInProgress);
                QTreeWidgetItem* twi = new QTreeWidgetItem();
                ElItem->addChild(twi);
                ui->trwChemicalComposition->setItemWidget(twi, 0, isotopDel);
                QObject::connect(isotopDel, &AIsotopeDelegate::RemoveIsotope, this, &MaterialInspectorWindow::onRemoveIsotope, Qt::QueuedConnection);
                QObject::connect(isotopDel, &AIsotopeDelegate::IsotopePropertiesChanged, this, &MaterialInspectorWindow::IsotopePropertiesChanged, Qt::QueuedConnection);
            }
    }
}

void MaterialInspectorWindow::on_cbShowIsotopes_clicked()
{
    ShowTreeWithChemicalComposition();
}

void flagButton(QPushButton* pb, bool flag)
{
    QString toRed = "QPushButton {color: red;}";
    QString s = pb->styleSheet();

    if (flag)
    {
        if (!s.contains(toRed)) s += toRed;
    }
    else
    {
        if (s.contains(toRed)) s.remove(toRed);
    }

    pb->setStyleSheet(s);
}

void MaterialInspectorWindow::FillNeutronTable()
{
    //      qDebug() << "Filling neutron table";
    int particleId = ui->cobParticle->currentIndex();
    if (particleId != MpCollection->getNeutronIndex()) return;

    ui->tabwNeutron->clearContents();
    ui->tabwNeutron->setRowCount(0);
    ui->tabwNeutron->setColumnCount(0);

    const AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    const MatParticleStructure & mp = tmpMaterial.MatParticle.at(particleId);
    const QVector<NeutralTerminatorStructure> & Terminators = mp.Terminators;

    const bool bCapture = mp.bCaptureEnabled;
    const bool bElastic = mp.bElasticEnabled && !mp.bUseNCrystal;
    if (!bCapture && !bElastic) return;

    int numElements = tmpMaterial.ChemicalComposition.countElements();
    int numIso = tmpMaterial.ChemicalComposition.countIsotopes();
    //      qDebug() << "..Starting update of neutron table.  Total isotopes:"<<numIso;
    ui->tabwNeutron->setRowCount(numIso);

    int numColumns = 1;
    if (bCapture) numColumns++;
    if (bElastic) numColumns++;
    ui->tabwNeutron->setColumnCount(numColumns);

    QTableWidgetItem* twi = new QTableWidgetItem("Element");
    twi->setTextAlignment(Qt::AlignCenter);
    ui->tabwNeutron->setHorizontalHeaderItem(0, twi);
    if (bCapture)
    {
        twi = new QTableWidgetItem("Capture");
        twi->setTextAlignment(Qt::AlignCenter);
        ui->tabwNeutron->setHorizontalHeaderItem(1, twi);
    }
    if (bElastic)
    {
        twi = new QTableWidgetItem("Elastic");
        twi->setTextAlignment(Qt::AlignCenter);
        ui->tabwNeutron->setHorizontalHeaderItem(numColumns-1, twi);
    }

    int row = 0;
    bool bIgnore = mp.bAllowAbsentCsData;
    bool bFoundMissing = false;
    for (int iElement=0; iElement<numElements; iElement++)
    {
        const AChemicalElement* el = tmpMaterial.ChemicalComposition.getElement(iElement);
        for (int iIso=0; iIso<el->countIsotopes(); iIso++)
        {
            QString name = el->Isotopes.at(iIso).Symbol + "-" + QString::number(el->Isotopes.at(iIso).Mass);
            //      qDebug() << "Updating" <<name;
            QTableWidgetItem* twi = new QTableWidgetItem(name);
            twi->setTextAlignment(Qt::AlignCenter);
            ui->tabwNeutron->setItem(row, 0, twi);
            if (bCapture)
            {
                const NeutralTerminatorStructure & t = Terminators.first();
                const ANeutronInteractionElement * absEl = t.getNeutronInteractionElement(row);
                //      qDebug() << "index:"<<row << "Defined absorption elements:" << t.IsotopeRecords.size();
                if (!absEl)
                {
                    message("Critical error - absorption element not found!", this);
                    return;
                }
                QWidget* w = new QWidget();
                QHBoxLayout* l = new QHBoxLayout();
                l->setContentsMargins(6,0,2,0);
                l->setSpacing(2);
                l->setAlignment(Qt::AlignCenter);
                  QPushButton* pbShow = new QPushButton("Show");
                  pbShow->setEnabled(!absEl->Energy.isEmpty());
                  pbShow->setMaximumWidth(50);
                  l->addWidget(pbShow);
                  QPushButton* pbLoad = new QPushButton("Load");
                  if (absEl->Energy.isEmpty())
                  {
                      if (!bIgnore) flagButton(pbLoad, true);
                      bFoundMissing = true;
                  }
                  pbLoad->setMaximumWidth(50);
                  l->addWidget(pbLoad);
                  QPushButton* pbReaction = new QPushButton("Reactions");
                  l->addWidget(pbReaction);
                  QLabel* lab = new QLabel("  ");
                  QIcon YellowIcon = GuiUtils::createColorCircleIcon( QSize(12,12), ( absEl->DecayScenarios.isEmpty() ? Qt::white : Qt::green ) );
                  lab->setPixmap(YellowIcon.pixmap(16,16));
                  l->addWidget( lab );
                w->setLayout(l);
                ui->tabwNeutron->setCellWidget(row, 1, w);

                QObject::connect(pbShow, &QPushButton::clicked, this, [iElement, iIso, this](){ onTabwNeutronsActionRequest(iElement, iIso, "ShowCapture"); });
                QObject::connect(pbLoad, &QPushButton::clicked, this, [iElement, iIso, this](){ onTabwNeutronsActionRequest(iElement, iIso, "LoadCapture"); });
                QObject::connect(pbReaction, &QPushButton::clicked, this, [iElement, iIso, this](){ onTabwNeutronsActionRequest(iElement, iIso, "Reactions"); });
            }
            if (bElastic)
            {
                const NeutralTerminatorStructure & t = Terminators.last();
                const ANeutronInteractionElement * scatEl = t.getNeutronInteractionElement(row);
                //      qDebug() << "index:"<<row << "Defined scatter elements:" << t.IsotopeRecords.size();
                if (!scatEl)
                {
                    message("Critical error - elastic scatter element not found!", this);
                    return;
                }
                QWidget* w = new QWidget();
                QHBoxLayout* l = new QHBoxLayout();
                l->setContentsMargins(6,0,2,0);
                l->setSpacing(2);
                l->setAlignment(Qt::AlignCenter);
                QPushButton* pbShow = new QPushButton("Show");
                pbShow->setMaximumWidth(50);
                pbShow->setEnabled(!scatEl->Energy.isEmpty());
                l->addWidget(pbShow);
                QPushButton* pbLoad = new QPushButton("Load");
                if (scatEl->Energy.isEmpty())
                {
                    if (!bIgnore) flagButton(pbLoad, true);
                    bFoundMissing = true;
                }

                pbLoad->setMaximumWidth(50);
                l->addWidget(pbLoad);
                w->setLayout(l);
                ui->tabwNeutron->setCellWidget(row, numColumns-1, w);

                QObject::connect(pbShow, &QPushButton::clicked, this, [iElement, iIso, this](){ onTabwNeutronsActionRequest(iElement, iIso, "ShowElastic"); });
                QObject::connect(pbLoad, &QPushButton::clicked, this, [iElement, iIso, this](){ onTabwNeutronsActionRequest(iElement, iIso, "LoadElastic"); });
            }
            row++;
        }
    }

    ui->labAssumeZeroForEmpty->setVisible(bFoundMissing && bIgnore); //warning - yellow circle

    ui->tabwNeutron->resizeColumnsToContents();
    ui->tabwNeutron->resizeRowsToContents();

    MpCollection->tmpMaterial.updateRuntimeProperties(MpCollection->fLogLogInterpolation, Detector->RandGen); //need to be here? counter-intuitive in indication!
}

int MaterialInspectorWindow::autoloadMissingCrossSectionData(bool bForceReload)
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    int newParticlesDefined = 0;

    //for neutron
    int neutronId = MpCollection->getNeutronIndex();
    if (neutronId != -1) //otherwise not defined in this configuration
    {
        MatParticleStructure& mp = tmpMaterial.MatParticle[neutronId];

        bool bCapture = mp.bCaptureEnabled;
        bool bElastic = mp.bElasticEnabled;
        if (!bCapture && !bElastic) return 0;

        QVector<NeutralTerminatorStructure>& Terminators = mp.Terminators;

        if (Terminators.size() != 2)
        {
            qWarning() << "||| Terminators size is not equal to two!";
            return 0;
        }
        NeutralTerminatorStructure& termAbs = Terminators[0];
        NeutralTerminatorStructure& termScat = Terminators[1];

        if (bCapture)
        {
            for (int iEl = 0; iEl<termAbs.IsotopeRecords.size(); iEl++)
                if (termAbs.IsotopeRecords.at(iEl).Energy.isEmpty() || bForceReload)
                {
                    autoLoadCrossSection(&termAbs.IsotopeRecords[iEl], "absorption");
                    newParticlesDefined += autoLoadReaction(termAbs.IsotopeRecords[iEl]);
                }
        }
        if (bElastic)
        {
            for (int iEl = 0; iEl<termScat.IsotopeRecords.size(); iEl++)
                if (termScat.IsotopeRecords.at(iEl).Energy.isEmpty() || bForceReload)
                    autoLoadCrossSection(&termScat.IsotopeRecords[iEl], "elastic scattering");
        }
    }

    return newParticlesDefined;
}

void MaterialInspectorWindow::on_tabwNeutron_customContextMenuRequested(const QPoint &pos)
{
    qDebug() << "Menu not implemented" << ui->tabwNeutron->currentRow() << ui->tabwNeutron->currentColumn()<<pos;
}

void MaterialInspectorWindow::on_cbCapture_clicked()
{
    int numNewPart = 0;
    if (OptionsConfigurator->isAutoloadEnabled())
        numNewPart += autoloadMissingCrossSectionData();

    FillNeutronTable(); //fill table if neutron is selected

    if (numNewPart > 0)
        updateTmpMatOnPartCollChange(numNewPart);

    setWasModified(true);
    updateWarningIcons();
}

void MaterialInspectorWindow::on_cbEnableScatter_clicked()
{
    int numNewPart = 0;
    if (OptionsConfigurator->isAutoloadEnabled())
        numNewPart += autoloadMissingCrossSectionData();

    FillNeutronTable(); //fill table if neutron is selected

    if (numNewPart > 0)
        updateTmpMatOnPartCollChange(numNewPart);

    setWasModified(true);
    updateWarningIcons();
}

void MaterialInspectorWindow::onTabwNeutronsActionRequest(int iEl, int iIso, const QString Action)
{
    if (MW->GlobSet.MaterialsAndParticlesSettings.isEmpty())
        on_actionNeutrons_triggered();

    //      qDebug() << "Element#"<<iEl << "Isotope#:"<<iIso <<Action;
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    int particleId = ui->cobParticle->currentIndex();
    QVector<NeutralTerminatorStructure>& Terminators = tmpMaterial.MatParticle[particleId].Terminators;

    int iIndex = tmpMaterial.ChemicalComposition.getNumberInJointIsotopeList(iEl, iIso);
    //      qDebug() << "Index:"<<iIndex;

    NeutralTerminatorStructure* term;
    ANeutronInteractionElement* element;
    TString yTitle;
    QString target;
    if (Action.contains("Elastic"))
    {
        term = &Terminators[1];
        if (iIndex<0 || iIndex>=term->IsotopeRecords.size())
        {
            message("Bad index!", this);
            return;
        }
        element = &term->IsotopeRecords[iIndex];
        yTitle = "Elastic scattering cross-section, barns";
        target = "elastic scattering";
    }
    else //capture
    {
        term = &Terminators[0];
        if (iIndex<0 || iIndex>=term->IsotopeRecords.size())
        {
            message("Bad index!", this);
            return;
        }
        element = &term->IsotopeRecords[iIndex];
        yTitle = "Absorption cross-section, barns";
        target = "absorption";
    }

    // -- Show --
    if (Action.contains("Show"))
    {
        //      qDebug() << "Show ->"<< yTitle << "->" << element->Name <<"-"<< element->Mass;
        QVector<double> x,y;
        for (int i=0; i<element->Energy.size(); i++)
          {
            x << 1.0e6 * element->Energy.at(i);         // keV -> meV
            y << 1.0e24 * element->CrossSection.at(i);  // cm2 to barns
          }

        MW->GraphWindow->ShowAndFocus();
        TString title = element->Name.toLocal8Bit().data();
        title += " - ";
        title += element->Mass;
        TGraph* gr = MW->GraphWindow->ConstructTGraph(x, y, title, "Energy, meV", yTitle, kRed, 2, 1, kRed, 0, 1);
        MW->GraphWindow->Draw(gr, "AP");

        TGraph* graphOver = constructInterpolationGraph(x, y);
        graphOver->SetLineColor(kRed);
        graphOver->SetLineWidth(1);
        MW->GraphWindow->Draw(graphOver, "L same");

        if (!element->CSfileHeader.isEmpty())
            MW->GraphWindow->ShowTextPanel(element->CSfileHeader, true, 0);
    }
    // -- Load --
    else if (Action.contains("Load"))
    {
        QString isotope = element->Name  + "-" + element->Mass;
        //      qDebug() << "Load" << target << "cross-section for" << isotope;
        if (OptionsConfigurator->isAutoloadEnabled())
        {
            bool fOK = autoLoadCrossSection(element, target);
            if (fOK)
            {
                FillNeutronTable();
                return;
            }
        }

        QString fileName = QFileDialog::getOpenFileName(this, "Load " + target + " cross-section data for " + isotope, MW->GlobSet.LastOpenDir, "Data files (*.txt *.dat); All files (*.*)");
        if (fileName.isEmpty()) return;
        MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

        doLoadCrossSection(element, fileName);
        FillNeutronTable();
    }
    // -- Reactions --
    else if (Action == "Reactions")
    {
        QStringList DefinedParticles;
        MpCollection->OnRequestListOfParticles(DefinedParticles);
        ANeutronReactionsConfigurator* d = new ANeutronReactionsConfigurator(&Terminators[0].IsotopeRecords[iIndex], DefinedParticles, this);
        QObject::connect(d, &ANeutronReactionsConfigurator::RequestDraw, this, &MaterialInspectorWindow::onRequestDraw);
        int res = d->exec();
        delete d;
        if (res != 0)
        {
            FillNeutronTable();
            setWasModified(true);
        }
    }
}

void MaterialInspectorWindow::on_pbMaterialInfo_clicked()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    if (ui->leChemicalComposition->text().isEmpty())
    {
        message("Chemical composition is not defined!", this);
        return;
    }

    double MAM = tmpMaterial.ChemicalComposition.getMeanAtomMass();
    QString str = "Mean atom mass: " + QString::number(MAM, 'g', 4) + " a.u.\n";
    double AtDens = tmpMaterial.density / MAM / 1.66054e-24;
    str += "Atom density: " + QString::number(AtDens, 'g', 4) + " cm-3\n";
    message(str, this);
}

void MaterialInspectorWindow::on_cbAllowAbsentCsData_clicked()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    int particleId = ui->cobParticle->currentIndex();
    tmpMaterial.MatParticle[particleId].bAllowAbsentCsData = ui->cbAllowAbsentCsData->isChecked();

    FillNeutronTable();
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbAutoLoadMissingNeutronCrossSections_clicked()
{
    autoloadMissingCrossSectionData();

    FillNeutronTable();
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbReloadAllNeutronCSs_clicked()
{
    autoloadMissingCrossSectionData(true);

    FillNeutronTable();
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbHelpNeutron_clicked()
{
    QString dir = OptionsConfigurator->getCrossSectionFirstDataDir();
    if (!dir.isEmpty())
       QDesktopServices::openUrl(QUrl("file:///"+dir, QUrl::TolerantMode));

    QDialog* d = new QDialog(this);

    QVBoxLayout* l = new QVBoxLayout();
        QString s = "ANTS2 was designed to simularte detectors for thermal neutrons only.\n";
        s += "Only two processes are considered: absorption and elastic scattering.\n";
        s += "Absorption can be followed with decay of the atom which captured the neutron.\n";
        s += "In this case the user can configure an arbitrary number of decay reactions and\n";
        s += "configure the secondary particles and their energies.\n\n";

        s += "The cross-section files can be downloaded, e.g., from IAEA site:\n";
        s += "https://www-nds.iaea.org/exfor/endf.htm\n\n";
        s += "For elastic cross-section, select N,EL reaction\n";
        s += "For total absorption, use N,NON reaction\n";
        s += "As 'Quantity' parameter, provide SIG\n";
        s += "Note that one data request can contain several isotopes, reactions and libraries selected.";
        QLabel* lab = new QLabel(s);
        l->addWidget(lab);
        QPushButton* pbS = new QPushButton("Go to IAEA site");
        l->addWidget(pbS);
        QPushButton* pbClose = new QPushButton("Close");
        l->addWidget(pbClose);
        pbClose->setDefault(true);
     d->setLayout(l);

     QFont f = lab->font();
     f.setPointSize(f.pointSize()+2);
     lab->setFont(f);

     connect(pbS, &QPushButton::clicked, []()
     {
         QDesktopServices::openUrl( QUrl("https://www-nds.iaea.org/exfor/endf.htm") );
     });
     connect(pbClose, SIGNAL(clicked(bool)), d, SLOT(accept()));

     d->exec();
}

void MaterialInspectorWindow::on_trwChemicalComposition_doubleClicked(const QModelIndex & /*index*/)
{
    if (!ui->cbShowIsotopes->isChecked())
    {
        ui->cbShowIsotopes->setChecked(true);
        ShowTreeWithChemicalComposition();
    }
}

void MaterialInspectorWindow::on_lePriT_editingFinished()
{
    if (bMessageLock) return;
    parseDecayOrRaiseTime(true);
}

void MaterialInspectorWindow::on_lePriT_raise_editingFinished()
{
    if (bMessageLock) return;
    parseDecayOrRaiseTime(false);
}

bool MaterialInspectorWindow::parseDecayOrRaiseTime(bool doParseDecay)
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    QString s = ( doParseDecay ? ui->lePriT->text() : ui->lePriT_raise->text() );
    s = s.simplified();

    QVector<APair_ValueAndWeight> & vec =
            ( doParseDecay ? tmpMaterial.PriScint_Decay : tmpMaterial.PriScint_Raise);

    vec.clear();
    bool bErrorDetected = false;

    bool bSingle;
    double tau = s.toDouble(&bSingle);
    if (bSingle)
        vec << APair_ValueAndWeight(tau, 1.0);
    else
    {
        QStringList sl = s.split('&', QString::SkipEmptyParts);

        for (const QString& sr : sl)
        {
            QStringList oneTau = sr.split(':', QString::SkipEmptyParts);
            if (oneTau.size() == 2)
            {
                bool bOK1, bOK2;
                double tau    = oneTau.at(0).toDouble(&bOK1);
                double weight = oneTau.at(1).toDouble(&bOK2);
                if (bOK1 && bOK2)
                    vec << APair_ValueAndWeight(tau, weight);
                else
                {
                    bErrorDetected = true;
                    break;
                }
            }
            else bErrorDetected = true;
        }
        if (vec.isEmpty()) bErrorDetected = true;
    }

    if (bErrorDetected)
    {
        bMessageLock = true;
        QString s = ( doParseDecay ? "Decay" : "Raise" );
        s += " time format error:\n\nUse a single double value of the time constant or,\n"
             "to define several exponential components, use this format:\n"
             "\n time_constant1 : stat_weight1  &  time_constant2 : stat_weight2  &  ...\ne.g., 25.5 : 0.25  &  250 : 0.75\n";
        message(s, this);
        bMessageLock = false;
    }
    else
        on_pbUpdateTmpMaterial_clicked();

    return !bErrorDetected;
}

void MaterialInspectorWindow::on_pbPriThelp_clicked()
{
    QString s = "The following is for both the decay and rise time generation:\n\n"
            "  If there is only one exponential component,"
            "  the time constant (\"decay time\") can be given directly.\n"
            "  To configure several exponential components, use\n"
            "  time_constant1 : stat_weight1  &  time_constant2 : stat_weight2  &  ...\n"
            "  e.g., 25.5 : 0.25  &  250 : 0.75\n"
            "  \n"
            "Model:\n"
            "  Yiping Shao, Phys. Med. Biol. 52 (2007) 1103–1117\n"
            "  The approach is generalised to have more than one rise/decay components.\n"
            "  Random generator is taken from G4Scintillation class of Geant4";
    message(s, this);
}

void MaterialInspectorWindow::on_pbPriT_test_clicked()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;

    tmpMaterial.updateRuntimeProperties(MpCollection->fLogLogInterpolation, Detector->RandGen); //to update sum of stat weights

    TH1D* h = new TH1D("h1", "", 1000, 0, 0);
    for (int i=0; i<1000000; i++)
        h->Fill( tmpMaterial.GeneratePrimScintTime(Detector->RandGen) );

    h->GetXaxis()->SetTitle("Time, ns");
    TString title = "Emission for ";
    title += tmpMaterial.name.toLatin1().data();
    h->SetTitle(title);
    MW->GraphWindow->Draw(h);
}

void MaterialInspectorWindow::on_actionNeutrons_triggered()
{
    OptionsConfigurator->setStarterDir(MW->GlobSet.LastOpenDir);
    OptionsConfigurator->showNormal();
}

void MaterialInspectorWindow::on_pbShowNcmat_clicked()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    int particleId = ui->cobParticle->currentIndex();

    MatParticleStructure& mp = tmpMaterial.MatParticle[particleId];

    NeutralTerminatorStructure& t = mp.Terminators.last();
    QString s = t.NCrystal_Ncmat;
    if (s.isEmpty()) s = "NCmat record is empty!";

    message(s, this);
}

void MaterialInspectorWindow::on_pbLoadNcmat_clicked()
{
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, "Load NCrystal ncmat file", MW->GlobSet.LastOpenDir, "Ncmat files (*.ncmat)");
    if (fileName.isEmpty()) return;

#ifdef __USE_ANTS_NCRYSTAL__
    try
    {
            NCrystal::disableCaching();
            const NCrystal::Scatter * sc = NCrystal::createScatter( fileName.toLatin1().data() );
            sc->ref();
            sc->unref();
    }
    catch (...)
    {
        message("NCrystal has rejected the provided configuration file!", this);
        return;
    }
#endif


    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    int particleId = ui->cobParticle->currentIndex();
    MatParticleStructure& mp = tmpMaterial.MatParticle[particleId];
    NeutralTerminatorStructure& t = mp.Terminators.last();

    LoadTextFromFile(fileName, t.NCrystal_Ncmat);

    updateInteractionGui();
    setWasModified(true);
}

void MaterialInspectorWindow::on_cbUseNCrystal_clicked(bool checked)
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    int particleId = ui->cobParticle->currentIndex();
    MatParticleStructure& mp = tmpMaterial.MatParticle[particleId];

    mp.bUseNCrystal = checked;

    FillNeutronTable();

#ifndef __USE_ANTS_NCRYSTAL__
    if (checked)
        message("ANTS2 was compiled without support for NCrystal library, check ants2.pro file", this);
#endif
}

void MaterialInspectorWindow::on_ledNCmatDcutoff_editingFinished()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    int particleId = ui->cobParticle->currentIndex();
    MatParticleStructure& mp = tmpMaterial.MatParticle[particleId];
    NeutralTerminatorStructure& t = mp.Terminators.last();

    t.NCrystal_Dcutoff = ui->ledNCmatDcutoff->text().toDouble();
}

void MaterialInspectorWindow::on_ledNcmatPacking_editingFinished()
{
    AMaterial& tmpMaterial = MpCollection->tmpMaterial;
    int particleId = ui->cobParticle->currentIndex();
    MatParticleStructure& mp = tmpMaterial.MatParticle[particleId];
    NeutralTerminatorStructure& t = mp.Terminators.last();

    t.NCrystal_Packing = ui->ledNcmatPacking->text().toDouble();
}

void MaterialInspectorWindow::on_cbUseNCrystal_toggled(bool checked)
{
#ifndef __USE_ANTS_NCRYSTAL__
    if (checked) ui->cbUseNCrystal->setStyleSheet("QCheckBox { color: red }");
    else ui->cbUseNCrystal->setStyleSheet("QCheckBox { color: gray }");
#endif
}

void MaterialInspectorWindow::on_pbCopyPrYieldToAll_clicked()
{
    if (!confirm("Set the same primary yield value for all particles?", this)) return;

    AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    double prYield = ui->ledPrimaryYield->text().toDouble();
    for (int iP = 0; iP < tmpMaterial.MatParticle.size(); iP++)
            tmpMaterial.MatParticle[iP].PhYield = prYield;
    tmpMaterial.PhotonYieldDefault = prYield;
    setWasModified(true);
}

void MaterialInspectorWindow::on_pbCopyIntrEnResToAll_clicked()
{
    if (!confirm("Set the same intrinsic energy resolution value for all particles?", this)) return;

    AMaterial & tmpMaterial = MpCollection->tmpMaterial;
    double EnRes = ui->ledIntEnergyRes->text().toDouble();
    for (int iP = 0; iP < tmpMaterial.MatParticle.size(); iP++)
            tmpMaterial.MatParticle[iP].IntrEnergyRes = EnRes;
    tmpMaterial.IntrEnResDefault = EnRes;
    setWasModified(true);
}

void MaterialInspectorWindow::on_cbTrackingAllowed_clicked()
{
    updateEnableStatus();
}

void MaterialInspectorWindow::on_cbTransparentMaterial_clicked()
{
    updateEnableStatus();
}

void MaterialInspectorWindow::on_pbSecScintHelp_clicked()
{
    QString s = "Diffusion is NOT active in \"Only photons\" simulation mode!\n"
            "\n"
            "If drift velosity is set to 0, diffusion is disabled in this material!\n"
            "\n"
            "Warning!\n"
            "There are no checks for travel back in time and superluminal speed of electrons!";
    message(s, this);
}

void MaterialInspectorWindow::on_pteComments_textChanged()
{
    if (!flagDisreguardChange) setWasModified(true);
}

#include "amaterialloader.h"
void MaterialInspectorWindow::AddMaterialFromLibrary(QWidget * parentWidget)
{
    AMaterialLoader ML(*MpCollection);

    bool bLoaded = ML.LoadTmpMatFromGui(parentWidget);
    if (!bLoaded) return;

    const QString name = MpCollection->tmpMaterial.name;
    MW->ReconstructDetector(true);   // TODO: go for detector directly  --> move to loader
    int index = MpCollection->FindMaterial(name);

    showMaterial(index);
}

void MaterialInspectorWindow::on_actionLoad_from_material_library_triggered()
{
    if (bMaterialWasModified)
    {
        int res = QMessageBox::question(this, "Add new material", "All unsaved changes will be lost. Continue?", QMessageBox::Yes | QMessageBox::Cancel);
        if (res == QMessageBox::Cancel)
            return;
    }

    AddMaterialFromLibrary(this);
}

void MaterialInspectorWindow::on_actionAdd_default_material_triggered()
{
    if (bMaterialWasModified)
    {
        int res = QMessageBox::question(this, "Add new material", "All unsaved changes will be lost. Continue?", QMessageBox::Yes | QMessageBox::Cancel);
        if (res == QMessageBox::Cancel)
            return;
    }

    MpCollection->AddNewMaterial("Not_defined", true);
    MW->ReconstructDetector(true);

    int index = ui->cobActiveMaterials->count() - 1;
    if (index > -1)
        showMaterial(index);
}
