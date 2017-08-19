//ANTS2
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "outputwindow.h"
#include "pms.h"
#include "eventsdataclass.h"
#include "windownavigatorclass.h"
#include "geometrywindowclass.h"
#include "particlesourcesclass.h"
#include "graphwindowclass.h"
#include "detectorclass.h"
#include "checkupwindowclass.h"
#include "globalsettingsclass.h"
#include "amaterialparticlecolection.h"
#include "ageomarkerclass.h"
#include "ajsontools.h"
#include "aparticleonstack.h"
#include "amessage.h"
#include "acommonfunctions.h"
#include "guiutils.h"

//Qt
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>

//Root
#include "TVirtualGeoTrack.h"
#include "TGeoManager.h"
#include "TH1D.h"

void MainWindow::SimParticleSourcesConfigToJson(QJsonObject &json)
{  
  QJsonObject masterjs;
    // control options
  QJsonObject cjs;
  cjs["EventsToDo"] = ui->sbGunEvents->text().toDouble();
  cjs["AllowMultipleParticles"] = ui->cbGunAllowMultipleEvents->isChecked();
  cjs["AverageParticlesPerEvent"] = ui->ledGunAverageNumPartperEvent->text().toDouble();
  cjs["DoS1"] = ui->cbGunDoS1->isChecked();
  cjs["DoS2"] = ui->cbGunDoS2->isChecked();
  cjs["ParticleTracks"] = ui->cbGunParticleTracks->isChecked();
  cjs["IgnoreNoHitsEvents"] = ui->cbIgnoreEventsWithNoHits->isChecked();
  cjs["IgnoreNoDepoEvents"] = ui->cbIgnoreEventsWithNoEnergyDepo->isChecked();
  masterjs["SourceControlOptions"] = cjs;
    //particle sources
  ParticleSources->writeToJson(masterjs);

  json["ParticleSourcesConfig"] = masterjs;
}

void MainWindow::ShowSource(int isource, bool clear)
{
  ParticleSourceStructure* p = ParticleSources->getSource(isource);

  int index = p->index;
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
             int k;
             for (k=0; k<3; k++) if (k!=i && k!=j) break;
             for (int s=-1; s<2; s+=2)
               {
                //qDebug()<<"i j k shift"<<i<<j<<k<<s;
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

  MainWindow::ShowTracks();
  Detector->GeoManager->SetCurrentPoint(X0,Y0,Z0);
  //Detector->GeoManager->DrawCurrentPoint(9);
  GeometryWindow->UpdateRootCanvas();
}

void MainWindow::on_pbGunTest_clicked()
{
  //MainWindow::on_pbGunShowSource_clicked();
  MainWindow::on_pbGunShowSource_toggled(true);

  QVector<double> activities;
  //forcing to 100% activity the currently selected source
  int isource = ui->cobParticleSource->currentIndex();
  for (int i=0; i<ParticleSources->size(); i++)
  {
      activities << ParticleSources->getSource(i)->Activity; //remember old
      if (i==isource) ParticleSources->getSource(i)->Activity = 1.0;
      else ParticleSources->getSource(i)->Activity = 0;
  }
  ParticleSources->Init();

  double Length = std::max(Detector->WorldSizeXY, Detector->WorldSizeZ)*0.4;
  double R[3], K[3];
  int numParticles = ui->sbGunTestEvents->value();
  bool bHaveSome = false;
  for (int iRun=0; iRun<numParticles; iRun++)
    {      
      QVector<GeneratedParticleStructure>* GP = ParticleSources->GenerateEvent();
      if (GP->size()>0)
          bHaveSome = true;
      else
      {
          if (iRun>2)
          {
             if (ui->cbSourceLimitmat->isChecked()) message("Did several attempts but no particles were generated!\n"
                                                            "Possible reason: generation is limited to a wrong material", this);
             else message("Did several attempts but no particles were generated!", this);
             return;
          }
      }
      for (int iP = 0; iP<GP->size(); iP++)
        {
          R[0] = (*GP)[iP].Position[0];
          R[1] = (*GP)[iP].Position[1];
          R[2] = (*GP)[iP].Position[2];
          K[0] = (*GP)[iP].Direction[0];
          K[1] = (*GP)[iP].Direction[1];
          K[2] = (*GP)[iP].Direction[2];

          Int_t track_index = Detector->GeoManager->AddTrack(1,22);
          TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);
          track->AddPoint(R[0], R[1], R[2], 0);
          track->AddPoint(R[0]+K[0]*Length, R[1]+K[1]*Length, R[2]+K[2]*Length, 0);
          track->SetLineWidth(1);
          track->SetLineColor(1+(*GP)[iP].ParticleId);
        }
      //clear and delete QVector with generated event
      GP->clear();
      delete GP;
    }

  MainWindow::ShowTracks();

  //restore activities of the sources
  for (int i=0; i<ParticleSources->size(); i++)
      ParticleSources->getSource(i)->Activity = activities.at(i);
}

void MainWindow::on_pbGunRefreshparticles_clicked()
{
    ui->lwGunParticles->clear(); //Cleaning ListView

    if (ParticleSources->size() == 0) return;
    int isource = ui->cobParticleSource->currentIndex();
    if (isource > ParticleSources->size()-1) return; //protection

    ParticleSourceStructure* ps = ParticleSources->getSource(isource);

    //Populating ListView
    int DefinedSourceParticles = ps->GunParticles.size();
    //qDebug() << "Defined particles for this source:"<< DefinedSourceParticles;

    for (int i=0; i<DefinedSourceParticles; i++)
      {
        GunParticleStruct* gps = ps->GunParticles[i];

        bool Individual = gps->Individual;

        QString str, str1;
        if (Individual) str = "";
        else str = ">";
        str1.setNum(i);
        str += str1 + "> ";
        //str += Detector->ParticleCollection[gps->ParticleId]->ParticleName;
        str += Detector->MpCollection->getParticleName(gps->ParticleId);
        if (gps->spectrum) str += " E=spec";
        else
          {
            str1.setNum(gps->energy);
            str += " E=" + str1;
          }

        if (Individual)
          {
            str += " W=";
            str1.setNum(gps->StatWeight);
            str += str1;
          }
        else
          {
            str += " Link:";
            str += QString::number(gps->LinkedTo);
            str += " P=";
            str += QString::number(gps->LinkingProbability);
            if (gps->LinkingOppositeDir)
                str += " Opposite";
          }
        ui->lwGunParticles->addItem(str);
        //qDebug() << str;
      }
  MainWindow::SourceUpdateThisParticleIndication();
}

void MainWindow::SourceUpdateThisParticleIndication()
{
  //showing selected row and updating data indication
  int isource = ui->cobParticleSource->currentIndex();
  if (isource<0 || isource>=ParticleSources->size()) return;
  int row = ui->lwGunParticles->currentRow();

  ParticleSourceStructure* ps = ParticleSources->getSource(isource);

  int DefinedSourceParticles = ps->GunParticles.size();
  if (DefinedSourceParticles > 0 && row>-1)
    {
      ui->fGunParticle->setEnabled(true);
      int part = ps->GunParticles[row]->ParticleId;
      ui->cobGunParticle->setCurrentIndex(part);

      GunParticleStruct* gps = ps->GunParticles[row];

      QString str;
      str.setNum(gps->StatWeight);
      ui->ledGunParticleWeight->setText(str);
      str.setNum(gps->energy);
      ui->ledGunEnergy->setText(str);
      ui->cbIndividualParticle->setChecked(gps->Individual);
      ui->leiParticleLinkedTo->setText( QString::number(gps->LinkedTo) );
      str.setNum(gps->LinkingProbability);
      ui->ledLinkingProbability->setText(str);
      ui->cbLinkingOpposite->setChecked(gps->LinkingOppositeDir);
      //Depending on the presence of spectrum data, modifing visiblility and enable/disable
      if (gps->spectrum == 0)
        {
          ui->pbGunShowSpectrum->setEnabled(false);
          ui->pbGunDeleteSpectrum->setEnabled(false);
          ui->ledGunEnergy->setEnabled(true);
          ui->fPartEnergy->setVisible(true);
        }
      else
        {
          ui->pbGunShowSpectrum->setEnabled(true);
          ui->pbGunDeleteSpectrum->setEnabled(true);
          ui->ledGunEnergy->setEnabled(false);
          ui->fPartEnergy->setVisible(false);
        }
    }
  else ui->fGunParticle->setEnabled(false);
}

void MainWindow::on_pbGunAddNew_clicked()
{
    int isource = ui->cobParticleSource->currentIndex();

    GunParticleStruct* tmp = new GunParticleStruct();
    tmp->ParticleId = ui->cobGunParticle->currentIndex();
    tmp->StatWeight = ui->ledGunParticleWeight->text().toDouble();
    tmp->energy = ui->ledGunEnergy->text().toDouble();
    tmp->spectrum = 0;
    ParticleSources->getSource(isource)->GunParticles.append(tmp);

    MainWindow::on_pbGunRefreshparticles_clicked();
    ui->lwGunParticles->setCurrentRow( ParticleSources->getSource(isource)->GunParticles.size()-1 );

    MainWindow::on_pbUpdateSources_clicked();
}

void MainWindow::on_pbGunRemove_clicked()
{
  int isource = ui->cobParticleSource->currentIndex();
  ParticleSourceStructure* ps = ParticleSources->getSource(isource);
  if (ps->GunParticles.size() < 2)
  {
      message("Source should contain at least one particle!");
      return;
  }

  int iparticle = ui->lwGunParticles->currentRow();
  if (iparticle == -1)
    {
      message("Select a particle to remove!", this);
      return;
    }
  delete ps->GunParticles[iparticle];
  ps->GunParticles.remove(iparticle);
  MainWindow::on_pbGunRefreshparticles_clicked();
  int errCode = ParticleSources->CheckSource(isource);
  if (errCode>0)
    {
      QString err = ParticleSources->getErrorString(errCode);
      message("Source checker reports an error: "+err, this);
    }
  MainWindow::on_pbUpdateSources_clicked();
}

void MainWindow::on_cobGunSourceType_currentIndexChanged(int index)
{
  QList <QString> s;
  switch (index)
    {
    default:
    case 0: s <<""<<""<<"";
      break;
    case 1: s <<"Size"<<""<<"";
      break;
    case 2: s <<"SizeX"<<"SizeY"<<"";
      break;
    case 3: s <<"Radius"<<""<<"";
      break;
    case 4: s <<"SizeX"<<"SizeY"<<"SizeZ";
      break;
    case 5: s <<"Radius"<<""<<"Height";
    }
  ui->lGun1DSize->setText(s[0]);
  ui->lGun2DSize->setText(s[1]);
  ui->lGun3DSize->setText(s[2]);
  ui->fGun1D->setDisabled(s[0].isEmpty());
  ui->fGun2D->setDisabled(s[1].isEmpty());
  ui->fGun3D->setDisabled(s[2].isEmpty());

  ui->fGunPhi->setEnabled(index != 0);
  ui->fGunTheta->setEnabled(index != 0);
  ui->fGunPsi->setEnabled(index > 1);

  if (index == 1)
  {
      ui->labGunPhi->setText("Phi");
      ui->labGunTheta->setText("Theta");
  }
  else
  {
      ui->labGunPhi->setText("around X");
      ui->labGunTheta->setText("around Y");
  }
}

void MainWindow::on_cobGunParticle_activated(int index)
{
     //next 4 obsolete?
   if (ParticleSources->size() == 0) return;
   int isource = ui->cobParticleSource->currentIndex();
   if (isource > ParticleSources->size() - 1) return;

   ParticleSourceStructure* ps = ParticleSources->getSource(isource);
   if (ps->GunParticles.isEmpty()) return;

   int particle = ui->lwGunParticles->currentRow();
   if (particle<0 || particle > ps->GunParticles.size()-1) return;
   ps->GunParticles[particle]->ParticleId = index;
   MainWindow::on_pbGunRefreshparticles_clicked();
   ui->lwGunParticles->setCurrentRow(particle);

   MainWindow::on_pbUpdateSources_clicked();
}

void MainWindow::on_ledGunEnergy_editingFinished()
{
  //if (BulkUpdate) return;
  int isource = ui->cobParticleSource->currentIndex();
  ParticleSourceStructure* ps = ParticleSources->getSource(isource);
  int particle = ui->lwGunParticles->currentRow();
  if (particle<0 || particle > ps->GunParticles.size()-1) return;

  ps->GunParticles[particle]->energy = ui->ledGunEnergy->text().toDouble();
  MainWindow::on_pbGunRefreshparticles_clicked();
  ui->lwGunParticles->setCurrentRow(particle);

  MainWindow::on_pbUpdateSources_clicked();
}

void MainWindow::on_ledGunParticleWeight_editingFinished()
{
  //if (BulkUpdate) return;
  int isource = ui->cobParticleSource->currentIndex();
  ParticleSourceStructure* ps = ParticleSources->getSource(isource);
  int particle = ui->lwGunParticles->currentRow();
  if (particle<0 || particle > ps->GunParticles.size()-1) return;
  ps->GunParticles[particle]->StatWeight = ui->ledGunParticleWeight->text().toDouble();
  MainWindow::on_pbGunRefreshparticles_clicked();
  ui->lwGunParticles->setCurrentRow(particle);

  MainWindow::on_pbUpdateSources_clicked();
}

void MainWindow::on_cbIndividualParticle_clicked(bool checked)
{
  //if (BulkUpdate) return;
  int isource = ui->cobParticleSource->currentIndex();
  ParticleSourceStructure* ps = ParticleSources->getSource(isource);
  int particle = ui->lwGunParticles->currentRow();
  if (particle<0 || particle > ps->GunParticles.size()-1) return;

  if (particle == 0 && !checked)
    {
      ui->cbIndividualParticle->setChecked(true);
      message("First particle should be 'Individual'", this);
      return;//update on_toggle
    }
  ps->GunParticles[particle]->Individual = checked;
  if (!checked) ps->GunParticles[particle]->LinkedTo = ui->leiParticleLinkedTo->text().toInt();
  MainWindow::on_pbGunRefreshparticles_clicked();
  ui->lwGunParticles->setCurrentRow(particle);

  MainWindow::on_pbUpdateSources_clicked();
}

void MainWindow::on_leiParticleLinkedTo_editingFinished()
{
  //if (BulkUpdate) return;
  int isource = ui->cobParticleSource->currentIndex();
  ParticleSourceStructure* ps = ParticleSources->getSource(isource);
  int particle = ui->lwGunParticles->currentRow();
  if (particle<0 || particle > ps->GunParticles.size()-1) return;
  int iLinkedTo = ui->leiParticleLinkedTo->text().toInt();
  if (iLinkedTo == particle)
    {
      ui->leiParticleLinkedTo->setText("0");
      message("Particle cannot be linked to itself!", this);
      MainWindow::on_leiParticleLinkedTo_editingFinished();
      return;
    }

  int TotParticles = ps->GunParticles.size();
  if (iLinkedTo>TotParticles-1 || iLinkedTo<0)
    {
      if (iLinkedTo == 0) return; //protection - no particles are defined
      ui->leiParticleLinkedTo->setText("0");
      message("Particle index is out of range: "+QString::number(TotParticles)+" particle are defined", this);
      MainWindow::on_leiParticleLinkedTo_editingFinished();
      return;
    }

  ps->GunParticles[particle]->LinkedTo = iLinkedTo;
  MainWindow::on_pbGunRefreshparticles_clicked();
  ui->lwGunParticles->setCurrentRow(particle);

  MainWindow::on_pbUpdateSources_clicked();
}

void MainWindow::on_ledLinkingProbability_editingFinished()
{
  double val = ui->ledLinkingProbability->text().toDouble();
  if (val<0 || val>1)
    {
      ui->ledLinkingProbability->setText("0");
      message("Error in probability value: should be between 0 and 1",this);
      val = 0;
    }

  int isource = ui->cobParticleSource->currentIndex();
  ParticleSourceStructure* ps = ParticleSources->getSource(isource);
  int particle = ui->lwGunParticles->currentRow();
  if (particle<0 || particle > ps->GunParticles.size()-1) return;
  ps->GunParticles[particle]->LinkingProbability = val;
  MainWindow::on_pbGunRefreshparticles_clicked();
  ui->lwGunParticles->setCurrentRow(particle);

  MainWindow::on_pbUpdateSources_clicked();
}

void MainWindow::on_cbLinkingOpposite_clicked(bool checked)
{
    int isource = ui->cobParticleSource->currentIndex();
    ParticleSourceStructure* ps = ParticleSources->getSource(isource);
    int particle = ui->lwGunParticles->currentRow();
    if (particle<0 || particle > ps->GunParticles.size()-1) return;
    int linkedTo = ui->leiParticleLinkedTo->text().toInt();
    if (linkedTo<0 || linkedTo > ps->GunParticles.size()-1) return;
    if (linkedTo == particle)
    {
        ps->GunParticles[particle]->LinkingOppositeDir = false;
        message("Cannot link particle to itself", this);
    }

    ps->GunParticles[particle]->LinkingOppositeDir = checked;
    MainWindow::on_pbGunRefreshparticles_clicked();
    ui->lwGunParticles->setCurrentRow(particle);

    MainWindow::on_pbUpdateSources_clicked();
}

void MainWindow::on_pbGunLoadSpectrum_clicked()
{
  QString fileName;
  fileName = QFileDialog::getOpenFileName(this, "Load energy spectrum", GlobSet->LastOpenDir, "Data files (*.dat *.txt);;All files (*)");
  qDebug()<<fileName;
  if (fileName.isEmpty()) return;
  GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

  int isource = ui->cobParticleSource->currentIndex();
  ParticleSourceStructure* ps = ParticleSources->getSource(isource);
  int particle = ui->lwGunParticles->currentRow();
  if (particle<0 || particle > ps->GunParticles.size()-1) return;
  ParticleSources->LoadGunEnergySpectrum(isource, particle, fileName);
  MainWindow::on_pbGunRefreshparticles_clicked();
  ui->lwGunParticles->setCurrentRow(particle);

  MainWindow::on_pbUpdateSources_clicked();
}

#include "TH1.h"
void MainWindow::on_pbGunShowSpectrum_clicked()
{
  int isource = ui->cobParticleSource->currentIndex();
  ParticleSourceStructure* ps = ParticleSources->getSource(isource);
  int particle = ui->lwGunParticles->currentRow();
  if (particle<0 || particle > ps->GunParticles.size()-1) return;
  ps->GunParticles[particle]->spectrum->GetXaxis()->SetTitle("Energy, keV");
  GraphWindow->Draw(ps->GunParticles[particle]->spectrum, "", true, false);
}

void MainWindow::on_pbGunDeleteSpectrum_clicked()
{
   int isource = ui->cobParticleSource->currentIndex();
   ParticleSourceStructure* ps = ParticleSources->getSource(isource);
   int particle = ui->lwGunParticles->currentRow();
   if (particle<0 || particle > ps->GunParticles.size()-1) return;
   if (ps->GunParticles[particle]->spectrum)
     {
       delete ps->GunParticles[particle]->spectrum;
       ps->GunParticles[particle]->spectrum = 0;
     }
   ui->pbGunShowSpectrum->setEnabled(false);
   ui->pbGunDeleteSpectrum->setEnabled(false);
   MainWindow::on_pbGunRefreshparticles_clicked();
   ui->lwGunParticles->setCurrentRow(particle);

   MainWindow::on_pbUpdateSources_clicked();
}

void MainWindow::on_ledGunAverageNumPartperEvent_editingFinished()
{
   double val = ui->ledGunAverageNumPartperEvent->text().toDouble();
   if (val<0)
     {
       message("Average number of particles per event should be more than 0", this);
       ui->ledGunAverageNumPartperEvent->setText("0.01");
     }
}

void MainWindow::on_pbRemoveSource_clicked()
{
  if (ParticleSources->size() == 0) return;
  int isource = ui->cobParticleSource->currentIndex();

  int ret = QMessageBox::question(this, "Remove particle source",
                                  "Are you sure - this will remove source " + ParticleSources->getSource(isource)->name,
                                  QMessageBox::Yes | QMessageBox::Cancel,
                                  QMessageBox::Cancel);
  if (ret != QMessageBox::Yes) return;

  ParticleSources->remove(isource);
  ui->cobParticleSource->removeItem(isource);
  ui->cobParticleSource->setCurrentIndex(isource-1);
  if (ParticleSources->size() == 0) clearParticleSourcesIndication();

  on_pbUpdateSimConfig_clicked();

  on_pbUpdateSourcesIndication_clicked();
  if (ui->pbGunShowSource->isChecked())
    {
      if (ParticleSources->size() == 0)
        {
           Detector->GeoManager->ClearTracks();
           ShowGeometry(false);
        }
        else
        ShowParticleSource_noFocus();
    }
}

void MainWindow::on_pbAddSource_clicked()
{
  ParticleSourceStructure* s = new ParticleSourceStructure();
  ParticleSources->append(s);
  ui->cobParticleSource->addItem(ParticleSources->getLastSource()->name);
  ui->cobParticleSource->setCurrentIndex(ParticleSources->size()-1);

  ui->ledGunParticleWeight->setText("1");
  ui->cbIndividualParticle->setChecked(true);
  on_pbGunAddNew_clicked(); //to add new particle and register the changes in config
  on_pbUpdateSourcesIndication_clicked();
  if (ui->pbGunShowSource->isChecked()) ShowParticleSource_noFocus();
}

void MainWindow::on_pbUpdateSources_clicked()
{
  int isource = ui->cobParticleSource->currentIndex();
  if (isource<0)
    {
      //qWarning() << "Attempt to update source with number <0";
      return;
    }

  if (BulkUpdate) return;
  if (DoNotUpdateGeometry) return;
  if (ShutDown) return;
  if (ParticleSources->size() == 0)
    {
      qWarning()<<"Attempt to update particle source while there are no defined ones!";
      return;
    }

  //qDebug()<<"Update source#"<<isource<<"   defined in total:"<<ParticleSources->size();

  if (isource >= ParticleSources->size())
    {
      message("Error: Attempting to update particle source parameters, but source number is out of bounds!",this);
      return;
    }

  ParticleSourceStructure* ps = ParticleSources->getSource(isource);

  ps->Activity = ui->ledSourceActivity->text().toDouble();
  ps->index = ui->cobGunSourceType->currentIndex();
  ps->X0 = ui->ledGunOriginX->text().toDouble();
  ps->Y0 = ui->ledGunOriginY->text().toDouble();
  ps->Z0 = ui->ledGunOriginZ->text().toDouble();
  ps->Phi = ui->ledGunPhi->text().toDouble();
  ps->Theta = ui->ledGunTheta->text().toDouble();
  ps->Psi = ui->ledGunPsi->text().toDouble();
  ps->size1 = ui->ledGun1DSize->text().toDouble();
  ps->size2 = ui->ledGun2DSize->text().toDouble();
  ps->size3 = ui->ledGun3DSize->text().toDouble();
  ps->CollPhi = ui->ledGunCollPhi->text().toDouble();
  ps->CollTheta = ui->ledGunCollTheta->text().toDouble();
  ps->Spread = ui->ledGunSpread->text().toDouble();
  ps->DoMaterialLimited = ui->cbSourceLimitmat->isChecked();
  ps->LimtedToMatName = ui->leSourceLimitMaterial->text();
  ParticleSources->checkLimitedToMaterial(ps);

  if (Detector->isGDMLempty())
    { //check world size
      double XYm = 0;
      double  Zm = 0;
      for (int isource = 0; isource < ParticleSources->size(); isource++)
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

  updateActivityIndication();    //update marker!
  if (ui->pbGunShowSource->isChecked()) ShowParticleSource_noFocus();
  //qDebug() << "...update sources done";
}

void MainWindow::on_pbUpdateSourcesIndication_clicked()
{
  //qDebug() << "Update sources inidcation. Defined sources:"<<ParticleSources->size();
  int isource = ui->cobParticleSource->currentIndex();

  int numSources = ParticleSources->size();
  ui->labPartSourcesDefined->setText(QString::number(numSources));
  clearParticleSourcesIndication();
  if (numSources == 0) return;
  ui->fParticleSources->setEnabled(true);
  ui->frSelectSource->setEnabled(true);

  for (int i=0; i<numSources; i++)
      ui->cobParticleSource->addItem(ParticleSources->getSource(i)->name);
  if (isource >= numSources) isource = 0;  
  if (isource == -1) isource = 0;

  ui->cobParticleSource->setCurrentIndex(isource);
  updateOneParticleSourcesIndication(ParticleSources->getSource(isource));
  on_pbGunRefreshparticles_clicked();
  on_leSourceLimitMaterial_textChanged("");  //update color only!
  updateActivityIndication();
}

void MainWindow::updateActivityIndication()
{
  double activity = ui->ledSourceActivity->text().toDouble();
  QSize size(ui->lSourceActive->height(), ui->lSourceActive->height());
  QIcon wIcon = createColorCircleIcon(size, Qt::yellow);
  if (activity == 0) ui->lSourceActive->setPixmap(wIcon.pixmap(16,16));
  else          ui->lSourceActive->setPixmap(QIcon().pixmap(16,16));

  double fraction = 0;
  double TotalActivity = ParticleSources->getTotalActivity();
  if (TotalActivity != 0) fraction = activity / TotalActivity * 100.0;
  ui->labOfTotal->setText(QString::number(fraction, 'g', 3)+"%");
}

void MainWindow::updateOneParticleSourcesIndication(ParticleSourceStructure* ps)
{
    bool BulkUpdateCopy = BulkUpdate; //it could be set outside to true already, do not want to reselt to false on exit
    BulkUpdate = true;

    ui->ledSourceActivity->setText(QString::number(ps->Activity));
    ui->cobGunSourceType->setCurrentIndex(ps->index);
    ui->ledGunOriginX->setText(QString::number(ps->X0));
    ui->ledGunOriginY->setText(QString::number(ps->Y0));
    ui->ledGunOriginZ->setText(QString::number(ps->Z0));
    ui->ledGunPhi->setText(QString::number(ps->Phi));
    ui->ledGunTheta->setText(QString::number(ps->Theta));
    ui->ledGunPsi->setText(QString::number(ps->Psi));
    ui->ledGun1DSize->setText(QString::number(ps->size1));
    ui->ledGun2DSize->setText(QString::number(ps->size2));
    ui->ledGun3DSize->setText(QString::number(ps->size3));
    ui->ledGunCollPhi->setText(QString::number(ps->CollPhi));
    ui->ledGunCollTheta->setText(QString::number(ps->CollTheta));
    ui->ledGunSpread->setText(QString::number(ps->Spread));

    ui->cbSourceLimitmat->setChecked(ps->DoMaterialLimited);
    ui->leSourceLimitMaterial->setText(ps->LimtedToMatName);

    BulkUpdate = BulkUpdateCopy;
}

void MainWindow::clearParticleSourcesIndication()
{
    ParticleSourceStructure ps;
    updateOneParticleSourcesIndication(&ps);
    ui->cobParticleSource->clear();
    ui->cobParticleSource->setCurrentIndex(-1);
    ui->fParticleSources->setEnabled(false);
    ui->frSelectSource->setEnabled(false);
}

//void MainWindow::on_pbGunShowSource_clicked()
//{
//   int isource = ui->cobParticleSource->currentIndex();
//   if (isource < 0) return;
//   if (isource >= ParticleSources->size())
//     {
//       message("Source number is out of bounds!",this);
//       return;
//     }
//   MainWindow::ShowSource(isource, true);
//}

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
        ShowGeometry();
      }
}

void MainWindow::ShowParticleSource_noFocus()
{
  int isource = ui->cobParticleSource->currentIndex();
  if (isource < 0) return;
  if (isource >= ParticleSources->size())
    {
      message("Source number is out of bounds!",this);
      return;
    }
  ShowSource(isource, true);
}

void MainWindow::on_lwGunParticles_currentRowChanged(int /*currentRow*/)
{
  MainWindow::SourceUpdateThisParticleIndication();
}

void MainWindow::on_pbSaveParticleSource_clicked()
{
  QString starter = (GlobSet->LibParticleSources.isEmpty()) ? GlobSet->LastOpenDir : GlobSet->LibParticleSources;
  QString fileName = QFileDialog::getSaveFileName(this, "Export particle source", starter, "Json files (*.json)");
  if (fileName.isEmpty()) return;
  QFileInfo file(fileName);
  if (file.suffix().isEmpty()) fileName += ".json";
  QJsonObject json, js;
  ParticleSources->writeSourceToJson(ui->cobParticleSource->currentIndex(), json);
  js["ParticleSource"] = json;
  SaveJsonToFile(js, fileName);
}

void MainWindow::on_pbLoadParticleSource_clicked()
{
  QMessageBox msgBox(this);
  msgBox.setText("You are about to replace this source with a source from a file.");
  msgBox.setInformativeText("Proceed?");
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Cancel);
  int ret = msgBox.exec();
  if (ret == QMessageBox::Cancel) return;

  QString starter = (GlobSet->LibParticleSources.isEmpty()) ? GlobSet->LastOpenDir : GlobSet->LibParticleSources;
  QString fileName = QFileDialog::getOpenFileName(this, "Import particle source", starter, "json files (*.json)");
  if (fileName.isEmpty()) return;
  int iSource = ui->cobParticleSource->currentIndex();
  QJsonObject json, js;
  bool ok = LoadJsonFromFile(json, fileName);
  if (!ok) return;
  if (!json.contains("ParticleSource"))
    {
      message("Json file format error", this);
      return;
    }
  //int oldPartCollSize = Detector->ParticleCollection.size();
  int oldPartCollSize = Detector->MpCollection->countParticles();
  js = json["ParticleSource"].toObject();
  ParticleSources->readSourceFromJson(iSource, js);
  ui->cobParticleSource->setItemText(iSource, ParticleSources->getSource(iSource)->name);

  MainWindow::on_pbUpdateSourcesIndication_clicked();
  //int newPartCollSize = Detector->ParticleCollection.size();
  int newPartCollSize = Detector->MpCollection->countParticles();
  if (oldPartCollSize != newPartCollSize)
    {
      //qDebug() << oldPartCollSize <<"->"<<newPartCollSize;
      QString str = "Warning!\nThe following particle";
      if (newPartCollSize-oldPartCollSize>1) str += "s were";
      else str += " was";
      str += " added:\n";
      for (int i=oldPartCollSize; i<newPartCollSize; i++)
          //str += Detector->ParticleCollection[i]->ParticleName + "\n";
          str += Detector->MpCollection->getParticleName(i) + "\n";
      str += "\nSet the interaction data for the detector materials!";
      message(str,this);
    }

  MainWindow::ListActiveParticles(); //also updates all cobs with particle names!
}

void MainWindow::on_pbSingleSourceShow_clicked()
{
    double r[3];
    r[0] = ui->ledSingleX->text().toDouble();
    r[1] = ui->ledSingleY->text().toDouble();
    r[2] = ui->ledSingleZ->text().toDouble();

    clearGeoMarkers();
    GeoMarkerClass* marks = new GeoMarkerClass("Source", 3, 10, kBlack);
    marks->SetNextPoint(r[0], r[1], r[2]);   
    GeoMarkers.append(marks);
    GeoMarkerClass* marks1 = new GeoMarkerClass("Source", 4, 3, kRed);
    marks1->SetNextPoint(r[0], r[1], r[2]);    
    GeoMarkers.append(marks1);

    ShowGeometry(false);
}

void MainWindow::on_pbRenameSource_clicked()
{
    int isource = ui->cobParticleSource->currentIndex();
    //qDebug() << "current source:"<<isource;
    if (isource<0 || isource>ParticleSources->size()-1) return;

    ParticleSourceStructure* ps = ParticleSources->getSource(isource);
    bool ok;
    QString text = QInputDialog::getText(this, "Rename particle source",
                                            "New name:", QLineEdit::Normal,
                                            ps->name, &ok);
    if (ok && !text.isEmpty())
    {
        ps->name = text;
        ui->cobParticleSource->setItemText(isource, text);
        MainWindow::on_pbUpdateSources_clicked();
    }
}

void MainWindow::on_pbAddParticleToStack_clicked()
{
    //check is this particle's energy in the defined range?
    double energy = ui->ledParticleStackEnergy->text().toDouble();
    int iparticle = ui->cobParticleToStack->currentIndex();
    QList<QString> mats;

    for (int imat=0; imat<MpCollection->countMaterials(); imat++)
      {
//        qDebug()<<imat;
//        qDebug()<<" mat:"<<(*MaterialCollection)[imat]->name;
        if ( !(*MpCollection)[imat]->MatParticle[iparticle].TrackingAllowed )
          {
//            qDebug()<<"  tracking not allowed";
            continue;
          }
        if ( (*MpCollection)[imat]->MatParticle[iparticle].MaterialIsTransparent )
          {
//            qDebug()<<"  user set as transparent for this particle!";
            continue;
          }

//        qDebug()<<" getting energy range...";
        if ((*MpCollection)[imat]->MatParticle[iparticle].InteractionDataX.isEmpty())
          {
//           qDebug()<<"  Interaction data are not loaded!";
           mats << (*MpCollection)[imat]->name;
           continue;
          }
        double minE = (*MpCollection)[imat]->MatParticle[iparticle].InteractionDataX.first();
//        qDebug()<<"  min energy:"<<minE;
        double maxE = (*MpCollection)[imat]->MatParticle[iparticle].InteractionDataX.last();
//        qDebug()<<"  max energy:"<<maxE;

        if (energy<minE || energy>maxE) mats << (*MpCollection)[imat]->name;
//        qDebug()<<"->"<<mats;
      }
//    qDebug()<<"reporting...";

    if (mats.size()>0)
      {
        //QString str = Detector->ParticleCollection[iparticle]->ParticleName;
        QString str = Detector->MpCollection->getParticleName(iparticle);
        str += " energy (" + ui->ledParticleStackEnergy->text() + " keV) ";
        str += "is conflicting with the defined interaction energy range of material";
        if (mats.size()>1) str += "s";
        str += ": ";
        for (int i=0; i<mats.size(); i++)
            str += mats[i]+", ";

        str.chop(2);
        message(str,this);
      }

    WindowNavigator->BusyOn();
    int numCopies = ui->sbNumCopies->value();
    ParticleStack.reserve(ParticleStack.size() + numCopies);
    for (int i=0; i<numCopies; i++)
    {
       AParticleOnStack *tmp = new AParticleOnStack(ui->cobParticleToStack->currentIndex(),
                                                  ui->ledParticleStackX->text().toDouble(), ui->ledParticleStackY->text().toDouble(), ui->ledParticleStackZ->text().toDouble(),
                                                  ui->ledParticleStackVx->text().toDouble(), ui->ledParticleStackVy->text().toDouble(), ui->ledParticleStackVz->text().toDouble(),
                                                  ui->ledParticleStackTime->text().toDouble(), ui->ledParticleStackEnergy->text().toDouble());
       ParticleStack.append(tmp);
    }
    MainWindow::on_pbRefreshStack_clicked();
    WindowNavigator->BusyOff(false);
}

void MainWindow::on_pbRefreshStack_clicked()
{
  ui->teParticleStack->clear();
  int elements = ParticleStack.size();


  if (ui->cbHideStackText->isChecked())
  {
     QString str;
     str.setNum(elements);
     str = "Stack contains "+str;
     if (elements == 1) str += " particle"; else str += " particles";
     ui->teParticleStack->append(str);
  }
  else
  {
    QString record, lastRecord;
    int counter = 0; //0 since it will inc on i==0
    int first = 0;
    for (int i=0; i<elements; i++)
      {
        //record = Detector->ParticleCollection[ParticleStack[i]->Id]->ParticleName + "   ";
        record = Detector->MpCollection->getParticleName(ParticleStack[i]->Id) + "   ";
        record += "x,y,z: (" + QString::number(ParticleStack[i]->r[0]) + ","
                             + QString::number(ParticleStack[i]->r[1]) + ","
                             + QString::number(ParticleStack[i]->r[2]) + ")   ";
        record += "i,j,k: (" + QString::number(ParticleStack[i]->v[0]) + ","
                             + QString::number(ParticleStack[i]->v[1]) + ","
                             + QString::number(ParticleStack[i]->v[2]) + ")   ";
        record += "e: " + QString::number(ParticleStack[i]->energy) + " keV   ";
        record += "t: " + QString::number(ParticleStack[i]->time);

        if (i == 0) lastRecord = record;

        if (record == lastRecord) counter++;
        else
          {
            QString out = QString::number(first);
            if (counter > 1) out += "-" + QString::number(first + counter - 1);
            out += "> " + lastRecord;
            ui->teParticleStack->append(out);
            counter = 1;
            first = i;
            lastRecord = record;
          }
      }

    if (!record.isEmpty())
      {
        QString out = QString::number(first);
        if (counter > 1) out += "-" + QString::number(first + counter - 1);
        out += "> " + lastRecord;
        ui->teParticleStack->append(out);
      }
  }

  ui->sbStackElement->setValue(elements-1);
  ui->pbTrackStack->setEnabled(elements!=0);
}

void MainWindow::on_pbRemoveFromStack_clicked()
{
    int element = ui->sbStackElement->value();
    int StackSize = ParticleStack.size();
    if (element > StackSize-1) return;
    delete ParticleStack[element];
    ParticleStack.remove(element);
    MainWindow::on_pbRefreshStack_clicked();
}

void MainWindow::on_pbClearAllStack_clicked()
{
    //qDebug() << "Clear particle stack triggered";
    EventsDataHub->clear();

    for (int i=0; i<ParticleStack.size(); i++)
        delete ParticleStack[i];
    ParticleStack.clear();
    MainWindow::on_pbRefreshStack_clicked();
}
