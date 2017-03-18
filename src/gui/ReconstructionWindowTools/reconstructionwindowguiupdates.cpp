#include "reconstructionwindow.h"
#include "ui_reconstructionwindow.h"
#include "eventsdataclass.h"
#include "reconstructionmanagerclass.h"
#include "mainwindow.h"
#include "outputwindow.h"
#include "detectorclass.h"
#include "alrfmoduleselector.h"
#include "apmgroupsmanager.h"

#include <QDebug>

void ReconstructionWindow::onRequestEventsGuiUpdate()
{
    ForbidUpdate = true; //--//
    if (ui->sbEventNumberInspect->value() > EventsDataHub->Events.size()-1)
       ui->sbEventNumberInspect->setValue(0);
    ForbidUpdate = false; //--//

    ui->dLoadedEnergy->setEnabled(EventsDataHub->fLoadedEventsHaveEnergyInfo);
    if (!EventsDataHub->fLoadedEventsHaveEnergyInfo) ui->cbActivateLoadedEnergyFilter->setChecked(false);
    if (EventsDataHub->isScanEmpty()) ui->cbShowActualPosition->setChecked(false);
    ui->fAnalysisAreaScan->setEnabled(false);

    if (EventsDataHub->ScanNumberOfRuns>1) //clearData sets ScanNumberOfRuns to 1
      if (EventsDataHub->Scan.size() > EventsDataHub->ScanNumberOfRuns) //if not single point scan
        ui->fAnalysisAreaScan->setEnabled(true);
    bool fScanEmpty = EventsDataHub->isScanEmpty();
    ui->cbShowActualPosition->setEnabled(!fScanEmpty);
    if (fScanEmpty) ui->cbPlotVsActualPosition->setChecked(false);
    ui->cbPlotVsActualPosition->setEnabled(!fScanEmpty);
    onManifestItemsGuiUpdate();
    SetProgress(0);
    RefreshNumEventsIndication();
    ShowStatistics();
    MW->Owindow->SetCurrentEvent(0); ///necessary?
}

void ReconstructionWindow::onManifestItemsGuiUpdate()
{
    bool fManifestEmpty = EventsDataHub->Manifest.isEmpty();
    if (fManifestEmpty) ui->cbShowManifestItems->setChecked(false);
    ui->cbShowManifestItems->setEnabled(!fManifestEmpty);
}

void ReconstructionWindow::OnEventsDataAdded()
{
  //qDebug() << "OnEvAdd--->Clear";
  EventsDataHub->clearReconstruction();
  //do default population of EventsDataHub->ReconstructionData array and check all filters that can be checked before performing reconstruction
  ReconstructionWindow::UpdateStatusAllEvents();
  ReconstructionWindow::onRequestEventsGuiUpdate();
}

void ReconstructionWindow::onUpdatePassiveIndication()
{
    int numPass = PMgroups->countStaticPassives();
    ui->twOptions->setTabIcon(3, (numPass> 0 ? YellowIcon : QIcon()) );
}

void ReconstructionWindow::RefreshNumEventsIndication()
{
   QString str;

   if ( EventsDataHub->isEmpty() )
     {
       str = "0";
       ui->fNoData->setVisible(true);
     }
   else
     {
       str.setNum(EventsDataHub->Events.size());
       ui->fNoData->setVisible(false);
     }
   ui->leoEventsTotal->setText(str);

   if (EventsDataHub->TimedEvents.isEmpty() )
     {
       str = "";
       ui->leoTimeBins->setText(str);
       ui->leoTimeBins->setEnabled(false);
       ui->labTimeBins->setEnabled(false);
     }
   else
     {
       str.setNum(EventsDataHub->getTimeBins());
       ui->leoTimeBins->setEnabled(true);
       ui->labTimeBins->setEnabled(true);
     }
   ui->leoTimeBins->setText(str);
}

void ReconstructionWindow::ShowStatistics()
{
  //qDebug()<<"   Show statistics triggered in Recon window";
  int GoodEvents;
  double AvChi2, AvDeviation;
  EventsDataHub->prepareStatisticsForEvents(Detector->LRFs->isAllLRFsDefined(), GoodEvents, AvChi2, AvDeviation, PMgroups->getCurrentGroup());

  ui->leoEventsPassingAllFilters->setText("0");
  ui->leoAverageChi2->setText("");
  ui->leoAverageDeviation->setText("");
  if (GoodEvents == 0) return;

  ui->leoEventsPassingAllFilters->setText( QString::number(GoodEvents));
  if (AvChi2 != -1)
    {
      ui->leoAverageChi2->setText( QString::number(AvChi2) );
      lastChi2 = AvChi2;
    }
  if (AvDeviation != -1)  ui->leoAverageDeviation->setText(QString::number(AvDeviation));
}
