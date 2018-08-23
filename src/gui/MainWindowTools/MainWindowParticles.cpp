//ANTS2
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "amaterialparticlecolection.h"
#include "materialinspectorwindow.h"
#include "outputwindow.h"
#include "detectorclass.h"
#include "aparticleonstack.h"
#include "particlesourcesclass.h"
#include "amessage.h"
#include "aconfiguration.h"
#include "atrackrecords.h"
#include "simulationmanager.h"

//Qt
#include <QMessageBox>
#include <QDebug>

//Root
#include "TColor.h"
#include "TROOT.h"

void MainWindow::ListActiveParticles()
{
    ui->lwParticles->clear();
    int NumParticles = Detector->MpCollection->countParticles();
    QString tmpStr, tmpStr2;
    for (int i=0; i<NumParticles; i++)
      {
        tmpStr2.setNum(i);
        tmpStr =tmpStr2+">  " + Detector->MpCollection->getParticleName(i);
        QListWidgetItem* pItem =new QListWidgetItem(tmpStr);
        ui->lwParticles->addItem(pItem);
      }
    MainWindow::on_pbRefreshParticles_clicked();
}

void MainWindow::on_pbShowColorCoding_pressed()
{
  for (int i=0; i<ui->lwParticles->count(); i++)
    {
      //TColor* rc = gROOT->GetColor(i +1 +ui->sbParticleTrackColorIndexAdd->value());
      TrackHolderClass th;
      SimulationManager->TrackBuildOptions.applyToParticleTrack(&th, i);
      TColor* rc = gROOT->GetColor(th.Color);

      if (!rc) continue;
      QColor qc = QColor(255*rc->GetRed(), 255*rc->GetGreen(), 255*rc->GetBlue(), 255*rc->GetAlpha());

      QListWidgetItem* pItem = ui->lwParticles->item(i);
      pItem->setTextColor(qc);
    }
}

void MainWindow::on_pbShowColorCoding_released()
{
  for (int i=0; i<ui->lwParticles->count(); i++)
    {
      QListWidgetItem* pItem = ui->lwParticles->item(i);
      pItem->setTextColor(Qt::black);
    }
}

void updateParticleCOB(QComboBox* cob, DetectorClass* Detector)
{
  int old = cob->currentIndex();

  cob->clear();
  for (int i=0; i<Detector->MpCollection->countParticles(); i++)
      cob->addItem(Detector->MpCollection->getParticleName(i));

  if (cob->count() != 0)
  {
     if (old>-1 && old<cob->count()) cob->setCurrentIndex(old);
     else cob->setCurrentIndex(0);
  }
}

void MainWindow::on_pbRefreshParticles_clicked()
{
  QList< QComboBox* > cobs;
  cobs << ui->cobParticleToInspect << ui->cobParticleToStack << ui->cobGunParticle;
  foreach (QComboBox* cob, cobs)
    updateParticleCOB(cob, Detector);

  Owindow->UpdateParticles();
}

void MainWindow::on_lwParticles_currentRowChanged(int currentRow)
{
   if (currentRow<0) return;

   ui->cobParticleToInspect->setCurrentIndex(currentRow);
   MainWindow::on_cobParticleToInspect_currentIndexChanged(ui->cobParticleToInspect->currentIndex()); //refresh indication
}

void MainWindow::on_leParticleName_editingFinished()
{
    ui->cobParticleToInspect->setCurrentIndex(-1);
}

void MainWindow::on_pbAddparticleToActive_clicked()
{
  QString name = ui->leParticleName->text();
  int charge = ui->ledParticleCharge->text().toInt();
  double mass = ui->ledMass->text().toDouble();
  AParticle::ParticleType type;

  if (charge == 0)
    if (name != "gamma" && name != "neutron")
      {
        message("Only 'gamma' and 'neutron' can be neutral!", this);
        ui->ledParticleCharge->setText( "1" );
        return;
      }

  if (name == "gamma")
    {
      type = AParticle::_gamma_; charge = 0; mass = 0;
    }
  else if (name == "neutron")
    {
      type = AParticle::_neutron_; charge = 0; mass = 1;
    }
  else type = AParticle::_charged_;

  int Id = MpCollection->getParticleId(name);
  if (Id != -1)
    {
      //particle with this name NOT found
      switch( QMessageBox::information( this, "", "Particle with this name alredy exists!\n"
                                                  "Note that 'gamma' and 'neutron' are reserved and cannot be modified", "Update", "Cancel",0, 1 ) )
        {
        case 0:
          MpCollection->UpdateParticle(Id, name, type, charge, mass);
          ui->cobParticleToInspect->setCurrentIndex(Id);
        default:
          break;
        }
    }
  else
    {
      //new particle
      bool fOk = MpCollection->AddParticle(name, type, charge, mass);
      if (fOk) ui->cobParticleToInspect->setCurrentIndex(ui->cobParticleToInspect->count()-1);
      onRequestDetectorGuiUpdate();
    }
}

void MainWindow::on_cobParticleToInspect_currentIndexChanged(int index)
{
    if (index<0) return;
    if (index > Detector->MpCollection->countParticles()-1)
      {
        qWarning() << "Bad particle index:"<<index;
        return;
      }
    const AParticle* p = Detector->MpCollection->getParticle(index);
    ui->leParticleName->setText(p->ParticleName);
    ui->ledParticleCharge->setText(QString::number(p->charge));
    ui->ledMass->setText(QString::number(p->mass));
}

void MainWindow::on_pbRemoveParticle_clicked()
{
    int Id = ui->cobParticleToInspect->currentIndex();
    if (Id == -1)
    {
        message("Select one of the defined particles", this);
        return;
    }

  switch( QMessageBox::information( this, "", "Remove particle " + MpCollection->getParticleName(Id) + " ?",
                                        "Yes", "Cancel", 0, 1 ) )
    {
      case 0: break;
      default: return;
    }

  QString err = Config->RemoveParticle(Id); //all updates are automatic, including GUI
  if (!err.isEmpty()) message(err, this);
}
