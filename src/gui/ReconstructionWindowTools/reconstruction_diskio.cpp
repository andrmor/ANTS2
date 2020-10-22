#include "reconstructionwindow.h"
#include "ui_reconstructionwindow.h"
#include "ajsontools.h"
#include "mainwindow.h"
#include "jsonparser.h"
#include "eventsdataclass.h"
#include "windownavigatorclass.h"
#include "aglobalsettings.h"
#include "detectorclass.h"
#include "amessage.h"
#include "globalsettingswindowclass.h"
#include "apmgroupsmanager.h"
#include "aconfiguration.h"

#include <QDebug>
#include <QFileDialog>
#include <QVBoxLayout>

#include "TTree.h"
#include "TFile.h"
#include "TH2D.h"

void ReconstructionWindow::writeMiscGUIsettingsToJson(QJsonObject &json)
{
  QJsonObject js;

  //Plot XY settings
  QJsonObject pj;
  pj["PlotXYfromX"] = ui->ledXfrom->text().toDouble();
  pj["PlotXYfromY"] = ui->ledYfrom->text().toDouble();
  pj["PlotXYtoX"] = ui->ledXto->text().toDouble();
  pj["PlotXYtoY"] = ui->ledYto->text().toDouble();
  pj["PlotXYbinsX"] = ui->sbXbins->value();
  pj["PlotXYbinsY"] = ui->sbYbins->value();
  pj["PlotXYoptionZ"] = ui->cobHowToAverageZ->currentIndex();
  pj["PlotXYfromZ"] = ui->ledZfrom->text().toDouble();
  pj["PlotXYtoZ"] = ui->ledZto->text().toDouble();
  js["PlotXYsettings"] = pj;

  //Blur settings
  QJsonObject bj;
  bj["BlurType"] = ui->cobBlurType->currentIndex();
  bj["BlurWidth"] = ui->ledBlurDelta->text().toDouble();
  bj["BlurSigma"] = ui->ledBlurSigma->text().toDouble();
  bj["BlurWidthZ"] = ui->ledBlurDeltaZ->text().toDouble();
  bj["BlurSigmaZ"] = ui->ledBlurSigmaZ->text().toDouble();
  js["Blur"] = bj;

  //ChPerPhe - peaks
  QJsonObject phePj;
  phePj["Bins"] = ui->sbFromPeaksBins->value();
  phePj["From"] = ui->ledFromPeaksFrom->text().toDouble();
  phePj["To"] = ui->ledFromPeaksTo->text().toDouble();
  phePj["Sigma"] = ui->ledFromPeaksSigma->text().toDouble();
  phePj["Threshold"] = ui->ledFromPeaksThreshold->text().toDouble();
  phePj["MaxPeaks"] = ui->sbFromPeaksMaxPeaks->value();
  js["ChPerPhePeaks"] = phePj;

  //ChPerPhe - stat
  QJsonObject pheSj;
  pheSj["Bins"] = ui->pbBinsChansPerPhEl->value();
  pheSj["MinRange"] = ui->ledMinRangeChanPerPhEl->text().toDouble();
  pheSj["MaxRange"] = ui->ledMaxRangeChanPerPhEl->text().toDouble();
  pheSj["Lower"] = ui->ledLowerLimitForChanPerPhEl->text().toDouble();
  pheSj["Upper"] = ui->ledUpperLimitForChanPerPhEl->text().toDouble();
  pheSj["ENF"] = ui->ledENFforChanPerPhEl->text().toDouble();
  js["ChPerPheStat"] = pheSj;

  json["ReconstructionWindow"] = js;
}

void ReconstructionWindow::updateGUIsettingsInConfig()
{
    QJsonObject js;

    if (MW->Detector->Config->JSON.contains("GUI"))
        js = MW->Detector->Config->JSON["GUI"].toObject();

    MW->Rwindow->writeMiscGUIsettingsToJson(js);
    MW->Detector->Config->JSON["GUI"] = js;
}

void ReconstructionWindow::readMiscGUIsettingsFromJson(QJsonObject &json)
{
   QJsonObject RWjon = json["ReconstructionWindow"].toObject();

   //PlotXY settings
   QJsonObject js = RWjon["PlotXYsettings"].toObject();
   JsonToLineEditDouble(js, "PlotXYfromX", ui->ledXfrom);
   JsonToLineEditDouble(js, "PlotXYfromY", ui->ledYfrom);
   JsonToLineEditDouble(js, "PlotXYtoX", ui->ledXto);
   JsonToLineEditDouble(js, "PlotXYtoY", ui->ledYto);
   JsonToLineEditDouble(js, "PlotXYfromZ", ui->ledZfrom);
   JsonToLineEditDouble(js, "PlotXYtoZ", ui->ledZto);
   JsonToSpinBox(js, "PlotXYbinsX", ui->sbXbins);
   JsonToSpinBox(js, "PlotXYbinsY", ui->sbYbins);
   JsonToComboBox(js, "PlotXYoptionZ", ui->cobHowToAverageZ);

   bool fSym = false;
   if (ui->ledXfrom->text() == QString("-")+ui->ledXto->text())
       if (ui->ledYfrom->text() == QString("-")+ui->ledYto->text())
           if (ui->ledXfrom->text() == ui->ledYfrom->text())
               if (ui->sbXbins->value() == ui->sbYbins->value())
                   fSym = true;
   ui->cbXYsymmetric->setChecked(fSym);

   //Blur settings
   QJsonObject bj = RWjon["Blur"].toObject();
   JsonToComboBox(bj, "BlurType", ui->cobBlurType);
   JsonToLineEditDouble(bj, "BlurWidth", ui->ledBlurDelta);
   JsonToLineEditDouble(bj, "BlurSigma", ui->ledBlurSigma);
   JsonToLineEditDouble(bj, "BlurWidthZ", ui->ledBlurDeltaZ);
   JsonToLineEditDouble(bj, "BlurSigmaZ", ui->ledBlurSigmaZ);

   //ChPerPhe - peaks
   if (RWjon.contains("ChPerPhePeaks"))
   {
       QJsonObject phePj = RWjon["ChPerPhePeaks"].toObject();
       JsonToSpinBox(phePj, "Bins", ui->sbFromPeaksBins);
       JsonToLineEditDouble(phePj, "From", ui->ledFromPeaksFrom);
       JsonToLineEditDouble(phePj, "To", ui->ledFromPeaksTo);
       JsonToLineEditDouble(phePj, "Sigma", ui->ledFromPeaksSigma);
       JsonToLineEditDouble(phePj, "Threshold", ui->ledFromPeaksThreshold);
       JsonToSpinBox(phePj, "MaxPeaks", ui->sbFromPeaksMaxPeaks);
   }

   //ChPerPhe - peaks
   if (RWjon.contains("ChPerPheStat"))
   {
       QJsonObject pheSj = RWjon["ChPerPheStat"].toObject();
       JsonToSpinBox(pheSj, "Bins", ui->pbBinsChansPerPhEl);
       JsonToLineEditDouble(pheSj, "MinRange", ui->ledMinRangeChanPerPhEl);
       JsonToLineEditDouble(pheSj, "MaxRange", ui->ledMaxRangeChanPerPhEl);
       JsonToLineEditDouble(pheSj, "Lower", ui->ledLowerLimitForChanPerPhEl);
       JsonToLineEditDouble(pheSj, "Upper", ui->ledUpperLimitForChanPerPhEl);
       JsonToLineEditDouble(pheSj, "ENF", ui->ledENFforChanPerPhEl);
   }
}

void ReconstructionWindow::on_pbSaveScanTree_clicked()
{
  if (EventsDataHub->isScanEmpty())
    {
      message("Scan data are not ready!", this);
      return;
    }
  if ( !EventsDataHub->isReconstructionReady() )
    {
      message("Need to run reconstruction first!", this);
      return;
    }

  QString fileName = QFileDialog::getSaveFileName(this, "Save sigma/distortion information", MW->GlobSet.LastOpenDir, "Root files (*.root)");
  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QString path = QFileInfo(fileName).absoluteDir().absolutePath();
  QString suffix = QFileInfo(fileName).suffix();
  if (suffix.isEmpty()) suffix = ".root";
  fileName = QFileInfo(fileName).completeBaseName();

  int numGroups = PMgroups->countPMgroups();
  for (int ig=0; ig<numGroups; ig++)
  {
      QString Name = fileName;
      if (numGroups>1) Name += "."+PMgroups->getGroupName(ig);
      Name.replace(" ", "_");
      Name = path + "/" + Name + "."+suffix;

      QByteArray ba = Name.toLocal8Bit();
      const char *c_str = ba.data();
      TFile f(c_str,"recreate");
      bool fOK = ReconstructionWindow::BuildResolutionTree(ig);
      if (!fOK)
        {
          qWarning() << "Failed to create resolution tree for group:"<<ig;
          f.Close();
          return;
        }
      EventsDataHub->ResolutionTree->Write();
      f.Close();
      EventsDataHub->ResolutionTree= 0;  //tree does not exist after save
  }
}

void ReconstructionWindow::on_pbSaveReconstructionAsText_clicked()
{
  //in this mode only time-integrated data are considered!
  if (EventsDataHub->isEmpty()) return;  
  if (!EventsDataHub->isReconstructionReady())
    {
      message("There are no reconstruction data to save!", this);
      return;
    }  

  QString fileName = QFileDialog::getSaveFileName(this, "Save reconstruction data", MW->GlobSet.LastOpenDir, "Data files (*.dat);;Text files (*.txt);;All files (*.*)");
  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QString path = QFileInfo(fileName).absoluteDir().absolutePath();
  QString suffix = QFileInfo(fileName).suffix();
  if (suffix.isEmpty()) suffix = ".dat";
  fileName = QFileInfo(fileName).completeBaseName();

  int numGroups = PMgroups->countPMgroups();
  for (int ig=0; ig<numGroups; ig++)
  {
      QString Name = fileName;
      if (numGroups>1) Name += "."+PMgroups->getGroupName(ig);
      Name.replace(" ", "_");
      Name = path + "/" + Name + "."+suffix;

      bool ok = EventsDataHub->saveReconstructionAsText(Name, ig);
      if (!ok)
      {
          message("Error writing the file:"+Name, this);
          return;
      }
  }
}

static bool DontShowTodayOnSaveTree = false;
void ReconstructionWindow::on_pbSaveReconstructionAsRootTree_clicked()
{
  //in this mode only time-integrated data are considered!
  if (EventsDataHub->isEmpty())
    {
      message("There are no events!", this);
      return;
    }
  if (!EventsDataHub->isReconstructionReady())
    {
      message("There are no reconstruction data to save!", this);
      return;
    }

 if (!DontShowTodayOnSaveTree)
  {
      QDialog D(this);
      D.setModal(true);
      QVBoxLayout* l = new QVBoxLayout(&D);

      QCheckBox* cbSig = new QCheckBox("Include PM signals", &D);
      l->addWidget(cbSig);
      cbSig->setChecked(MW->GlobSet.RecTreeSave_IncludePMsignals);
      QCheckBox* cbRho = new QCheckBox("Include rho array (distance reconstructed->PM center)", &D);
      l->addWidget(cbRho);
      cbRho->setChecked(MW->GlobSet.RecTreeSave_IncludeRho);
      QCheckBox* cbTrue = new QCheckBox("Include true data for simulation/calibration datasets", &D);
      l->addWidget(cbTrue);
      cbTrue->setChecked(MW->GlobSet.RecTreeSave_IncludeTrue);

      QPushButton* pb = new QPushButton("Confirm", &D);
      l->addWidget(pb);
      connect(pb, SIGNAL(clicked(bool)), &D, SLOT(accept()));
      QCheckBox* cbDont = new QCheckBox("Don't show this during this session", &D);
      cbDont->setToolTip("Setting can be configured in MainWindow menu->Settings->Data_export");
      l->addWidget(cbDont);

      D.exec();
      if (D.result() == QDialog::Rejected) return;

      DontShowTodayOnSaveTree =  (cbDont->isChecked());
      MW->GlobSet.RecTreeSave_IncludePMsignals = cbSig->isChecked();
      MW->GlobSet.RecTreeSave_IncludeRho = cbRho->isChecked();
      MW->GlobSet.RecTreeSave_IncludeTrue = cbTrue->isChecked();
      MW->GlobSetWindow->updateGUI();
  }

  QString fileName = QFileDialog::getSaveFileName(this, "Save reconsruction data as Root Tree", MW->GlobSet.LastOpenDir, "Root files (*.root)");
  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QString path = QFileInfo(fileName).absoluteDir().absolutePath();
  QString suffix = QFileInfo(fileName).suffix();
  if (suffix.isEmpty()) suffix = ".root";
  fileName = QFileInfo(fileName).completeBaseName();

  MW->WindowNavigator->BusyOn();
  qApp->processEvents();

  int numGroups = PMgroups->countPMgroups();
  for (int ig=0; ig<numGroups; ig++)
  {
      QString Name = fileName;
      if (numGroups>1) Name += "."+PMgroups->getGroupName(ig);
      Name.replace(" ", "_");
      Name = path + "/" + Name + "."+suffix;

      bool ok = EventsDataHub->saveReconstructionAsTree(Name, MW->Detector->PMs,
                                                        MW->GlobSet.RecTreeSave_IncludePMsignals,
                                                        MW->GlobSet.RecTreeSave_IncludeRho,
                                                        MW->GlobSet.RecTreeSave_IncludeTrue,
                                                        ig);
      if (!ok)
      {
          message("Error writing the file:"+Name, this);
          return;
      }
  }
  MW->WindowNavigator->BusyOff();
}
