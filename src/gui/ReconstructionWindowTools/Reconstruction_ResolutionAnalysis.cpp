#include "reconstructionwindow.h"
#include "ui_reconstructionwindow.h"
#include "mainwindow.h"
#include "graphwindowclass.h"
#include "outputwindow.h"
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "ageomarkerclass.h"
#include "apositionenergyrecords.h"
#include "amessage.h"
#include "apmgroupsmanager.h"

#include <QDebug>
#include <QFileDialog>

#include "TTree.h"
#include "TFile.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TGeoManager.h"

void ReconstructionWindow::on_pbPointShowDistribution_clicked()
{
  //if data are not ready
  if (EventsDataHub->isScanEmpty())
    {
      message("There is no scan data!", this);
      return;
    }
  int CurrentGroup = PMgroups->getCurrentGroup();
  if (!EventsDataHub->isReconstructionReady(CurrentGroup))
    {
      message("Reconstruction not ready! Run reconstruction first", this);
      return;
    }

  //inits
  int selector = ui->cobWhatToShow->currentIndex();
  auto hist1D = new TH1D("hist1ShowPDist", "", ui->sbAnalysisNumBins->value(), 0,0);
#if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
  hist1D->SetBit(TH1::kCanRebin);
#endif

  int node = ui->sbAnalysisEventNumber->value();  
  int ithis = node*EventsDataHub->ScanNumberOfRuns;
  double rActual[3];
  for (int i=0; i<3;i++) rActual[i] = EventsDataHub->Scan[ithis]->Points[0].r[i];
  int totEvents = EventsDataHub->Events.size();

  //averages
  double sum[3] = {0.0, 0.0, 0.0};
  int num = 0;
  for (int ievRelative = 0; ievRelative < EventsDataHub->ScanNumberOfRuns; ievRelative++)
    {
      int iev = ievRelative + node*EventsDataHub->ScanNumberOfRuns;
      if (iev>totEvents-1) break;
      if (!EventsDataHub->ReconstructionData[CurrentGroup][iev]->GoodEvent) continue;
//        qDebug()<<"processing event#"<<iev;
      for (int j=0;j<3; j++) sum[j] += EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[j]; /// *** !!! only single events are implemented!
      num++;
    }
  if (num==0) num=1;
  for (int j=0;j<3; j++) sum[j] /= num;

  //deltas
  double delta[3] = {0.0, 0.0, 0.0};
  for (int ievRelative = 0; ievRelative < EventsDataHub->ScanNumberOfRuns; ievRelative++)
    {
      int iev = ievRelative + node*EventsDataHub->ScanNumberOfRuns;
      if (iev>totEvents-1) break;
      if (!EventsDataHub->ReconstructionData[CurrentGroup][iev]->GoodEvent) continue;
      for (int j=0;j<3; j++) delta[j] = (EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[j] - sum[j]);

      switch (selector)
        {
          case 0: //uses below
          case 1: //uses below
          case 2:
            hist1D->Fill( sum[selector] + delta[selector] );
            break;
          default:
            break;
        }
    }

  MW->GraphWindow->Draw(hist1D);
  MW->GraphWindow->LastDistributionShown = "ResolutionAnalysis";
}

void ReconstructionWindow::on_sbAnalysisEventNumber_valueChanged(int arg1)
{   
   int CurrentGroup = PMgroups->getCurrentGroup();
   if (EventsDataHub->isScanEmpty() || !EventsDataHub->isReconstructionReady(CurrentGroup))
     {
       ui->sbAnalysisEventNumber->setValue(0);
       message("No data!", this);
       ui->twData->setCurrentIndex(0);
       return;
     }
   qDebug() << "Number of runs in scan:"<< EventsDataHub->ScanNumberOfRuns;
   if (EventsDataHub->ScanNumberOfRuns == 1)
     {
       message("Scan has to be performed with multiple run option!",this);
       return;
     }

   int events = EventsDataHub->Scan.size();
   int nodes = events / EventsDataHub->ScanNumberOfRuns;
   if (arg1 > nodes-1)
     {
       ui->sbAnalysisEventNumber->setValue(0);
       return;
     }

   ReconstructionWindow::AnalyzeScanNode();
}

void ReconstructionWindow::on_pbAnalyzeResolution_clicked()
{
    if (EventsDataHub->isScanEmpty())
      {
        message("Scan data are not ready!", this);
        return;
      }
    int CurrentGroup = PMgroups->getCurrentGroup();
    if (!EventsDataHub->isReconstructionReady(CurrentGroup))
      {
        message("Run reconstruction before using this tool!", this);
        return;
      }

    int events = EventsDataHub->Scan.size();
    int nodes = events / EventsDataHub->ScanNumberOfRuns;
    if (nodes*EventsDataHub->ScanNumberOfRuns < events) nodes++;
    if (ui->sbAnalysisEventNumber->value() > nodes-1)
      {
        ui->sbAnalysisEventNumber->setValue(0);
        return;
      }    
    MW->Owindow->show();
    MW->Owindow->raise();
    MW->Owindow->activateWindow();
    ReconstructionWindow::AnalyzeScanNode();
}

void ReconstructionWindow::AnalyzeScanNode()
{
    if (EventsDataHub->isScanEmpty())
      {
        message("Scan is empty!", this);
        return;
      }
    int CurrentGroup = PMgroups->getCurrentGroup();
    if (!EventsDataHub->isReconstructionReady(CurrentGroup))
      {
        message("Reconstruction was not yet performed!", this);
        return;
      }

    if (EventsDataHub->ScanNumberOfRuns <= 1)
      {
        message("Scan data should contain multiple runs!", this);
        return;
      }

    int events = EventsDataHub->Scan.size();   
    int nodes = events / EventsDataHub->ScanNumberOfRuns;
    if (nodes*EventsDataHub->ScanNumberOfRuns<events) nodes++;
    qDebug()<<"Events:"<<events<<" nodes: "<<nodes;

    int node = ui->sbAnalysisEventNumber->value();
    if (node>nodes-1)
      {
        message("Invalid number of node!", this);
        return;
      }

    MW->clearGeoMarkers();
    MW->Detector->GeoManager->ClearTracks();
    int ithis = node*EventsDataHub->ScanNumberOfRuns;
    double rActual[3];
    for (int i=0; i<3;i++) rActual[i] = EventsDataHub->Scan[ithis]->Points[0].r[i];

    //averages
    double sum[3] = {0.0, 0.0, 0.0};
    int totEvents = EventsDataHub->Events.size();
    int num = 0;
    for (int ievRelative = 0; ievRelative < EventsDataHub->ScanNumberOfRuns; ievRelative++)
      {
        int iev = ievRelative + node*EventsDataHub->ScanNumberOfRuns;
        if (iev > totEvents-1) break;
        if (!EventsDataHub->ReconstructionData[CurrentGroup][iev]->GoodEvent) continue;
        //        qDebug()<<"processing event#"<<iev;
        for (int j=0;j<3; j++) sum[j] += EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[j]; ///WARNING *** !!! muiltiple events not supported!
        num++;
      }
    double factor = 0;
    if (num > 0) factor = 1.0/num;
    for (int j=0;j<3; j++) sum[j] *= factor;
    qDebug()<<"av X="<<sum[0]<<" av Y="<<sum[1]<<" av Z="<<sum[2];

    //deltas
    double sigma[3] = {0.0, 0.0, 0.0};
    for (int ievRelative = 0; ievRelative < EventsDataHub->ScanNumberOfRuns; ievRelative++)
      {
        int iev = ievRelative + node*EventsDataHub->ScanNumberOfRuns;
        if (iev>totEvents-1) break;
        if (!EventsDataHub->ReconstructionData[CurrentGroup][iev]->GoodEvent) continue;
        for (int j=0;j<3; j++)
          sigma[j] += (EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[j] - sum[j])*(EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[j] - sum[j]);
      }

    factor = 0;
    if (num > 1) factor = 1.0/(num-1);
    for (int j=0;j<3; j++) sigma[j] = sqrt(sigma[j]*factor);
    qDebug()<<"sigma X="<<sigma[0]<<" sigma Y="<<sigma[1]<<" sigma Z="<<sigma[2];

    //vizualization
    QString str, str1;
    MW->Owindow->OutText("-------------------------------");
    MW->Owindow->OutText("Node#: "+QString::number(node));
    MW->Owindow->OutText("Number of available good events: "+QString::number(num));
    MW->Owindow->OutText("True XYZ coordinates [mm]:");
    str = "  ";
    for (int i=0; i<3; i++)
      {
        str1.setNum(rActual[i]);
        str+=str1+"   ";
      }
    MW->Owindow->OutText(str);
    MW->Owindow->OutText("Reconstructed XYZ coordinates [mm]:");
    str = "  ";
    for (int i=0; i<3; i++)
      {
        str1.setNum(sum[i]);
        str+=str1+"   ";
      }
    MW->Owindow->OutText(str);
    MW->Owindow->OutText("Sigmas [mm]:");
    str = "  ";
    for (int i=0; i<3; i++)
      {
        str1.setNum(sigma[i]);
        str+=str1+"   ";
      }
    MW->Owindow->OutText(str);

    MW->Owindow->SetTab(0);

    //vizualization
    GeoMarkerClass* marks = new GeoMarkerClass("Recon", 6, 2, kRed);
    int iev = 0;
    for (int ievRelative = 0; ievRelative < EventsDataHub->ScanNumberOfRuns; ievRelative++)
      {
        iev = ievRelative + node*EventsDataHub->ScanNumberOfRuns;
        if (iev>totEvents-1) break;
        if (!EventsDataHub->ReconstructionData[CurrentGroup][iev]->GoodEvent) continue;
        //MW->DotsTGeo.append(DotsTGeoStruct(EventsDataHub->ReconstructionData[iev]->Points[0].r, kRed));
        marks->SetNextPoint(EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[0], EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[1], EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[2]);
      }
    MW->GeoMarkers.append(marks);

    GeoMarkerClass* marks1 = new GeoMarkerClass("Scan", 6, 2, kBlue);
    //MW->DotsTGeo.append(DotsTGeoStruct( EventsDataHub->SimulatedScan[iev]->Points[0].r, kBlue));
    marks1->SetNextPoint(EventsDataHub->Scan[iev]->Points[0].r[0], EventsDataHub->Scan[iev]->Points[0].r[1], EventsDataHub->Scan[iev]->Points[0].r[2]);  
    MW->GeoMarkers.append(marks1);

    MW->ShowGeometry(false);
}

bool ReconstructionWindow::BuildResolutionTree(int igroup)
{
  if (EventsDataHub->isScanEmpty())
    {
      message("Scan data are not ready!", this);
      return false;
    }
  if ( !EventsDataHub->isReconstructionReady(igroup) )
    {
      message("Need to run reconstruction first!", this);
      return false;
    }

  bool ok = EventsDataHub->createResolutionTree(igroup);
  if (!ok) message("Error while making tree: data not consistent", this);
  return ok;
}

void ReconstructionWindow::on_pbShowSigmaVsArea_clicked()
{
  bool ok = ReconstructionWindow::BuildResolutionTree(PMgroups->getCurrentGroup());
  if (!ok)
    {
      message("failed to build the resolution tree!", this);
      return;
    }
  if (!EventsDataHub->ResolutionTree)
    {
      message("Cannot find the resolution tree!", this);
      return;
    }

  TH1::AddDirectory(true);
  auto hist2D = new TH2D("tmpHist","tmpHist",ui->sbASBinsX->value(),ui->ledASXfrom->text().toDouble(), ui->ledASXto->text().toDouble(),
                                             ui->sbASBinsY->value(),ui->ledASYfrom->text().toDouble(), ui->ledASYto->text().toDouble());
  hist2D->SetXTitle("x, mm");
  hist2D->SetYTitle("y, mm");

  EventsDataHub->ResolutionTree->SetMarkerStyle(20);

  switch (ui->cobShowWhichSigma->currentIndex())
    {
    case 0:
      EventsDataHub->ResolutionTree->Draw("y:x>>tmpHist", "sigmax", "colz");
      break;
    case 1:
      EventsDataHub->ResolutionTree->Draw("y:x>>tmpHist", "sigmay", "colz");
      break;
    case 2:
      EventsDataHub->ResolutionTree->Draw("y:x>>tmpHist", "sigmaz", "colz");
      break;
    }

  TH1::AddDirectory(false);
  gDirectory->RecursiveRemove(hist2D);

  MW->GraphWindow->Draw(hist2D, "colz");
}
