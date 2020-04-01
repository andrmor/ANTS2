//ANTS2
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aparticlesourcerecord.h"
#include "outputwindow.h"
#include "apmhub.h"
#include "eventsdataclass.h"
#include "windownavigatorclass.h"
#include "geometrywindowclass.h"
#include "asourceparticlegenerator.h"
#include "afileparticlegenerator.h"
#include "ascriptparticlegenerator.h"
#include "graphwindowclass.h"
#include "detectorclass.h"
#include "checkupwindowclass.h"
#include "aglobalsettings.h"
#include "amaterialparticlecolection.h"
#include "ageomarkerclass.h"
#include "ajsontools.h"
#include "aparticlerecord.h"
#include "amessage.h"
#include "acommonfunctions.h"
#include "guiutils.h"
#include "aparticlesourcedialog.h"
#include "asimulationmanager.h"
#include "exampleswindow.h"
#include "aconfiguration.h"

//Qt
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QHBoxLayout>

//Root
#include "TVirtualGeoTrack.h"
#include "TGeoManager.h"
#include "TH1D.h"

void MainWindow::SimParticleSourcesConfigToJson(QJsonObject &json)
{
    QJsonObject psjs;
        //control options
        QJsonObject cjs;
            QString str;
                switch (ui->twParticleGenerationMode->currentIndex())
                {
                default: qWarning() << "Save sim config: unknown particle generation mode";
                case 0 : str = "Sources"; break;
                case 1 : str = "File"; break;
                case 2 : str = "Script"; break;
                }
            cjs["ParticleGenerationMode"] = str;
            cjs["EventsToDo"] = ui->sbGunEvents->text().toDouble();
            cjs["AllowMultipleParticles"] = ui->cbGunAllowMultipleEvents->isChecked(); //--->ps
            cjs["AverageParticlesPerEvent"] = ui->ledGunAverageNumPartperEvent->text().toDouble(); //--->ps
            cjs["TypeParticlesPerEvent"] = ui->cobPartPerEvent->currentIndex(); //--->ps
            cjs["DoS1"] = ui->cbGunDoS1->isChecked();
            cjs["DoS2"] = ui->cbGunDoS2->isChecked();
            cjs["IgnoreNoHitsEvents"] = ui->cbIgnoreEventsWithNoHits->isChecked();
            cjs["IgnoreNoDepoEvents"] = ui->cbIgnoreEventsWithNoEnergyDepo->isChecked();
            cjs["ClusterMergeRadius"] = ui->ledClusterRadius->text().toDouble();
        psjs["SourceControlOptions"] = cjs;

        //Particle generation
        //--particle sources
        SimulationManager->ParticleSources->writeToJson(psjs);
        //--from file
        QJsonObject fjs;
            SimulationManager->FileParticleGenerator->writeToJson(fjs);
        psjs["GenerationFromFile"] = fjs;
        //--from script
        QJsonObject sjs;
            SimulationManager->ScriptParticleGenerator->writeToJson(sjs);
        psjs["GenerationFromScript"] = sjs;

    json["ParticleSourcesConfig"] = psjs;
}

void MainWindow::ShowSource(const AParticleSourceRecord* p, bool clear)
{
  int index = p->shape;
  double X0 = p->X0;
  double Y0 = p->Y0;
  double Z0 = p->Z0;
  double Phi = p->Phi*3.1415926535/180.0;
  double Theta = p->Theta*3.1415926535/180.0;
  double Psi = p->Psi*3.1415926535/180.0;
  double size1 = p->size1;
  double size2 = p->size2;
  double size3 = p->size3;
  double CollPhi = p->CollPhi*3.1415926535/180.0;
  double CollTheta = p->CollTheta*3.1415926535/180.0;
  double Spread = p->Spread*3.1415926535/180.0;

  //calculating unit vector along 1D direction
  TVector3 VV(sin(Theta)*sin(Phi), sin(Theta)*cos(Phi), cos(Theta));
  //qDebug()<<VV[0]<<VV[1]<<VV[2];
  if (clear) Detector->GeoManager->ClearTracks();

  TVector3 V[3];
  V[0].SetXYZ(size1, 0, 0);
  V[1].SetXYZ(0, size2, 0);
  V[2].SetXYZ(0, 0, size3);
  for (int i=0; i<3; i++)
   {
    V[i].RotateX(Phi);
    V[i].RotateY(Theta);
    V[i].RotateZ(Psi);
   }
  switch (index)
    {
     case (1):
      { //linear source
        Int_t track_index = Detector->GeoManager->AddTrack(1,22);
        TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);
        track->AddPoint(X0+VV[0]*size1, Y0+VV[1]*size1, Z0+VV[2]*size1, 0);
        track->AddPoint(X0-VV[0]*size1, Y0-VV[1]*size1, Z0-VV[2]*size1, 0);
        track->SetLineWidth(3);
        track->SetLineColor(9);
        break;
      }
     case (2):
      { //area source - square
        Int_t track_index = Detector->GeoManager->AddTrack(1,22);
        TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);
        track->AddPoint(X0-V[0][0]-V[1][0], Y0-V[0][1]-V[1][1], Z0-V[0][2]-V[1][2], 0);
        track->AddPoint(X0+V[0][0]-V[1][0], Y0+V[0][1]-V[1][1], Z0+V[0][2]-V[1][2], 0);
        track->AddPoint(X0+V[0][0]+V[1][0], Y0+V[0][1]+V[1][1], Z0+V[0][2]+V[1][2], 0);
        track->AddPoint(X0-V[0][0]+V[1][0], Y0-V[0][1]+V[1][1], Z0-V[0][2]+V[1][2], 0);
        track->AddPoint(X0-V[0][0]-V[1][0], Y0-V[0][1]-V[1][1], Z0-V[0][2]-V[1][2], 0);
        track->SetLineWidth(3);
        track->SetLineColor(9);
        break;
      }
     case (3):
      { //area source - round
        Int_t track_index = Detector->GeoManager->AddTrack(1,22);
        TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);
        TVector3 Circ;
        for (int i=0; i<51; i++)
        {
            double x = size1*cos(3.1415926535/25.0*i);
            double y = size1*sin(3.1415926535/25.0*i);
            Circ.SetXYZ(x,y,0);
            Circ.RotateX(Phi);
            Circ.RotateY(Theta);
            Circ.RotateZ(Psi);
            track->AddPoint(X0+Circ[0], Y0+Circ[1], Z0+Circ[2], 0);
        }
        track->SetLineWidth(3);
        track->SetLineColor(9);
        break;
      }

    case (4):
      { //volume source - box
        for (int i=0; i<3; i++)
         for (int j=0; j<3; j++)
           {
             if (j==i) continue;
             //third k
             int k = 0;
             for (; k<2; k++)
               if (k!=i && k!=j) break;
             for (int s=-1; s<2; s+=2)
               {
                //  qDebug()<<"i j k shift"<<i<<j<<k<<s;
                Int_t track_index = Detector->GeoManager->AddTrack(1,22);
                TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);
                track->AddPoint(X0-V[i][0]-V[j][0]+V[k][0]*s, Y0-V[i][1]-V[j][1]+V[k][1]*s, Z0-V[i][2]-V[j][2]+V[k][2]*s, 0);
                track->AddPoint(X0+V[i][0]-V[j][0]+V[k][0]*s, Y0+V[i][1]-V[j][1]+V[k][1]*s, Z0+V[i][2]-V[j][2]+V[k][2]*s, 0);
                track->AddPoint(X0+V[i][0]+V[j][0]+V[k][0]*s, Y0+V[i][1]+V[j][1]+V[k][1]*s, Z0+V[i][2]+V[j][2]+V[k][2]*s, 0);
                track->AddPoint(X0-V[i][0]+V[j][0]+V[k][0]*s, Y0-V[i][1]+V[j][1]+V[k][1]*s, Z0-V[i][2]+V[j][2]+V[k][2]*s, 0);
                track->AddPoint(X0-V[i][0]-V[j][0]+V[k][0]*s, Y0-V[i][1]-V[j][1]+V[k][1]*s, Z0-V[i][2]-V[j][2]+V[k][2]*s, 0);
                track->SetLineWidth(3);
                track->SetLineColor(9);
               }
           }
        break;
      }
    case(5):
      { //volume source - cylinder
      TVector3 Circ;
      Int_t track_index = Detector->GeoManager->AddTrack(1,22);
      TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);
      double z = size3;
      for (int i=0; i<51; i++)
      {
          double x = size1*cos(3.1415926535/25.0*i);
          double y = size1*sin(3.1415926535/25.0*i);
          Circ.SetXYZ(x,y,z);
          Circ.RotateX(Phi);
          Circ.RotateY(Theta);
          Circ.RotateZ(Psi);
          track->AddPoint(X0+Circ[0], Y0+Circ[1], Z0+Circ[2], 0);
      }
      track->SetLineWidth(3);
      track->SetLineColor(9);
      track_index = Detector->GeoManager->AddTrack(1,22);
      track = Detector->GeoManager->GetTrack(track_index);
      z = -z;
      for (int i=0; i<51; i++)
            {
                double x = size1*cos(3.1415926535/25.0*i);
                double y = size1*sin(3.1415926535/25.0*i);
                Circ.SetXYZ(x,y,z);
                Circ.RotateX(Phi);
                Circ.RotateY(Theta);
                Circ.RotateZ(Psi);
                track->AddPoint(X0+Circ[0], Y0+Circ[1], Z0+Circ[2], 0);
            }
      track->SetLineWidth(3);
      track->SetLineColor(9);
      break;
      }
    }

  TVector3 K(sin(CollTheta)*sin(CollPhi), sin(CollTheta)*cos(CollPhi), cos(CollTheta)); //collimation direction
  Int_t track_index = Detector->GeoManager->AddTrack(1,22);
  TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);
  double Klength = std::max(Detector->WorldSizeXY, Detector->WorldSizeZ)*0.5; //20 before

  track->AddPoint(X0, Y0, Z0, 0);
  track->AddPoint(X0+K[0]*Klength, Y0+K[1]*Klength, Z0+K[2]*Klength, 0);
  track->SetLineWidth(2);
  track->SetLineColor(9);

  TVector3 Knorm = K.Orthogonal();
  TVector3 K1(K);
  K1.Rotate(Spread, Knorm);
  for (int i=0; i<8; i++)  //drawing spread
  {
      Int_t track_index = Detector->GeoManager->AddTrack(1,22);
      TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);

      track->AddPoint(X0, Y0, Z0, 0);
      track->AddPoint(X0+K1[0]*Klength, Y0+K1[1]*Klength, Z0+K1[2]*Klength, 0);
      K1.Rotate(3.1415926535/4.0, K);

      track->SetLineWidth(1);
      track->SetLineColor(9);
  }

  GeometryWindow->DrawTracks();
  //Detector->GeoManager->SetCurrentPoint(X0,Y0,Z0);
  //Detector->GeoManager->DrawCurrentPoint(9);
}

void MainWindow::on_pbGunTest_clicked()
{
    GeometryWindow->ShowAndFocus();
    gGeoManager->ClearTracks();

    if (ui->twParticleGenerationMode->currentIndex() == 0)
    {
        if (ui->pbGunShowSource->isChecked())
        {
            for (int i=0; i<SimulationManager->ParticleSources->countSources(); i++)
                ShowSource(SimulationManager->ParticleSources->getSource(i), false);
        }
    }
    else GeometryWindow->ShowGeometry();

    AParticleGun* pg;
    switch (ui->twParticleGenerationMode->currentIndex())
    {
    case 0: pg = SimulationManager->ParticleSources; break;
    case 1: pg = SimulationManager->FileParticleGenerator; break;
    case 2: pg = SimulationManager->ScriptParticleGenerator; break;
    default:
        message("This generation mode is not implemented!", this);
        return;
    }

    ui->pbStopScan->setEnabled(true);
    QFont font = ui->pbStopScan->font();
    font.setBold(true);
    ui->pbStopScan->setFont(font);
    WindowNavigator->BusyOn();

    TestParticleGun(pg, ui->sbGunTestEvents->value()); //script generator is aborted on click of the stop button!

    ui->pbStopScan->setEnabled(false);
    ui->pbStopScan->setText("stop");
    font.setBold(false);
    ui->pbStopScan->setFont(font);
    WindowNavigator->BusyOff();
}

void MainWindow::TestParticleGun(AParticleGun* Gun, int numParticles)
{
    clearGeoMarkers();

    bool bOK = Gun->Init();
    if (!bOK)
    {
        message("Failed to initialize particle gun!\n" + Gun->GetErrorString(), this);
        return;
    }
    Gun->SetStartEvent(0);

    double Length = std::max(Detector->WorldSizeXY, Detector->WorldSizeZ)*0.4;
    double R[3], K[3];
    QVector<AParticleRecord*> GP;
    for (int iRun=0; iRun<numParticles; iRun++)
    {
        bool bOK = Gun->GenerateEvent(GP);
        if (!bOK) break;
//        if (GP.isEmpty() && iRun > 2)
//        {
//            message("Did several attempts but no particles were generated!", this);
//            break;
//        }
        for (const AParticleRecord * p : GP)
        {
            R[0] = p->r[0];
            R[1] = p->r[1];
            R[2] = p->r[2];

            K[0] = p->v[0];
            K[1] = p->v[1];
            K[2] = p->v[2];

            int track_index = Detector->GeoManager->AddTrack(1, 22);
            TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);
            track->AddPoint(R[0], R[1], R[2], 0);
            track->AddPoint(R[0] + K[0]*Length, R[1] + K[1]*Length, R[2] + K[2]*Length, 0);
            SimulationManager->TrackBuildOptions.applyToParticleTrack(track, p->Id);

            GeoMarkerClass* marks = new GeoMarkerClass("t", 7, 1, SimulationManager->TrackBuildOptions.getParticleColor(p->Id));
            marks->SetNextPoint(R[0], R[1], R[2]);
            GeoMarkers.append(marks);

            delete p;
        }
        GP.clear();
    }

    GeometryWindow->ShowTracksAndMarkers();
}

void MainWindow::on_ledGunAverageNumPartperEvent_editingFinished()
{
    double val = ui->ledGunAverageNumPartperEvent->text().toDouble();
    if (val<0)
    {
        message("Average number of particles per event should be more than 0", this);
        ui->ledGunAverageNumPartperEvent->setText("1");
    }
}

void MainWindow::on_pbRemoveSource_clicked()
{
    int isource = ui->lwDefinedParticleSources->currentRow();
    if (isource == -1)
    {
        message("Select a source to remove", this);
        return;
    }
    if (isource >= SimulationManager->ParticleSources->countSources())
    {
        message("Error - bad source index!", this);
        return;
    }

    int ret = QMessageBox::question(this, "Remove particle source",
                                    "Are you sure you want to remove source " + SimulationManager->ParticleSources->getSource(isource)->name,
                                    QMessageBox::Yes | QMessageBox::Cancel,
                                    QMessageBox::Cancel);
    if (ret != QMessageBox::Yes) return;

    SimulationManager->ParticleSources->remove(isource);

    on_pbUpdateSimConfig_clicked();
    on_pbUpdateSourcesIndication_clicked();
    if (ui->pbGunShowSource->isChecked())
    {
        if (SimulationManager->ParticleSources->countSources() == 0)
        {
            Detector->GeoManager->ClearTracks();
            GeometryWindow->ShowGeometry(false);
        }
        else
            ShowParticleSource_noFocus();
    }
}

void MainWindow::on_pbAddSource_clicked()
{
    AParticleSourceRecord* s = new AParticleSourceRecord();
    s->GunParticles << new GunParticleStruct();
    SimulationManager->ParticleSources->append(s);

    on_pbUpdateSourcesIndication_clicked();
    ui->lwDefinedParticleSources->setCurrentRow( SimulationManager->ParticleSources->countSources()-1 );
}

void MainWindow::on_pbUpdateSourcesIndication_clicked()
{
    int numSources = SimulationManager->ParticleSources->countSources();

    int curRow = ui->lwDefinedParticleSources->currentRow();
    ui->lwDefinedParticleSources->clear();

    for (int i=0; i<numSources; i++)
    {
        AParticleSourceRecord* pr = SimulationManager->ParticleSources->getSource(i);
        QListWidgetItem* item = new QListWidgetItem();
        ui->lwDefinedParticleSources->addItem(item);

        QFrame* fr = new QFrame();
        fr->setFrameShape(QFrame::Box);
        QHBoxLayout* l = new QHBoxLayout();
        l->setContentsMargins(3, 2, 3, 2);
            QLabel* lab = new QLabel(pr->name);
            lab->setMinimumWidth(110);
            QFont f = lab->font();
            f.setBold(true);
            lab->setFont(f);
        l->addWidget(lab);
        l->addWidget(new QLabel(pr->getShapeString() + ','));
        l->addWidget(new QLabel( QString("%1 particle%2").arg(pr->GunParticles.size()).arg( pr->GunParticles.size()>1 ? "s" : "" ) ) );
        l->addStretch();

        l->addWidget(new QLabel("Fraction:"));
            QLineEdit* e = new QLineEdit(QString::number(pr->Activity));
            e->setMaximumWidth(50);
            e->setMinimumWidth(50);
            QDoubleValidator* val = new QDoubleValidator(this);
            val->setBottom(0);
            e->setValidator(val);
            QObject::connect(e, &QLineEdit::editingFinished, [pr, e, this]()
            {
                double newVal = e->text().toDouble();
                if (pr->Activity == newVal) return;
                pr->Activity = newVal;
                e->clearFocus();
                emit this->RequestUpdateSimConfig();
            });
        l->addWidget(e);

            double totAct = SimulationManager->ParticleSources->getTotalActivity();
            double per = ( totAct == 0 ? 0 : 100.0 * pr->Activity / totAct );
            QString t = QString("%1%").arg(per, 3, 'g', 3);
            lab = new QLabel(t);
            lab->setMinimumWidth(45);
        l->addWidget(lab);

        fr->setLayout(l);
        item->setSizeHint(fr->sizeHint());
        ui->lwDefinedParticleSources->setItemWidget(item, fr);
        item->setSizeHint(fr->sizeHint());

        e->setVisible(numSources > 1);
    }

    if (curRow < 0 || curRow >= ui->lwDefinedParticleSources->count())
        curRow = 0;
    ui->lwDefinedParticleSources->setCurrentRow(curRow);
}

void MainWindow::on_pbGunShowSource_toggled(bool checked)
{
    if (checked)
    {
        GeometryWindow->ShowAndFocus();
        ShowParticleSource_noFocus();
    }
    else
    {
        Detector->GeoManager->ClearTracks();
        GeometryWindow->ShowGeometry();
    }
}

void MainWindow::on_lwDefinedParticleSources_itemDoubleClicked(QListWidgetItem *)
{
    on_pbEditParticleSource_clicked();
}

void MainWindow::on_lwDefinedParticleSources_itemClicked(QListWidgetItem *)
{
    if (ui->pbGunShowSource->isChecked()) ShowParticleSource_noFocus();
}

void MainWindow::ShowParticleSource_noFocus()
{
  int isource = ui->lwDefinedParticleSources->currentRow();
  if (isource < 0) return;
  if (isource >= SimulationManager->ParticleSources->countSources())
    {
      message("Source number is out of bounds!",this);
      return;
    }
  ShowSource(SimulationManager->ParticleSources->getSource(isource), true);
}

void MainWindow::on_pbSaveParticleSource_clicked()
{
    int isource = ui->lwDefinedParticleSources->currentRow();
    if (isource == -1)
    {
        message("Select a source to remove", this);
        return;
    }
    if (isource >= SimulationManager->ParticleSources->countSources())
    {
        message("Error - bad source index!", this);
        return;
    }

    QString starter = (GlobSet.LibParticleSources.isEmpty()) ? GlobSet.LastOpenDir : GlobSet.LibParticleSources;
    QString fileName = QFileDialog::getSaveFileName(this, "Save particle source", starter, "Json files (*.json)");
    if (fileName.isEmpty()) return;
    QFileInfo file(fileName);
    if (file.suffix().isEmpty()) fileName += ".json";

    QJsonObject json, js;
    SimulationManager->ParticleSources->getSource(isource)->writeToJson(json, *MpCollection);
    js["ParticleSource"] = json;
    bool bOK = SaveJsonToFile(js, fileName);
    if (!bOK) message("Failed to save json to file: "+fileName, this);
}

void MainWindow::on_pbLoadParticleSource_clicked()
{
    QString starter = (GlobSet.LibParticleSources.isEmpty()) ? GlobSet.LastOpenDir : GlobSet.LibParticleSources;
    QString fileName = QFileDialog::getOpenFileName(this, "Import particle source", starter, "json files (*.json)");
    if (fileName.isEmpty()) return;

    QJsonObject json, js;
    bool ok = LoadJsonFromFile(json, fileName);
    if (!ok)
    {
        message("Cannot open file: "+fileName, this);
        return;
    }
    if (!json.contains("ParticleSource"))
    {
        message("Json file format error", this);
        return;
    }

    int oldPartCollSize = Detector->MpCollection->countParticles();
    js = json["ParticleSource"].toObject();

    AParticleSourceRecord* ps = new AParticleSourceRecord();
    ps->readFromJson(js, *MpCollection);
    SimulationManager->ParticleSources->append(ps);
    //SimulationManager->ParticleSources->readSourceFromJson( SimulationManager->ParticleSources->countSources()-1, js );

    onRequestDetectorGuiUpdate();
    on_pbUpdateSimConfig_clicked();

    int newPartCollSize = Detector->MpCollection->countParticles();
    if (oldPartCollSize != newPartCollSize)
    {
        //qDebug() << oldPartCollSize <<"->"<<newPartCollSize;
        QString str = "Warning!\nThe following particle";
        if (newPartCollSize-oldPartCollSize>1) str += "s were";
        else str += " was";
        str += " added:\n";
        for (int i=oldPartCollSize; i<newPartCollSize; i++)
            str += Detector->MpCollection->getParticleName(i) + "\n";
        str += "\nSet the interaction data for the detector materials!";
        message(str,this);
    }

    ui->lwDefinedParticleSources->setCurrentRow(ui->lwDefinedParticleSources->count()-1);
    if (ui->pbGunShowSource->isChecked()) ShowParticleSource_noFocus();
}

void MainWindow::on_pbSingleSourceShow_clicked()
{
    double r[3];
    r[0] = ui->ledSingleX->text().toDouble();
    r[1] = ui->ledSingleY->text().toDouble();
    r[2] = ui->ledSingleZ->text().toDouble();
    GeometryWindow->ShowPoint(r);
}

void MainWindow::on_pbEditParticleSource_clicked()
{
    int isource = ui->lwDefinedParticleSources->currentRow();
    if (isource == -1)
    {
        message("Select a source to edit", this);
        return;
    }
    if (isource >= SimulationManager->ParticleSources->countSources())
    {
        message("Error - bad source index!", this);
        return;
    }

    AParticleSourceDialog d(*this, SimulationManager->ParticleSources->getSource(isource));
    int res = d.exec();
    if (res == QDialog::Rejected) return;

    SimulationManager->ParticleSources->replace(isource, d.getResult());

    AParticleSourceRecord* ps = SimulationManager->ParticleSources->getSource(isource);
    SimulationManager->ParticleSources->checkLimitedToMaterial(ps);

    if (Detector->isGDMLempty())
      { //check world size
        double XYm = 0;
        double  Zm = 0;
        for (int isource = 0; isource < SimulationManager->ParticleSources->countSources(); isource++)
          {
            double msize =   ps->size1;
            UpdateMax(msize, ps->size2);
            UpdateMax(msize, ps->size3);

            UpdateMax(XYm, fabs(ps->X0)+msize);
            UpdateMax(XYm, fabs(ps->Y0)+msize);
            UpdateMax(Zm,  fabs(ps->Z0)+msize);
          }

        double currXYm = Detector->WorldSizeXY;
        double  currZm = Detector->WorldSizeZ;
        if (XYm>currXYm || Zm>currZm)
          {
            //need to override
            Detector->fWorldSizeFixed = true;
            Detector->WorldSizeXY = std::max(XYm,currXYm);
            Detector->WorldSizeZ =  std::max(Zm,currZm);
            MainWindow::ReconstructDetector();
          }
      }

    on_pbUpdateSimConfig_clicked();
    if (ui->pbGunShowSource->isChecked()) ShowParticleSource_noFocus();
}

#include "ageoobject.h"
void containsMonsGrids(const AGeoObject * obj, bool & bGrid, bool & bMon)
{
    if (obj->isDisabled()) return;

    if (obj->ObjectType->isMonitor()) bMon = true;
    if (obj->ObjectType->isGrid()) bGrid = true;

    for (const AGeoObject * o : obj->HostedObjects)
        containsMonsGrids(o, bGrid, bMon);
}

#include "asandwich.h"
void MainWindow::on_pbParticleSourcesSimulate_clicked()
{
    MainWindow::writeSimSettingsToJson(Config->JSON);

    ELwindow->QuickSave(0);
    fStartedFromGUI = true;
    fSimDataNotSaved = false; // to disable the warning

    //-- test G4 settings
    if (G4SimSet.bTrackParticles)
    {
        QString Errors, Warnings, txt;

        if (G4SimSet.SensitiveVolumes.isEmpty())
            txt += "Sensitive volumes are not set!\n"
                       "Geant4 simulation will not collect any deposition information\n"
                       "There will be no photon generation in Ants2\n\n";

        bool bMonitors = false;
        bool bGrids = false;
        containsMonsGrids(Detector->Sandwich->World, bGrids, bMonitors);
        /*
        if (bMonitors)
            Warnings+= "\nConfig contains active Monitor(s).\n"
                       "Monitors will be treated as normal volumes\n";
        */
        if (bGrids)
            Warnings+= "\nConfig contains optical grids.\n"
                       "The grid will NOT be expanded:\n"
                       "Geant4 will see only the elementary cell of the grid.\n";

        MpCollection->CheckReadyForGeant4Sim(Errors, Warnings, Detector->Sandwich->World);

        if (!Errors.isEmpty())
        {
            txt += QStringLiteral("\nERRORS:\n");
            txt += Errors;
            txt += '\n';
        }

        if (!Warnings.isEmpty())
        {
            txt += QStringLiteral("\nWarnings:\n");
            txt += Warnings;
        }

        if (!txt.isEmpty())
        {
            QMessageBox msgBox(this);
            msgBox.setText(txt);
            msgBox.setInformativeText("Start simulation?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Cancel) return;
        }
    }
    //

    startSimulation(Config->JSON);
}

// ---- from file ----

void MainWindow::on_pbGenerateFromFile_Help_clicked()
{
    QString s = "File should contain particle records, one line per particle.\n\n"
                "Record format:\n"
                "ParticleId  Energy  StartX  StartY  StartZ  DirX  DirY  DirZ Time *\n\n"
                "where optional '*' indicates that the event is not finished:\n"
                "the next particle will be generated within the same event.\n"
                "Energy in keV, position in mm, direction is unitary vector and time in ns.\n\n"
                "If a line does not start with an integer, it is ignored,\n"
                "but it is recommended to start comments with '#' symbol.";

    QString t = "File generated by G4ants, can be ascii or binary";

    message( (ui->cobGenerateFromFile_FileFormat->currentIndex() == 0 ? s : t), this);
}

void MainWindow::on_pbGenerateFromFile_Change_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select a file with particle generation data", GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
    ui->leGenerateFromFile_FileName->setText(fileName);
    on_leGenerateFromFile_FileName_editingFinished();
}

void MainWindow::on_leGenerateFromFile_FileName_editingFinished()
{
    QString newName = ui->leGenerateFromFile_FileName->text();
    if (newName == SimulationManager->FileParticleGenerator->GetFileName()) return;

    SimulationManager->FileParticleGenerator->SetFileName(newName);
    on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_cobGenerateFromFile_FileFormat_activated(int index)
{
    SimulationManager->FileParticleGenerator->SetFileFormat(static_cast<AFileMode>(index));
    on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_pbGenerateFromFile_Check_clicked()
{
    AFileParticleGenerator* pg = SimulationManager->FileParticleGenerator;
    pg->InvalidateFile();
    if (!pg->Init())
        message(pg->GetErrorString(), this);

    updateFileParticleGeneratorGui();
}

void MainWindow::updateFileParticleGeneratorGui()
{
    AFileParticleGenerator* pg = SimulationManager->FileParticleGenerator;

    ui->leGenerateFromFile_FileName->setText(pg->GetFileName());
    ui->cobGenerateFromFile_FileFormat->setCurrentIndex(static_cast<int>(pg->GetFileFormat()));

    QFileInfo fi(pg->GetFileName());
    if (!fi.exists())
    {
        ui->labGenerateFromFile_info->setText("File not found");
        return;
    }

    QString s;
    if (pg->IsValidated())
    {
        s += QString("%1 events in the file").arg(pg->NumEventsInFile);
        if (pg->statNumEmptyEventsInFile > 0) s += QString(", %1 empty events").arg(pg->statNumEmptyEventsInFile);
        if (pg->statNumMultipleEvents > 0) s += QString(", %1 multiple events").arg(pg->statNumMultipleEvents);

        s += "\n";

        QString pd;
        for (int ip = 0; ip < pg->statParticleQuantity.size(); ip++)
        {
            if (pg->statParticleQuantity.at(ip) > 0)
                pd += QString("  %2 %1\n").arg(MpCollection->getParticleName(ip)).arg(pg->statParticleQuantity.at(ip));
        }
        if (!pd.isEmpty()) s += "Particle distribution:\n" + pd;
    }
    else s = "Click 'Analyse file' to see statistics";

    ui->labGenerateFromFile_info->setText(s);
}

// --- by script

#include "ajavascriptmanager.h"
#include "ascriptwindow.h"
#include "aparticlegenerator_si.h"
#include "amath_si.h"

void MainWindow::updateScriptParticleGeneratorGui()
{
    ui->pteParticleGenerationScript->clear();
    ui->pteParticleGenerationScript->appendPlainText(SimulationManager->ScriptParticleGenerator->GetScript());
}

void MainWindow::on_pbParticleGenerationScript_clicked()
{
    AJavaScriptManager* sm = new AJavaScriptManager(Detector->RandGen);
    AScriptWindow* sw = new AScriptWindow(sm, true, this);
    sw->EnableAcceptReject();

    int NumThreads = 1;
    AParticleGenerator_SI* gen = new AParticleGenerator_SI(*Detector->MpCollection, Detector->RandGen, 0, &NumThreads);
    QVector<AParticleRecord*> GP;
    gen->configure(&GP);
    gen->setObjectName("gen");
    sw->RegisterInterface(gen, "gen"); //takes ownership
    AMath_SI* math = new AMath_SI(Detector->RandGen);
    math->setObjectName("math");
    sw->RegisterInterface(math, "math"); //takes ownership

    sw->UpdateGui();

    QObject::connect(sw, &AScriptWindow::onFinish,
                     [&GP, sw](bool bError)
                     {
                        if (!bError) message(QString("Script generated %1 particle%2").arg(GP.size()).arg(GP.size()==1?"":"s"), sw);
                        for (AParticleRecord* p : GP) delete p;
                        GP.clear();
                     }
    );


    QString Script = SimulationManager->ScriptParticleGenerator->GetScript();
    QString Example = "gen.AddParticle(0,  100+math.random(),  10*math.random(), 10*math.random(), 0,   0, 0, 1)\n"
                      "gen.AddParticleIsotropic(0,  111,  0, 0, -5)";
    sw->ConfigureForLightMode(&Script, "Optical override: custom script", Example);
    sw->setWindowModality(Qt::ApplicationModal);
    sw->show();
    GuiUtils::AssureWidgetIsWithinVisibleArea(sw);

    while (sw->isVisible())
    {
        QCoreApplication::processEvents();
        QThread::usleep(200);
    }

    bool bWasAccepted = sw->isAccepted();
    for (AParticleRecord* p : GP) delete p;
    GP.clear();
    delete sw; //also deletes script manager

    if (bWasAccepted) SimulationManager->ScriptParticleGenerator->SetScript(Script);
    on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_pteParticleGenerationScript_customContextMenuRequested(const QPoint &)
{
    on_pbParticleGenerationScript_clicked();
}
