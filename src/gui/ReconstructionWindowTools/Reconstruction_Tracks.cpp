//ANTS2
#include "reconstructionwindow.h"
#include "ui_reconstructionwindow.h"
#include "mainwindow.h"
#include "eventsdataclass.h"

//Root
#include "TVirtualGeoTrack.h"

void ReconstructionWindow::ReconstructTrack()
{
  //  "Top" -> "World"
  /*
  //if (MW->Reconstructor->isTimedEventsEmpty())
  if (EventsDataHub->TimedEvents.isEmpty())
    {
      message("There are no time-resolved data!", this);
      return;
    }

  if (ui->cobReconstructionAlgorithm->currentIndex()!=0 && ui->cobReconstructionAlgorithm->currentIndex()!=3)
    if (!MW->SensLRF->isAllLRFsDefined())
    {
      message("LRFs are not defined!", this);
      ui->twOptions->setCurrentIndex(2);
      return;
    }

  int iev = ui->sbEventNumberInspectTrack->value();
  qDebug()<<"Reconstructing track for event #"<<iev;  
  int TimeBins = EventsDataHub->getTimeBins();
  qDebug()<<"  Timed bins:"<<TimeBins;
  double TimeFrom = 0;
  double TimeTo = TimeBins;
  if (EventsDataHub->fSimulatedData)
  {
      TimeFrom = EventsDataHub->LastSimSet.TimeFrom;
      TimeTo = EventsDataHub->LastSimSet.TimeTo;
  }
  //double TimeFrom = MW->Reconstructor->getTimeFrom();
  //double TimeTo = MW->Reconstructor->getTimeTo();
  double TimeBinLength = (TimeTo - TimeFrom)/TimeBins;

  QVector<ReconstructionStruct*> ThisTrack;
  ThisTrack.resize(0);

  for (int itime = 0; itime<TimeBins; itime++)
    {
      ReconstructionStruct* ThisTimeCell = new ReconstructionStruct();
      // *ThisTimeCell = *MW->Reconstructor->reconstructEvent(iev, itime);
      ReconstructionStruct* result;
      if (ui->cobReconstructionAlgorithm->currentIndex() == 4) result = EventsDataHub->ReconstructionData[iev];
      else
        {
          QJsonObject json;
          ReconstructionWindow::writeReconSettingsToJson(json);
          ReconstructionWindow::writeFilterSettingsToJson(json);
          ReconstructionWindow::writePMrelatedInfoToJson(json);
          result = MW->Reconstructor->reconstructOneEvent(json, iev);
        }
      *ThisTimeCell = *result;

      int thisTime = TimeFrom + itime*TimeBinLength;
      qDebug()<<" This TimeBin X Y flag:"<<ThisTimeCell->Points[0].r[0]<<ThisTimeCell->Points[0].r[1]<<ThisTimeCell->ReconstructionOK;
      if (!ThisTimeCell->ReconstructionOK)
        {
          ThisTrack.append(ThisTimeCell);
          continue;  //if reconstruction of X-Y failed, contiue to the next time bin
        }
      //now we reconstruct z assuming that the drift ended at the center of the secondary scintillator

      MW->GeoManager->SetCurrentPoint(ThisTimeCell->Points[0].r[0], ThisTimeCell->Points[0].r[1], 0);
      //MW->GeoManager->SetCurrentPoint(0, 0, 0);
      MW->GeoManager->SetCurrentDirection(0, 0, 1.);
      MW->GeoManager->FindNode();

      QString VolName;
      do
      {
         //going up until enter Top
         MW->GeoManager->FindNextBoundaryAndStep();
         VolName = MW->GeoManager->GetCurrentVolume()->GetName();
         qDebug()<<VolName;
      }
      while (VolName != "Top");
      qDebug()<<"Found Top on the way up";

      MW->GeoManager->SetCurrentDirection(0, 0, -1.); //going downward
      do
      {
         //going down until enter Sec scintillator ("SecScint")
         MW->GeoManager->FindNextBoundaryAndStep();
         VolName = MW->GeoManager->GetCurrentVolume()->GetName();
         qDebug()<<VolName;
         if (VolName == "Top")
           {
             qDebug()<<"SecScint not found for this XY!";
             ThisTimeCell->ReconstructionOK = false;
             ThisTrack.append(ThisTimeCell);
             continue;  //failed to find SecScint, so contiue to the next time bin
           }
      }
      while (VolName != "SecScint");
      qDebug()<<"Found SecSci on the way down";

      double offsetZZ = MW->GeoManager->GetCurrentPoint()[2];
      int ThisMatIndex = MW->GeoManager->GetCurrentVolume()->GetMaterial()->GetIndex();      
      double DriftVelocity = 0.01 * (*MW->MaterialCollection)[ThisMatIndex]->e_driftVelocity;

      MW->GeoManager->FindNextBoundary(); //does not step - we are still inside SecScint!
      double halfWidth = 0.5 * MW->GeoManager->GetStep();
      offsetZZ -= halfWidth; //offset set to the middle of the sec scintillator

      qDebug()<<"zzOffset"<<offsetZZ<<"thisTime"<<thisTime;
      double timeToCrossHalfSecScint =  halfWidth / DriftVelocity;
      if (thisTime < timeToCrossHalfSecScint)
        {
          ThisTimeCell->Points[0].r[2] = offsetZZ - thisTime * DriftVelocity;
          qDebug()<<"found Z inside the secondary scintillator!";
        }
      else
         {
          //have to start checking other volumes

          thisTime -= timeToCrossHalfSecScint; //gong back in time
          offsetZZ -= halfWidth; //offset z and time set to the lower border of SecScint

          MW->GeoManager->FindNextBoundaryAndStep(); //entered the next volume: before were at the border of SecScint!

          ThisTimeCell->ReconstructionOK = false;  //flag for the cycle below, if we will find, it will be set to true
          do
            {
             //going down untill enter Top ("Top") or matched the time we looking for
             VolName = MW->GeoManager->GetCurrentVolume()->GetName();
             qDebug()<<"entered "<<VolName;
             qDebug()<<"zzOffset"<<offsetZZ<<"thisTime"<<thisTime;
             ThisMatIndex = MW->GeoManager->GetCurrentVolume()->GetMaterial()->GetIndex();             
             double DriftVelocity = 0.01 * (*MW->MaterialCollection)[ThisMatIndex]->e_driftVelocity;

              //step to the next volume
             MW->GeoManager->FindNextBoundaryAndStep();
             qDebug()<<"did a step";

             double Step = MW->GeoManager->GetStep(); //that was the width of this material
             double MaxTime = Step / DriftVelocity;
             if (MaxTime > thisTime) {
                 //done - Z is in this volume!
                 ThisTimeCell->Points[0].r[2] = offsetZZ - thisTime * DriftVelocity;
                 ThisTimeCell->ReconstructionOK = true;
                 qDebug()<<"Done! Z="<<ThisTimeCell->Points[0].r[2];
                 break; //break the do...while
             }
               //if Z is not in this volume
             thisTime -= MaxTime;
             offsetZZ -= MaxTime * DriftVelocity;

             VolName = MW->GeoManager->GetCurrentVolume()->GetName();
             qDebug()<<"next volume: "<<VolName;
            }
          while (VolName != "Top");
          qDebug()<<"<-finished the search loop";
         }

      //ReconstructionOK is properly updated
      ThisTrack.append(ThisTimeCell);

  }//all time bins done

  for (int itime = 0; itime < TimeBins; itime++) qDebug()<<"OK?"<<ThisTrack[itime]->ReconstructionOK<<"  XYZ"<<ThisTrack[itime]->Points[0].r[0]<<ThisTrack[itime]->Points[0].r[1]<<ThisTrack[itime]->Points[0].r[2];

  //visualization
  MW->GeoManager->ClearTracks();
  MW->DotsTGeo.resize(0);


     // qDebug()<<"Vizualizaing particle gun actual positions...";
     // ReconstructionWindow::VisualizeParticleGunActualPositions(iev);

  //qDebug()<<"Vizualizing EnergyVector data...";
  ReconstructionWindow::VisualizeEnergyVector(iev);
   // }

  //Show ThisTrack
  Int_t track_index = MW->GeoManager->AddTrack(2,22); //  Here track_index is the index of the newly created track in the array of primaries. One can get the pointer of this track and make it known as current track by the manager class:
  TVirtualGeoTrack *track = MW->GeoManager->GetTrack(track_index);
  track->SetLineColor(kRed);
  track->SetLineWidth(2);

  for (int itime=0; itime<TimeBins; itime++)
    {
      if (!ThisTrack[itime]->ReconstructionOK) continue;
      MW->DotsTGeo.append(DotsTGeoStruct(ThisTrack[itime]->Points[0].r, kRed));
      track->AddPoint(ThisTrack[itime]->Points[0].r[0],ThisTrack[itime]->Points[0].r[1],ThisTrack[itime]->Points[0].r[2],0);
    }

  MW->ShowTracks();
  */
}


