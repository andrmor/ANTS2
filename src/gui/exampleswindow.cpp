#include "exampleswindow.h"
#include "ui_exampleswindow.h"
#include "mainwindow.h"
#include "windownavigatorclass.h"
#include "geometrywindowclass.h"
#include "reconstructionwindow.h"
#include "aglobalsettings.h"
#include "ajsontools.h"
#include "amessage.h"
#include "aconfiguration.h"

//Qt
#include <QDebug>
#include <QFileDialog>
#include <QDialog>
#include <QJsonDocument>
#include <QDateTime>

ExamplesWindow::ExamplesWindow(QWidget *parent, MainWindow *mw) :
  AGuiWindow("examples", parent),
  ui(new Ui::ExamplesWindow)
{  
  MW = mw;
  ui->setupUi(this);

  Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
  windowFlags |= Qt::Tool;
  windowFlags |= Qt::WindowStaysOnTopHint;
  windowFlags |= Qt::WindowCloseButtonHint;
  this->setWindowFlags( windowFlags );

  MainPointer = -1;
  SubPointer = -1;
  ExamplePointer = -1;
  MainTypeNames.resize(0);
  SubTypeNames.resize(0);
  Examples.resize(0);
  ListedExamples.resize(0);

  ui->cbDoNotShowExamplesOnStart->setChecked(!MW->GlobSet.ShowExamplesOnStart);

  QString fileName = MW->GlobSet.QuicksaveDir + "/QuickSave0.json";
  QString s = "Not available";
  QFileInfo fi(fileName);
  if (fi.exists())
      s = "Saved at " + fi.lastModified().toString("hh:mm:ss  on  dd MMM yyyy");
  ui->labLastOnExit->setText(s);

  ExamplesWindow::BuildExampleRecord();

  //menu properties
  QString mss = ui->menuFile->styleSheet();
  mss += "; QMenu::tooltip {wakeDelay: 1;}";
  ui->menuFile->setStyleSheet(mss);
  ui->menuFile->setToolTipsVisible(true);
  ui->menuFile->setToolTipDuration(1000);
}

ExamplesWindow::~ExamplesWindow()
{  
  //qDebug()<<"destructor of ExamplesWindow started";
    delete ui;
}

void ExamplesWindow::SaveConfig(QString fileName, bool DetConstructor, bool SimSettings, bool ReconstrSettings)
{
    MW->writeDetectorToJson(MW->Config->JSON);
      MW->writeExtraGuiToJson(MW->Config->JSON);
    MW->writeSimSettingsToJson(MW->Config->JSON);
    MW->Rwindow->writeToJson(MW->Config->JSON);
    MW->Config->UpdateLRFmakeJson();
    MW->Config->UpdateLRFv3makeJson();

    MW->Config->SaveConfig(fileName, DetConstructor, SimSettings, ReconstrSettings);
}

void ExamplesWindow::on_cbDoNotShowExamplesOnStart_toggled(bool checked)
{
  MW->GlobSet.ShowExamplesOnStart = !checked;
}

void ExamplesWindow::BuildExampleRecord()
{
  QString fileName = MW->GlobSet.ExamplesDir + "/ExamplesTree.txt";
  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
      {
          qWarning("!--Cannot find or open ExamplesTree.txt");
          return;
      }

  QTextStream in(&file);

  while(!in.atEnd())
      {
        QString line = in.readLine();
        QStringList fields = line.split(":", QString::SkipEmptyParts);
        if (fields.size()<2) continue;

        if (fields[0] == "define")
          {
            if (fields.size() == 2)
              {
                //defining main type
                ExamplesWindow::AddNewMainType(fields[1].simplified());
              }
            else if (fields.size() == 3)
              {
                ExamplesWindow::AddNewSubType(fields[1].simplified(), fields[2].simplified());
              }
            else
              {
                //wrong number of fields!
                qWarning()<<"!--Wrong number of fields found while processing ExamplesTree.txt"<<line;
              }
          }
        else if (fields[0] == "add")
          {
            if (fields.size() != 5 )
              {
                //wrong number of fields!
                qWarning()<<"!--Wrong number of fields found while processing ExamplesTree.txt: "<<line;
              }
            else
              {
                ExamplesWindow::AddNewExample(fields[1].simplified(), fields[2].simplified(), fields[3], fields[4]);
              }

          }
    }

  file.close();

  //filling ui data
  for (int i=0; i<MainTypeNames.size(); i++) ui->lwMainType->addItem(MainTypeNames[i]);
}

void ExamplesWindow::AddNewMainType(QString name)
{
  for (int i=0; i<MainTypeNames.size(); i++)
     if (name == MainTypeNames[i])
       {
          qWarning()<<"!--Attempt to register already existent MainTypeName during ExampleTree build";
          return;
       }
  MainTypeNames.append(name);
  SubTypeNames.resize(SubTypeNames.size()+1);
  SubTypeNames.last().resize(0);
}

void ExamplesWindow::AddNewSubType(QString maintype, QString subtype)
{
  for (int i=0; i<MainTypeNames.size(); i++)
     if (maintype == MainTypeNames[i])
       {
         //it is indeed a registered maintype
         //qDebug()<<maintype<<"->"<<i;
         for (int j=0; j<SubTypeNames[i].size(); j++)
           if (subtype == SubTypeNames[i][j])
             {
               qWarning()<<"Attempt to register already existent SubTypeName during ExampleTree build:"<<subtype;
               return;
             }

         SubTypeNames[i].append(subtype);
         //qDebug()<<"adding"<<subtype<<"for"<<maintype<<"for index="<<i;
         return;
       }
  qWarning()<<"!--Attempt to register subtype for a non-existent MainType during ExampleTree build:"<<maintype;
  return;
}

void ExamplesWindow::AddNewExample(QString maintype, QString subtype, QString filename, QString description)
{
  for (int i=0; i<MainTypeNames.size(); i++)
     if (maintype == MainTypeNames[i])
       {
         //it is indeed a registered maintype
//         qDebug()<<maintype<<"->"<<i;
         for (int j=0; j<SubTypeNames[i].size(); j++)
           if (subtype == SubTypeNames[i][j])
             {
               //it is indeed a registered subtype
//               qDebug()<<maintype<<subtype<<j;

               Examples.append(DetectorExampleStructure(i, j, filename, description));
               return;
             }

         qWarning()<<"!--Attempt to register example for a non-existent SubType during ExampleTree build:"<<subtype;
         return;
       }
  qWarning()<<"!--Attempt to register example for a non-existent MainType during ExampleTree build:"<<maintype;
  return;
}

void ExamplesWindow::on_lwMainType_currentRowChanged(int currentRow)
{
   if (currentRow == -1) return;

   MainPointer = currentRow;
   //qDebug()<<currentRow<<SubTypeNames[MainPointer].size();

   SubPointer = -1;
   ExamplePointer = -1;
   ui->lwSubType->clear();
   ui->lwExample->clear();
   ui->teExampleDescription->clear();
   for (int i=0; i<SubTypeNames[MainPointer].size(); i++) ui->lwSubType->addItem(SubTypeNames[MainPointer][i]);

   if (SubTypeNames[MainPointer].size() == 1) ui->lwSubType->setCurrentRow(0);
}

void ExamplesWindow::on_lwSubType_currentRowChanged(int currentRow)
{
   if (currentRow == -1) return;

   SubPointer = currentRow;

   ExamplePointer = -1;
   ui->lwExample->clear();
   ui->teExampleDescription->clear();

   //building list of examples and showing them
   ListedExamples.clear();
   for (int i=0; i<Examples.size(); i++)
     {
       if (Examples[i].MainType != MainPointer) continue;
       if (Examples[i].SubType != SubPointer) continue;

       //adding this one
       ListedExamples.append(i);
       ui->lwExample->addItem(Examples[i].filename);
     }

   if (ListedExamples.size() == 1) ui->lwExample->setCurrentRow(0);
}

void ExamplesWindow::on_lwExample_currentRowChanged(int currentRow)
{
  if (currentRow == -1)
    {
      ui->pbLoadExample->setEnabled(false);
      return;
    }

  ui->pbLoadExample->setEnabled(true);
  ExamplePointer = currentRow;
  ui->teExampleDescription->setText(Examples[ListedExamples[currentRow]].Description);
}

void ExamplesWindow::on_pbLoadExample_clicked()
{
  if (ExamplePointer == -1)
    {
      message("Example is not selected!", this);
      return;
    }
  this->close();
  MW->GeometryWindow->bDisableDraw = true;

  QString filename = MW->GlobSet.ExamplesDir + "/" + Examples[ListedExamples[ExamplePointer]].filename;

  bool bOK = MW->Config->LoadConfig(filename);
  if (!bOK) message(MW->Config->ErrorString, MW);

  MW->show();
  MW->raise();
  MW->GeometryWindow->show();
  MW->GeometryWindow->raise();

  MW->GeometryWindow->bDisableDraw = false;
  MW->GeometryWindow->ShowGeometry();
}

void ExamplesWindow::on_lwExample_doubleClicked(const QModelIndex &index)
{
  ui->lwExample->setCurrentRow(index.row());
  on_pbLoadExample_clicked();
}

void ExamplesWindow::on_pbSaveSessings_clicked()
{
  QString fileName = QFileDialog::getSaveFileName(this,"Save configuration", MW->GlobSet.LastOpenDir, "Json files (*.json)");
  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
  QFileInfo file(fileName);
  if(file.suffix().isEmpty()) fileName += ".json";
  MW->ELwindow->SaveConfig(fileName, !ui->cbSkipDetectorConstructor->isChecked(), !ui->cbSkipSimConfig->isChecked(), !ui->cbSkipRecConfig->isChecked());
}

void ExamplesWindow::on_pbLoadSettings_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Load configuration", MW->GlobSet.LastOpenDir, "All files (*.*)");
  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  MW->GeometryWindow->bDisableDraw = true; //->
  bool bOK = MW->Config->LoadConfig(fileName, !ui->cbSkipDetectorConstructor->isChecked(), !ui->cbSkipSimConfig->isChecked(), !ui->cbSkipRecConfig->isChecked());
  if (!bOK) message(MW->Config->ErrorString, this);
  else
  {
      this->close();
      if (MW->GeometryWindow->isVisible()) MW->GeometryWindow->ShowGeometry();
      MW->show();
  }
  MW->GeometryWindow->bDisableDraw = false; //<-
}

void ExamplesWindow::on_pbLoadLast_clicked()
{
  QuickLoad(0, this);
}

void ExamplesWindow::QuickLoad(int i, QWidget *parent)
{
  QString fileName = MW->GlobSet.QuicksaveDir + "/QuickSave" + QString::number(i) + ".json";
  if (!QFileInfo(fileName).exists())
  {
      QString s;
      if (i==0) s = "Autosave file not found";
      else      s = "Quick save slot # " + QString::number(i) + " is empty";
      message(s, parent);
      return;
  }

  MW->GeometryWindow->bDisableDraw = true;
  bool bOK = MW->Config->LoadConfig(fileName);
  MW->GeometryWindow->bDisableDraw = false;

  MW->showNormal();

  this->close();
  if (MW->GeometryWindow->isVisible()) MW->GeometryWindow->ShowGeometry();

  MW->showNormal();
  MW->activateWindow();

  if (!bOK) message(MW->Config->ErrorString, MW);
}

QString ExamplesWindow::getQuickSlotMessage(int i)
{
    QString fileName = MW->GlobSet.QuicksaveDir + "/QuickSave" + QString::number(i) + ".json";
    QString s;
    if (i==0) s = "Save on exit configuration file not found";
    else      s = "Quick save slot # " + QString::number(i) + " is empty";
    QFileInfo fi(fileName);
    if (fi.exists())
        s = "Saved at " + fi.lastModified().toString("hh:mm:ss  on  dd MMM yyyy");
    return s;
}

void ExamplesWindow::QuickSave(int i)
{
  QString fileName = MW->GlobSet.QuicksaveDir + "/QuickSave" + QString::number(i) + ".json";
  MW->ELwindow->SaveConfig(fileName);
}

void ExamplesWindow::on_actionQuick_save_1_triggered()
{
    QuickSave(1);
}

void ExamplesWindow::on_actionQuick_save_2_triggered()
{
    QuickSave(2);
}

void ExamplesWindow::on_actionQuick_save_3_triggered()
{
    QuickSave(3);
}

void ExamplesWindow::on_actionQuick_load_1_triggered()
{
    QuickLoad(1, this);
}

void ExamplesWindow::on_actionQuick_load_2_triggered()
{
    QuickLoad(2, this);
}

void ExamplesWindow::on_actionQuick_load_3_triggered()
{
    QuickLoad(3, this);
}

void ExamplesWindow::on_actionQuick_save_1_hovered()
{
    ui->actionQuick_save_1->setToolTip(getQuickSlotMessage(1));
}

void ExamplesWindow::on_actionQuick_save_2_hovered()
{
    ui->actionQuick_save_2->setToolTip(getQuickSlotMessage(2));
}

void ExamplesWindow::on_actionQuick_save_3_hovered()
{
    ui->actionQuick_save_3->setToolTip(getQuickSlotMessage(3));
}

void ExamplesWindow::on_actionQuick_load_1_hovered()
{
    ui->actionQuick_load_1->setToolTip(getQuickSlotMessage(1));
}

void ExamplesWindow::on_actionQuick_load_2_hovered()
{
    ui->actionQuick_load_2->setToolTip(getQuickSlotMessage(2));
}

void ExamplesWindow::on_actionQuick_load_3_hovered()
{
    ui->actionQuick_load_3->setToolTip(getQuickSlotMessage(3));
}

void ExamplesWindow::on_actionCreate_new_detector_triggered()
{
    MW->Config->LoadConfig(MW->GlobSet.ExamplesDir + "/Simplest.json");
    hide();
    if (MW->GeometryWindow->isVisible()) MW->GeometryWindow->ShowGeometry(false);
}
