#include "detectoraddonswindow.h"
#include "ui_detectoraddonswindow.h"
#include "mainwindow.h"
#include "apmhub.h"
#include "apmtype.h"
#include "checkupwindowclass.h"
#include "geometrywindowclass.h"
#include "ajavascriptmanager.h"
#include "ascriptwindow.h"
#include "detectorclass.h"
#include "aglobalsettings.h"
#include "ageo_si.h"
#include "asandwich.h"
#include "aslab.h"
#include "slabdelegate.h"
#include "ageotreewidget.h"
#include "ageoobject.h"
#include "ageoshape.h"
#include "ageotype.h"
#include "amessage.h"
#include "acommonfunctions.h"
#include "ageometrytester.h"
#include "amaterialparticlecolection.h"
#include "aconfiguration.h"
#include "ajsontools.h"
#include "afiletools.h"
#include "ascriptwindow.h"
#include "ageoconsts.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QDesktopServices>
#include <QEvent>

#include "TGeoManager.h"
#include "TGeoTrack.h"
#include "TVirtualGeoTrack.h"
#include "TColor.h"
#include "TROOT.h"
#include "TGeoBBox.h"
#include "TGeoTube.h"
#include "TGeoBoolNode.h"
#include "TGeoCompositeShape.h"

DetectorAddOnsWindow::DetectorAddOnsWindow(QWidget * parent, MainWindow * MW, DetectorClass * detector) :
  AGuiWindow("detector", parent),
  ui(new Ui::DetectorAddOnsWindow),
  MW(MW), Detector(detector)
{
  ui->setupUi(this);

  Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
  windowFlags |= Qt::WindowCloseButtonHint;
  this->setWindowFlags(windowFlags);

  ui->pbBackToSandwich->setEnabled(false);

  // world tree widget
  twGeo = new AGeoTreeWidget(Detector->Sandwich);
  ui->saGeo->setWidget(twGeo);
  connect(twGeo, SIGNAL(RequestListOfParticles(QStringList&)), Detector->MpCollection, SLOT(OnRequestListOfParticles(QStringList&)));
  connect(twGeo, &AGeoTreeWidget::RequestShowMonitor, this, &DetectorAddOnsWindow::OnrequestShowMonitor);

  // prototype tree widget
  ui->saPrototypes->setWidget(twGeo->twPrototypes);

  // Object editor
  QVBoxLayout* l = new QVBoxLayout();
  l->setContentsMargins(0,0,0,0);
  ui->frObjectEditor->setLayout(l);
  l->addWidget(twGeo->GetEditWidget());
  connect(twGeo, &AGeoTreeWidget::RequestRebuildDetector, this, &DetectorAddOnsWindow::onReconstructDetectorRequest);
  connect(twGeo, &AGeoTreeWidget::RequestFocusObject,     this, &DetectorAddOnsWindow::FocusVolume);
  connect(twGeo, &AGeoTreeWidget::RequestHighlightObject, this, &DetectorAddOnsWindow::ShowObject);
  connect(twGeo, &AGeoTreeWidget::RequestShowObjectRecursive, this, &DetectorAddOnsWindow::ShowObjectRecursive);
  connect(twGeo, &AGeoTreeWidget::RequestShowAllInstances, this, &DetectorAddOnsWindow::ShowAllInstances);
  connect(twGeo->GetEditWidget(), &AGeoWidget::requestEnableGeoConstWidget, this, &DetectorAddOnsWindow::onRequestEnableGeoConstWidget);
  connect(twGeo, &AGeoTreeWidget::RequestNormalDetectorDraw, MW, &MainWindow::ShowGeometrySlot);
  connect(twGeo, &AGeoTreeWidget::RequestShowPrototypeList, this, &DetectorAddOnsWindow::onRequestShowPrototypeList);
  connect(Detector->Sandwich, &ASandwich::RequestGuiUpdate, this, &DetectorAddOnsWindow::onSandwichRebuild);
  QPalette palette = ui->frObjectEditor->palette();
  palette.setColor( backgroundRole(), QColor( 240, 240, 240 ) );
  ui->frObjectEditor->setPalette( palette );
  ui->frObjectEditor->setAutoFillBackground( true );

  connect(this, &DetectorAddOnsWindow::requestDelayedRebuildAndRestoreDelegate, twGeo, &AGeoTreeWidget::rebuildDetectorAndRestoreCurrentDelegate, Qt::QueuedConnection);

  QPalette p = ui->pteTP->palette();
  p.setColor(QPalette::Active, QPalette::Base, QColor(220,220,220));
  p.setColor(QPalette::Inactive, QPalette::Base, QColor(220,220,220));
  ui->pteTP->setPalette(p);
  ui->pteTP->setReadOnly(true);

  //UpdateGUI();  // on load will trigger, even if default startup detector not found,. will trigger rebuild detector -> update gui

  QDoubleValidator* dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);
  QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
  for (QLineEdit * w : list) if (w->objectName().startsWith("led")) w->setValidator(dv);

  ui->cbAutoCheck->setChecked( MW->GlobSet.PerformAutomaticGeometryCheck );
  on_cbAutoCheck_stateChanged(111);

  if (!MW->PythonScriptWindow) ui->actionTo_Python->setEnabled(false);
  ui->saPrototypes->setVisible(false);

  connect(ui->menuUndo_redo, &QMenu::aboutToShow, this, &DetectorAddOnsWindow::updateMenuIndication);
}

DetectorAddOnsWindow::~DetectorAddOnsWindow()
{
    delete ui; ui = nullptr;
}

void DetectorAddOnsWindow::onReconstructDetectorRequest()
{
  //qDebug() << "onReconstructDetectorRequest triggered";
  if (MW->DoNotUpdateGeometry) return; //if bulk update in progress

  MW->ReconstructDetector();
  if (!Detector->ErrorString.isEmpty())
  {
      message("Errors were detected during detector construction:\n\n" + Detector->ErrorString, this);
  }

  if (ui->cbAutoCheck->isChecked())
  {
      int nooverlaps = MW->CheckUpWindow->CheckGeoOverlaps();
      if (nooverlaps != 0) MW->CheckUpWindow->show();
  }
}

void DetectorAddOnsWindow::UpdateGUI()
{
    qDebug() << ">DAwindow: updateGui";
    UpdateGeoTree();
    UpdateDummyPMindication();
    ui->pbBackToSandwich->setEnabled(!Detector->isGDMLempty());

    QString str = "Show prototypes";
    int numProto = Detector->Sandwich->Prototypes->HostedObjects.size();
    if (numProto > 0) str += QString(" (%1)").arg(numProto);
    ui->cbShowPrototypes->setText(str);
    QFont font = ui->cbShowPrototypes->font(); font.setBold(numProto > 0); ui->cbShowPrototypes->setFont(font);
}

void DetectorAddOnsWindow::SetTab(int tab)
{
  if (tab<0) return;
  if (tab>ui->tabwidAddOns->count()-1) return;
  ui->tabwidAddOns->setCurrentIndex(tab);
}

void DetectorAddOnsWindow::on_pbConvertToDummies_clicked()
{
    QList<int> ToAdd;
    bool ok = ExtractNumbersFromQString(ui->lePMlist->text(), &ToAdd);
    if (!ok)
    {
        message("Input error!", this);
        return;
    }

    int iMaxPM = MW->PMs->count()-1;
    for (int & i : ToAdd)
        if (i < 0 || i > iMaxPM)
        {
            message("Range error!", this);
            return;
        }

    bool SawLower = false;
    bool SawUpper = false;
    qSort(ToAdd.begin(), ToAdd.end()); //sorted - starts from smallest

    for (int iadd=ToAdd.size()-1; iadd>-1; iadd--)
    {
        //adding dummy
        APMdummyStructure dpm;

        int ipm = ToAdd[iadd];
        //       qDebug()<<"PM->dummy  ToAddindex="<<iadd<<"pm number="<<ipm;

        const APm &PM = MW->PMs->at(ipm);
        dpm.r[0] = PM.x;
        dpm.r[1] = PM.y;
        dpm.r[2] = PM.z;

        dpm.Angle[0] = PM.phi;
        dpm.Angle[1] = PM.theta;
        dpm.Angle[2] = PM.psi;

        dpm.PMtype = PM.type;
        dpm.UpperLower = PM.upperLower;

        Detector->PMdummies.append(dpm);
        //       qDebug()<<"dummy added";

        //deleting PM
        int ul, index;
        Detector->findPM(ipm, ul, index);
        if (index == -1)
        {
            message("Something went wrong...", this);
            return;
        }
        Detector->PMarrays[ul].PositionsAnglesTypes.remove(index);
        //       qDebug()<<"PM removed";
        if (ul == 0) SawUpper = true;
        if (ul == 1) SawLower = true;
    }
    //  qDebug()<<"All list done!";

    //updating array type
    if (SawUpper)
    {
        if (Detector->PMarrays[0].Regularity == 0)
            Detector->PMarrays[0].Regularity = 1;
    }
    else if (SawLower)
    {
        if (Detector->PMarrays[1].Regularity == 0)
            Detector->PMarrays[1].Regularity = 1;
    }

    MW->updatePMArrayDataIndication();
    MW->NumberOfPMsHaveChanged();
    MW->ReconstructDetector();
}

void DetectorAddOnsWindow::on_sbDummyPMindex_valueChanged(int arg1)
{
  if (arg1 > Detector->PMdummies.count()-1 )
    {
      ui->sbDummyPMindex->setValue(0);
      if (Detector->PMdummies.count() == 0) return;
    }
  UpdateDummyPMindication();
}

void DetectorAddOnsWindow::on_pbDeleteDummy_clicked()
{
  int idpm = ui->sbDummyPMindex->value();
  if (idpm < Detector->PMdummies.size()) Detector->PMdummies.remove(idpm);
  else
    {
      message("Error: index is out of bounds!", this);
      return;
    }
  if (idpm > Detector->PMdummies.size()-1)
    if (idpm != 0) ui->sbDummyPMindex->setValue(Detector->PMdummies.size()-1);
  UpdateDummyPMindication();
  MW->ReconstructDetector();
}

void DetectorAddOnsWindow::on_pbConvertDummy_clicked()
{
  int idpm = ui->sbDummyPMindex->value();
  if (idpm >= Detector->PMdummies.size())
    {
      message("Error: index is out of bounds!", this);
      return;
    }

  ConvertDummyToPM(idpm);

  if (idpm > Detector->PMdummies.size()-1)
    if (idpm != 0) ui->sbDummyPMindex->setValue(Detector->PMdummies.size()-1);
  UpdateDummyPMindication();
  MW->ReconstructDetector();
}

void DetectorAddOnsWindow::ConvertDummyToPM(int idpm)
{
  APMdummyStructure* dum = &Detector->PMdummies[idpm];
  int ul = dum->UpperLower;
  Detector->PMarrays[ul].PositionsAnglesTypes.append(APmPosAngTypeRecord(dum->r[0], dum->r[1], dum->r[2],
                                                                         dum->Angle[0], dum->Angle[1], dum->Angle[2],
                                                                         dum->PMtype ));
  //removing dummy
  Detector->PMdummies.remove(idpm);
}

void DetectorAddOnsWindow::UpdateGeoTree(QString name)
{
    twGeo->UpdateGui(name);
    updateGeoConstsIndication();
}

void DetectorAddOnsWindow::ShowTab(int tab)
{
    if (tab>-1 && tab<ui->tabwidAddOns->count())
        ui->tabwidAddOns->setCurrentIndex(tab);
}

void DetectorAddOnsWindow::on_pbConvertAllToPMs_clicked()
{
   for (int idpm = Detector->PMdummies.size()-1; idpm>-1; idpm--)
     DetectorAddOnsWindow::ConvertDummyToPM(idpm);
   ui->sbDummyPMindex->setValue(Detector->PMdummies.size()-1);
   MW->ReconstructDetector();
}

void DetectorAddOnsWindow::on_pbUpdateDummy_clicked()
{
  //  qDebug() << "Dummy PMs size:"<<  Detector->PMdummies.size();
  int idpm = ui->sbDummyPMindex->value();
  if (idpm >= Detector->PMdummies.size())
    {
      message("Error: index is out of bounds!", this);
      return;
    }

  int type = ui->sbDummyType->value();
  if (type >= MW->PMs->countPMtypes())
    {
      message("Error: PM type is out of bounds!", this);
      return;
    }

  APMdummyStructure dpm;
  dpm.PMtype = type;
  dpm.UpperLower = ui->cobDummyUpperLower->currentIndex();
  dpm.r[0] = ui->ledDummyX->text().toDouble();
  dpm.r[1] = ui->ledDummyY->text().toDouble();
  dpm.r[2] = ui->ledDummyZ->text().toDouble();
  dpm.Angle[0] = ui->ledDummyPhi->text().toDouble();
  dpm.Angle[1] = ui->ledDummyTheta->text().toDouble();
  dpm.Angle[2] = ui->ledDummyPsi->text().toDouble();
  Detector->PMdummies[idpm] = dpm;

  UpdateDummyPMindication();
  MW->ReconstructDetector();
}

void DetectorAddOnsWindow::on_pbCreateNewDummy_clicked()
{
  int type = ui->sbDummyType->value();
  if (type >= MW->PMs->countPMtypes())
    {
      message("Error: PM type is out of bounds!", this);
      return;
    }

  APMdummyStructure dpm;
  dpm.PMtype = type;
  dpm.UpperLower = ui->cobDummyUpperLower->currentIndex();
  dpm.r[0] = ui->ledDummyX->text().toDouble();
  dpm.r[1] = ui->ledDummyY->text().toDouble();
  dpm.r[2] = ui->ledDummyZ->text().toDouble();
  dpm.Angle[0] = ui->ledDummyPhi->text().toDouble();
  dpm.Angle[1] = ui->ledDummyTheta->text().toDouble();
  dpm.Angle[2] = ui->ledDummyPsi->text().toDouble();
  Detector->PMdummies.append(dpm);

  //DetectorAddOnsWindow::UpdateDummyPMindication();
  ui->sbDummyPMindex->setValue(Detector->PMdummies.size()-1);
  MW->ReconstructDetector();
}

void DetectorAddOnsWindow::UpdateDummyPMindication()
{
  bool bThereAre = !Detector->PMdummies.isEmpty();

  ui->pbDeleteDummy->setEnabled(bThereAre);
  ui->pbUpdateDummy->setEnabled(bThereAre);
  ui->pbConvertDummy->setEnabled(bThereAre);
  ui->frDummyEdit->setEnabled(bThereAre);

  int idpm = ui->sbDummyPMindex->value();
  if (idpm < 0 || idpm >= Detector->PMdummies.count() ) return;

  ui->sbDummyType->setValue(Detector->PMdummies[idpm].PMtype);
  ui->leoDummyType->setText(MW->PMs->getType(Detector->PMdummies[idpm].PMtype)->Name);
  ui->cobDummyUpperLower->setCurrentIndex(Detector->PMdummies[idpm].UpperLower);
  ui->ledDummyX->setText(QString::number(Detector->PMdummies[idpm].r[0]));
  ui->ledDummyY->setText(QString::number(Detector->PMdummies[idpm].r[1]));
  ui->ledDummyZ->setText(QString::number(Detector->PMdummies[idpm].r[2]));
  ui->ledDummyPhi->setText(QString::number(Detector->PMdummies[idpm].Angle[0]));
  ui->ledDummyTheta->setText(QString::number(Detector->PMdummies[idpm].Angle[1]));
  ui->ledDummyPsi->setText(QString::number(Detector->PMdummies[idpm].Angle[2]));
}

void DetectorAddOnsWindow::on_sbDummyType_valueChanged(int arg1)
{
  if (arg1 >= MW->PMs->countPMtypes())
    {      
      ui->sbDummyType->setValue(0);
      return;
    }
}

void DetectorAddOnsWindow::on_pbLoadDummyPMs_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load file with dummy PMs", MW->GlobSet.LastOpenDir, "Text files (*.dat *.txt);;All files (*.*)");
    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
    Detector->PMdummies.resize(0);
    loadDummyPMs(fileName);
    ui->sbDummyPMindex->setValue(0);
    UpdateDummyPMindication();
    MW->ReconstructDetector();
}

void DetectorAddOnsWindow::loadDummyPMs(const QString & DFile)
{
    QFile file(DFile);
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
        message("Cannot open file with dummy PMs:\n"+file.fileName()+"\n"+file.errorString(), this);
    else
    {
        QTextStream in(&file);
        QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
        while(!in.atEnd())
        {
            QString line = in.readLine();
            QStringList fields = line.split(rx, QString::SkipEmptyParts);
            if (fields.size() == 8)
            {
                APMdummyStructure dpm;
                dpm.PMtype = fields[0].toInt();
                dpm.UpperLower = fields[1].toInt();
                dpm.r[0] = fields[2].toDouble();
                dpm.r[1] = fields[3].toDouble();
                dpm.r[2] = fields[4].toDouble();
                dpm.Angle[0] = fields[5].toDouble();
                dpm.Angle[1] = fields[6].toDouble();
                dpm.Angle[2] = fields[7].toDouble();
                Detector->PMdummies.append(dpm);
            }
        }
        file.close();
    }
}

//------------//-----------------//----------------------------//

void DetectorAddOnsWindow::ShowObject(QString name)
{
    DetectorAddOnsWindow::HighlightVolume(name);
    MW->GeometryWindow->SetAsActiveRootWindow();
    Detector->GeoManager->ClearTracks();
    MW->GeometryWindow->ShowGeometry(true, false, false);
}

void DetectorAddOnsWindow::FocusVolume(QString name)
{
    MW->GeometryWindow->FocusVolume(name);
}

bool drawIfFound(TGeoNode* node, TString name)
{
    //qDebug() << node->GetName()<<"  of  "<<node->GetVolume()->GetName();
    if (node->GetName() == name)
    {
        //qDebug() << "Found!!!";
        TGeoVolume* vol = node->GetVolume();
        //qDebug() << vol->CountNodes();
        vol->SetLineColor(2);
        gGeoManager->SetTopVisible(true);
        vol->Draw("2");
        return true;
    }

    int totNodes = node->GetNdaughters();
    //qDebug() << "Nodes:"<<totNodes;
    for (int i=0; i<totNodes; i++)
    {
        //qDebug() << "#"<<i;
        TGeoNode* daugtherNode = node->GetDaughter(i);
        //qDebug() << daugtherNode;
        if ( drawIfFound(daugtherNode, name) ) return true;
      }
    return false;
}

void DetectorAddOnsWindow::ShowObjectRecursive(QString name)
{
    MW->GeometryWindow->ShowAndFocus();

    TString tname = name.toLatin1().data();
    tname += "_0";
    bool found = drawIfFound(Detector->GeoManager->GetTopNode(), tname);
    if (!found)
    {
        tname = name.toLatin1().data();
        tname += "_1";
        drawIfFound(Detector->GeoManager->GetTopNode(), tname);
    }
    MW->GeometryWindow->UpdateRootCanvas();
}

void DetectorAddOnsWindow::ShowAllInstances(QString name)
{
    QVector<AGeoObject*> InstancesNotDiscriminated;
    Detector->Sandwich->World->findAllInstancesRecursive(InstancesNotDiscriminated);

    MW->GeometryWindow->ShowAndFocus();

    TObjArray * list = Detector->GeoManager->GetListOfVolumes();
    const int size = list->GetEntries();
    QSet<QString> set;

    //select those active ones which have the same prototype
    for (AGeoObject * inst : InstancesNotDiscriminated)
    {
        const ATypeInstanceObject * insType = static_cast<const ATypeInstanceObject*>(inst->ObjectType);
        if (insType->PrototypeName == name && inst->fActive)
            for (AGeoObject * obj : inst->HostedObjects)
            {
                /*
                TString tname = obj->Name.toLatin1().data();
                tname += "_0";
                qDebug() << "Looking for" << tname;
                bool found = drawIfFound(Detector->GeoManager->GetTopNode(), tname);
                if (!found)
                {
                    tname = name.toLatin1().data();
                    tname += "_1";
                    drawIfFound(Detector->GeoManager->GetTopNode(), tname);
                }*/

                //HighlightVolume(obj->Name);

                if (obj->ObjectType->isArray() || obj->ObjectType->isHandlingSet())
                {
                    QVector<AGeoObject*> vec;
                    obj->collectContainingObjects(vec);
                    for (AGeoObject * obj1 : vec)
                        set << obj1->Name;
                }
                else    set << obj->Name;
            }

        for (int iVol = 0; iVol < size; iVol++)
        {
            TGeoVolume* vol = (TGeoVolume*)list->At(iVol);
            if (!vol) break;
            const QString name = vol->GetName();
            if (set.contains(name))
            {
                vol->SetLineColor(kRed);
                vol->SetLineWidth(3);
            }
            else vol->SetLineColor(kGray);
        }
    }

    MW->GeometryWindow->UpdateRootCanvas();
}

void DetectorAddOnsWindow::OnrequestShowMonitor(const AGeoObject * mon)
{
    if (!mon->ObjectType->isMonitor())
    {
        qWarning() << "This is not a monitor!";
        return;
    }

    const ATypeMonitorObject * tmo = static_cast<const ATypeMonitorObject*>(mon->ObjectType);
    const AMonitorConfig & c = tmo->config;

    double length1 = c.size1;
    double length2 = c.size2;
    if (c.shape == 1) length2 = length1;

    Detector->GeoManager->ClearTracks();
    Int_t track_index = Detector->GeoManager->AddTrack(1,22);
    TVirtualGeoTrack * track = Detector->GeoManager->GetTrack(track_index);

    double worldPos[3];
    mon->getPositionInWorld(worldPos);
    const double & x = worldPos[0];
    const double & y = worldPos[1];
    const double & z = worldPos[2];
    //qDebug() << "World pos:"<< x << y << z;

    double hl[3] = {-length1, 0, 0}; //local coordinates
    double vl[3] = {0, -length2, 0}; //local coordinates
    double mhl[3]; //master coordinates (world)
    double mvl[3]; //master coordinates (world)

    TGeoNavigator * navigator = gGeoManager->GetCurrentNavigator();
    if (!navigator)
    {
        qDebug() << "Show monitor: Current navigator does not exist, creating new";
        navigator = gGeoManager->AddNavigator();
    }
    navigator->FindNode(x, y, z);
    //qDebug() << navigator->GetCurrentVolume()->GetName();
    navigator->LocalToMasterVect(hl, mhl); //qDebug() << mhl[0]<< mhl[1]<< mhl[2];
    navigator->LocalToMasterVect(vl, mvl);

    track->AddPoint(x+mhl[0], y+mhl[1], z+mhl[2], 0);
    track->AddPoint(x-mhl[0], y-mhl[1], z-mhl[2], 0);
    track->AddPoint(x, y, z, 0);
    track->AddPoint(x+mvl[0], y+mvl[1], z+mvl[2], 0);
    track->AddPoint(x-mvl[0], y-mvl[1], z-mvl[2], 0);
    track->SetLineWidth(4);
    track->SetLineColor(kBlack);

    //show orientation
    double l[3] = {0,0, std::max(length1,length2)}; //local coordinates
    double m[3]; //master coordinates (world)
    navigator->LocalToMasterVect(l, m);
    if (c.bUpper)
    {
        track_index = Detector->GeoManager->AddTrack(1,22);
        track = Detector->GeoManager->GetTrack(track_index);
        track->AddPoint(x, y, z, 0);
        track->AddPoint(x+m[0], y+m[1], z+m[2], 0);
        track->SetLineWidth(4);
        track->SetLineColor(kRed);
    }
    if (c.bLower)
    {
        track_index = Detector->GeoManager->AddTrack(1,22);
        track = Detector->GeoManager->GetTrack(track_index);
        track->AddPoint(x, y, z, 0);
        track->AddPoint(x-m[0], y-m[1], z-m[2], 0);
        track->SetLineWidth(4);
        track->SetLineColor(kRed);
    }
    MW->GeometryWindow->DrawTracks();
}

void DetectorAddOnsWindow::onRequestEnableGeoConstWidget(bool flag)
{
    ui->tabwConstants->setEnabled(flag);
}

void DetectorAddOnsWindow::HighlightVolume(const QString & VolName)
{
    AGeoObject * obj = Detector->Sandwich->World->findObjectByName(VolName);
    if (!obj) return;

    QSet<QString> set;
    if (obj->ObjectType->isArray() || obj->ObjectType->isHandlingSet())
    {
        QVector<AGeoObject*> vec;
        obj->collectContainingObjects(vec);
        for (AGeoObject * obj : vec)
            set << obj->Name;
    }
    else    set << VolName;

    TObjArray* list = Detector->GeoManager->GetListOfVolumes();
    int size = list->GetEntries();

    for (int iVol = 0; iVol < size; iVol++)
    {
        TGeoVolume* vol = (TGeoVolume*)list->At(iVol);
        if (!vol) break;
        const QString name = vol->GetName();
        if (set.contains(name))
        {
            vol->SetLineColor(kRed);
            vol->SetLineWidth(3);
        }
        else vol->SetLineColor(kGray);
    }
}

/*
void DetectorAddOnsWindow::on_pbUseScriptToAddObj_clicked()
{
    QList<QTreeWidgetItem*> list = twGeo->selectedItems();
    if (list.size() == 1) ObjectScriptTarget = list.first()->text(0);
    else ObjectScriptTarget = "World";

    AGeoObject* obj = Detector->Sandwich->World->findObjectByName(ObjectScriptTarget);
    if (!obj)
      {
        ObjectScriptTarget = "World";
        obj = Detector->Sandwich->World;
      }
    if (obj->LastScript.isEmpty())
      Detector->AddObjPositioningScript = Detector->Sandwich->World->LastScript;
    else
      Detector->AddObjPositioningScript = obj->LastScript;

    MW->extractGeometryOfLocalScriptWindow();
    delete MW->GenScriptWindow; MW->GenScriptWindow = 0;

    AJavaScriptManager* jsm = new AJavaScriptManager(MW->Detector->RandGen);
    MW->GenScriptWindow = new AScriptWindow(jsm, true, this);

    QString example = "ClearAll()\nfor (var i=0; i<3; i++)\n Box('Test'+i, 10,5,2, 0, 'PrScint', (i-1)*20,i*2,-i*5,  0,0,0)";
    QString title = ( ObjectScriptTarget.isEmpty() ? "Add objects script" : QString("Add objects script. Script will be stored in object ") + ObjectScriptTarget );
    MW->GenScriptWindow->ConfigureForLightMode(&Detector->AddObjPositioningScript, title, example);

    AddObjScriptInterface = new AGeo_SI(Detector);
    MW->GenScriptWindow->RegisterInterfaceAsGlobal(AddObjScriptInterface);
    MW->GenScriptWindow->RegisterCoreInterfaces();

    connect(AddObjScriptInterface, &AGeo_SI::AbortScriptEvaluation, this, &DetectorAddOnsWindow::ReportScriptError);
    connect(AddObjScriptInterface, &AGeo_SI::requestShowCheckUpWindow, MW->CheckUpWindow, &CheckUpWindowClass::showNormal);
    connect(MW->GenScriptWindow, &AScriptWindow::success, this, &DetectorAddOnsWindow::AddObjScriptSuccess);

    MW->recallGeometryOfLocalScriptWindow();
    MW->GenScriptWindow->UpdateGui();
    MW->GenScriptWindow->show();
}
*/

void DetectorAddOnsWindow::AddObjScriptSuccess()
{
  AddObjScriptInterface->UpdateGeometry(ui->cbAutoCheck->isChecked());
  if (!ObjectScriptTarget.isEmpty())
    {
      AGeoObject* obj = Detector->Sandwich->World->findObjectByName(ObjectScriptTarget);
      if (obj)
        obj->LastScript = Detector->AddObjPositioningScript;
  }
}

void DetectorAddOnsWindow::ReportScriptError(QString ErrorMessage)
{
    MW->GenScriptWindow->clearOutputText();
    MW->GenScriptWindow->ReportError(ErrorMessage, -1);
}

void DetectorAddOnsWindow::on_pbSaveTGeo_clicked()
{
  QString starter = MW->GlobSet.LastOpenDir;
  QFileDialog *fileDialog = new QFileDialog;
  fileDialog->setDefaultSuffix("gdml");
  QString fileName = fileDialog->getSaveFileName(this, "Export detector geometry", starter, "GDML files (*.gdml)");
  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QString err = Detector->exportToGDML(fileName);

  if (!err.isEmpty()) message(err, this);
}

void ShowNodes(const TGeoNode* node, int level)
{
    int numNodes = node->GetNdaughters();
    qDebug() << QString("+").repeated(level)
             << node->GetName()<<node->GetVolume()->GetName()<<node->GetVolume()->GetShape()->ClassName()<<numNodes;

    for (int i=0; i<numNodes; i++)
        ShowNodes(node->GetDaughter(i), level+1);
}

#include <TVector3.h>
#include <TMatrixD.h>
#include <TMath.h>

// input: R - rotation matrix
// return: vector of Euler angles in X-convention (Z0, X, Z1)
// based on the pseudocode by David Eberly from
// https://www.geometrictools.com/Documentation/EulerAngles.pdf
TVector3 euler(TMatrixD R)
{
    double tol = 1e-6; // tolerance to detect special cases
    double Pi = TMath::Pi();
    double thetaZ0, thetaX, thetaZ1; // Euler angles

    if (R(2,2) < 1.-tol) {
        if (R(2,2) > -1.+tol) {
            thetaX = acos(R(2,2));
            thetaZ0 = atan2(R(0,2), -R(1,2));
            thetaZ1 = atan2(R(2,0), R(2,1));
        } else { // r22 == -1.
            thetaX = Pi;
            thetaZ0 = -atan2(-R(0,1), R(0,0));
            thetaZ1 = 0.;
        }
    } else { // r22 == +1.
        thetaX = 0.;
        thetaZ0 = atan2(-R(0,1), R(0,0));
        thetaZ1 = 0.;
    }
    return TVector3(thetaZ0, thetaX, thetaZ1);
}

void processNonComposite(QString Name, TGeoShape* Tshape, const TGeoMatrix* Matrix, QVector<AGeoObject*>& LogicalObjects)
{    
    //qDebug() << Name;
    TGeoTranslation trans(*Matrix);
    //qDebug() << "Translation:"<<trans.GetTranslation()[0]<<trans.GetTranslation()[1]<<trans.GetTranslation()[2];
    TGeoRotation rot(*Matrix);
    double phi, theta, psi;
    rot.GetAngles(phi, theta, psi);
    //qDebug() << "Rotation:"<<phi<<theta<<psi;

    AGeoObject* GeoObj = new AGeoObject(Name);
    for (int i=0; i<3; i++) GeoObj->Position[i] = trans.GetTranslation()[i];
    GeoObj->Orientation[0] = phi; GeoObj->Orientation[1] = theta; GeoObj->Orientation[2] = psi;
    delete GeoObj->Shape;
    GeoObj->Shape = AGeoShape::GeoShapeFactory(Tshape->ClassName());
    if (!GeoObj->Shape)
    {
        qWarning() << "Unknown TGeoShape:"<<Tshape->ClassName();
        GeoObj->Shape = new AGeoBox();
    }
    bool fOK = GeoObj->Shape->readFromTShape(Tshape);
    if (!fOK)
    {
        qWarning() << "Not implemented: import data from"<<Tshape->ClassName()<< "to ANTS2 object";
        GeoObj->Shape = new AGeoBox();
    }
    LogicalObjects << GeoObj;
}

bool isLogicalObjectsHaveName(const QVector<AGeoObject*>& LogicalObjects, const QString name)
{
    for (AGeoObject* obj : LogicalObjects)
        if (obj->Name == name) return true;
    return false;
}

void processTCompositeShape(TGeoCompositeShape* Tshape, QVector<AGeoObject*>& LogicalObjects, QString& GenerationString )
{
    TGeoBoolNode* n = Tshape->GetBoolNode();
    if (!n)
    {
        qWarning() << "Failed to read BoolNode in TCompositeShape!";
    }

    TGeoBoolNode::EGeoBoolType operation = n->GetBooleanOperator(); // kGeoUnion, kGeoIntersection, kGeoSubtraction
    QString operationStr;
    switch (operation)
    {
    case TGeoBoolNode::kGeoUnion:
        operationStr = " + "; break;
    case TGeoBoolNode::kGeoIntersection:
        operationStr = " * "; break;
    case TGeoBoolNode::kGeoSubtraction:
        operationStr = " - "; break;
    default:
        qCritical() << "Unknown EGeoBoolType!";
        exit(333111);
    }
    //qDebug() << "UnionIntersectSubstr:"<<operationStr;

    TGeoShape* left = n->GetLeftShape();
    QString leftName;
    TGeoCompositeShape* CompositeShape = dynamic_cast<TGeoCompositeShape*>(left);
    if (CompositeShape)
    {
        QString tmp;
        processTCompositeShape(CompositeShape, LogicalObjects, tmp);
        if (tmp.isEmpty())
            qWarning() << "Error processing TGeoComposite: no generation string obtained";
        leftName = " (" + tmp + ") ";
    }
    else
    {
        QString leftNameBase = leftName = left->GetName();
        while (isLogicalObjectsHaveName(LogicalObjects, leftName))
            leftName = leftNameBase + "_" + AGeoObject::GenerateRandomName();       
        processNonComposite(leftName, left, n->GetLeftMatrix(), LogicalObjects);
    }

    TGeoShape* right = n->GetRightShape();
    QString rightName;
    CompositeShape = dynamic_cast<TGeoCompositeShape*>(right);
    if (CompositeShape)
    {
        QString tmp;
        processTCompositeShape(CompositeShape, LogicalObjects, tmp);
        if (tmp.isEmpty())
            qWarning() << "Error processing TGeoComposite: no generation string obtained";
        rightName = " (" + tmp + ") ";
    }
    else
    {
        QString rightNameBase = rightName = right->GetName();
        while (isLogicalObjectsHaveName(LogicalObjects, rightName))
            rightName = rightNameBase + "_" + AGeoObject::GenerateRandomName();        
        processNonComposite(rightName, right, n->GetRightMatrix(), LogicalObjects);
    }

    GenerationString = " " + leftName + operationStr + rightName + " ";
    //qDebug() << leftName << operationStr << rightName;
}


void readGeoObjectTree(AGeoObject* obj, const TGeoNode* node,
                       AMaterialParticleCollection* mp, const QString PMtemplate,
                       DetectorClass* Detector, TGeoNavigator* navi, TString path)
{
    obj->Name = node->GetName();
    //qDebug() << "\nNode name:"<<obj->Name<<"Num nodes:"<<node->GetNdaughters();
    path += node->GetName();

    //material
    QString mat = node->GetVolume()->GetMaterial()->GetName();
    obj->Material = mp->FindMaterial(mat);
    obj->color = obj->Material+1;
    obj->fExpanded = true;

    //shape
    TGeoShape* Tshape = node->GetVolume()->GetShape();
    QString Sshape = Tshape->ClassName();
    //qDebug() << "TGeoShape:"<<Sshape;
    AGeoShape* Ashape = AGeoShape::GeoShapeFactory(Sshape);
    bool fOK = false;
    if (!Ashape) qWarning() << "TGeoShape was not recognized - using box";
    else
    {
        delete obj->Shape; //delete default one
        obj->Shape = Ashape;

        fOK = Ashape->readFromTShape(Tshape);  //composite has special procedure!
        if (Ashape->getShapeType() == "TGeoCompositeShape")
        {
            //TGeoShape -> TGeoCompositeShape
            TGeoCompositeShape* tshape = static_cast<TGeoCompositeShape*>(Tshape);

            //AGeoObj converted to composite type:
            delete obj->ObjectType;
            obj->ObjectType = new ATypeCompositeObject();
            //creating container for logical objects:
            AGeoObject* logicals = new AGeoObject();
            logicals->Name = "CompositeSet_"+obj->Name;
            delete logicals->ObjectType;
            logicals->ObjectType = new ATypeCompositeContainerObject();
            obj->addObjectFirst(logicals);

            QVector<AGeoObject*> AllLogicalObjects;
            QString GenerationString;
            processTCompositeShape(tshape, AllLogicalObjects, GenerationString);            
            AGeoComposite* cshape = static_cast<AGeoComposite*>(Ashape);
            cshape->GenerationString = "TGeoCompositeShape(" + GenerationString + ")";
            for (AGeoObject* ob : AllLogicalObjects)
            {
                ob->Material = obj->Material;
                logicals->addObjectLast(ob);
                cshape->members << ob->Name;
            }
            //qDebug() << cshape->GenerationString;// << cshape->members;

            fOK = true;
        }
    }
    //qDebug() << "Shape:"<<Sshape<<"Read success:"<<fOK;
    if (!fOK) qDebug() << "Failed to read shape for:"<<obj->Name;

    //position + angles
    const TGeoMatrix* matrix = node->GetMatrix();
    TGeoTranslation trans(*matrix);
    for (int i=0; i<3; i++) obj->Position[i] = trans.GetTranslation()[i];
    TGeoRotation mrot(*matrix);
    mrot.GetAngles(obj->Orientation[0], obj->Orientation[1], obj->Orientation[2]);
    //qDebug() << "xyz:"<<obj->Position[0]<< obj->Position[1]<< obj->Position[2]<< "phi,theta,psi:"<<obj->Orientation[0]<<obj->Orientation[1]<<obj->Orientation[2];

    //hosted nodes
    int totNodes = node->GetNdaughters();
    //qDebug() << "Number of hosted nodes:"<<totNodes;
    for (int i=0; i<totNodes; i++)
    {
        TGeoNode* daugtherNode = node->GetDaughter(i);
        QString name = daugtherNode->GetName();
        //qDebug() << i<< name;


        if (name.startsWith(PMtemplate))
        {
            //qDebug() << "  Found PM!";
            //qDebug() << "  path:"<<path+"/"+daugtherNode->GetName();
            navi->cd(path+"/"+daugtherNode->GetName());
            //qDebug() << navi->GetCurrentNode()->GetName();
            double PosLocal[3] = {0,0,0};
            double PosGlobal[3];
            navi->LocalToMaster(PosLocal, PosGlobal);

            double VecLocalX[3] = {1,0,0};
            double VecLocalY[3] = {0,1,0};
            double VecLocalZ[3] = {0,0,1};
            double VecGlobal[3];
            TMatrixD rm(3,3);
            navi->LocalToMasterVect(VecLocalX, VecGlobal);
            for (int i=0; i<3; i++) rm(i,0) = VecGlobal[i];
            navi->LocalToMasterVect(VecLocalY, VecGlobal);
            for (int i=0; i<3; i++) rm(i,1) = VecGlobal[i];
            navi->LocalToMasterVect(VecLocalZ, VecGlobal);
            for (int i=0; i<3; i++) rm(i,2) = VecGlobal[i];

            TVector3 eu = euler(rm);
            double radToGrad = 180.0/TMath::Pi();
            double phi = eu[0]*radToGrad;
            double theta = eu[1]*radToGrad;
            double psi = eu[2]*radToGrad;

            TGeoVolume* vol = daugtherNode->GetVolume();
            for (int PmType = 0;PmType<Detector->PMs->countPMtypes(); PmType++)
                if (vol == Detector->PMs->getType(PmType)->tmpVol)
                {
                     //qDebug() << " Registering as type:"<<PmType;
                     Detector->PMarrays[0].PositionsAnglesTypes.append(APmPosAngTypeRecord(PosGlobal[0], PosGlobal[1], PosGlobal[2],
                                                                                           phi, theta, psi, PmType));
                     break;
                }
        }
        else
        {
            //not a PM
            AGeoObject* inObj = new AGeoObject(name);
            obj->addObjectLast(inObj);
            readGeoObjectTree(inObj, daugtherNode, mp, PMtemplate, Detector, navi, path+"/");
        }
    }    
}

bool DetectorAddOnsWindow::GDMLtoTGeo(const QString& fileName)
{
    QString txt;
    bool bOK = LoadTextFromFile(fileName, txt);
    if (!bOK)
    {
        message("Cannot read the file", this);
        return false;
    }

    if (txt.contains("unit=\"cm\"") || txt.contains("unit=\"m\""))
    {
        message("Cannot load GDML files with length units other than \"mm\"", this);
        return false;
    }

    txt.replace("unit=\"mm\"", "unit=\"cm\"");
    QString tmpFileName = MW->GlobSet.TmpDir + "/gdmlTMP.gdml";
    bOK = SaveTextToFile(tmpFileName, txt);
    if (!bOK)
    {
        message("Conversion failed - tmp file cannot be allocated", this);
        return false;
    }

    Detector->GeoManager = TGeoManager::Import(tmpFileName.toLatin1());
    QFile(tmpFileName).remove();
    return true;
}

void DetectorAddOnsWindow::on_pmParseInGeometryFromGDML_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load GDML file", MW->GlobSet.LastOpenDir, "GDML files (*.gdml)");
    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
    QFileInfo fi(fileName);
    if (fi.suffix() != "gdml")
      {
        message("Only GDML files are accepted!", this);
        return;
      }

    MW->Config->LoadConfig(MW->GlobSet.ExamplesDir + "/Empty.json");

    QString PMtemplate = ui->lePMtemplate->text();
    if (PMtemplate.isEmpty()) PMtemplate = "_.._#"; //clumsy, but otherwise propagate changes to readGeoObjectTree

    delete Detector->GeoManager; Detector->GeoManager = nullptr;
    GDMLtoTGeo(fileName.toLatin1());
    if (!Detector->GeoManager || !Detector->GeoManager->IsClosed())
    {
        message("Load failed!", this);
        Detector->BuildDetector();
        return;
    }
    qDebug() << "--> tmp GeoManager loaded from GDML file";

    const TGeoNode* top = Detector->GeoManager->GetTopNode();
    //ShowNodes(top, 0); //just qDebug output

    //==== materials ====
    AMaterialParticleCollection tmpMats;
    TObjArray* list = Detector->GeoManager->GetListOfVolumes();
    int size = list->GetEntries();
    qDebug() << "  Number of defined volumes:"<<size;
    for (int i=0; i<size; i++)
    {
        TGeoVolume* vol = (TGeoVolume*)list->At(i);
        QString MatName = vol->GetMaterial()->GetName();
        int iMat = tmpMats.FindMaterial(MatName);
        if (iMat == -1)
        {
          tmpMats.AddNewMaterial(MatName);
          qDebug() << "Added mat:"<<MatName;
        }
    }
    QJsonObject mats;
    tmpMats.writeToJson(mats);
    Detector->MpCollection->readFromJson(mats);

    //==== PM types ====
    Detector->PMs->clearPMtypes();
    Detector->PMarrays[0].Regularity = 2;
    Detector->PMarrays[0].fActive = true;
    Detector->PMarrays[1].fActive = false;
    Detector->PMarrays[0].PositionsAnglesTypes.clear();
    Detector->PMarrays[1].PositionsAnglesTypes.clear();
    int counter = 0;
    for (int i=0; i<size; i++)
    {
        TGeoVolume* vol = (TGeoVolume*)list->At(i);
        QString Vname = vol->GetName();
        if (!Vname.startsWith(PMtemplate)) continue;

        QString PMshape = vol->GetShape()->ClassName();
        qDebug() << "Found new PM type:"<<Vname<<"Shape:"<<PMshape;
        APmType *type = new APmType();
        type->Name = PMtemplate + QString::number(counter);
        type->MaterialIndex = tmpMats.FindMaterial(vol->GetMaterial()->GetName());
        type->tmpVol = vol;
        if (PMshape=="TGeoBBox")
        {
            TGeoBBox* b = static_cast<TGeoBBox*>(vol->GetShape());
            type->SizeX = 2.0*b->GetDX();
            type->SizeY = 2.0*b->GetDY();
            type->SizeZ = 2.0*b->GetDZ();
            type->Shape = 0;
        }
        else if (PMshape=="TGeoTube" || PMshape=="TGeoTubeSeg")
        {
            TGeoTube* b = static_cast<TGeoTube*>(vol->GetShape());
            type->SizeX = 2.0*b->GetRmax();
            type->SizeZ = 2.0*b->GetDz();
            type->Shape = 1;
        }
        else
        {
            qWarning() << "non-implemented sensor shape:"<<PMshape<<" - making cylinder";
            type->SizeX = 12.3456789;
            type->SizeZ = 12.3456789;
            type->Shape = 1;
        }
        Detector->PMs->appendNewPMtype(type);
        counter++;
    }
    if (counter==0) Detector->PMs->appendNewPMtype(new APmType()); //maybe there are no PMs in the file or template error

    //==== geometry ====
    qDebug() << "Processing geometry";
    Detector->Sandwich->clearWorld();
    readGeoObjectTree(Detector->Sandwich->World, top, &tmpMats, PMtemplate, Detector, Detector->GeoManager->GetCurrentNavigator(), "/");
    Detector->Sandwich->World->makeItWorld(); //just to reset the name
    AGeoBox * wb = dynamic_cast<AGeoBox*>(Detector->Sandwich->World->Shape);
    if (wb)
    {
        Detector->Sandwich->setWorldSizeXY( std::max(wb->dx, wb->dy) );
        Detector->Sandwich->setWorldSizeZ(wb->dz);
    }
    Detector->Sandwich->setWorldSizeFixed(wb);

    Detector->GeoManager->FindNode(0,0,0);
    //qDebug() << "----------------------------"<<Detector->GeoManager->GetPath();

    Detector->writeToJson(MW->Config->JSON);
    //SaveJsonToFile(MW->Config->JSON, "D:/temp/CONFIGJSON.json");
    qDebug() << "Rebuilding detector...";
    Detector->BuildDetector(); 
}

QString DetectorAddOnsWindow::loadGDML(const QString& fileName, QString& gdml)
{
    QFileInfo fi(fileName);
    if (fi.suffix() != "gdml")
        return "Only GDML files are accepted!";

    QFile f(fileName);
    if (!f.open(QFile::ReadOnly | QFile::Text))
        return QString("Cannot open file %1").arg(fileName);

    QTextStream in(&f);
    gdml = in.readAll();

    if (gdml.contains("unit=\"cm\"") || gdml.contains("unit=\"m\""))
        return "Cannot load GDML files with length units other than \"mm\"";

    gdml.replace("unit=\"mm\"", "unit=\"cm\"");
    return "";
}

void DetectorAddOnsWindow::resizeEvent(QResizeEvent *event)
{
    if (!isVisible()) return;

    int AllW = ui->tabwConstants->width() - 3;

    int SecW = AllW * 0.33333;
    if (SecW > 50) SecW = 50;

    int FirstPlusThird = AllW - SecW;

    int FirstW = 0.4 * FirstPlusThird;
    if (FirstW > 150) FirstW = 150;

    ui->tabwConstants->setColumnWidth(0, FirstW);
    ui->tabwConstants->setColumnWidth(1, SecW);
    ui->tabwConstants->setColumnWidth(2, FirstPlusThird - FirstW);

    AGuiWindow::resizeEvent(event);
}

void DetectorAddOnsWindow::on_pbLoadTGeo_clicked()
{
    QString gdml;

    QString fileName = QFileDialog::getOpenFileName(this, "Load GDML file", MW->GlobSet.LastOpenDir, "GDML files (*.gdml)");
    if (fileName.isEmpty()) return;
    MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    const QString err = loadGDML(fileName, gdml);
    if (!err.isEmpty())
    {
        message(err, this);
        return;
    }

    //attempting to load and validity check
    bool fOK = Detector->importGDML(gdml);
    if (fOK)
    {
        //qDebug() << "--> GDML successfully registered";
        MW->NumberOfPMsHaveChanged();
        MW->GeometryWindow->ShowGeometry();
    }
    else message(Detector->ErrorString, this);

    MW->onGDMLstatusChage(fOK); //update MW GUI
    ui->pbBackToSandwich->setEnabled(fOK);
}

void DetectorAddOnsWindow::on_pbBackToSandwich_clicked()
{
  Detector->clearGDML();
  MW->onGDMLstatusChage(false); //update MW GUI
  MW->ReconstructDetector();
  ui->pbBackToSandwich->setEnabled(false);
}

void DetectorAddOnsWindow::on_pbRootWeb_clicked()
{
  QDesktopServices::openUrl( QUrl("http://gdml.web.cern.ch/GDML/doc/GDMLmanual.pdf") );
}

void DetectorAddOnsWindow::on_pbCheckGeometry_clicked()
{
    MW->CheckUpWindow->CheckGeoOverlaps();
    MW->CheckUpWindow->show();
}

void DetectorAddOnsWindow::on_cbAutoCheck_clicked(bool checked)
{
    MW->GlobSet.PerformAutomaticGeometryCheck = checked;
    if (!checked) MW->CheckUpWindow->hide();
}

void DetectorAddOnsWindow::on_pbRunTestParticle_clicked()
{
   ui->pteTP->clear();
   AGeometryTester Tester(Detector->GeoManager);

   double Start[3];
   double Dir[3];

   Start[0] = ui->ledTPx->text().toDouble();
   Start[1] = ui->ledTPy->text().toDouble();
   Start[2] = ui->ledTPz->text().toDouble();
   Dir[0]   = ui->ledTPi->text().toDouble();
   Dir[1]   = ui->ledTPj->text().toDouble();
   Dir[2]   = ui->ledTPk->text().toDouble();

   NormalizeVector(Dir);
   //qDebug() << Dir[0]<<Dir[1]<<Dir[2];

   Tester.Test(Start, Dir);

   ui->pteTP->appendHtml("Objects on the path:");
   for (int i=0; i<Tester.Record.size(); i++)
   {
       const AGeometryTesterReportRecord& r = Tester.Record.at(i);

       QString s;
       s += "<pre>";
       s += " " + r.volName;
       s += " (#" + QString::number(r.nodeIndex) + ")";
       s += " of ";
       TColor* rc = gROOT->GetColor(r.matIndex+1);
       int red = 255*rc->GetRed();
       int green = 255*rc->GetGreen();
       int blue = 255*rc->GetBlue();
       s += "<span style = \"color:rgb("+QString::number(red)+", "+QString::number(green)+", "+QString::number(blue)+")\">";
       s += Detector->MpCollection->getMaterialName(r.matIndex);
       s += "</span>";
       s += ",  from ";
       s += "(" + QString::number(r.startX)+", "+ QString::number(r.startY)+", "+ QString::number(r.startZ)+")";

       if (i != Tester.Record.size()-1)
       {
           s += "  to  ";
           const AGeometryTesterReportRecord& r1 = Tester.Record.at(i+1);
           s += "("+QString::number(r1.startX)+", "+ QString::number(r1.startY)+", "+ QString::number(r1.startZ)+")";
           double dx2 = r.startX - r1.startX; dx2 *= dx2;
           double dy2 = r.startY - r1.startY; dy2 *= dy2;
           double dz2 = r.startZ - r1.startZ; dz2 *= dz2;
           double distance = sqrt( dx2 + dy2 + dz2 );
           s += "  Distance: "+QString::number(distance) + " mm.";
       }
       else
       {
           s += " and until escaped.";
       }
       s += "</pre>";

       ui->pteTP->appendHtml(s);
   }

   if (MW->GeometryWindow->isVisible())
   {
       Detector->GeoManager->ClearTracks();

       for (int i=0; i<Tester.Record.size(); i++)
       {
           const AGeometryTesterReportRecord& r = Tester.Record.at(i);
           TGeoTrack* track = new TGeoTrack(1, 10);
           track->SetLineColor(r.matIndex+1);
           track->SetLineWidth(4);
           track->AddPoint(r.startX, r.startY, r.startZ, 0);
           if (i != Tester.Record.size()-1)
           {
               const AGeometryTesterReportRecord& r1 = Tester.Record.at(i+1);
               track->AddPoint(r1.startX, r1.startY, r1.startZ, 0);
           }
           else
               track->AddPoint(Tester.escapeX, Tester.escapeY, Tester.escapeZ, 0);

          Detector->GeoManager->AddTrack(track);
       }

       MW->GeometryWindow->ShowGeometry(false);
       MW->GeometryWindow->DrawTracks();
   }
}

void DetectorAddOnsWindow::on_cbAutoCheck_stateChanged(int)
{
  bool checked = ui->cbAutoCheck->isChecked();
  QColor col = (checked ? Qt::black : Qt::red);
  QPalette p = ui->cbAutoCheck->palette();
  p.setColor(QPalette::Active, QPalette::WindowText, col );
  p.setColor(QPalette::Inactive, QPalette::WindowText, col );
  ui->cbAutoCheck->setPalette(p);
}

#include "aonelinetextedit.h"
#include "ageobasedelegate.h"
#include <QTabWidget>
void DetectorAddOnsWindow::updateGeoConstsIndication()
{
    ui->tabwConstants->clearContents();

    const AGeoConsts & GC = AGeoConsts::getConstInstance();
    const int numConsts = GC.countConstants();

    bGeoConstsWidgetUpdateInProgress = true; // -->
        ui->tabwConstants->setRowCount(numConsts + 1);
        ui->tabwConstants->setColumnWidth(1, 50);
        for (int i = 0; i <= numConsts; i++)
        {
            const QString Name  =      ( i == numConsts ? ""  : GC.getName(i));
            const QString Value =      ( i == numConsts ? "0" : QString::number(GC.getValue(i)) );
            const QString Expression = ( i == numConsts ? ""  : GC.getExpression(i) );

            QTableWidgetItem * newItem = new QTableWidgetItem(Name);
            QString Comment = GC.getComment(i);
            newItem->setToolTip(Comment);
            ui->tabwConstants->setItem(i, 0, newItem);

            ALineEditWithEscape * edit = new ALineEditWithEscape(Value, ui->tabwConstants);
            edit->setValidator(new QDoubleValidator(edit));
            edit->setFrame(false);
            connect(edit, &ALineEditWithEscape::editingFinished, [this, i, edit](){this->onGeoConstEditingFinished(i, edit->text()); });
            connect(edit, &ALineEditWithEscape::escapePressed,   [this, i](){this->onGeoConstEscapePressed(i); });
            ui->tabwConstants->setCellWidget(i, 1, edit);

            AOneLineTextEdit * ed = new AOneLineTextEdit(ui->tabwConstants);
            AGeoBaseDelegate::configureHighligherAndCompleter(ed, i);
            ed->setText(Expression);
            ed->setFrame(false);
            connect(ed, &AOneLineTextEdit::editingFinished, [this, i, ed](){this->onGeoConstExpressionEditingFinished(i, ed->text()); });
            connect(ed, &AOneLineTextEdit::escapePressed,   [this, i](){this->onGeoConstEscapePressed(i); });
            ui->tabwConstants->setCellWidget(i, 2, ed);

            if (!Expression.isEmpty()) edit->setEnabled(false);
        }
    bGeoConstsWidgetUpdateInProgress = false; // <--
}

QString DetectorAddOnsWindow::createScript(QString &script, bool usePython)
{
    QString CommentStr = "//";
    int indent = 0;
    QString VarStr;
    QString indentStr;

    script += "== Auto-generated script ==\n\n";

    if (!usePython)
    {
        VarStr = "var ";
        indentStr = ""; //"  ";
    }
    else
    {
        CommentStr = "#";
        indent = 0;
        script += "true = True\n\nfalse = False\n\n";     // for now
    }
    script.insert(0, CommentStr);

    script += indentStr + CommentStr + "Defined materials:\n";
    for (int i=0; i<Detector->MpCollection->countMaterials(); i++)
        script += indentStr + VarStr + Detector->MpCollection->getMaterialName(i) + "_mat = " + QString::number(i) + "\n";
    script += "\n";

    AGeoObject * World = Detector->Sandwich->World;
    QString geoScr = AGeoConsts::getConstInstance().exportToScript(World, CommentStr, VarStr);
    if (!geoScr.simplified().isEmpty())
    {
        script += indentStr + CommentStr + "Geometry constants:\n";
        script += geoScr;
    }

    //script += indentStr + CommentStr + "Set all PM arrays to fully custom regularity, so PM Z-positions will not be affected by slabs\n";
    //script += indentStr + "pms.SetAllArraysFullyCustom()\n";
    //script += indentStr + CommentStr + "Remove all slabs and objects\n";
    script += indentStr + "geo.RemoveAllExceptWorld()\n";
    script += "\n";

    twGeo->commonSlabToScript(script, indentStr);
    script += "\n";

    QString protoString;
    twGeo->objectMembersToScript(Detector->Sandwich->Prototypes, protoString, indent, true, true, usePython);
    if (!protoString.simplified().isEmpty())
    {
        script += indentStr + CommentStr + "Prototypes:";
        script += protoString;
        script += "\n\n";
    }

    script += indentStr + CommentStr + "Geometry:";
    twGeo->objectMembersToScript(World, script, indent, true, true, usePython);

    script += "\n\n" + indentStr + "geo.UpdateGeometry(true)";

    return script;
}

void DetectorAddOnsWindow::onGeoConstEditingFinished(int index, QString strNewValue)
{
    //qDebug() << "GeoConst value changed! index/text are:" << index << strNewValue;
    AGeoConsts & GC = AGeoConsts::getInstance();

    if (index == GC.countConstants()) return; // nothing to do yet - this constant is not yet defined

    bool ok;
    double val = strNewValue.toDouble(&ok);
    if (!ok)
    {
        message("Bad format of the edited value of geometry constant!", this);
        updateGeoConstsIndication();
        return;
    }

    if (val == GC.getValue(index)) return;

    GC.setNewValue(index, val);
    emit requestDelayedRebuildAndRestoreDelegate();
}

void DetectorAddOnsWindow::onGeoConstExpressionEditingFinished(int index, QString newValue)
{
    //qDebug() << "Geo const expression changed! index/text are:" << index << newValue;
    AGeoConsts & GC = AGeoConsts::getInstance();

    if (index == GC.countConstants()) return; // nothing to do yet - this constant is not yet defined
    bool ok;
    newValue.toDouble(&ok);
    if (ok)
    {
        onGeoConstEditingFinished(index, newValue);
        return;
    }

    if (newValue == GC.getExpression(index)) return;

    QString errorStr = GC.setNewExpression(index, newValue);
    if (!errorStr.isEmpty())
    {
        message(errorStr, this);
        updateGeoConstsIndication();
        return;
    }

    emit requestDelayedRebuildAndRestoreDelegate();
}

void DetectorAddOnsWindow::onGeoConstEscapePressed(int /*index*/)
{
    updateGeoConstsIndication();
}

void DetectorAddOnsWindow::onRequestShowPrototypeList()
{
    ui->cbShowPrototypes->setChecked(true);
}

void DetectorAddOnsWindow::updateMenuIndication()
{
    ui->actionUndo->setEnabled(MW->Config->isUndoAvailable());
    ui->actionRedo->setEnabled(MW->Config->isRedoAvailable());
}

void DetectorAddOnsWindow::onSandwichRebuild()
{
    qDebug() << "--onSandwichRebuild";
    twGeo->UpdateGui("");
}

void DetectorAddOnsWindow::on_tabwConstants_cellChanged(int row, int column)
{
    if (column != 0) return; // only name change or new
    if (bGeoConstsWidgetUpdateInProgress) return;
    //qDebug() << "Geo const name changed";

    AGeoConsts & GC = AGeoConsts::getInstance();
    const int numConsts = GC.countConstants();

    if (numConsts == row)
    {
        //qDebug() << "Attempting to add new geometry constant";
        QString Name = ui->tabwConstants->item(row, 0)->text().simplified();

        QLineEdit * le = dynamic_cast<QLineEdit*>(ui->tabwConstants->cellWidget(row, 1));
        if (!le)
        {
            message("Something went wrong!", this);
            return;
        }
        bool ok;
        double Value = le->text().toDouble(&ok);
        if (!ok) Value = 0;

        QString errorStr = GC.addNewConstant(Name, Value, -1);
        if (!errorStr.isEmpty())
        {
            message(errorStr, this);
            updateGeoConstsIndication();
            return;
        }
        //MW->writeDetectorToJson(MW->Config->JSON);
        emit requestDelayedRebuildAndRestoreDelegate();
    }
    else
    {
        //qDebug() << "Attempting to change name of a geometry constant";
        QString newName = ui->tabwConstants->item(row, 0)->text().simplified();
        QString errorStr;
        bool ok = GC.rename(row, newName, Detector->Sandwich->World, errorStr);
        if (!ok)
        {
            if (!errorStr.isEmpty()) message(errorStr, this);
            updateGeoConstsIndication();
            return;
        }
        else
        {
            emit requestDelayedRebuildAndRestoreDelegate();
        }
    }

    updateGeoConstsIndication();
}

#include <QMenu>
void DetectorAddOnsWindow::on_tabwConstants_customContextMenuRequested(const QPoint &pos)
{
    AGeoConsts & GC = AGeoConsts::getInstance();
    int index = ui->tabwConstants->currentRow();

    QMenu menu;
    QAction * removeA = menu.addAction("Remove selected constant"); removeA->setEnabled(index != -1 && index != GC.countConstants());
    QAction * addAboveA = menu.addAction("Add new constant above"); addAboveA->setEnabled(index != -1 && index != GC.countConstants());

    menu.addSeparator();

    QAction * setCommentA = menu.addAction("Add comment"); setCommentA->setEnabled(index != -1 && index != GC.countConstants());


    QAction * selected = menu.exec(ui->tabwConstants->mapToGlobal(pos));
    if (selected == removeA)
    {
        if (!GC.isIndexValid(index)) return;

        QString name = GC.getName(index);
        if (!name.isEmpty())
        {
            QString constUsingIt = GC.isGeoConstInUse(QRegExp("\\b"+name+"\\b"), index);
            if (!constUsingIt.isEmpty())
            {
                message(QString("\"%1\" cannot be removed.\nThe first geometric constant using it:\n\n%2").arg(name).arg(constUsingIt), this);
                return;
            }
            const AGeoObject * obj = Detector->Sandwich->World->isGeoConstInUseRecursive(QRegExp("\\b"+name+"\\b"));
            if (obj)
            {
                message(QString("\"%1\" cannot be removed.\nThe first object using it:\n\n%2").arg(name).arg(obj->Name), this);
                return;
            }
        }

        GC.removeConstant(index);
        MW->writeDetectorToJson(MW->Config->JSON);
        updateGeoConstsIndication();
        emit requestDelayedRebuildAndRestoreDelegate();
    }
    else if (selected == addAboveA)
    {
        GC.addNoNameConstant(index);
        MW->writeDetectorToJson(MW->Config->JSON);
        updateGeoConstsIndication();
    }
    else if (selected == setCommentA)
    {
        QString txt = inputString("New comment (empty to remove)", this);
        GC.setNewComment(index, txt);
        MW->writeDetectorToJson(MW->Config->JSON);
        updateGeoConstsIndication();
    }
}

void ALineEditWithEscape::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        event->accept();
        emit escapePressed();
    }
    else QLineEdit::keyPressEvent(event);
}

void DetectorAddOnsWindow::on_actionUndo_triggered()
{
    bool ok = MW->Config->isUndoAvailable();
    if (!ok)
        message("Undo is not available!", this);
    else
    {
        QString err = MW->Config->doUndo();
        if (!err.isEmpty()) message(err, this);
    }
}

void DetectorAddOnsWindow::on_actionRedo_triggered()
{
    bool ok = MW->Config->isRedoAvailable();
    if (!ok)
        message("Redo is not available!", this);
    else
    {
        QString err = MW->Config->doRedo();
        if (!err.isEmpty()) message(err, this);
    }
}

void DetectorAddOnsWindow::on_actionHow_to_use_drag_and_drop_triggered()
{
    QString s = "Drag & drop can be used to move items\n"
                "  from one container to another\n"
                "\n"
                "Drop when Alt or Shift or Control is pressed\n"
                "  changes the item order:\n"
                "  the dragged item is inserted between two object\n"
                "  according to the drop indicator";
    message(s, this);
}

void DetectorAddOnsWindow::on_actionTo_JavaScript_triggered()
{
    QString script;
    createScript(script, false);
    MW->ScriptWindow->onLoadRequested(script);
    MW->ScriptWindow->showNormal();
    MW->ScriptWindow->raise();
    MW->ScriptWindow->activateWindow();
}

void DetectorAddOnsWindow::on_actionTo_Python_triggered()
{
    if (!MW->PythonScriptWindow)
    {
        message("ANTS2 was compiled without Python support. Enable it in ants2.pro");
        return;
    }

    QString script;
    createScript(script, true);
    MW->PythonScriptWindow->onLoadRequested(script);
    MW->PythonScriptWindow->showNormal();
    MW->PythonScriptWindow->raise();
    MW->PythonScriptWindow->activateWindow();
}

void DetectorAddOnsWindow::on_cbShowPrototypes_toggled(bool checked)
{
    ui->saPrototypes->setVisible(checked);
}

void DetectorAddOnsWindow::on_actionAdd_top_lightguide_triggered()
{
    if (!Detector->Sandwich->World->containsUpperLightGuide())
        twGeo->AddLightguide(true);
}

void DetectorAddOnsWindow::on_actionAdd_bottom_lightguide_triggered()
{
    if (!Detector->Sandwich->World->containsLowerLightGuide())
        twGeo->AddLightguide(false);
}
