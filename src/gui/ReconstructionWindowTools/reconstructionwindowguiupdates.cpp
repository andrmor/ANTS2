#include "reconstructionwindow.h"
#include "ui_reconstructionwindow.h"
#include "eventsdataclass.h"
#include "areconstructionmanager.h"
#include "mainwindow.h"
#include "outputwindow.h"
#include "detectorclass.h"
#include "apmgroupsmanager.h"
#include "sensorlrfs.h"

#include <QDebug>

void ReconstructionWindow::onRequestEventsGuiUpdate()
{
    ForbidUpdate = true; //--//
    if (ui->sbEventNumberInspect->value() > EventsDataHub->Events.size()-1)
       ui->sbEventNumberInspect->setValue(0);
    ForbidUpdate = false; //--//

    ui->dLoadedEnergy->setEnabled(EventsDataHub->fLoadedEventsHaveEnergyInfo || !EventsDataHub->isScanEmpty());
    if (!EventsDataHub->fLoadedEventsHaveEnergyInfo) ui->cbActivateLoadedEnergyFilter->setChecked(false);
    ui->fAnalysisAreaScan->setEnabled(false);

    if (EventsDataHub->ScanNumberOfRuns>1) //clearData sets ScanNumberOfRuns to 1
      if (EventsDataHub->Scan.size() > EventsDataHub->ScanNumberOfRuns) //if not single point scan
        ui->fAnalysisAreaScan->setEnabled(true);
    bool fScanEmpty = EventsDataHub->isScanEmpty();
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
    bool bYellow = (numPass > 0);

    if (!bYellow)
    {
        int iAlg = ui->cobReconstructionAlgorithm->currentIndex();
        if (iAlg == 1 || iAlg == 2 || iAlg == 4)
            if (ui->cbDynamicPassiveByDistance->isChecked() || ui->cbDynamicPassiveBySignal->isChecked())
                bYellow = true;
    }

    ui->twOptions->setTabIcon(1, ( bYellow ? YellowIcon : QIcon()) );
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

void ReconstructionWindow::ShowStatistics(bool bCopyToTextLog)
{
  //qDebug()<<"   Show statistics triggered in Recon window";
  int GoodEvents;
  double AvChi2, AvDeviation;
  EventsDataHub->prepareStatisticsForEvents(Detector->LRFs->isAllLRFsDefined(), GoodEvents, AvChi2, AvDeviation, PMgroups->getCurrentGroup());

  ui->leoEventsPassingAllFilters->setText("0");
  ui->leoAverageChi2->setText("");
  ui->leoAverageDeviation->setText("");
  if (GoodEvents == 0) return;

  QString txt("\nReconstruction -> ");
  txt += "Good events: "+QString::number(GoodEvents);

  ui->leoEventsPassingAllFilters->setText( QString::number(GoodEvents));
  if (AvChi2 != -1 && AvChi2 != 0)
  {
      ui->leoAverageChi2->setText( QString::number(AvChi2) );
      txt += "   Average chi2: "+QString::number(AvChi2);
      lastChi2 = AvChi2;
  }
  if (AvDeviation != -1)
  {
      ui->leoAverageDeviation->setText(QString::number(AvDeviation));
      txt += "   Average XY deviation: "+QString::number(AvDeviation);
  }

  if (bCopyToTextLog) MW->Owindow->OutText(txt);
}
