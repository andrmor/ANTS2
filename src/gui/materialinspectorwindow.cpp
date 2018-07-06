#include "materialinspectorwindow.h"
#include "ui_materialinspectorwindow.h"

#include "mainwindow.h"
#include "amaterialparticlecolection.h"
#include "graphwindowclass.h"
#include "windownavigatorclass.h"
#include "outputwindow.h"
#include "detectorclass.h"
#include "globalsettingsclass.h"
#include "ajsontools.h"
#include "afiletools.h"
#include "amessage.h"
#include "acommonfunctions.h"
#include "guiutils.h"
#include "ainternetbrowser.h"
#include "amatparticleconfigurator.h"
#include "amaterialcomposition.h"
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

//Root
#include "TGraph.h"
#include "TH1.h"
#include "TAxis.h"
#include "TGeoManager.h"
#include "TMultiGraph.h"
#include "TAttLine.h"
#include "TAttMarker.h"

MaterialInspectorWindow::MaterialInspectorWindow(QWidget* parent, MainWindow *mw, DetectorClass* detector) :
    QMainWindow(parent),
    ui(new Ui::MaterialInspectorWindow)
{
    MW = mw;
    Detector = detector;
    ui->setupUi(this);
    this->move(15,15);
    //this->setFixedSize(this->size());
    this->setFixedWidth(this->width());
    bClearInProgress = false;
    NeutronInfoDialog = 0;

    Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
    windowFlags |= Qt::WindowCloseButtonHint;
    this->setWindowFlags( windowFlags );

    SetWasModified(false);
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

    //QString styleGrey = "QLabel { background-color: #F0F0F0; }";  //setting grey background color
    //ui->laBackground->setStyleSheet(styleGrey);
    //ui->laBackground_2->setStyleSheet(styleGrey);
    //ui->laBackground_3->setStyleSheet(styleGrey);

    flagDisreguardChange = false;
    fLockTable = false;
    LastSelectedParticle = 0;

    //RedIcon = createColorCircleIcon(ui->labIsotopeDensityNotSet->size(), Qt::red);
    //QIcon YellowIcon = createColorCircleIcon(ui->labNeutra_TotalInteractiondataMissing->size(), Qt::yellow);
    //ui->labAutoLoadElastic->setPixmap(YellowIcon.pixmap(16,16));
    //ui->labAutoLoadElastic->setVisible(false);

    QString str = "Open XCOM page by clicking the button below.\n"
                  "Select the material composition and generate the file (use cm2/g units).\n"
                  "Copy the entire page including the header to a text file.\n"
                  "Import the file by clicking \"Import from XCOM\" button.";
    ui->pbImportXCOM->setToolTip(str);

    OptionsConfigurator = new AMatParticleConfigurator(MW->GlobSet, this);
}

MaterialInspectorWindow::~MaterialInspectorWindow()
{
    if (NeutronInfoDialog)
    {
        ANeutronInfoDialog* NeutronInfoDialogCopy = NeutronInfoDialog;
        NeutronInfoDialog = 0;
        delete NeutronInfoDialogCopy;
    }

    delete OptionsConfigurator;
    delete ui;
}

void MaterialInspectorWindow::SetWasModified(bool flag)
{
  QString s = "  ";
  if (flag) s = "<html><head/><body><p><span style=\" font-size:10pt; color:#ff0000;\">Material was modified: Click one of above to confirm</span></p></body></html>";
  ui->labMatWasModified->setText(s);
}

void MaterialInspectorWindow::UpdateActiveMaterials()
{   
    int current = ui->cobActiveMaterials->currentIndex();

    ui->cobActiveMaterials->clear();    
    for (int i=0;i<MW->MpCollection->countMaterials();i++)
        ui->cobActiveMaterials->addItem( (*MW->MpCollection)[i]->name);

    if (current>-1 && current<ui->cobActiveMaterials->count())
    {
        ui->cobActiveMaterials->setCurrentIndex(current);
        on_cobActiveMaterials_activated(current);
        UpdateIndicationTmpMaterial();
    }
}

void MaterialInspectorWindow::on_pbAddNewMaterial_clicked()
{
   MaterialInspectorWindow::on_pbAddToActive_clicked(); //processes both update and add
}

void MaterialInspectorWindow::on_pbAddToActive_clicked()
{
    if ( !parseDecayTime() ) return;

    MW->MpCollection->tmpMaterial.updateRuntimeProperties(MW->MpCollection->fLogLogInterpolation);

    //checkig this material
    QString error = MW->MpCollection->CheckTmpMaterial();
    if (!error.isEmpty())
      {
        message(error, this);
        return;
      }

    QString name = MW->MpCollection->tmpMaterial.name;
    int index = MW->MpCollection->FindMaterial(name);
    if (index > -1)
      {
//        qDebug()<<"This name already in use! index = "<<index;
        switch( QMessageBox::information( this, "", "Update properties for material "+name+"?", "Overwrite", "Cancel", 0, 1 ) )
          {
           case 0:  break;  //overwrite
           default: return; //cancel
          }        
      }
    else
      {
        //new material
        index = MW->MpCollection->countMaterials(); //then it will be appended, so index is = current size
        MW->AddMaterialToCOBs(MW->MpCollection->tmpMaterial.name);
      }
    MW->MpCollection->CopyTmpToMaterialCollection(); //if absent, new material is created!
    MW->UpdateMaterialListEdit();

    ui->cobActiveMaterials->setCurrentIndex(index);
    SetWasModified(false);

    MW->ReconstructDetector(true);
}

void MaterialInspectorWindow::on_pbShowTotalInteraction_clicked()
{
    MaterialInspectorWindow::ShowTotalInteraction();
}

void MaterialInspectorWindow::on_pbShowStoppingPower_clicked()
{
    MaterialInspectorWindow::ShowTotalInteraction();
}

void MaterialInspectorWindow::setLogLog(bool flag)
{
  ui->actionUse_log_log_interpolation->setChecked(flag);
}

void MaterialInspectorWindow::on_cobActiveMaterials_activated(int index)
{
    if ( MW->MpCollection->countMaterials() == 0) return;
    if ( (*MW->MpCollection)[index]->MatParticle.size() == 0)
      {
        qWarning()<<"Error: MatParticle is empty for this material!";
        return;
      }
    MW->MpCollection->CopyMaterialToTmp(index);

    MaterialInspectorWindow::UpdateIndicationTmpMaterial();

//    //resetting particle-related section
//    int indexToSet = (LastSelectedParticle < Detector->MpCollection->countParticles()) ? LastSelectedParticle : 0;
//    ui->cobParticle->setCurrentIndex(indexToSet);
//    on_pbUpdateInteractionIndication_clicked();

    SetWasModified(false);
    ui->pbRename->setText("Rename "+ui->cobActiveMaterials->currentText());
}

void MaterialInspectorWindow::UpdateWaveButtons()
{   
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

  if (tmpMaterial.PrimarySpectrum_lambda.size()>0)
    {
      ui->pbShowPrimSpectrum->setEnabled(true);
      ui->pbDeletePrimSpectrum->setEnabled(true);
    }
  else
    {
      ui->pbShowPrimSpectrum->setEnabled(false);
      ui->pbDeletePrimSpectrum->setEnabled(false);
    }

  if (tmpMaterial.SecondarySpectrum_lambda.size()>0)
    {
      ui->pbShowSecSpectrum->setEnabled(true);
      ui->pbDeleteSecSpectrum->setEnabled(true);
    }
  else
    {
      ui->pbShowSecSpectrum->setEnabled(false);
      ui->pbDeleteSecSpectrum->setEnabled(false);
    }

  if (tmpMaterial.nWave_lambda.size()>0)
    {
      ui->pbShowNlambda->setEnabled(true);
      ui->pbDeleteNlambda->setEnabled(true);
    }
  else
    {
      ui->pbShowNlambda->setEnabled(false);
      ui->pbDeleteNlambda->setEnabled(false);
    }

  if (tmpMaterial.absWave_lambda.size()>0)
    {
      ui->pbShowABSlambda->setEnabled(true);
      ui->pbDeleteABSlambda->setEnabled(true);
    }
  else
    {
      ui->pbShowABSlambda->setEnabled(false);
      ui->pbDeleteABSlambda->setEnabled(false);
    }

  ui->pbShowReemProbLambda->setEnabled( !tmpMaterial.reemisProbWave_lambda.isEmpty() );
  ui->pbDeleteReemisProbLambda->setEnabled( !tmpMaterial.reemisProbWave_lambda.isEmpty() );
}

void MaterialInspectorWindow::UpdateIndicationTmpMaterial()
{
    //qDebug() << "UpdateIndicationTmpMaterial triggered";
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

    QString str;
    ui->leName->setText(tmpMaterial.name);  

    str.setNum(tmpMaterial.density, 'g');
    ui->ledDensity->setText(str);

    ui->leChemicalComposition->setText( tmpMaterial.ChemicalComposition.getCompositionString() );
    ShowTreeWithChemicalComposition();
    tmpMaterial.updateNeutronDataOnCompositionChange(MW->MpCollection);

    str.setNum(tmpMaterial.n, 'g');
    ui->ledN->setText(str);
    str.setNum(tmpMaterial.abs, 'g');
    ui->ledAbs->setText(str);
    str.setNum(tmpMaterial.reemissionProb, 'g');
    ui->ledReemissionProbability->setText(str);

    if (tmpMaterial.rayleighMFP > 0)  ui->ledRayleigh->setText(QString::number(tmpMaterial.rayleighMFP));
    else ui->ledRayleigh->setText("");
    ui->ledRayleighWave->setText(QString::number(tmpMaterial.rayleighWave));

    ui->ledPriT_raise->setText( QString::number(tmpMaterial.PriScintRaiseTime) );
    ui->cobPriT_model->setCurrentIndex(tmpMaterial.PriScintModel);
    if (tmpMaterial.PriScintDecayTimeVector.size() == 0)
        str = "0";
    else if (tmpMaterial.PriScintDecayTimeVector.size() == 1)
        str = QString::number(tmpMaterial.PriScintDecayTimeVector.first().second);
    else
    {
        str.clear();
        for (const QPair<double,double>& pair : tmpMaterial.PriScintDecayTimeVector)
        {
            str += QString::number(pair.first);
            str += ":";
            str += QString::number(pair.second);
            str += " & ";
        }
        str.chop(3);
    }
    ui->lePriT->setText(str);

    str.setNum(tmpMaterial.W*1000.0, 'g'); //keV->eV
    ui->ledW->setText(str);
    str.setNum(tmpMaterial.SecYield, 'g');
    ui->ledSecYield->setText(str);
    str.setNum(tmpMaterial.SecScintDecayTime, 'g');
    ui->ledSecT->setText(str);    
    str.setNum(tmpMaterial.e_driftVelocity, 'g');
    ui->ledEDriftVelocity->setText(str);

    int tmp = LastSelectedParticle;
    ui->cobParticle->clear();    
    int lastSelected_cobYieldForParticle = ui->cobYieldForParticle->currentIndex();
    ui->cobYieldForParticle->clear();
    int numPart = Detector->MpCollection->countParticles();
    for (int i=0; i<numPart; i++)
      {
        const QString name = Detector->MpCollection->getParticleName(i);
        ui->cobParticle->addItem(name);
        ui->cobYieldForParticle->addItem(name);
      }
    if (lastSelected_cobYieldForParticle<0) lastSelected_cobYieldForParticle = 0;
    if (lastSelected_cobYieldForParticle<numPart) ui->cobYieldForParticle->setCurrentIndex(lastSelected_cobYieldForParticle);

    int iPartForYield = ui->cobYieldForParticle->currentIndex();
    if (iPartForYield<0)
      {
        iPartForYield = 0;
        ui->cobYieldForParticle->setCurrentIndex(0);
      }
    double val = tmpMaterial.MatParticle.at(iPartForYield).PhYield;
    ui->ledPrimaryYield->setText(QString::number(val));
    ui->cbSameYieldForAll->setChecked( isAllSameYield(val) );

    LastSelectedParticle = tmp;
    if (LastSelectedParticle < numPart) ui->cobParticle->setCurrentIndex(LastSelectedParticle);
    else ui->cobParticle->setCurrentIndex(0);
    on_pbUpdateInteractionIndication_clicked();

    MaterialInspectorWindow::UpdateWaveButtons();
}

void MaterialInspectorWindow::on_pbUpdateInteractionIndication_clicked()
{
  //qDebug() << "on_pbUpdateIndication_clicked";
  int particleId = ui->cobParticle->currentIndex();
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

  flagDisreguardChange = true; //to skip auto-update "modified!" sign

  if (particleId <0 || particleId > MW->MpCollection->countParticles()-1)
  {  //on bad particle index
      ui->pbShowTotalInteraction->setEnabled(false);
      flagDisreguardChange = false;
      return;
  }

  const MatParticleStructure& mp = tmpMaterial.MatParticle.at(particleId);

  LastSelectedParticle = particleId;
  bool TrackingAllowed = mp.TrackingAllowed;
  ui->cbTrackingAllowed->setChecked(TrackingAllowed);
  ui->fEnDepProps->setEnabled(TrackingAllowed);

  bool MaterialIsTransparent = mp.MaterialIsTransparent;
  ui->cbTransparentMaterial->setChecked(MaterialIsTransparent);
  ui->swMainMatParticle->setEnabled(!MaterialIsTransparent);
  QFont font = ui->cbTransparentMaterial->font();
  font.setBold(MaterialIsTransparent);
  ui->cbTransparentMaterial->setFont(font);

  ui->ledIntEnergyRes->setText( QString::number(mp.IntrEnergyRes) );

  ui->pbShowTotalInteraction->setEnabled(true);

  const AParticle::ParticleType type = Detector->MpCollection->getParticleType(particleId);

  if (type == AParticle::_charged_)
  {
     //charged particle
     ui->swMainMatParticle->setCurrentIndex(0);
     if (mp.InteractionDataX.size()>0) ui->pbShowStoppingPower->setEnabled(true);
     else ui->pbShowStoppingPower->setEnabled(false);
     on_ledMFPenergy_2_editingFinished();
  }
  else //neutral particles
  {      
      ui->swMainMatParticle->setCurrentIndex(1);

      if (type == AParticle::_neutron_)
      {
          ui->swNeutral->setCurrentIndex(1);
          ui->cbCapture->setChecked(mp.bCaptureEnabled);
          ui->cbEnableScatter->setChecked(mp.bEllasticEnabled);
          ui->cbAllowAbsentCsData->setChecked(mp.bAllowAbsentCsData);

          FillNeutronTable();
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
          message("Critical error: unknown neutral particle", this);
          flagDisreguardChange = false;
          return;
        }
  }

  flagDisreguardChange = false;
}

void MaterialInspectorWindow::on_pbUpdateTmpMaterial_clicked()
{  
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

    tmpMaterial.name = ui->leName->text();
    tmpMaterial.density = ui->ledDensity->text().toDouble();    
    tmpMaterial.n = ui->ledN->text().toDouble();
    tmpMaterial.abs = ui->ledAbs->text().toDouble();
    tmpMaterial.reemissionProb = ui->ledReemissionProbability->text().toDouble();
    tmpMaterial.PriScintRaiseTime = ui->ledPriT_raise->text().toDouble();
    tmpMaterial.PriScintModel = ui->cobPriT_model->currentIndex();

    double prYield = ui->ledPrimaryYield->text().toDouble();
    if (ui->cbSameYieldForAll->isChecked())
      {
        for (int iP=0; iP<MW->MpCollection->countParticles(); iP++)
          tmpMaterial.MatParticle[iP].PhYield = prYield;
      }
    else tmpMaterial.MatParticle[ui->cobYieldForParticle->currentIndex()].PhYield = prYield;

    const int ParticleId = ui->cobParticle->currentIndex();
    tmpMaterial.MatParticle[ParticleId].TrackingAllowed = ui->cbTrackingAllowed->isChecked();
    tmpMaterial.MatParticle[ParticleId].bCaptureEnabled = ui->cbCapture->isChecked();
    tmpMaterial.MatParticle[ParticleId].bEllasticEnabled = ui->cbEnableScatter->isChecked();

    tmpMaterial.W = ui->ledW->text().toDouble()*0.001; //eV -> keV
    tmpMaterial.SecYield = ui->ledSecYield->text().toDouble();
    tmpMaterial.SecScintDecayTime = ui->ledSecT->text().toDouble();   
    tmpMaterial.e_driftVelocity = ui->ledEDriftVelocity->text().toDouble();

    on_ledGammaDiagnosticsEnergy_editingFinished(); //gamma - update MFP
    on_ledMFPenergy_2_editingFinished();            //charged - update projected range
}

void MaterialInspectorWindow::on_pbLoadDeDr_clicked()
{
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Load dE/dr data", MW->GlobSet->LastOpenDir, "Data files (*.dat)");

  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

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
  on_pbUpdateInteractionIndication_clicked();
  on_pbWasModified_clicked();
}

void MaterialInspectorWindow::SetParticleSelection(int index)
{
  ui->cobParticle->setCurrentIndex(index);
  on_pbUpdateInteractionIndication_clicked();
}

void MaterialInspectorWindow::SetMaterial(int index)
{
  ui->cobActiveMaterials->setCurrentIndex(index);
  on_cobActiveMaterials_activated(index);

  UpdateIndicationTmpMaterial();
}

void MaterialInspectorWindow::on_pbLoadThisScenarioCrossSection_clicked()
{
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, "Load mass interaction coefficient data.\n"
                                            "The file should contain 4 colums: energy[keV], photoelectric_data[cm2/g], compton_data[cm2/g], pair_production_data[cm2/g]", MW->GlobSet->LastOpenDir, "Data files (*.dat *.txt);;All files(*)");

    if (fileName.isEmpty()) return;
    MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
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

    on_pbUpdateInteractionIndication_clicked();
    on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_ledIntEnergyRes_editingFinished()
{
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

    double newVal = ui->ledIntEnergyRes->text().toDouble();
    if (newVal<0)
    {
        ui->ledIntEnergyRes->setText( QString::number(tmpMaterial.MatParticle[ui->cobParticle->currentIndex()].IntrEnergyRes, 'g', 4) );
        message("Intrinsic energy resolution cannot be negative", this);
    }
    else
        tmpMaterial.MatParticle[ui->cobParticle->currentIndex()].IntrEnergyRes = newVal;
}

void MaterialInspectorWindow::on_pbImportStoppingPowerFromTrim_clicked()
{
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, "Import stopping power data from a Trim file", MW->GlobSet->LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*)");
    //qDebug()<<fileName;
    if (fileName.isEmpty()) return;
    MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

    int particleId = ui->cobParticle->currentIndex();
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

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

            MaterialInspectorWindow::on_pbWasModified_clicked();
        }

        if (flagWrongTermination)
        {        
            message("Unexpected termination!");
            return;
        }

        MW->Owindow->OutText("File "+fileName+ " with Trim data has been loaded.");
        file.close();
        ui->pbShowStoppingPower->setEnabled(true);
        on_pbUpdateInteractionIndication_clicked();
}

void MaterialInspectorWindow::on_pbImportXCOM_clicked()
{
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Import partial interaction coefficients from an XCOM file", MW->GlobSet->LastOpenDir, "Text and data files (*.dat *.txt);;All files (*)");
  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
      {
          message("Cannot open "+fileName, this);
          return;
      }
  QTextStream in(&file);

  int particleId = ui->cobParticle->currentIndex();
  importXCOM(in, particleId);
  on_pbUpdateInteractionIndication_clicked();
  on_pbWasModified_clicked();
}

bool MaterialInspectorWindow::importXCOM(QTextStream &in, int particleId)
{
  if (Detector->MpCollection->getParticleType(particleId) != AParticle::_gamma_)
    {
      message("It is not gamma particle!", this);
      return false;
    }

  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

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

  MaterialInspectorWindow::on_pbWasModified_clicked();
  return true;
}

bool MaterialInspectorWindow::isAllSameYield(double val)
{  
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

  for (int iP=0; iP<MW->MpCollection->countParticles(); iP++)
      if (tmpMaterial.MatParticle.at(iP).PhYield != val) return false;
  return true;
}

void MaterialInspectorWindow::AddMatToCobs(QString str)
{
   ui->cobActiveMaterials->addItem(str);  
}

void MaterialInspectorWindow::on_pbLoadPrimSpectrum_clicked()
{
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Load primary scintillation spectrum", MW->GlobSet->LastOpenDir, "Data files (*.dat *.txt);;All files (*.*)");

  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

  LoadDoubleVectorsFromFile(fileName, &tmpMaterial.PrimarySpectrum_lambda, &tmpMaterial.PrimarySpectrum);  //cleans previous data

  int numPoints = tmpMaterial.PrimarySpectrum_lambda.size();
  QString str;
  str.setNum(numPoints);
  MW->Owindow->OutText(fileName + " - loaded "+str+" data points");

  if (numPoints>0)
    {
      ui->pbShowPrimSpectrum->setEnabled(true);
      ui->pbDeletePrimSpectrum->setEnabled(true);     
    }
  else
    {
      ui->pbShowPrimSpectrum->setEnabled(false);
      ui->pbDeletePrimSpectrum->setEnabled(false);
    }
  MaterialInspectorWindow::on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbShowPrimSpectrum_clicked()
{
  //MW->ShowGraphWindow();
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  MW->GraphWindow->MakeGraph(&tmpMaterial.PrimarySpectrum_lambda, &tmpMaterial.PrimarySpectrum, kRed, "Wavelength, nm", "Photon flux, a.u.");
}

void MaterialInspectorWindow::on_pbDeletePrimSpectrum_clicked()
{
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  tmpMaterial.PrimarySpectrum_lambda.resize(0);
  tmpMaterial.PrimarySpectrum.resize(0); 

  ui->pbShowPrimSpectrum->setEnabled(false);
  ui->pbDeletePrimSpectrum->setEnabled(false);
  MaterialInspectorWindow::on_pbWasModified_clicked();
}

void MaterialInspectorWindow::ConvertToStandardWavelengthes(QVector<double>* sp_x, QVector<double>* sp_y, QVector<double>* y)
{
  y->resize(0);

  //qDebug()<<"Data range:"<<sp_x->at(0)<<sp_x->at(sp_x->size()-1);
  double xx, yy;
  int points = MW->WaveNodes;
  for (int i=0; i<points; i++)
    {
      xx = MW->WaveFrom + MW->WaveStep*i;
      if (xx <= sp_x->at(0)) yy = sp_y->at(0);
      else
        {
          if (xx >= sp_x->at(sp_x->size()-1)) yy = sp_y->at(sp_x->size()-1);
          else
            {
              //general case
              yy = GetInterpolatedValue(xx, sp_x, sp_y); //reusing interpolation function from functions.h
            }
        }
//      qDebug()<<xx<<yy;
      y->append(yy);
  }
}

//void MaterialInspectorWindow::WriteElasticAutoToJson(QJsonObject &json)
//{
//    json = QJsonObject();
//    ElasticAutoConfig->writeToJson(json);
//}

void MaterialInspectorWindow::on_pbLoadSecSpectrum_clicked()
{
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Load secondary scintillation spectrum", MW->GlobSet->LastOpenDir, "Data files (*.dat);;All files (*.*)");

  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

  LoadDoubleVectorsFromFile(fileName, &tmpMaterial.SecondarySpectrum_lambda, &tmpMaterial.SecondarySpectrum);  //cleans previous data

  int numPoints = tmpMaterial.SecondarySpectrum_lambda.size();
  QString str;
  str.setNum(numPoints); 
  MW->Owindow->OutText(fileName + " - loaded "+str+" data points");

  if (numPoints>0)
    {
      ui->pbShowSecSpectrum->setEnabled(true);
      ui->pbDeleteSecSpectrum->setEnabled(true);     
    }
  else
    {
      ui->pbShowSecSpectrum->setEnabled(false);
      ui->pbDeleteSecSpectrum->setEnabled(false);
    }
  MaterialInspectorWindow::on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbShowSecSpectrum_clicked()
{
  //MW->ShowGraphWindow();
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  MW->GraphWindow->MakeGraph(&tmpMaterial.SecondarySpectrum_lambda, &tmpMaterial.SecondarySpectrum, kRed, "Wavelength, nm", "Photon flux, a.u.");
}

void MaterialInspectorWindow::on_pbDeleteSecSpectrum_clicked()
{
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  tmpMaterial.SecondarySpectrum_lambda.resize(0);
  tmpMaterial.SecondarySpectrum.resize(0);  

  ui->pbShowSecSpectrum->setEnabled(false);
  ui->pbDeleteSecSpectrum->setEnabled(false);
  MaterialInspectorWindow::on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbLoadNlambda_clicked()
{
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Load refractive index data", MW->GlobSet->LastOpenDir, "Data files (*.dat *.txt);;All files (*.*)");

  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

  LoadDoubleVectorsFromFile(fileName, &tmpMaterial.nWave_lambda, &tmpMaterial.nWave);  //cleans previous data too

  int numPoints = tmpMaterial.nWave_lambda.size();
  QString str;
  str.setNum(numPoints);
  MW->Owindow->OutText(fileName + " - loaded "+str+" data points");

  if (numPoints>0)
    {
      ui->pbShowNlambda->setEnabled(true);
      ui->pbDeleteNlambda->setEnabled(true);    
    }
  else
    {
      ui->pbShowNlambda->setEnabled(false);
      ui->pbDeleteNlambda->setEnabled(false);
    }
  MaterialInspectorWindow::on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbShowNlambda_clicked()
{
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  MW->GraphWindow->MakeGraph(&tmpMaterial.nWave_lambda, &tmpMaterial.nWave, kRed, "Wavelength, nm", "Refractive index");
}

void MaterialInspectorWindow::on_pbDeleteNlambda_clicked()
{
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  tmpMaterial.nWave_lambda.resize(0);
  tmpMaterial.nWave.resize(0);  

  ui->pbShowNlambda->setEnabled(false);
  ui->pbDeleteNlambda->setEnabled(false);

  MaterialInspectorWindow::on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbLoadABSlambda_clicked()
{
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Load exponential bulk absorption data", MW->GlobSet->LastOpenDir, "Data files (*.dat *.txt);;All files (*.*)");

  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

  LoadDoubleVectorsFromFile(fileName, &tmpMaterial.absWave_lambda, &tmpMaterial.absWave);  //cleans previous data too

  int numPoints = tmpMaterial.absWave_lambda.size();
  QString str;
  str.setNum(numPoints);
  MW->Owindow->OutText(fileName + " - loaded "+str+" data points");

  if (numPoints>0)
    {
      ui->pbShowABSlambda->setEnabled(true);
      ui->pbDeleteABSlambda->setEnabled(true);     
    }
  else
    {
      ui->pbShowABSlambda->setEnabled(false);
      ui->pbDeleteABSlambda->setEnabled(false);
    }
  MaterialInspectorWindow::on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbShowABSlambda_clicked()
{
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  MW->GraphWindow->MakeGraph(&tmpMaterial.absWave_lambda, &tmpMaterial.absWave, kRed, "Wavelength, nm", "Attenuation coefficient, mm-1");
}

void MaterialInspectorWindow::on_pbDeleteABSlambda_clicked()
{
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  tmpMaterial.absWave_lambda.resize(0);
  tmpMaterial.absWave.resize(0); 

  ui->pbShowABSlambda->setEnabled(false);
  ui->pbDeleteABSlambda->setEnabled(false);
  MaterialInspectorWindow::on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbShowReemProbLambda_clicked()
{
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
    MW->GraphWindow->MakeGraph(&tmpMaterial.reemisProbWave_lambda, &tmpMaterial.reemisProbWave, kRed, "Wavelength, nm", "Reemission probability");
}

void MaterialInspectorWindow::on_pbLoadReemisProbLambda_clicked()
{
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, "Load reemission probability vs wavelength", MW->GlobSet->LastOpenDir, "Data files (*.dat *.txt);;All files (*.*)");

    if (fileName.isEmpty()) return;
    MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

    LoadDoubleVectorsFromFile(fileName, &tmpMaterial.reemisProbWave_lambda, &tmpMaterial.reemisProbWave);  //cleans previous data too

    int numPoints = tmpMaterial.reemisProbWave_lambda.size();
    QString str;
    str.setNum(numPoints);
    MW->Owindow->OutText(fileName + " - loaded "+str+" data points");

    ui->pbShowReemProbLambda->setEnabled(numPoints>0);
    ui->pbDeleteReemisProbLambda->setEnabled(numPoints>0);
    on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbDeleteReemisProbLambda_clicked()
{
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
    tmpMaterial.reemisProbWave_lambda.clear();
    tmpMaterial.reemisProbWave.clear();

    ui->pbShowReemProbLambda->setEnabled(false);
    ui->pbDeleteReemisProbLambda->setEnabled(false);
    on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbWasModified_clicked()
{
  if (flagDisreguardChange) return;
  SetWasModified(true);

  UpdateActionButtons();
}

bool MaterialInspectorWindow::event(QEvent * e)
{
 //  if (!MW->isVisible()) return QMainWindow::event(e); //if we are closing the application

    switch(e->type())
    {
      /*
        case QEvent::WindowActivate :
         {
            //focussed!
//            qDebug()<<"in";
            if (MW->GeometryWindow->isVisible())
               {
                 MaterialStructure& tmpMaterial = MW->MaterialCollection->tmpMaterial;
                 QString name = tmpMaterial.name;
                 int size = MW->MaterialCollection->size();
                 bool flagFound = false;
                 int index;
                 for (index = 0; index < size; index++)
                    if (!name.compare( (*MW->MaterialCollection)[index]->name,Qt::CaseSensitive)) {flagFound = true; break;}

                 if (!flagFound) index = 111111;
                 MW->ColorVolumes(2, index);
                 //MW->ShowGeometry(2);
                 MW->ShowGeometry(false, true, false);
                 //this->raise();
               }
            break ;
          }
        case QEvent::WindowDeactivate :
            // lost focus
            if (DoNotDefocus) break;            
//            qDebug()<<"out!";
            //if (MW->GeometryWindow->isVisible()) MW->ShowGeometry(3);
            if (MW->GeometryWindow->isVisible()) MW->ShowGeometry(false, true, true);
            break;
*/
      case QEvent::Hide :
        if (MW->WindowNavigator) MW->WindowNavigator->HideWindowTriggered("mat");
        if (OptionsConfigurator->isVisible()) OptionsConfigurator->hide();
        break;
      case QEvent::Show :
        if (MW->WindowNavigator) MW->WindowNavigator->ShowWindowTriggered("mat");
        break;
      default:
        break;
    } ;
    return QMainWindow::event(e);
}

void MaterialInspectorWindow::on_pbGammaDiagnosticsPhComptRation_clicked()
{
  int particleId = ui->cobParticle->currentIndex();
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

  if (tmpMaterial.MatParticle[particleId].Terminators.size() < 2) return;

  int elements0 = tmpMaterial.MatParticle[particleId].Terminators[0].PartialCrossSection.size();
  int elements1 = tmpMaterial.MatParticle[particleId].Terminators[1].PartialCrossSection.size();
  if (elements0 == 0 && elements1 == 0) return;

  if (elements0 !=elements1)
    {
      message("Error: mismatch in number of entries!");
      return;
    }

  if (elements0 == 0) return;
  double nan;
  QString str = "nan";
  nan = str.toDouble();

  QVector<double> *X;
  X = &tmpMaterial.MatParticle[particleId].Terminators[0].PartialCrossSectionEnergy;
  QVector<double> Y;
  for (int i=0; i<elements0; i++)
    {
      double sum = tmpMaterial.MatParticle[particleId].Terminators[0].PartialCrossSection[i] + tmpMaterial.MatParticle[particleId].Terminators[1].PartialCrossSection[i];
      if (sum == 0) Y.append(nan);
      else Y.append(tmpMaterial.MatParticle[particleId].Terminators[0].PartialCrossSection[i] / sum);
    }

  //MW->ShowGraphWindow();
  MW->GraphWindow->MakeGraph(X, &Y, kRed, "Energy, keV", "Fraction of photoelectrics");
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
  MaterialInspectorWindow::on_pbUpdateTmpMaterial_clicked();
}

void MaterialInspectorWindow::on_cbTrackingAllowed_toggled(bool checked)
{  
  ui->fEnDepProps->setEnabled(checked);

  QFont font = ui->cbTrackingAllowed->font();
  font.setBold(checked);
  ui->cbTrackingAllowed->setFont(font);

  //ui->lineClear->setVisible(checked);
  ui->fEnDepProps->setVisible(checked);  
}

void MaterialInspectorWindow::on_leName_textChanged(const QString& /*name*/)
{
    //on text change - on chage this is a signal that it will be another material. These properties are recalculated anyway on
    //accepting changes/new material
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
    tmpMaterial.absWaveBinned.resize(0);
    tmpMaterial.reemissionProbBinned.resize(0);
    tmpMaterial.nWaveBinned.resize(0);
    tmpMaterial.GeoMat = 0;  //no delete! the original material has to have them
    tmpMaterial.GeoMed = 0;
    tmpMaterial.PrimarySpectrumHist = 0;
    tmpMaterial.SecondarySpectrumHist = 0;

    UpdateActionButtons();
}

void MaterialInspectorWindow::UpdateActionButtons()
{
    //Buttons status
    QString name = ui->leName->text();
    int iMat = Detector->MpCollection->FindMaterial(name);
    if (iMat == -1)
    {
        // Material with this name does not exist
        ui->pbAddToActive->setEnabled(false);  //update button
        ui->pbAddNewMaterial->setEnabled(true);
        ui->pbRename->setEnabled(true);
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
  if (MW->MpCollection->getParticleType(particleId) != AParticle::_gamma_) return;

  ui->leoGammaDiagnosticsCoefficient->setText("n.a.");
  ui->leoMFPgamma->setText("n.a.");

  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

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
  int LogLogInterpolation = Detector->MpCollection->fLogLogInterpolation;
  double InteractionCoefficient = GetInterpolatedValue(energy, &tmpMaterial.MatParticle[particleId].InteractionDataX, &tmpMaterial.MatParticle[particleId].InteractionDataF, LogLogInterpolation);
  //qDebug()<<energy<<InteractionCoefficient;
  str.setNum(InteractionCoefficient, 'g', 4);

  double MeanFreePath = 10.0/InteractionCoefficient/Density;  //1/(cm2/g)/(g/cm3) - need in mm (so that 10.)
  str1.setNum(MeanFreePath, 'g', 4);

  ui->leoGammaDiagnosticsCoefficient->setText(str);
  ui->leoMFPgamma->setText(str1);
}

void MaterialInspectorWindow::on_cbTransparentMaterial_clicked()
{
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  int particleId = ui->cobParticle->currentIndex();
  tmpMaterial.MatParticle[particleId].MaterialIsTransparent = ui->cbTransparentMaterial->isChecked();
  on_pbUpdateInteractionIndication_clicked();
  on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbComments_clicked()
{
  QDialog* dialog = new QDialog(this);

  QString str = ui->leName->text();
  dialog->setWindowTitle(str);

  QPushButton *okButton = new QPushButton("Close");
  connect(okButton,SIGNAL(clicked()),dialog,SLOT(accept()));

  QTextEdit *text = new QTextEdit();
  text->append(MW->MpCollection->tmpMaterial.Comments);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(text);
  mainLayout->addWidget(okButton);

  dialog->setLayout(mainLayout);
  dialog->exec();

  MW->MpCollection->tmpMaterial.Comments = text->document()->toPlainText();
  SetWasModified(true);
  delete dialog;
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

    MW->MpCollection->tmpMaterial.rayleighWave = wave;
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
  MW->MpCollection->tmpMaterial.rayleighMFP = ray;
}

void MaterialInspectorWindow::on_pbRemoveRayleigh_clicked()
{
  ui->ledRayleigh->setText("");
  MW->MpCollection->tmpMaterial.rayleighMFP = 0;
  MaterialInspectorWindow::on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbShowUsage_clicked()
{
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
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
  if (!ui->labMatWasModified->text().simplified().isEmpty())
    {
      message("Material properties were modified!\nUpdate, add as new or cancel changes before renaming");
      return;
    }

  QString name = ui->leName->text();
  int aMat = ui->cobActiveMaterials->currentIndex();
  if (aMat<0) return;
  QString aName = (*Detector->MpCollection)[aMat]->name;
  if (name == aName) return; //not changed

  for (int i=0; i<Detector->MpCollection->countMaterials(); i++)
    if (i!=aMat && name==(*Detector->MpCollection)[i]->name)
      {
        message("There is already a material with name "+name, this);
        return;
      }
  (*Detector->MpCollection)[aMat]->name = name; 
  ui->pbRename->setText("Rename\n"+name);

  MW->ReconstructDetector(true);
  MaterialInspectorWindow::on_leName_textChanged(ui->leName->text());
}

void MaterialInspectorWindow::on_ledMFPenergy_2_editingFinished()
{
  /*
  int particleId = ui->cobParticle->currentIndex();
  if (MW->MpCollection->getParticleType(particleId) != AParticle::_charged_) return;

  if (ui->ledMFPenergy_2->text().isEmpty())
    {
      ui->leMFP_2->setText("");
      return;
    }

  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

  double energy;
  energy = ui->ledMFPenergy_2->text().toDouble();

  if ( tmpMaterial.MatParticle[particleId].InteractionDataX.size() < 2)
    {
      ui->leMFP->setText("no data");
      return;
    }
  if (energy<tmpMaterial.MatParticle[particleId].InteractionDataX.first() || energy > tmpMaterial.MatParticle[particleId].InteractionDataX.last())
    {
      ui->leMFP->setText("out range");
      return;
    }

  int LogLogInterpolation = Detector->MpCollection->fLogLogInterpolation;
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
  */
}

void MaterialInspectorWindow::on_actionSave_material_triggered()
{
  //checkig this material
  QString error = MW->MpCollection->CheckTmpMaterial();
  if ( !error.isEmpty() )
    {
      message(error, this);
      return;
    }

  QString starter = (MW->GlobSet->LibMaterials.isEmpty()) ? MW->GlobSet->LastOpenDir : MW->GlobSet->LibMaterials;
  int imat = ui->cobActiveMaterials->currentIndex();
  if (imat == -1) starter += "/Material_";
  else starter += "/Material_"+(*Detector->MpCollection)[imat]->name;
  QString fileName = QFileDialog::getSaveFileName(this,"Save material", starter, "Material files (*.mat *.json);;All files (*.*)");
  if (fileName.isEmpty()) return;
  QFileInfo fileInfo(fileName);
  if (fileInfo.suffix().isEmpty()) fileName += ".mat";

  QJsonObject json, js;
  Detector->MpCollection->writeMaterialToJson(imat, json);
  js["Material"] = json;
  SaveJsonToFile(js, fileName);
}

void MaterialInspectorWindow::on_actionLoad_material_triggered()
{
  QString starter = (MW->GlobSet->LibMaterials.isEmpty()) ? MW->GlobSet->LastOpenDir : MW->GlobSet->LibMaterials;
  QString fileName = QFileDialog::getOpenFileName(this, "Load material", starter, "Material files (*mat *.json);;All files (*.*)");
  if (fileName.isEmpty()) return;

  QJsonObject json, js;
  LoadJsonFromFile(json, fileName);
  if (!json.contains("Material"))
    {
      message("File format error: Json with material settings not found", this);
      return;
    }
  js = json["Material"].toObject();
  Detector->MpCollection->tmpMaterial.readFromJson(js, Detector->MpCollection);

  MaterialInspectorWindow::on_pbWasModified_clicked();
  MaterialInspectorWindow::UpdateWaveButtons();
  MW->ListActiveParticles();

  ui->cobActiveMaterials->setCurrentIndex(-1); //to avoid confusion (and update is disabled for -1)
  MaterialInspectorWindow::UpdateIndicationTmpMaterial(); //refresh indication of tmpMaterial
  MaterialInspectorWindow::UpdateWaveButtons(); //refresh button state for Wave-resolved properties
  SetParticleSelection(0);
}

void MaterialInspectorWindow::on_actionClear_Interaction_for_this_particle_triggered()
{
  QMessageBox *msgBox = new QMessageBox( this );
  msgBox->setWindowTitle("Confirmation");
  msgBox->setText("Clear interaction data for the selected material and particle?");
  msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox->setDefaultButton(QMessageBox::Yes);

  if (msgBox->exec() == QMessageBox::No)
    {
      if (msgBox) delete msgBox;
      return;
    }
  if (msgBox) delete msgBox;

    int i = ui->cobParticle->currentIndex();
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

    tmpMaterial.MatParticle[i].TrackingAllowed = true;
    tmpMaterial.MatParticle[i].MaterialIsTransparent = true;
    tmpMaterial.MatParticle[i].PhYield=0;
    tmpMaterial.MatParticle[i].IntrEnergyRes=0;
    tmpMaterial.MatParticle[i].InteractionDataX.resize(0);
    tmpMaterial.MatParticle[i].InteractionDataF.resize(0);
    tmpMaterial.MatParticle[i].Terminators.resize(0);

    tmpMaterial.MatParticle[i].DataSource.clear();
    tmpMaterial.MatParticle[i].DataString.clear();
    tmpMaterial.MatParticle[i].bCaptureEnabled = true;
    tmpMaterial.MatParticle[i].bEllasticEnabled = false;

    on_pbUpdateInteractionIndication_clicked();
    on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_actionClear_interaction_for_all_particles_triggered()
{
  QMessageBox *msgBox = new QMessageBox( this );
  msgBox->setWindowTitle("Confirmation");
  msgBox->setText("Clear interaction data for the selected material and ALL particles?");
  msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox->setDefaultButton(QMessageBox::Yes);

  if (msgBox->exec() == QMessageBox::No)
    {
      if (msgBox) delete msgBox;
      return;
    }
  if (msgBox) delete msgBox;

  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

    //for (int i=0; i<Detector->ParticleCollection.size(); i++)
    for (int i=0; i<Detector->MpCollection->countParticles(); i++)
    {
        tmpMaterial.MatParticle[i].TrackingAllowed = true;
        tmpMaterial.MatParticle[i].MaterialIsTransparent = true;
        tmpMaterial.MatParticle[i].PhYield=0;
        tmpMaterial.MatParticle[i].IntrEnergyRes=0;
        tmpMaterial.MatParticle[i].bCaptureEnabled=true;
        tmpMaterial.MatParticle[i].bEllasticEnabled=false;
        tmpMaterial.MatParticle[i].InteractionDataX.resize(0);
        tmpMaterial.MatParticle[i].InteractionDataF.resize(0);
        tmpMaterial.MatParticle[i].Terminators.resize(0);

        tmpMaterial.MatParticle[i].DataSource.clear();
        tmpMaterial.MatParticle[i].DataString.clear();
    }

    on_pbUpdateInteractionIndication_clicked();
    on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_actionUse_log_log_interpolation_triggered()
{
  Detector->MpCollection->fLogLogInterpolation = ui->actionUse_log_log_interpolation->isChecked();
  MW->ReconstructDetector(true);
}

void MaterialInspectorWindow::on_pbShowPhotoelectric_clicked()
{
  int particleId = ui->cobParticle->currentIndex();
  showProcessIntCoefficient(particleId, 0);
}

void MaterialInspectorWindow::on_pbShowCompton_clicked()
{
  int particleId = ui->cobParticle->currentIndex();
  showProcessIntCoefficient(particleId, 1);
}

void MaterialInspectorWindow::on_pbShowPairProduction_clicked()
{
    int particleId = ui->cobParticle->currentIndex();
    showProcessIntCoefficient(particleId, 2);
}

void MaterialInspectorWindow::showProcessIntCoefficient(int particleId, int TermScenario)
{
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  if (TermScenario > tmpMaterial.MatParticle[particleId].Terminators.size()-1)
  {
      message("Not defined in the current configuration!", this);
      return;
  }

  int elements = tmpMaterial.MatParticle[particleId].Terminators[TermScenario].PartialCrossSection.size();
  if (elements<1) return;

  Color_t color;
  TString title;
  switch (TermScenario)
  {
  case 0: color = kGreen; title = "Photoelectric"; break;
  case 1: color = kBlue; title = "Compton scattering"; break;
  case 2: color = kMagenta; title = "Pair production"; break;
  }
  MW->GraphWindow->SetLog(true, true);
  QVector<double> &X = tmpMaterial.MatParticle[particleId].Terminators[TermScenario].PartialCrossSectionEnergy;
  QVector<double> &Y = tmpMaterial.MatParticle[particleId].Terminators[TermScenario].PartialCrossSection;
  TGraph* gr = MW->GraphWindow->MakeGraph(&X, &Y, color, "Energy, keV", "Interaction coefficient, cm2/g", 2, 1, 1, 0, "", true);
  gr->SetTitle( title );
  MW->GraphWindow->Draw(gr, "AP");

  TGraph* graphOver = constructInterpolationGraph(X, Y);
  graphOver->SetLineColor(color);
  graphOver->SetLineWidth(1);
  MW->GraphWindow->Draw(graphOver, "SAME L");
}

void MaterialInspectorWindow::on_pbShowAllForGamma_clicked()
{
  int particleId = ui->cobParticle->currentIndex();
  AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  if (tmpMaterial.MatParticle[particleId].Terminators.size()<2) return;

  MW->GraphWindow->SetLog(true, true);

  TMultiGraph* mg = new TMultiGraph("a1", "Gamma interaction");

  for (int i=0; i<3; i++)
    {
      TString opt, title;
      Color_t color;
      int Lwidth = 1;
      if (i==0)
      {
          color = kGreen;
          opt = "AL";
          title = "Photoelectric";
          Lwidth = 2;
      }
      else if (i==1)
      {
          color = kBlue;
          opt = "L same";
          title = "Compton scattering";
          Lwidth = 1;
      }
      else
      {
          if (tmpMaterial.MatParticle[particleId].Terminators.size() == 2) break;
          color = kMagenta;
          opt = "L same";
          title = "Pair production";
          Lwidth = 1;
      }

      QVector<double> &X = tmpMaterial.MatParticle[particleId].Terminators[i].PartialCrossSectionEnergy;
      QVector<double> &Y = tmpMaterial.MatParticle[particleId].Terminators[i].PartialCrossSection;
      //have to remove zeros!
      QVector<double> x, y;
      for (int i=0; i<X.size(); i++)
      {
          if (Y.at(i)>0)
          {
              x << X.at(i);
              y << Y.at(i);
          }
      }

      TGraph* gr = constructInterpolationGraph(x, y);
      gr->SetTitle(title);
      gr->SetLineColor(color);
      gr->SetLineWidth(Lwidth);
      mg->Add(gr, opt);
    }
  MW->GraphWindow->Draw(mg, "AL");

  mg->GetXaxis()->SetTitle("Energy, keV"); //axis titles can be drawn only after graph was shown...
  mg->GetYaxis()->SetTitle("Mass interaction coefficient, cm2/g");
  mg->GetXaxis()->SetTitleOffset((Float_t)1.1);
  mg->GetYaxis()->SetTitleOffset((Float_t)1.3);
  MW->GraphWindow->UpdateRootCanvas();

  MW->GraphWindow->on_pbAddLegend_clicked();
}

void MaterialInspectorWindow::ShowTotalInteraction()
{
  MW->GraphWindow->SetLog(true, true);

  int particleId = ui->cobParticle->currentIndex();
  int entries = MW->MpCollection->tmpMaterial.MatParticle[particleId].InteractionDataX.size();
  if (entries<1) return;

  const AParticle::ParticleType type = Detector->MpCollection->getParticleType(particleId);

  QVector<double> X(MW->MpCollection->tmpMaterial.MatParticle[particleId].InteractionDataX);
  QVector<double> Y(MW->MpCollection->tmpMaterial.MatParticle[particleId].InteractionDataF);

  //qDebug() << MW->MpCollection->tmpMaterial.MatParticle[particleId].InteractionDataX << MW->MpCollection->tmpMaterial.MatParticle[particleId].InteractionDataF;

  TString Title="", Xtitle, Ytitle;
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
      qCritical()<<"Unknown particle type!";
      exit(666);
    }

  TGraph* gr = MW->GraphWindow->ConstructTGraph(X, Y, Title, Xtitle, Ytitle, kRed, 2, 1, kRed, 0, 1);
  MW->GraphWindow->Draw(gr, "AP");

  TGraph* graphOver = constructInterpolationGraph(X, Y);
  graphOver->SetLineColor(kRed);
  graphOver->SetLineWidth(1);
  MW->GraphWindow->Draw(graphOver, "L same");
}

TGraph *MaterialInspectorWindow::constructInterpolationGraph(QVector<double> X, QVector<double> Y)
{
  int entries = X.size();

  QVector<double> xx;
  QVector<double> yy;
  int LogLogInterpolation = Detector->MpCollection->fLogLogInterpolation;
  for (int i=1; i<entries; i++)
    for (int j=1; j<50; j++)
      {
        double previousOne = X[i-1];
        double thisOne = X[i];
        double XX = previousOne + 0.02* j * (thisOne-previousOne);
        xx << XX;
        double YY;
        if (XX < X.last()) YY = GetInterpolatedValue(XX, &X, &Y, LogLogInterpolation);
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

  if (fOK) MW->MpCollection->tmpMaterial.MatParticle[particleId].DataString = ui->leChemicalComposition->text();

  on_pbUpdateInteractionIndication_clicked();
  on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbShowXCOMdata_clicked()
{
  int particleId = ui->cobParticle->currentIndex();
  const AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

  QString s = tmpMaterial.MatParticle.at(particleId).DataSource;

  MW->Owindow->SetTab(0);
  MW->Owindow->showNormal();
  MW->Owindow->raise();
  MW->Owindow->activateWindow();

  MW->Owindow->OutText(s);
}

void MaterialInspectorWindow::on_cobYieldForParticle_activated(int index)
{
  const AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
  ui->ledPrimaryYield->setText( QString::number(tmpMaterial.MatParticle.at(index).PhYield) );
}

void MaterialInspectorWindow::on_ledPrimaryYield_textChanged(const QString &arg1)
{
    if (arg1.toDouble() != MW->MpCollection->tmpMaterial.MatParticle.at(ui->cobYieldForParticle->currentIndex()).PhYield)
      on_pbWasModified_clicked();
}

bool MaterialInspectorWindow::doLoadCrossSection(ANeutronInteractionElement *element, QString fileName)
{
    QVector<double> x, y;
    int res = LoadDoubleVectorsFromFile(fileName, &x, &y);
    if (res == 0)
    {
        double Multiplier;
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
        on_pbWasModified_clicked();
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

    qDebug() << "Autoload cross-section from file: " <<fileName;

    return doLoadCrossSection(element, fileName);
}

void MaterialInspectorWindow::on_pbConfigureAutoElastic_clicked()
{
   OptionsConfigurator->setStarterDir(MW->GlobSet->LastOpenDir);
   OptionsConfigurator->showNormal();
}

//--------------------------------------------------

void MaterialInspectorWindow::onAddIsotope(AChemicalElement *element)
{
    element->Isotopes << AIsotope(element->Symbol, 777, 0);

    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
    tmpMaterial.ChemicalComposition.CalculateMeanAtomMass();
    tmpMaterial.updateNeutronDataOnCompositionChange(MW->MpCollection);

    ShowTreeWithChemicalComposition();
    FillNeutronTable();
    on_pbWasModified_clicked();
}

void MaterialInspectorWindow::onRemoveIsotope(AChemicalElement *element, int isotopeIndexInElement)
{
    if (element->Isotopes.size()<2)
    {
        message("Cannot remove the last isotope!", this);
        return;
    }
    element->Isotopes.removeAt(isotopeIndexInElement);

    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
    tmpMaterial.ChemicalComposition.CalculateMeanAtomMass();
    tmpMaterial.updateNeutronDataOnCompositionChange(MW->MpCollection);

    ShowTreeWithChemicalComposition();
    FillNeutronTable();
    on_pbWasModified_clicked();
}

void MaterialInspectorWindow::IsotopePropertiesChanged(const AChemicalElement * /*element*/, int /*isotopeIndexInElement*/)
{
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
    tmpMaterial.ChemicalComposition.CalculateMeanAtomMass();
    tmpMaterial.updateNeutronDataOnCompositionChange(MW->MpCollection);

    ShowTreeWithChemicalComposition();
    FillNeutronTable();
    on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbShowStatisticsOnElastic_clicked()
{
    NeutronInfoDialog = new ANeutronInfoDialog(&MW->MpCollection->tmpMaterial, ui->cobParticle->currentIndex(), MW->MpCollection->fLogLogInterpolation,
                                                    ui->cbCapture->isChecked(), ui->cbEnableScatter->isChecked(), MW->GraphWindow, this);

    NeutronInfoDialog->setWindowFlags(NeutronInfoDialog->windowFlags() | Qt::WindowStaysOnTopHint);
    NeutronInfoDialog->show();
    MW->WindowNavigator->DisableAllButGraphWindow(true);
    NeutronInfoDialog->setEnabled(true);
    do
    {
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
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

    QDialog* d = new QDialog(this);
    d->setWindowTitle("Enter chemical composition");

    QVBoxLayout* L = new QVBoxLayout();
        QHBoxLayout* l = new QHBoxLayout();
        QLineEdit* le = new QLineEdit(ui->leChemicalComposition->text(), this);
        le->setMinimumSize(400,25);
        QPushButton* pb = new QPushButton("Confirm", this);
        l->addWidget(le);
        l->addWidget(pb);
        connect(pb, SIGNAL(clicked(bool)), d, SLOT(accept()));
    L->addLayout(l);
    L->addWidget(new QLabel("Examples of valid formatting:"));
    L->addWidget(new QLabel("H2O:9 + NaCl:0.2 - means 9 parts of H2O and 0.2 parts of NaCl"));
    L->addWidget(new QLabel("C2 H5 OH"));
    L->addWidget(new QLabel("C22H10N205"));
    L->addWidget(new QLabel("\nWarning: pressing \"Confirm\" button resets custom isotope composition!"));
    d->setLayout(L);

    while (d->exec() != 0)
    {
        //QString newComp = le->text();
        //      qDebug() << newComp;

        AMaterialComposition& mc = tmpMaterial.ChemicalComposition;
        mc.configureNaturalAbunances(OptionsConfigurator->getNatAbundFileName());
        QString error = mc.setCompositionString(le->text());
        if (!error.isEmpty())
        {
            message(error, d);
            continue;
        }

        ui->leChemicalComposition->setText(mc.getCompositionString());
        //message(mc.print(), this);
        ShowTreeWithChemicalComposition();
        break;
    }

    tmpMaterial.updateNeutronDataOnCompositionChange(MW->MpCollection);

    if (OptionsConfigurator->isAutoloadEnabled()) autoloadMissingCrossSectionData();

    if (ui->cobParticle->currentIndex() == MW->MpCollection->getNeutronIndex()) FillNeutronTable(); //fill table if neutron is selected

    on_pbWasModified_clicked();
}

void MaterialInspectorWindow::ShowTreeWithChemicalComposition()
{
    bClearInProgress = true;
    ui->trwChemicalComposition->clear();
    bClearInProgress = false;

    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
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
                //        QObject::connect(isotopDel, &AElasticIsotopeDelegate::LoadClicked, this, &MaterialInspectorWindow::onLoadElementCrossClicked, Qt::QueuedConnection);
                //        QObject::connect(isotopDel, &AElasticIsotopeDelegate::RequestActivateModifiedStatus, this, &MaterialInspectorWindow::on_pbWasModified_clicked, Qt::QueuedConnection);
                //        QObject::connect(isotopDel, &AElasticIsotopeDelegate::RequestActivateModifiedStatus, this, &MaterialInspectorWindow::on_ledMFPenergyEllastic_editingFinished, Qt::QueuedConnection);
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
    ui->tabwNeutron->clearContents();
    ui->tabwNeutron->setRowCount(0);
    ui->tabwNeutron->setColumnCount(0);

    bool bCapture = ui->cbCapture->isChecked();
    bool bElastic = ui->cbEnableScatter->isChecked();
    if (!bCapture && !bElastic) return;

    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
    int particleId = ui->cobParticle->currentIndex();
    QVector<NeutralTerminatorStructure>& Terminators = tmpMaterial.MatParticle[particleId].Terminators;

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
    bool bIgnore = tmpMaterial.MatParticle[particleId].bAllowAbsentCsData;
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
                NeutralTerminatorStructure& t = Terminators.first();
                ANeutronInteractionElement* absEl = t.getNeutronInteractionElement(row);
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
                  if (!bIgnore)
                    if (absEl->Energy.isEmpty())
                        flagButton(pbLoad, true);
                  pbLoad->setMaximumWidth(50);
                  l->addWidget(pbLoad);
                  QPushButton* pbReaction = new QPushButton("Reactions");
                  l->addWidget(pbReaction);
                  QLabel* lab = new QLabel("  ");
                  QIcon YellowIcon = createColorCircleIcon( QSize(12,12), ( absEl->DecayScenarios.isEmpty() ? Qt::white : Qt::yellow ) );
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
                NeutralTerminatorStructure& t = Terminators.last();
                ANeutronInteractionElement* scatEl = t.getNeutronInteractionElement(row);
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
                if (!bIgnore)
                    if (scatEl->Energy.isEmpty())
                        flagButton(pbLoad, true);
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
    ui->tabwNeutron->resizeColumnsToContents();
    ui->tabwNeutron->resizeRowsToContents();

    tmpMaterial.updateRuntimeProperties(MW->MpCollection->fLogLogInterpolation);
}

void MaterialInspectorWindow::autoloadMissingCrossSectionData()
{
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

    //for neutron
    int neutronId = MW->MpCollection->getNeutronIndex();
    if (neutronId != -1) //otherwise not dfefined in this configuration
    {
        MatParticleStructure& mp = tmpMaterial.MatParticle[neutronId];

        bool bCapture = mp.bCaptureEnabled;
        bool bElastic = mp.bEllasticEnabled;
        if (!bCapture && !bElastic) return;

        QVector<NeutralTerminatorStructure>& Terminators = mp.Terminators;

        if (Terminators.size() != 2)
        {
            qWarning() << "||| Terminators size is not equal to two!";
            return;
        }
        NeutralTerminatorStructure& termAbs = Terminators[0];
        NeutralTerminatorStructure& termScat = Terminators[1];

        if (bCapture)
        {
            for (int iEl = 0; iEl<termAbs.IsotopeRecords.size(); iEl++)
                if (termAbs.IsotopeRecords.at(iEl).Energy.isEmpty())
                    autoLoadCrossSection( &termAbs.IsotopeRecords[iEl], "absorption");
        }
        if (bElastic)
        {
            for (int iEl = 0; iEl<termScat.IsotopeRecords.size(); iEl++)
                if (termScat.IsotopeRecords.at(iEl).Energy.isEmpty())
                    autoLoadCrossSection( &termScat.IsotopeRecords[iEl], "elastic scattering");
        }
    }
}

void MaterialInspectorWindow::on_tabwNeutron_customContextMenuRequested(const QPoint &pos)
{
    qDebug() << "Menu not implemented" << ui->tabwNeutron->currentRow() << ui->tabwNeutron->currentColumn()<<pos;
}

void MaterialInspectorWindow::on_cbCapture_clicked()
{
    FillNeutronTable();
}

void MaterialInspectorWindow::on_cbEnableScatter_clicked()
{
    FillNeutronTable();
}

void MaterialInspectorWindow::onTabwNeutronsActionRequest(int iEl, int iIso, const QString Action)
{
    if (MW->GlobSet->MaterialsAndParticlesSettings.isEmpty())
        on_pbConfigureAutoElastic_clicked();

    //      qDebug() << "Element#"<<iEl << "Isotope#:"<<iIso <<Action;
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
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

        QString fileName = QFileDialog::getOpenFileName(this, "Load " + target + " cross-section data for " + isotope, MW->GlobSet->LastOpenDir, "Data files (*.txt *.dat); All files (*.*)");
        if (fileName.isEmpty()) return;
        MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

        doLoadCrossSection(element, fileName);
        FillNeutronTable();
    }
    // -- Reactions --
    else if (Action == "Reactions")
    {
        QStringList DefinedParticles;
        MW->MpCollection->OnRequestListOfParticles(DefinedParticles);
        ANeutronReactionsConfigurator* d = new ANeutronReactionsConfigurator(&Terminators[0].IsotopeRecords[iIndex], DefinedParticles, this);
        int res = d->exec();
        delete d;
        if (res != 0)
        {
            FillNeutronTable();
            on_pbWasModified_clicked();
        }
    }
}

void MaterialInspectorWindow::on_pbMaterialInfo_clicked()
{
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

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
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;
    int particleId = ui->cobParticle->currentIndex();
    tmpMaterial.MatParticle[particleId].bAllowAbsentCsData = ui->cbAllowAbsentCsData->isChecked();

    FillNeutronTable();
    on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbAutoLoadMissingNeutronCrossSections_clicked()
{
    autoloadMissingCrossSectionData();

    FillNeutronTable();
    on_pbWasModified_clicked();
}

void MaterialInspectorWindow::on_pbHelpNeutron_clicked()
{
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

    parseDecayTime();
}

bool MaterialInspectorWindow::parseDecayTime()
{
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

    QString s = ui->lePriT->text().simplified();

    tmpMaterial.PriScintDecayTimeVector.clear();
    bool bErrorDetected = false;

    bool bSingle;
    double tau = s.toDouble(&bSingle);
    if (bSingle)
        tmpMaterial.PriScintDecayTimeVector << QPair<double, double>(1.0, tau);
    else
    {
        QStringList sl = s.split('&', QString::SkipEmptyParts);

        for (const QString& sr : sl)
        {
            QStringList oneTau = sr.split(':', QString::SkipEmptyParts);
            if (oneTau.size() == 2)
            {
                bool bOK1, bOK2;
                double weight = oneTau.at(0).toDouble(&bOK1);
                double tau    = oneTau.at(1).toDouble(&bOK2);
                if (bOK1 && bOK2)
                    tmpMaterial.PriScintDecayTimeVector << QPair<double, double>(weight, tau);
                else
                {
                    bErrorDetected = true;
                    break;
                }
            }
            else bErrorDetected = true;
        }
        if (tmpMaterial.PriScintDecayTimeVector.isEmpty()) bErrorDetected = true;
    }

    if (bErrorDetected)
    {
        bMessageLock = true;
        message("Decay time format error:\nuse a double value or weight1:decay1 & weight2:decay2 & ...", this);
        bMessageLock = false;
    }
    else
        on_pbUpdateTmpMaterial_clicked();

    return !bErrorDetected;
}

void MaterialInspectorWindow::on_pbPriThelp_clicked()
{
    QString s = "Decay time:\n"
            "  If there is only one exponential decay component,"
            "  it can be given directly.\n"
            "  To configure several exponential decay components, use\n"
            "  stat_weight1:decay_time1 & stat_weight2:decay_time2 & ...\n"
            "  e.g., 1:25.5 & 1:250\n\n"
            "Raise time:\n"
            "  Provide raise time, the emission delay is calculated as 1 - exp{-t/raise_time}\n\n"
            "Model:\n"
            "  If \"Sum\" is selected, the photon emission time is calculated as follows:\n"
            "  first the delay due to the raise time is generated,\n"
            "  then decay time is generated. The emission time is sum of those values.\n\n"
            "  If \"Shao\" is selected, the emission time is calculated as in:\n"
            "  Yiping Shao, Phys. Med. Biol. 52 (2007) 1103–1117\n"
            "  http://www.iss.infn.it/topem/TOF-PET/shao-model-timing.pdf\n"
            "  The formula is generalised to have more than one decay components.\n"
            "  Random generator was provided by Evgeny Tolotchko";
    message(s, this);
}

void MaterialInspectorWindow::on_ledPriT_raise_textChanged(const QString &arg1)
{
    ui->cobPriT_model->setVisible(arg1 != "0");
}

void MaterialInspectorWindow::on_pbPriT_test_clicked()
{
    AMaterial& tmpMaterial = MW->MpCollection->tmpMaterial;

    tmpMaterial.updateRuntimeProperties(MW->MpCollection->fLogLogInterpolation); //to update sum of stat weights

    QMessageBox mb(this);
    if (ui->cobPriT_model->currentIndex() == 1)
    {
        MW->WindowNavigator->BusyOn();
        mb.setWindowFlags(mb.windowFlags() | Qt::WindowStaysOnTopHint);
        mb.setStandardButtons(0);
        mb.setText("calculating...");
        mb.show();
        QCoreApplication::processEvents();
    }

    TH1D* h = new TH1D("h1", "", 1000, 0, 0);
    for (int i=0; i<1000000; i++)
        h->Fill( tmpMaterial.GeneratePrimScintTime(Detector->RandGen) );

    if (ui->cobPriT_model->currentIndex() == 1)
    {
        mb.hide();
        MW->WindowNavigator->BusyOff();
    }


    h->GetXaxis()->SetTitle("Time, ns");
    TString title = "Emission for ";
    title += tmpMaterial.name.toLatin1().data();
    h->SetTitle(title);
    MW->GraphWindow->Draw(h);
}
