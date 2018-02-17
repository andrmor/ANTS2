//ANTS2
#include "mainwindow.h"
#include "eventsdataclass.h"
#include "reconstructionwindow.h"
#include "ui_reconstructionwindow.h"
#include "ageomarkerclass.h"
#include "graphwindowclass.h"
#include "windownavigatorclass.h"
#include "reconstructionmanagerclass.h"
#include "globalsettingsclass.h"
#include "apositionenergyrecords.h"
#include "amessage.h"
#include "graphwindowclass.h"

#include <QDebug>

#include "TColor.h"
#include "TLine.h"
#include "TH1D.h"

#ifdef ANTS_FLANN
 #include "nnmoduleclass.h"
 #include "flann/flann.hpp"
#endif

void ReconstructionWindow::on_pbNNshowSpectrum_clicked()
{
#ifdef ANTS_FLANN
  if (EventsDataHub->isEmpty()) return;

  UpdateStatusAllEvents();

  MW->WindowNavigator->BusyOn();

  int numEvents = EventsDataHub->Events.size();
  bool ShowDots = (!EventsDataHub->isReconstructionDataEmpty() && ui->cbNNshowPos->isChecked());
  double FilterMin = ui->ledNNmin->text().toDouble();
  double FilterMax = ui->ledNNmax->text().toDouble();
  int AverageOver = ui->sbNNaverageOver->value();

  MW->WindowNavigator->BusyOn();
  qApp->processEvents();
  bool ok = MW->ReconstructionManager->KNNmodule->Filter.prepareNNfilter(AverageOver + 1); //consider 0 point itself
  if (!ok)
    {
      MW->WindowNavigator->BusyOff();
      message("Failed to prepare NN module!", this);
      return;
    }

  // indication
  float minD = 1.0e10, maxD = 0.0;
  for (int iev=0; iev<numEvents; iev++)
    {      
      float dist = MW->ReconstructionManager->KNNmodule->Filter.getAverageDist(iev);
      if (dist<minD) minD = dist;
      if (dist>maxD) maxD = dist;      
    }  
  qDebug() << "Found min max:"<<minD<<maxD;
  if (ui->cbActivateNNfilter->isChecked())
    {
      if (minD<FilterMin) minD = FilterMin;
      if (maxD>FilterMax) maxD = FilterMax;
    }

  Int_t MyPalette[100];
  if (ShowDots)
    {
      MW->clearGeoMarkers();
      MW->ShowGeometry();

      Double_t r[]    = {0., 0.0, 1.0, 1.0, 1.0};
      Double_t g[]    = {0., 0.0, 0.0, 1.0, 1.0};
      Double_t b[]    = {0., 1.0, 0.0, 0.0, 1.0};
      Double_t stop[] = {0., .25, .50, .75, 1.0};
      Int_t FI = TColor::CreateGradientColorTable(5, stop, r, g, b, 100);
      for (int i=0;i<100;i++) MyPalette[i] = FI+i;
    }

  auto hist1D = new TH1D("NNtmp1","NN_distance", MW->GlobSet->BinsX, minD, maxD);
  auto hist1D2 = new TH1D("NNtmp2","NN_distance", MW->GlobSet->BinsX, minD, maxD);
  for (int iev=0; iev<numEvents; iev++)
    {
       float dist = MW->ReconstructionManager->KNNmodule->Filter.getAverageDist(iev);

       if (!EventsDataHub->isScanEmpty())
         {
           if (EventsDataHub->Scan.at(iev)->Points.size() == 1) hist1D->Fill(dist);
           else hist1D2->Fill(dist);
         }
       else hist1D->Fill(dist);

       if (ShowDots)
         {
           int icol = dist / maxD * 99.9;
           int col = MyPalette[icol];
           GeoMarkerClass* marks = new GeoMarkerClass("kNN", 7, 1, col);
           for (int i=0; i<EventsDataHub->ReconstructionData[0][iev]->Points.size(); i++)
             //if (!EventsDataHub->isScanEmpty()) MW->DotsTGeo.append(DotsTGeoStruct(EventsDataHub->SimulatedScan.at(iev)->Points[i].r, col));
             //else MW->DotsTGeo.append(DotsTGeoStruct(EventsDataHub->ReconstructionData[iev]->Points[i].r, col));
             if (!EventsDataHub->isScanEmpty())
               marks->SetNextPoint(EventsDataHub->Scan.at(iev)->Points[i].r[0], EventsDataHub->Scan.at(iev)->Points[i].r[1], EventsDataHub->Scan.at(iev)->Points[i].r[2]);
             else
               marks->SetNextPoint(EventsDataHub->ReconstructionData[0][iev]->Points[i].r[0], EventsDataHub->ReconstructionData[0][iev]->Points[i].r[1], EventsDataHub->ReconstructionData[0][iev]->Points[i].r[2]);
           MW->GeoMarkers.append(marks);
         }
    }

  MW->GraphWindow->Draw(hist1D, "", false);
  if (hist1D2->GetEntries() != 0)
    {
      hist1D2->SetLineColor(kRed);
      MW->GraphWindow->Draw(hist1D2, "same", false);
    }
  if (ShowDots)
    {
      for (int i=0; i<100; i++)
        {          
          GeoMarkerClass* marks = new GeoMarkerClass("kNN", 7, 1, MyPalette[i]);
          marks->SetNextPoint(0,0,-10.0 + 0.2*i);
          MW->GeoMarkers.append(marks);
        }

      MW->ShowGeometry(false);
    } 

  MW->WindowNavigator->BusyOff();

  //showing range
  double minY = 0;
  double maxY = MW->GraphWindow->getCanvasMaxY();
  TLine* L1 = new TLine(FilterMin, minY, FilterMin, maxY);
  L1->SetLineColor(kGreen);
  MW->GraphWindow->Draw(L1, "same", false);
  TLine* L2 = new TLine(FilterMax, minY, FilterMax, maxY);
  L2->SetLineColor(kGreen);
  MW->GraphWindow->Draw(L2, "same", true);

  MW->GraphWindow->LastDistributionShown = "NNSpectrum";

#endif
}

void ReconstructionWindow::on_pbResetKNNfilter_clicked()
{
#ifdef ANTS_FLANN
  MW->ReconstructionManager->KNNmodule->Filter.clear();
  UpdateStatusAllEvents();
#endif
}

void ReconstructionWindow::on_pbCalibrateX_clicked()
{
#ifdef ANTS_FLANN
  MW->WindowNavigator->BusyOn();
  qApp->processEvents();
  bool ok = MW->ReconstructionManager->KNNmodule->Reconstructor.calibrate('x');
  if (!ok) message(MW->ReconstructionManager->KNNmodule->Reconstructor.ErrorString, this);
  MW->WindowNavigator->BusyOff();
#endif
}

void ReconstructionWindow::on_pbCalibrateY_clicked()
{
#ifdef ANTS_FLANN
  MW->WindowNavigator->BusyOn();
  qApp->processEvents();
  bool ok = MW->ReconstructionManager->KNNmodule->Reconstructor.calibrate('y');
  if (!ok) message(MW->ReconstructionManager->KNNmodule->Reconstructor.ErrorString, this);
  MW->WindowNavigator->BusyOff();
#endif
}

void ReconstructionWindow::on_pbCalibrateXY_clicked()
{
#ifdef ANTS_FLANN
  MW->WindowNavigator->BusyOn();
  qApp->processEvents();
  bool ok = MW->ReconstructionManager->KNNmodule->Reconstructor.calibrate('b');
  if (!ok) message(MW->ReconstructionManager->KNNmodule->Reconstructor.ErrorString, this);
  MW->WindowNavigator->BusyOff();
#endif
}

void ReconstructionWindow::on_cobKNNcalibration_activated(int index)
{
#ifdef ANTS_FLANN
    if (index == 0)
      {
        onKNNreadyXchanged( MW->ReconstructionManager->KNNmodule->Reconstructor.isXready(), MW->ReconstructionManager->KNNmodule->Reconstructor.countEventsXset() );
        onKNNreadyYchanged( MW->ReconstructionManager->KNNmodule->Reconstructor.isYready(), MW->ReconstructionManager->KNNmodule->Reconstructor.countEventsYset() );
      }
    else
      {
        onKNNreadyXchanged( MW->ReconstructionManager->KNNmodule->Reconstructor.isXYready(), MW->ReconstructionManager->KNNmodule->Reconstructor.countEventsXYset() );
      }
#endif
}

void ReconstructionWindow::onKNNreadyXchanged(bool ready, int events)
{
   if (ui->cobKNNcalibration->currentIndex() == 0)
     {
       ui->lKNNreadyX->setText( ready ? "Ready" : "NOT ready" );
       ui->lEventsTrainingSetX->setText( QString::number(events) );
     }
   else     
     {
       ui->lKNNreadyXY->setText( ready ? "Ready" : "NOT ready" );
       ui->lEventsTrainingSetXY->setText( QString::number(events) );
     }
}

void ReconstructionWindow::onKNNreadyYchanged(bool ready, int events)
{
   ui->lKNNreadyY->setText( ready ? "Ready" : "NOT ready" );
   ui->lEventsTrainingSetY->setText( QString::number(events) );
}

void ReconstructionWindow::on_pbKNNupdate_clicked()
{
#ifdef ANTS_FLANN
  MW->ReconstructionManager->KNNmodule->Reconstructor.clear();
#endif
}

void ReconstructionWindow::on_sbKNNuseNeighboursInRec_editingFinished()
{
#ifdef ANTS_FLANN
  int num = ui->sbKNNuseNeighboursInRec->value();
  int neighb = ui->sbKNNnumNeighbours->value();
  if (num > neighb)
    {
      ui->sbKNNuseNeighboursInRec->setValue(neighb);
      message("Cannot be larger than the number of neighbours defined in calibration!", this);
    }
#endif
}
