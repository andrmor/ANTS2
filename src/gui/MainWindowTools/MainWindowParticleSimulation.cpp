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
#include "asandwich.h"
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
                switch (ui->cobParticleGenerationMode->currentIndex())
                {
                default: qWarning() << "During saving sim config: unknown particle generation mode";
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
            cjs["ClusterMerge"] = ui->cbMergeClusters->isChecked();
            cjs["ClusterMergeTime"] = ui->ledClusterTimeDif->text().toDouble();
        psjs["SourceControlOptions"] = cjs;

        //Particle generation
        //--particle sources
        SimulationManager->Settings.partSimSet.SourceGenSettings.writeToJson(psjs);
        //--from file
        QJsonObject fjs;
            SimulationManager->Settings.partSimSet.FileGenSettings.writeToJson(fjs);
        psjs["GenerationFromFile"] = fjs;
        //--from script
        QJsonObject sjs;
            SimulationManager->Settings.partSimSet.ScriptGenSettings.writeToJson(sjs);
        psjs["GenerationFromScript"] = sjs;

    json["ParticleSourcesConfig"] = psjs;
}

void MainWindow::ShowSource(const AParticleSourceRecord * p, bool clear)
{
    if (!p) return;

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
     case 0:
     {
      Detector->GeoManager->SetCurrentPoint(X0,Y0,Z0);
      Detector->GeoManager->DrawCurrentPoint(9);
      GeometryWindow->ClearGeoMarkers();
      GeoMarkerClass* marks = new GeoMarkerClass("Source", 3, 10, kBlack);
      marks->SetNextPoint(X0, Y0, Z0);
      GeometryWindow->GeoMarkers.append(marks);
      GeoMarkerClass* marks1 = new GeoMarkerClass("Source", 4, 3, kBlack);
      marks1->SetNextPoint(X0, Y0, Z0);
      GeometryWindow->GeoMarkers.append(marks1);
      GeometryWindow->ShowGeometry(false);
      break;
     }
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
  const double WorldSizeXY = Detector->Sandwich->getWorldSizeXY();
  const double WorldSizeZ  = Detector->Sandwich->getWorldSizeZ();
  double Klength = std::max(WorldSizeXY, WorldSizeZ)*0.5; //20 before

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
}

void MainWindow::on_pbGunTest_clicked()
{
    WindowNavigator->BusyOn();   // -->

    GeometryWindow->ShowAndFocus();
    Detector->GeoManager->ClearTracks();
    if (ui->cobParticleGenerationMode->currentIndex() == 0)
    {
        if (ui->pbGunShowSource->isChecked())
        {
            for (int i = 0; i < SimulationManager->Settings.partSimSet.SourceGenSettings.getNumSources(); i++)
                ShowSource(SimulationManager->Settings.partSimSet.SourceGenSettings.getSourceRecord(i), false);
        }
    }
    else GeometryWindow->ShowGeometry();

    AParticleGun* pg;
    switch (ui->cobParticleGenerationMode->currentIndex())
    {
    case 0:
        pg = SimulationManager->ParticleSources;
        break;
    case 1:
        pg = SimulationManager->FileParticleGenerator;
        SimulationManager->FileParticleGenerator->InitWithCheck(SimulationManager->Settings.partSimSet.FileGenSettings, false);
        break;
    case 2:
        pg = SimulationManager->ScriptParticleGenerator;
        break;
    default:
        message("This generation mode is not implemented!", this);
        WindowNavigator->BusyOff(); // <--
        return;
    }

    ui->pbStopScan->setEnabled(true);
    QFont font = ui->pbStopScan->font();
    font.setBold(true);
    ui->pbStopScan->setFont(font);

    //SimulationManager->FileParticleGenerator->SetValidationMode(AFileParticleGenerator::Relaxed);
    SimulationManager->Settings.partSimSet.FileGenSettings.ValidationMode = AFileGenSettings::Relaxed;
    TestParticleGun(pg, ui->sbGunTestEvents->value()); //script generator is aborted on click of the stop button!

    ui->pbStopScan->setEnabled(false);
    ui->pbStopScan->setText("stop");
    font.setBold(false);
    ui->pbStopScan->setFont(font);

    WindowNavigator->BusyOff();  // <--
}

void MainWindow::TestParticleGun(AParticleGun* Gun, int numParticles)
{
    GeometryWindow->ClearGeoMarkers();
    bool bOK = Gun->Init();
    if (!bOK)
    {
        message("Failed to initialize particle gun!\n" + Gun->GetErrorString(), this);
        return;
    }
    Gun->SetStartEvent(0);
    if (ui->cobParticleGenerationMode->currentIndex() == 1) updateFileParticleGeneratorGui();

    const double WorldSizeXY = Detector->Sandwich->getWorldSizeXY();
    const double WorldSizeZ  = Detector->Sandwich->getWorldSizeZ();
    double Length = std::max(WorldSizeXY, WorldSizeZ)*0.4;
    double R[3], K[3];
    QVector<AParticleRecord*> GP;
    int numTracks = 0;
    for (int iRun=0; iRun<numParticles; iRun++)
    {
        bool bOK = Gun->GenerateEvent(GP, iRun);
        if (bOK && numTracks < 1000)
        {
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
                GeometryWindow->GeoMarkers.append(marks);

                ++numTracks;
                if (numTracks > 1000) break;
            }
        }

        for (const AParticleRecord * p : GP) delete p;
        GP.clear();

        if (!bOK) break;
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

    const int numSources = SimulationManager->Settings.partSimSet.SourceGenSettings.getNumSources();
    if (isource >= numSources)
    {
        message("Error - bad source index!", this);
        return;
    }

    const QString SourceName = SimulationManager->Settings.partSimSet.SourceGenSettings.getSourceRecord(isource)->name;
    int ret = QMessageBox::question(this, "Remove particle source",
                                    "Remove source " + SourceName + " ?",
                                    QMessageBox::Yes | QMessageBox::Cancel,
                                    QMessageBox::Cancel);
    if (ret != QMessageBox::Yes) return;

    SimulationManager->Settings.partSimSet.SourceGenSettings.remove(isource);

    on_pbUpdateSimConfig_clicked();
    on_pbUpdateSourcesIndication_clicked();
    if (ui->pbGunShowSource->isChecked())
    {
        if (numSources == 0)
        {
            Detector->GeoManager->ClearTracks();
            GeometryWindow->ShowGeometry(false);
        }
        else ShowParticleSource_noFocus();
    }
}

void MainWindow::on_pbAddSource_clicked()
{
    AParticleSourceRecord* s = new AParticleSourceRecord();
    s->GunParticles << new GunParticleStruct();
    SimulationManager->Settings.partSimSet.SourceGenSettings.append(s);

    on_pbUpdateSimConfig_clicked();
    //on_pbUpdateSourcesIndication_clicked();
    ui->lwDefinedParticleSources->setCurrentRow( SimulationManager->Settings.partSimSet.SourceGenSettings.getNumSources() - 1 );
}

void MainWindow::on_pbAddSource_customContextMenuRequested(const QPoint &)
{
    int index = ui->lwDefinedParticleSources->currentRow();
    if (index == -1)
    {
        message("Select a source to clone", this);
        return;
    }

    bool ok = SimulationManager->Settings.partSimSet.SourceGenSettings.clone(index);
    if (!ok) return;

    on_pbUpdateSimConfig_clicked();
    //on_pbUpdateSourcesIndication_clicked();
    ui->lwDefinedParticleSources->setCurrentRow(index+1);
}

void MainWindow::on_pbUpdateSourcesIndication_clicked()
{
    ASourceGenSettings & SourceGenSettings = SimulationManager->Settings.partSimSet.SourceGenSettings;
    const int numSources = SourceGenSettings.getNumSources();

    int curRow = ui->lwDefinedParticleSources->currentRow();
    ui->lwDefinedParticleSources->clear();

    for (int i = 0; i < numSources; i++)
    {
        AParticleSourceRecord * pr = SourceGenSettings.getSourceRecord(i);
        QListWidgetItem * item = new QListWidgetItem();
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

            double totAct = SourceGenSettings.getTotalActivity();
            double per = ( totAct == 0 ? 0 : 100.0 * pr->Activity / totAct );
            QString t = (per == 0 ? "-Off-" : QString("%1%").arg(per, 3, 'g', 3) );
            lab = new QLabel(t);
            lab->setMinimumWidth(45);
        l->addWidget(lab);

        fr->setLayout(l);
        item->setSizeHint(fr->sizeHint());
        ui->lwDefinedParticleSources->setItemWidget(item, fr);
        item->setSizeHint(fr->sizeHint());

        bool bVis = (numSources > 1);
        if (!bVis && pr->Activity == 0) bVis = true;
        e->setVisible(bVis);
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
        GeometryWindow->ClearGeoMarkers();
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
    const int isource = ui->lwDefinedParticleSources->currentRow();
    if (isource < 0) return;
    if (isource >= SimulationManager->Settings.partSimSet.SourceGenSettings.getNumSources())
    {
        message("Source number is out of bounds!", this);
        return;
    }
    ShowSource(SimulationManager->Settings.partSimSet.SourceGenSettings.getSourceRecord(isource), true);
}

void MainWindow::on_pbSaveParticleSource_clicked()
{
    const int isource = ui->lwDefinedParticleSources->currentRow();
    if (isource == -1)
    {
        message("Select a source to remove", this);
        return;
    }
    if (isource >= SimulationManager->Settings.partSimSet.SourceGenSettings.getNumSources())
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
    SimulationManager->Settings.partSimSet.SourceGenSettings.getSourceRecord(isource)->writeToJson(json, *MpCollection);
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
    SimulationManager->Settings.partSimSet.SourceGenSettings.append(ps);
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
    ASourceGenSettings & SourceGenSettings = SimulationManager->Settings.partSimSet.SourceGenSettings;
    const int numSources = SourceGenSettings.getNumSources();
    if (isource >= numSources)
    {
        message("Error - bad source index!", this);
        return;
    }

    ParticleSourceDialog = new AParticleSourceDialog(*this, SourceGenSettings.getSourceRecord(isource));

    int res = ParticleSourceDialog->exec(); // if detector is rebuild (this->readSimSettingsFromJson() is triggered), ParticleSourceDialog is signal-blocked and rejected
    if (res == QDialog::Rejected)
    {
        delete ParticleSourceDialog; ParticleSourceDialog = nullptr;
        return;
    }

    SourceGenSettings.replace(isource, ParticleSourceDialog->getResult());
    delete ParticleSourceDialog; ParticleSourceDialog = nullptr;

    AParticleSourceRecord * ps = SourceGenSettings.getSourceRecord(isource);
    ps->updateLimitedToMat(*Detector->MpCollection);

    if (Detector->isGDMLempty())
    { //check world size
        double XYm = 0;
        double  Zm = 0;
        for (int isource = 0; isource < numSources; isource++)
        {
            double msize =   ps->size1;
            UpdateMax(msize, ps->size2);
            UpdateMax(msize, ps->size3);

            UpdateMax(XYm, fabs(ps->X0)+msize);
            UpdateMax(XYm, fabs(ps->Y0)+msize);
            UpdateMax(Zm,  fabs(ps->Z0)+msize);
        }

        double currXYm = Detector->Sandwich->getWorldSizeXY();
        double  currZm = Detector->Sandwich->getWorldSizeZ();
        if (XYm > currXYm || Zm > currZm)
          {
            //need to override
            Detector->Sandwich->setWorldSizeFixed(true);
            Detector->Sandwich->setWorldSizeXY( std::max(XYm, currXYm) );
            Detector->Sandwich->setWorldSizeZ ( std::max(Zm,  currZm) );
            ReconstructDetector();
          }
    }

    on_pbUpdateSimConfig_clicked();
    if (ui->pbGunShowSource->isChecked()) ShowParticleSource_noFocus();
}

#include "ageoobject.h"
#include "atypegeoobject.h"
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
    writeSimSettingsToJson(Config->JSON);

    if (ExitParticleSettings.SaveParticles)
    {
        const QString fileName = ExitParticleSettings.FileName;
        if (QFileInfo(fileName).exists())
        {
            bool bContinue = confirm("File configured to save exiting particles already exists!\nContinue?\n\n" + fileName, this);
            if (!bContinue) return;
        }
    }

    ELwindow->QuickSave(0);
    fStartedFromGUI = true;
    fSimDataNotSaved = false; // to disable the warning

    //-- test G4 settings
    if (G4SimSet.bTrackParticles)
    {
        QString Errors, Warnings, txt;

        if ( G4SimSet.SensitiveVolumes.isEmpty() && (ui->cbGunDoS1->isChecked() || ui->cbGunDoS2->isChecked()) )
            txt += "Sensitive volumes are not set!\n"
                       "Geant4 simulation will not collect any deposition information\n"
                       "There will be no photon generation in Ants2\n\n";

        bool bMonitors = false;
        bool bGrids = false;
        containsMonsGrids(Detector->Sandwich->World, bGrids, bMonitors);
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

    startSimulation(Config->JSON);
}

// ---- from file ----

void MainWindow::on_pbGenerateFromFile_Help_clicked()
{
    QString s = "Ants2 supports 3 formats:\n"
                "\n"
                "1. Simplistic (ascii file)\n"
                "File should contain particle records, one line per particle.\n"
                "Record format: ParticleId  Energy  X  Y  Z  DirX  DirY  DirZ Time *\n"
                "ParticleID is the particle index in the config, not particle name!\n"
                "The optional '*' at the end indicates that the event is not finished:\n"
                "the next particle will be generated within the same event.\n"
                "Comment lines should start with '#' symbol.\n"
                "\n"
                "2. G4ants-generated, ascii file\n"
                "Each event is marked with a special line,\n"
                "starting with '#' symbol immediately followed by the event index.\n"
                "The first event should have index of 0: #0\n"
                "Each record of a particle occupies one line, the format is:\n"
                "ParticleName Energy X Y Z DirX DirY DirZ Time\n"
                "\n"
                "3. G4ants-generated, binary file\n"
                "Each new entry starts either with a (char)EE or a (char)FF\n"
                "0xEE is followed by the event number (int),\n"
                "0xFF is followed by the particle record, which is\n"
                "ParticleName Energy X Y Z DirX DirY DirZ Time\n"
                "where ParticleName is 0-terminated string and the rest are doubles\n"
                "\n"
                "Energy is in keV, Position is in mm,\n"
                "Direction is unitary vector and Time is in ns.\n";

    message(s, this);
}

void MainWindow::on_pbGenerateFromFile_Change_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select a file with particle generation data", GlobSet.LastOpenDir, "All files (*)");
    if (fileName.isEmpty()) return;
    GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
    ui->leGenerateFromFile_FileName->setText(fileName);
    on_leGenerateFromFile_FileName_editingFinished();
}

void MainWindow::on_leGenerateFromFile_FileName_editingFinished()
{
    QString newName = ui->leGenerateFromFile_FileName->text();
    if (newName == SimulationManager->Settings.partSimSet.FileGenSettings.FileName) return;

    SimulationManager->Settings.partSimSet.FileGenSettings.FileName = newName;
    SimulationManager->Settings.partSimSet.FileGenSettings.invalidateFile();
    on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_pbGenerateFromFile_Check_clicked()
{
    AFileGenSettings & FileGenSettings = SimulationManager->Settings.partSimSet.FileGenSettings;

    FileGenSettings.ValidationMode = (ui->cbGeant4ParticleTracking->isChecked() ? AFileGenSettings::Relaxed : AFileGenSettings::Strict);

    AFileParticleGenerator* pg = SimulationManager->FileParticleGenerator;
    WindowNavigator->BusyOn();  // -->
    bool bOK = pg->InitWithCheck(FileGenSettings, ui->cbFileCollectStatistics->isChecked());
    WindowNavigator->BusyOff(); // <--

    if (!bOK) message(pg->GetErrorString(), this);
    updateFileParticleGeneratorGui();
}

void MainWindow::updateFileParticleGeneratorGui()
{
    const AFileGenSettings & FileGenSettings = SimulationManager->Settings.partSimSet.FileGenSettings;
    const QString & FileName = FileGenSettings.FileName;

    ui->leGenerateFromFile_FileName->setText(FileName);
    ui->lwFileStatistics->clear();
    ui->lwFileStatistics->setEnabled(false);
    qApp->processEvents();

    QFileInfo fi(FileName);
    if (!fi.exists())
    {
        ui->labGenerateFromFile_info->setText("File not found");
        return;
    }

    QString s = "Format: ";
    s += FileGenSettings.getFormatName();

    if (FileGenSettings.isValidated())
    {
        s += QString("  Events: %1").arg(FileGenSettings.NumEventsInFile);
        if (FileGenSettings.statNumEmptyEventsInFile > 0) s += QString(", %1 empty").arg(FileGenSettings.statNumEmptyEventsInFile);
        if (FileGenSettings.statNumMultipleEvents    > 0) s += QString(", %1 multiple").arg(FileGenSettings.statNumMultipleEvents);

        ui->lwFileStatistics->setEnabled(FileGenSettings.ParticleStat.size() > 0);
        for (const AParticleInFileStatRecord & rec : FileGenSettings.ParticleStat)
        {
            ui->lwFileStatistics->addItem( QString("%1 \t# %2 \t <E>: %4 keV")
                                           .arg(rec.NameQt)
                                           .arg(rec.Entries)
                                           //.arg( QString::number(rec.Energy, 'g', 6) )
                                           .arg( QString::number(rec.Energy / rec.Entries, 'g', 6) ) );
        }
    }
    else s += " Click 'Analyse file' to see statistics";

    ui->labGenerateFromFile_info->setText(s);
}

#include <QClipboard>
void MainWindow::on_lwFileStatistics_customContextMenuRequested(const QPoint &pos)
{
    QMenu myMenu;
    myMenu.addSeparator();
    myMenu.addAction("Copy all to clipboard");
    myMenu.addSeparator();

    QPoint globalPos = ui->lwFileStatistics->mapToGlobal(pos);
    QAction* selectedItem = myMenu.exec(globalPos);

    if (selectedItem)
    {
        if (selectedItem->iconText() == "Copy all to clipboard")
         {
            QString txt;

            for (AParticleInFileStatRecord & rec : SimulationManager->Settings.partSimSet.FileGenSettings.ParticleStat)
            {
                txt += QString("%1 \t# %2 \t <E>: %4 keV\n")
                                               .arg(rec.NameQt)
                                               .arg(rec.Entries)
                                               //.arg( QString::number(rec.Energy, 'g', 6) )
                                               .arg( QString::number(rec.Energy / rec.Entries, 'g', 6) );
            }

            QClipboard * clipboard = QApplication::clipboard();
            clipboard->setText(txt);
         }
    }
}

// --- by script

#include "ajavascriptmanager.h"
#include "ascriptwindow.h"
#include "aparticlegenerator_si.h"
#include "amath_si.h"

void MainWindow::updateScriptParticleGeneratorGui()
{
    ui->pteParticleGenerationScript->clear();
    ui->pteParticleGenerationScript->appendPlainText(SimulationManager->Settings.partSimSet.ScriptGenSettings.Script);
}

void MainWindow::on_pbParticleGenerationScript_clicked()
{
    AJavaScriptManager* sm = new AJavaScriptManager(Detector->RandGen);
    AScriptWindow* sw = new AScriptWindow(sm, true, this);
    sw->setAcceptRejectVisible();

    //int NumThreads = 1;
    AParticleGenerator_SI* gen = new AParticleGenerator_SI(*Detector->MpCollection, Detector->RandGen);//, 0, &NumThreads);
    QVector<AParticleRecord*> GP;
    gen->configure(&GP, 0);
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


    QString Script = SimulationManager->Settings.partSimSet.ScriptGenSettings.Script;
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

    if (bWasAccepted) SimulationManager->Settings.partSimSet.ScriptGenSettings.Script = Script;
    on_pbUpdateSimConfig_clicked();
}

void MainWindow::on_pteParticleGenerationScript_customContextMenuRequested(const QPoint &)
{
    on_pbParticleGenerationScript_clicked();
}
