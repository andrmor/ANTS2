//ANTS2
#include "outputwindow.h"
#include "ui_outputwindow.h"
#include "mainwindow.h"
#include "graphwindowclass.h"
#include "pms.h"
#include "pmtypeclass.h"
#include "windownavigatorclass.h"
#include "guiutils.h"
#include "amaterialparticlecolection.h"
#include "eventsdataclass.h"
#include "detectorclass.h"
#include "dynamicpassiveshandler.h"
#include "globalsettingsclass.h"
#include "apositionenergyrecords.h"
#include "amessage.h"
#include "apmgroupsmanager.h"
#include "amonitor.h"
#include "asandwich.h"
#include "ageoobject.h"
#include "detectoraddonswindow.h"

//ROOT
#include "TGraph2D.h"
#include "TH2C.h"
#include "TH1D.h"
#include "TMath.h"
#include "TH1.h"

//Qt
#include <QDebug>
#include <QGraphicsItem>
#include <QString>
#include <QBitArray>
#include <QStandardItemModel>

OutputWindow::OutputWindow(QWidget *parent, MainWindow *mw, EventsDataClass *eventsDataHub) :
    QMainWindow(parent),
    ui(new Ui::OutputWindow)
{
    MW = mw;
    EventsDataHub = eventsDataHub;
    GVscale = 10.0;
    ui->setupUi(this);
    bForbidUpdate = false;

    this->setWindowTitle("Output");

    Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
    windowFlags |= Qt::WindowCloseButtonHint;
    this->setWindowFlags( windowFlags );

    modelPMhits = 0;

    ui->pbSiPMpixels->setEnabled(false);
    ui->sbTimeBin->setEnabled(false);
    ui->pbRefreshViz->setVisible(false);

    //Graphics view
    scaleScene = new QGraphicsScene(this);
    ui->gvScale->setScene(scaleScene);

    scene = new QGraphicsScene(this);
    //qDebug() << "This scene pointer:"<<scene;
    gvOut = new myQGraphicsView(ui->tabViz);
    gvOut->setGeometry(0,0,325,325);
    gvOut->setScene(scene);

    gvOut->setDragMode(QGraphicsView::ScrollHandDrag); //if zoomed, can use hand to center needed area
    gvOut->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    gvOut->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gvOut->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gvOut->setRenderHints(QPainter::Antialiasing);  

    ui->tabwinDiagnose->setCurrentIndex(0);
}

OutputWindow::~OutputWindow()
{
  clearGrItems();
  delete ui;  
}

void OutputWindow::PMnumChanged()
{
  ui->sbPMnumberToShowTime->setValue(0);
  SiPMpixels.clear();
}

void OutputWindow::SetCurrentEvent(int iev)
{  
  bForbidUpdate = true;
  //if (iev == ui->sbEvent->value()) on_sbEvent_valueChanged(iev); else
  ui->sbEvent->setValue(iev);
  bForbidUpdate = false;

  RefreshData();
}

void OutputWindow::on_pbShowPMtime_clicked()
{      
    if (EventsDataHub->TimedEvents.isEmpty()) return;

    int CurrentEvent = ui->sbEvent->value();
    if (CurrentEvent >= EventsDataHub->TimedEvents.size())
      {
        message("Invalid event number!", this);
        return;
      }

    int bins = EventsDataHub->LastSimSet.TimeBins;
    double from = EventsDataHub->LastSimSet.TimeFrom;
    double to   = EventsDataHub->LastSimSet.TimeTo;

    int ipm = ui->sbPMnumberToShowTime->value();
    if (ipm>MW->PMs->count()-1)
      {
        ui->sbPMnumberToShowTime->setValue(0);
        if (MW->PMs->count()==0) return;
        on_pbShowPMtime_clicked();
        return;
      }

    auto hist1D = new TH1D("Time_spec_ev", "Time of detection", bins, from, to);
#if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
    hist1D->SetBit(TH1::kCanRebin);
#endif
    for (int i=0; i<bins; i++)
       hist1D->SetBinContent(i+1, EventsDataHub->TimedEvents[CurrentEvent][i][ipm]);
    hist1D->GetXaxis()->SetTitle("Time, ns");
    hist1D->GetYaxis()->SetTitle("Signal");
    MW->GraphWindow->Draw(hist1D);
}

void OutputWindow::RefreshPMhitsTable()
{
    bool fHaveData = !EventsDataHub->isEmpty();    
    int CurrentEvent = ui->sbEvent->value();
    if (fHaveData && CurrentEvent>EventsDataHub->Events.size()-1) fHaveData = false; //protection

    int columns = MW->PMs->count() + 1; //+sum over all PMs  
    int rows = 0;    
    if (fHaveData && EventsDataHub->isTimed())
        rows = EventsDataHub->TimedEvents[0].size();

    if (modelPMhits)
      {
        ui->tvPMhits->setModel(0);
        delete modelPMhits;
      }

    modelPMhits = new QStandardItemModel(rows+1,columns,this);

    ui->tvPMhits->horizontalHeader()->setVisible(true);
    ui->tvPMhits->verticalHeader()->setVisible(true);

    //qDebug()<<"rows: "<<rows<<"columns: "<<columns;

    QString tmpStr;
    //prepare headers
      //horizontal
    modelPMhits->setHorizontalHeaderItem(0, new QStandardItem("Sum"));
    for (int i=1; i<columns;i++)
      {
        tmpStr.setNum(i-1);
        tmpStr = "#"+tmpStr;
        modelPMhits->setHorizontalHeaderItem(i, new QStandardItem(tmpStr));
      }
      //vertical
    if (rows>0) modelPMhits->setVerticalHeaderItem(0, new QStandardItem("Sum(t)"));
    else modelPMhits->setVerticalHeaderItem(0, new QStandardItem(""));
    for (int i=0; i<rows;++i)
      {
        tmpStr.setNum(i);
        modelPMhits->setVerticalHeaderItem(i+1, new QStandardItem("Time bin "+tmpStr));
      }

    //filling with data
    for (int iColumn=0; iColumn<columns; iColumn++)
      {        
        if (!fHaveData) tmpStr = "";
        else
          {
            if (iColumn == 0)
              {
                //top is sum
                double sum = 0;               
                for (int ipm=0; ipm<MW->PMs->count(); ipm++) sum += EventsDataHub->Events[CurrentEvent][ipm];

                tmpStr.setNum(sum, 'g', 6);
              }
            else
              {
                //if (Events->isEmpty()) tmpStr = "-";
                tmpStr.setNum( EventsDataHub->Events[CurrentEvent][iColumn-1], 'g', 6);
              }
          }
        QStandardItem* item = new QStandardItem(tmpStr);
        item->setTextAlignment(Qt::AlignCenter);
        modelPMhits->setItem(0,iColumn,item);

        //the rest is time-resolved info
        for (int j=0; j<rows; j++)
          {
            if (!fHaveData) tmpStr = "";
            else
              {
                if (iColumn == 0)
                  {
                    double sum=0;                    
                    for (int ipm=0; ipm<MW->PMs->count(); ipm++) sum += EventsDataHub->TimedEvents[CurrentEvent][j][ipm];
                    tmpStr.setNum(sum, 'g', 6);
                  }
                else tmpStr.setNum( EventsDataHub->TimedEvents[CurrentEvent][j][iColumn-1], 'g', 6);
              }

           item = new QStandardItem(tmpStr);
           item->setTextAlignment(Qt::AlignCenter);
           modelPMhits->setItem(j+1,iColumn,item);
          }
      }
    ui->tvPMhits->setModel(modelPMhits);
    ui->tvPMhits->resizeColumnsToContents();    
    updateSignalTableWidth();
}

void OutputWindow::clearGrItems()
{
  //qDebug() << "Item cleanup. This scene:"<<scene;
  for (int i=0; i<grItems.size(); i++)
    {
      if (grItems[i]->scene() != scene)
        qDebug() << i << grItems[i] << grItems[i]->scene();
      //QGraphicsTextItem *gr = dynamic_cast<QGraphicsTextItem*>(grItems[i]);
      //if (gr) qDebug() << i << gr->document()->toHtml();
      scene->removeItem(grItems[i]);
      delete grItems[i];
    }
  grItems.clear();
}

void OutputWindow::on_sbPMnumberToShowTime_valueChanged(int arg1)
{
    if (arg1 > MW->PMs->count()-1) ui->sbPMnumberToShowTime->setValue(0);
}

void OutputWindow::OutTextClear()
{
    ui->teOut->clear();
}

void OutputWindow::OutText(QString str)
{
   ui->teOut->append(str);
}

void OutputWindow::SetTab(int index)
{
    if (index <0) return;
    if (index > ui->tabwinDiagnose->count()-1) return;
    ui->tabwinDiagnose->setCurrentIndex(index);
}

void OutputWindow::on_pbSiPMpixels_clicked()
{
    if (SiPMpixels.isEmpty())
      {
        message("Pixels are shown only for photon source simulation with one event!");
        return;
      }

    int ipm = ui->sbPMnumberToShowTime->value();
    if ( ipm>MW->PMs->count()-1 || !MW->PMs->isSiPM(ipm) )
      {
        MW->GraphWindow->close();
        return;
      }

    int iTime = 0;
    if (EventsDataHub->isTimed())
    {
        iTime = ui->sbTimeBin->value();
        if (iTime > EventsDataHub->getTimeBins()-1)
        {
            ui->sbTimeBin->setValue(0);
            MW->GraphWindow->close();
            return;
        }
    }

    int binsX = MW->PMs->PixelsX(ipm);
    int binsY = MW->PMs->PixelsY(ipm);
//    qDebug()<<binsX<<binsY;

    auto hist = new TH2C("histOutW","x vs y",binsX,0,binsX, binsY, -binsY,0);

    const PMtypeClass *typ = MW->PMs->getTypeForPM(ipm);
    for (int iX=0; iX<binsX; iX++)
     for (int iY=0; iY<binsY; iY++)
      {         
        if (SiPMpixels[ipm].testBit((iTime*typ->PixelsY + iY)*typ->PixelsX + iX)) hist->Fill(iX, -iY);
      }

    MW->GraphWindow->Draw(hist, "col");
}

void OutputWindow::on_sbTimeBin_valueChanged(int arg1)
{
   if (SiPMpixels.isEmpty()) return;
   if (EventsDataHub->TimedEvents.isEmpty()) return;
   if (arg1 > EventsDataHub->TimedEvents[0].size()-1) ui->sbTimeBin->setValue(0);

   OutputWindow::on_pbSiPMpixels_clicked();
}

void OutputWindow::showParticleHistString(int iRec, int level)
{
    const EventHistoryStructure* h = EventsDataHub->EventHistory.at(iRec);

    QString s = QString("+").repeated(level);

    s += QString::number(iRec);
    s += "> " + MW->Detector->MpCollection->getParticleName(h->ParticleId);// + " (id: "+QString::number(h->ParticleId)+")";

    for (int m=0; m < h->Deposition.size(); m++)
    {
        if (m != 0) s += " ->";
        int MatId = h->Deposition[m].MaterialId;
        s += " " + (*MW->MpCollection)[MatId]->name +"_";
        s += QString::number(h->Deposition[m].Distance, 'g', MW->GlobSet->TextLogPrecision)+"_mm";
        double depo = h->Deposition[m].DepositedEnergy;
        if (depo>0)
        {
            s += "_";
            s += "<b>";
            s += QString::number(depo, 'g', MW->GlobSet->TextLogPrecision);
            s += "</b>";
            s += "_keV";
            TotalEnergyDeposited += depo;
        }
    }
    s += " ";
    switch (h->Termination)
      {
      case EventHistoryStructure::Escaped:                  s += "escaped"; break;
      case EventHistoryStructure::AllEnergyDisspated:       s += "stopped"; break;
      case EventHistoryStructure::Photoelectric:            s += "photoelectric"; break;
      case EventHistoryStructure::ComptonScattering:        s += "compton"; break;
      case EventHistoryStructure::NeutronAbsorption:                  s += "capture"; break;
      case EventHistoryStructure::EllasticScattering:       s += "elastic"; break;
      case EventHistoryStructure::CreatedOutside:           s += "created outside the defined geometry"; break;
      case EventHistoryStructure::FoundUntrackableMaterial: s += "found untrackable material"; break;
      case EventHistoryStructure::PairProduction:           s += "pair production"; break;
      case EventHistoryStructure::StoppedOnMonitor:         s += "stopped on monitor"; break;
      default:                                              s += "UNKNOWN TYPE"; break;
      }
    ui->pteOut->appendHtml(s);
}

void OutputWindow::addParticleHistoryLogLine(int iRec, int level)
{
    showParticleHistString(iRec, level);
    for (int i=0; i<secs.at(iRec).size(); i++)
        addParticleHistoryLogLine(secs.at(iRec).at(i), level+1);
}

void OutputWindow::ShowEventHistoryLog()
{  
  if (EventsDataHub->EventHistory.isEmpty())
    {
     message("Particle log is empty!\n"
             "Simulation_options/Accelerators/Do_logs_and_statistics has to be activated before simulation!", this);
     return;
    }

  int size = EventsDataHub->EventHistory.size();
  //preparing list of secondaries
  secs.resize(size);
  for (QVector<int>& v : secs) v.clear();
  for (int i=0; i<size; i++)
  {
      const int& secOf = EventsDataHub->EventHistory.at(i)->SecondaryOf;
      if (secOf>-1) secs[secOf] << i;
  }

  QString s = "\n=====================\n"
              "Log of particle interactions\n"
              "---------------------";
  //OutText(s);
  ui->pteOut->appendPlainText(s);

  TotalEnergyDeposited=0;
  for (int i=0; i<size; i++)
    {
      if (EventsDataHub->EventHistory.at(i)->SecondaryOf > -1) continue; //secondary are already shown
      addParticleHistoryLogLine(i, 0);
    }
  s  = "---------------------\n";
  s += "Total energy deposited: " + QString::number(TotalEnergyDeposited, 'g', MW->GlobSet->TextLogPrecision) + " keV\n";
  s += "=====================\n";
  //SetTab(0);
  ui->pteOut->appendPlainText(s);
}

void OutputWindow::updateSignalTableWidth()
{
  ui->layPMhitsTable->invalidate();
  qApp->processEvents();
  int w = 0;
  for (int i=0; i<ui->tvPMhits->horizontalHeader()->count(); i++)
    w += ui->tvPMhits->columnWidth(i);

  int max = ui->labPMsig->width();
//  qDebug() << "counting:"<<w<<"max:"<<max;
  if (w > max) w = max;

  ui->tvPMhits->setMaximumWidth(w+10);
  ui->tvPMhits->setMinimumWidth(w+10);
  ui->layPMhitsTable->invalidate();
}

void OutputWindow::resizeEvent(QResizeEvent *)
{
  if (!this->isVisible()) return;

  int GVsize = ui->tabwinDiagnose->height()-28;//325;
  gvOut->resize(GVsize, GVsize);
  ui->gvScale->move(GVsize-325+330, 20);
  ui->fGVScale->move(GVsize-325+330, 33);
  ui->fGVpanel->move(GVsize-325+384, 0);
  OutputWindow::ResetViewport();

  updateSignalTableWidth();
}

void OutputWindow::ResetViewport()
{
  if (MW->PMs->count() == 0) return;

  //calculating viewing area
  double Xmin =  1e10;
  double Xmax = -1e10;
  double Ymin =  1e10;
  double Ymax = -1e10;
  for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
      double x = MW->PMs->at(ipm).x;
      double y = MW->PMs->at(ipm).y;
      int type = MW->PMs->at(ipm).type;
      double size = MW->PMs->getType(type)->SizeX;

      if (x-size < Xmin) Xmin = x-size;
      if (x+size > Xmax) Xmax = x+size;
      if (y-size < Ymin) Ymin = y-size;
      if (y+size > Ymax) Ymax = y+size;
    }

  double Xdelta = Xmax-Xmin;
  double Ydelta = Ymax-Ymin;

  scene->setSceneRect((Xmin - 0.5*Xdelta)*GVscale, (Ymin - 0.5*Ydelta)*GVscale, (Xmax-Xmin + 1.0*Xdelta)*GVscale, (Ymax-Ymin + 1.0*Ydelta)*GVscale);
  gvOut->fitInView( (Xmin - 0.01*Xdelta)*GVscale, (Ymin - 0.01*Ydelta)*GVscale, (Xmax-Xmin + 0.02*Xdelta)*GVscale, (Ymax-Ymin + 0.02*Ydelta)*GVscale, Qt::KeepAspectRatio);
}

void OutputWindow::InitWindow()
{
  bool shown = this->isVisible();
  this->showNormal();
  SetTab(2);
  RefreshData();
  OutputWindow::resizeEvent(0);
  //SetTab(0);
  if (!shown) this->hide();
}

void OutputWindow::on_pbRefreshViz_clicked()
{
  OutputWindow::RefreshData();
}

void OutputWindow::RefreshData()
{
  bool fHaveData = !EventsDataHub->isEmpty();

  if (!fHaveData) ui->labTotal->setText("-");
  else ui->labTotal->setText(QString::number(EventsDataHub->Events.size()));

  //check current event
  int CurrentEvent = ui->sbEvent->value();
  int CurrentGroup = MW->Detector->PMgroups->getCurrentGroup();
  if (CurrentEvent > EventsDataHub->Events.size()-1)
    {
      CurrentEvent = EventsDataHub->Events.size()-1;
      if (CurrentEvent < 0) CurrentEvent = 0;
      ui->sbEvent->setValue(CurrentEvent);
    }
  if (ui->sbPMnumberToShowTime->value() > MW->PMs->count()-1)
    ui->sbPMnumberToShowTime->setValue(0);

  bool fHaveSiPMpixData = !SiPMpixels.isEmpty();
  bool fTimeResolved = !EventsDataHub->TimedEvents.isEmpty();

  ui->pbSiPMpixels->setEnabled(fHaveSiPMpixData);
  ui->sbTimeBin->setEnabled(fTimeResolved);
  ui->pbShowPMtime->setEnabled(fTimeResolved);

  //dynamic passives for indication
  DynamicPassivesHandler *Passives = new DynamicPassivesHandler(MW->Detector->PMs, MW->Detector->PMgroups, EventsDataHub);
  if (EventsDataHub->isEmpty() || CurrentGroup>EventsDataHub->RecSettings.size()-1)
    Passives->init(0, CurrentGroup);
  else
    {
      Passives->init(&EventsDataHub->RecSettings[CurrentGroup], CurrentGroup); //just to copy static passives
      if (EventsDataHub->fReconstructionDataReady)
        if (EventsDataHub->RecSettings.at(CurrentGroup).fUseDynamicPassives)
          Passives->calculateDynamicPassives(CurrentEvent, EventsDataHub->ReconstructionData.at(CurrentGroup).at(CurrentEvent));
    }

  OutputWindow::RefreshPMhitsTable();
  //qDebug()<<"table updated";

  //updating viz
  scene->clear();
  double MaxSignal = 0.0;
  if (!fHaveData) MaxSignal = 1.0;
  else
    {
      for (int i=0; i<MW->PMs->count(); i++)
        if (Passives->isActive(i))
          if ( EventsDataHub->Events[CurrentEvent][i] > MaxSignal) MaxSignal = EventsDataHub->Events[CurrentEvent][i];
    }
  if (MaxSignal<=1.0e-25) MaxSignal = 1.0;
  //qDebug()<<"MaxSignal="<<MaxSignal<<"  selector="<<selector;



  updateSignalLabels(MaxSignal);
  addPMitems( (fHaveData ? &EventsDataHub->Events.at(CurrentEvent) : 0), MaxSignal, Passives); //add icons with PMs to the scene
  if (ui->cbShowPMsignals->isChecked())
    addTextitems( (fHaveData ? &EventsDataHub->Events.at(CurrentEvent) : 0), MaxSignal, Passives); //add icons with signal text to the scene
  updateSignalScale();

  //Monitors
  updateMonitors();

  delete Passives;
}

void OutputWindow::updateMonitors()
{
    int numMonitors = MW->Detector->Sandwich->MonitorsRecords.size();
    ui->frMonitors->setVisible(numMonitors != 0);
    ui->labNoMonitors->setVisible(numMonitors == 0);
    if (numMonitors>0)
    {
        int oldNum = ui->cobMonitor->currentIndex();
        ui->cobMonitor->clear();
        for (int i=0; i<numMonitors; i++)
        {
            const AGeoObject* obj = MW->Detector->Sandwich->MonitorsRecords.at(i);
            ui->cobMonitor->addItem(obj->Name);
        }
        if (oldNum>-1 && oldNum<numMonitors) ui->cobMonitor->setCurrentIndex(oldNum);

        int imon = ui->cobMonitor->currentIndex();
        const AGeoObject* monObj = MW->Detector->Sandwich->MonitorsRecords.at(imon);
        const ATypeMonitorObject* mon = dynamic_cast<const ATypeMonitorObject*>(monObj->ObjectType);
        if (mon)
        {
            ui->frMonitors->setEnabled(true);
            int numDet = 0;
            if (imon < EventsDataHub->SimStat->Monitors.size())
                if (EventsDataHub->SimStat->Monitors.at(imon)->getXY())
                    numDet = EventsDataHub->SimStat->Monitors.at(imon)->getXY()->GetEntries();
            ui->leDetections->setText( QString::number(numDet) );

            bool bPhotonMode = mon->config.PhotonOrParticle == 0;
            ui->pbMonitorShowWave->setVisible(bPhotonMode);
            ui->pbMonitorShowTime->setVisible(bPhotonMode);
            ui->pbMonitorShowEnergy->setVisible(!bPhotonMode);
        }
        else
        {
            ui->frMonitors->setEnabled(false);
            qWarning() << "Something is wrong: this is not a monitor object!";
        }
    }
}

void OutputWindow::addPMitems(const QVector<float> *vector, double MaxSignal, DynamicPassivesHandler *Passives)
{
  for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
      //pen
      QPen pen(Qt::black);
      int size = 6.0 * MW->PMs->SizeX(ipm) / 30.0;
      pen.setWidth(size);

      //brush
      QBrush brush(Qt::white);

      double sig = ( vector ? vector->at(ipm) : 0 );

      if (sig > 0)
        {
          QColor color = QColor(0,0,0);
          int level = 255.0*sig/MaxSignal;
          if (level>255) level = 255;
//            qDebug()<<"Level = "<<level;

          if (level <64) color.setRgb(0, level*4,255);
          else if (level < 128) color.setRgb(0,255,255 - (level-64)*4);
          else if (level < 192) color.setRgb((level-128)*4,255,0);
          else color.setRgb(255,255 -(level-192)*4,0);

          brush.setColor(color);
        }
      else
        {
          if (sig == 0) brush.setColor(Qt::white);
          else brush.setColor(Qt::black);
        }

      const APm &PM = MW->PMs->at(ipm);
      if (Passives)
          if (Passives->isPassive(ipm)) brush.setColor(Qt::black);

      QGraphicsItem* tmp;
      if (MW->PMs->getType(PM.type)->Shape == 0)
        {
          double sizex = MW->PMs->getType(PM.type)->SizeX*GVscale;
          double sizey = MW->PMs->getType(PM.type)->SizeY*GVscale;
          tmp = scene->addRect(-0.5*sizex, -0.5*sizey, sizex, sizey, pen, brush);
        }
      else if (MW->PMs->getType(PM.type)->Shape == 1)
        {
          double diameter = MW->PMs->getType(PM.type)->SizeX*GVscale;
          tmp = scene->addEllipse( -0.5*diameter, -0.5*diameter, diameter, diameter, pen, brush);
        }
      else
        {
          double radius = 0.5*MW->PMs->getType(PM.type)->SizeX*GVscale;
          QPolygon polygon;
          for (int j=0; j<7; j++)
            {
              double angle = 3.1415926535/3.0 * j + 3.1415926535/2.0;
              double x = radius*cos(angle);
              double y = radius*sin(angle);
              polygon << QPoint(x, y);
            }
          tmp = scene->addPolygon(polygon, pen, brush);
        }

      if (ui->cbViewFromBelow->isChecked()) tmp->setZValue(-PM.z);
      else tmp->setZValue(PM.z);

      tmp->setRotation(-PM.psi);
      tmp->setTransform(QTransform().translate(PM.x*GVscale, -PM.y*GVscale)); //minus!!!!
    }
}

void OutputWindow::addTextitems(const QVector<float> *vector, double MaxSignal, DynamicPassivesHandler *Passives)
{
  for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
      const APm &PM = MW->PMs->at(ipm);
      double size = 0.5*MW->PMs->getType( PM.type )->SizeX;
      //io->setTextWidth(40);

      double sig = ( vector ? vector->at(ipm) : 0 );
      //qDebug()<<"sig="<<sig;
      QString text = QString::number(sig, 'g', 6);
      //text = "<CENTER>" + text + "</CENTER>";

      //color correction for dark blue
      QGraphicsSimpleTextItem *io = scene->addSimpleText(text);

      if (
              ( sig != 0 && sig < 0.15*MaxSignal )
           ||
              ( Passives && Passives->isPassive(ipm) )
         )
        io->setBrush(Qt::white);

      int wid = (io->boundingRect().width()-6)/6;
      double x = ( PM.x -wid*size*0.125 -0.1*size )*GVscale;
      double y = (-PM.y - 0.3*size  ) *GVscale; //minus y to invert the scale!!!
      io->setPos(x, y);
      io->setScale(0.4*size);

      if (ui->cbViewFromBelow->isChecked()) io->setZValue(-PM.z+0.001);
      else io->setZValue(PM.z+0.001);
    }
}

void OutputWindow::updateSignalScale()
{
  scaleScene->clear();
  for (int i=0; i<16; i++)
    {
       int level = 8+i*16;
       QBrush brush(Qt::white);
       QColor color = QColor(0,0,0);

       if (level <64) color.setRgb(0, level*4,255);
       else if (level < 128) color.setRgb(0,255,255 - (level-64)*4);
       else if (level < 192) color.setRgb((level-128)*4,255,0);
       else color.setRgb(255,255 -(level-192)*4,0);

       brush.setColor(color);

       QPen pen(Qt::white);
       scaleScene->addRect( 0, 16.0*(-i+7.5), 35, 16, pen, brush);
    }
}

void OutputWindow::updateSignalLabels(double MaxSignal)
{
  QString str;
  str.setNum(MaxSignal, 'g', 4);
  ui->labHitsMax->setText(str);

  double lowmid = 0.25*MaxSignal;
  if (lowmid>0)
    {
      str.setNum(lowmid, 'g', 4);
      ui->labLowMid->setText(str);
    }
  else ui->labLowMid->setText("");

  double mid = 0.5*MaxSignal;
  if (mid > 0 && mid !=lowmid)
    {
      str.setNum(mid, 'g', 4);
      ui->labMid->setText(str);
    }
  else ui->labMid->setText("");

  double highmid = 0.75*MaxSignal;
  if (highmid > 0 && highmid !=mid)
    {
      str.setNum(highmid, 'g', 4);
      ui->labHighMid->setText(str);
    }
  else ui->labHighMid->setText("");
}

void OutputWindow::on_tabwinDiagnose_currentChanged(int index)
{
  if (index == 2) OutputWindow::on_pbRefreshViz_clicked();

  ui->frEventNumber->setVisible( index != 3);
  ui->pbClearText->setVisible( index==0 || index==3 );
  gvOut->update();
}

bool OutputWindow::event(QEvent *event)
{
  if (!MW->WindowNavigator) return QMainWindow::event(event);

  if (event->type() == QEvent::Hide) MW->WindowNavigator->HideWindowTriggered("out");
  if (event->type() == QEvent::Show) MW->WindowNavigator->ShowWindowTriggered("out");

  return QMainWindow::event(event);
}

void OutputWindow::on_pbWaveSpectrum_clicked()
{
  ASimulationStatistics* d = EventsDataHub->SimStat;
  if (d->isEmpty())
  {
      message("There are no data!", this);
      return;
  }

  TH1D* spec = d->getWaveSpectrum();
  if (!spec || spec->GetEntries() == 0 || spec->Integral()==0)
    {
      message("Wavelength data are empty!\n\n"
              "Or make sure the following is set before simulation:\n"
              "* Simulation_options/Wave/Wavelength-resolved is checked", this);
      return;
    }

  //converting to wavelength
  int nBins = spec->GetNbinsX();
  //qDebug() << nBins << MW->WaveNodes;
  if (MW->EventsDataHub->LastSimSet.fWaveResolved)
    {
      auto WavelengthSpectrum = new TH1D("","Wavelength of detected photons", nBins-1, MW->WaveFrom, MW->WaveTo);
      for (int i=1; i<nBins+1; i++) //0 - underflow, n+1 - overflow
          WavelengthSpectrum->SetBinContent(i, spec->GetBinContent(i));
      WavelengthSpectrum->GetXaxis()->SetTitle("Wavelength, nm");
      MW->GraphWindow->Draw(WavelengthSpectrum);
    }
  else
    {
      spec->GetXaxis()->SetTitle("Wave index");
      spec->SetTitle("Wave index of detected photons");
      MW->GraphWindow->Draw(spec, "", true, false);
    }
}

void OutputWindow::on_pbTimeSpectrum_clicked()
{
  ASimulationStatistics* d = EventsDataHub->SimStat;
  if (d->isEmpty())
  {
      message("There are no data!", this);
      return;
  }

  TH1D* spec = d->getTimeSpectrum();
  if (!spec || spec->GetEntries() == 0 || spec->Integral()==0)
    {
      message("Time data are empty!\n"
              "Make sure the following is set before simulation:\n"
              "Simulation_options/Accelerators/Do_logs_and_statistics is checked\n", this);
      return;
    }

  spec->GetXaxis()->SetTitle("Time, ns");
  spec->SetTitle("Time of photon detection");
  MW->GraphWindow->Draw(spec, "", true, false);
}

void OutputWindow::on_pbAngleSpectrum_clicked()
{
  ASimulationStatistics* d = EventsDataHub->SimStat;
  if (d->isEmpty())
  {
      message("There are no data!", this);
      return;
  }

  TH1D* spec = d->getAngularDistr();
  if (!spec || spec->GetEntries() == 0 || spec->Integral()==0)
    {
      message("Angle of incidence data are empty!\n"
              "Make sure the following settings were configured before simulation:\n"
              "* Simulation_options/Accelerators/Do_logs_and_statistics is checked\n"
              "* Simulation_options/Angule/Take_into_account_PM_angular_response is checked", this);
      return;
    }

  spec->GetXaxis()->SetTitle("Angle of incidence, degrees");
  spec->SetTitle("Incidence angle of detected photons");
  MW->GraphWindow->Draw(spec, "", true, false);
}

void OutputWindow::on_pbNumTransitionsSpectrum_clicked()
{  
  ASimulationStatistics* d = EventsDataHub->SimStat;
  if (d->isEmpty())
  {
      message("There are no data!", this);
      return;
  }

   TH1D* spec = d->getTransitionSpectrum();

   if (!spec || spec->GetEntries() == 0 || spec->Integral()==0)
     {
       message("Transition data are empty!\n"
               "Simulation_options/Accelerators/Do_logs_and_statistics has to be activated before simulation!\n", this);
       return;
     }

   spec->GetXaxis()->SetTitle("Number of cycles in tracking");
   spec->SetTitle("Number of tracking cycles for detected photons");
   MW->GraphWindow->Draw(spec, "", true, false);
}

void OutputWindow::on_sbEvent_valueChanged(int arg1)
{
  if (arg1 == 0 && EventsDataHub->Events.isEmpty()) return;
  if (EventsDataHub->Events.isEmpty()) //protection
    {      
      ui->sbEvent->setValue(0);
      return;
    }

  if (arg1 > EventsDataHub->Events.size()-1)
    {
      ui->sbEvent->setValue(0);
      arg1 = 0;
      return; //already triggered "on change" = this procedure
    } 

  if (ui->tabwinDiagnose->currentIndex() == 0 && !bForbidUpdate) ShowOneEventLog(arg1);
  else on_pbRefreshViz_clicked();
}

void OutputWindow::on_pbResetViewport_clicked()
{
    OutputWindow::RefreshData();
    OutputWindow::ResetViewport();
}

void OutputWindow::on_pbClearText_clicked()
{
    if (ui->tabwinDiagnose->currentIndex() == 3) ui->pteOut->clear();
    else ui->teOut->clear();
}

void OutputWindow::ShowOneEventLog(int iev)
{
  if (!isVisible()) show();
  raise();

  int precision = MW->GlobSet->TextLogPrecision;
  QString str = "Event# " + QString::number(iev);

  if (!EventsDataHub->isScanEmpty())
      if ( !EventsDataHub->Scan[iev]->GoodEvent )
        str += "       'Bad' event: " + MW->NoiseTypeDescriptions[EventsDataHub->Scan[iev]->EventType].replace("\n", " ");
  OutText(str);

  QString str2;
  if (!EventsDataHub->isScanEmpty())
    {
      int iPoints = EventsDataHub->Scan[iev]->Points.size();
      for (int iP=0; iP<iPoints; iP++)
        {
          str2.setNum(EventsDataHub->Scan[iev]->Points[iP].r[0], 'g', precision);
          str = "->   ";
          str +="<font color=\"blue\">True position:</font>  X = "+str2+" mm  Y = ";
          str2.setNum(EventsDataHub->Scan[iev]->Points[iP].r[1], 'g', precision);
          str +=str2+" mm   Z = ";
          str2.setNum(EventsDataHub->Scan[iev]->Points[iP].r[2], 'g', precision);
          str +=str2+" mm";
          str2.setNum(EventsDataHub->Scan[iev]->Points[iP].energy);
              str += "   photons/energy = "+str2;

          OutText(str);
        }
      if (iPoints==0) OutText("No energy was deposited.");
      OutText("   -----");
    }

  int CurrentGroup = MW->Detector->PMgroups->getCurrentGroup();
  if (EventsDataHub->isReconstructionReady(CurrentGroup))
  {
      int numGroups = MW->Detector->PMgroups->countPMgroups();
      if (numGroups>1) OutText("Current sensor group: "+QString::number(CurrentGroup));
      for (int iGroup=0; iGroup<numGroups; iGroup++)
      {
         if (numGroups>1) OutText("--->>>Group "+QString::number(iGroup));
         AReconRecord* result = EventsDataHub->ReconstructionData[iGroup][iev];
         if (!result->ReconstructionOK)
           {
             OutText("->   Position reconstruction failed!");
             OutText("========");
             OutText("");
             return;
           }

         int Points = result->Points.size();
         for (int iP=0; iP<Points; iP++)
           {
             str2.setNum(result->Points[iP].r[0], 'g', precision);
             str = "->   ";
             str += "<font color=\"red\">Reconstruction:</font>  X = "+str2+" mm  Y = ";
             str2.setNum(result->Points[iP].r[1], 'g', precision);
             str +=str2+" mm   Z = ";
             str2.setNum(result->Points[iP].r[2], 'g', precision);
             str +=str2+" mm";
             str2.setNum(result->Points[iP].energy);
             str += "   Energy = "+str2;
             OutText(str);
           }

         str2.setNum(result->chi2);
         str = "-> Chi2 = "+str2;
         OutText(str);

         QString state = (result->GoodEvent) ? "<font color=\"green\">Pass</font>" : "<font color=\"red\">Fail</font>";
         OutText("-> Filters status: "+state);
      }
  }
  OutText("========");
  OutText("");
  SetTab(0);
}

void OutputWindow::on_pbShowSumSignals_clicked()
{
    if (EventsDataHub->isEmpty()) return;

    bool fGains = ui->cbTakeGainIntoAccount->isChecked();
    if (fGains && MW->Detector->PMgroups->countPMgroups()>1)
    {
        message("Gain correction not supported if there are several sensor groups", this); /// *** !!! can enable if groups do not intersect and there are no unassigned PMs
        fGains = false;
        ui->cbTakeGainIntoAccount->setChecked(false);
    }
    bool fGood = ui->cbOnlyGood->isChecked();

    auto hist1D = new TH1F("SumSig","Distribution of sum signal", MW->GlobSet->BinsX, 0,0);
  #if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
    hist1D->SetBit(TH1::kCanRebin);
  #endif

    for (int iEvent=0; iEvent<EventsDataHub->Events.size(); iEvent++)
      {
        if (fGood && !EventsDataHub->ReconstructionData.at(0).at(iEvent)->GoodEvent) continue;

        float sum = 0;
        for (int ipm=0; ipm<MW->Detector->PMs->count(); ipm++)
          {
            if (MW->Detector->PMgroups->isStaticPassive(ipm)) continue;

            float sig = EventsDataHub->Events[iEvent].at(ipm);
            if (fGains)
                sig /= MW->Detector->PMgroups->Groups.at(0)->PMS.at(ipm).gain;
            sum += sig;
          }
        hist1D->Fill(sum);
      }

    MW->GraphWindow->Draw(hist1D);
}

void OutputWindow::on_pbShowSignals_clicked()
{
    if (EventsDataHub->isEmpty()) return;

    bool fGains = ui->cbTakeGainIntoAccount->isChecked();
    if (fGains && MW->Detector->PMgroups->countPMgroups()>1)
    {
        message("Gain correction not supported if there are several sensor groups", this); /// *** !!! can enable if groups do not intersect and there are no unassigned PMs
        fGains = false;
        ui->cbTakeGainIntoAccount->setChecked(false);
    }
    bool fGood = ui->cbOnlyGood->isChecked();
    int ipm = ui->sbPMnumberToShowTime->value();
    if (ipm > EventsDataHub->getNumPMs()-1)
    {
        message("Bad PM number", this);
        return;
    }

    TString title = "Distribution of signal for PM# ";
    title += ipm;
    auto hist1D = new TH1F("Sig", title, MW->GlobSet->BinsX, 0,0);
  #if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
    hist1D->SetBit(TH1::kCanRebin);
  #endif

    for (int iEvent=0; iEvent<EventsDataHub->Events.size(); iEvent++)
      {
        if (fGood && !EventsDataHub->ReconstructionData.at(0).at(iEvent)->GoodEvent) continue;

        if (MW->Detector->PMgroups->isStaticPassive(ipm)) continue;

        float sig = EventsDataHub->Events[iEvent].at(ipm);
        if (fGains)
            sig /= MW->Detector->PMgroups->Groups.at(0)->PMS.at(ipm).gain;
        hist1D->Fill(sig);
      }

    MW->GraphWindow->Draw(hist1D);
}

void OutputWindow::on_pbAverageSignals_clicked()
{
  if (EventsDataHub->isEmpty()) return;

  bool fGains = ui->cbTakeGainIntoAccount->isChecked();
  if (fGains && MW->Detector->PMgroups->countPMgroups()>1)
  {
      message("Gain correction not supported if there are several sensor groups", this); /// *** !!! can enable if groups do not intersect and there are no unassigned PMs
      fGains = false;
      ui->cbTakeGainIntoAccount->setChecked(false);
  }
  bool fGood = ui->cbOnlyGood->isChecked();

  auto hist1D = new TH1F("MaxSigDist","Distribution of max signal", MW->GlobSet->BinsX, 0,0);
#if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
  hist1D->SetBit(TH1::kCanRebin);
#endif

  for (int iEvent=0; iEvent<EventsDataHub->Events.size(); iEvent++)
    {
      if (fGood && !EventsDataHub->ReconstructionData.at(0).at(iEvent)->GoodEvent) continue;

      float max = -1e10;
      for (int ipm=0; ipm<MW->Detector->PMs->count(); ipm++)
        {
          if (MW->Detector->PMgroups->isStaticPassive(ipm)) continue;

          float sig = EventsDataHub->Events[iEvent].at(ipm);          
          if (fGains)
              sig /= MW->Detector->PMgroups->Groups.at(0)->PMS.at(ipm).gain;
          if (sig>max) max=sig;
        }
      hist1D->Fill(max);
    }

  MW->GraphWindow->Draw(hist1D);
}

void OutputWindow::ShowGeneratedPhotonsLog()
{
  if (EventsDataHub->GeneratedPhotonsHistory.isEmpty())
  {
      message("Photon history log is empty!\n"
              "Simulation_options/Accelerators/Do_logs_and_statistics has to be activated before simulation!\n"
              "Also, photon sources simulations do not generate this log!", this);
      return;
  }

  QString s = "\n=====================\n"
              "Log of genereated photons\n"
              "---------------------\n";
  for (int i=0; i<EventsDataHub->GeneratedPhotonsHistory.size(); i++)
        {
          QString out, str;

          str.setNum(EventsDataHub->GeneratedPhotonsHistory[i].event);
          out = str + "/";
          str.setNum(EventsDataHub->GeneratedPhotonsHistory[i].index);
          out += str + "> ";

          //out += MW->Detector->ParticleCollection[EventsDataHub->GeneratedPhotonsHistory[i].ParticleId]->ParticleName;
          out += MW->Detector->MpCollection->getParticleName(EventsDataHub->GeneratedPhotonsHistory[i].ParticleId);
          out += " in " + (*MW->MpCollection)[EventsDataHub->GeneratedPhotonsHistory[i].MaterialId]->name;

          str.setNum(EventsDataHub->GeneratedPhotonsHistory[i].Energy);
          out += "  deposited: " + str + " keV";

          out += "  Primary photons: ";
          str.setNum(EventsDataHub->GeneratedPhotonsHistory[i].PrimaryPhotons);
          out += str;

          out += "  Secondary photons: ";
          str.setNum(EventsDataHub->GeneratedPhotonsHistory[i].SecondaryPhotons);
          out += str;

          s += out + "\n";
        }
  s += "=====================\n";

  //SetTab(0);
  //OutText(s);
  ui->pteOut->appendPlainText(s);
}

void OutputWindow::on_pbShowParticldeLog_clicked()
{
    ShowEventHistoryLog();
}

void OutputWindow::on_pbShowPhotonLog_clicked()
{
    ShowGeneratedPhotonsLog();
}

void OutputWindow::on_pbShowPhotonProcessesLog_clicked()
{
    ShowPhotonProcessesLog();
}

void OutputWindow::ShowPhotonProcessesLog()
{
  ASimulationStatistics* d = EventsDataHub->SimStat;
  if (d->isEmpty())
  {
      message("There are no data!", this);
      return;
  }

  QString s = "\n=====================\n";
  s += "Optical processes:\n";
  s += "---------------------\n";
  s += "Fresnel transmission: " + QString::number(d->FresnelTransmitted) + "\n"+
       "Fresnel reflection: " + QString::number(d->FresnelReflected) + "\n"+
       "Bulk absorption: " + QString::number(d->BulkAbsorption) + "\n"+
       "Rayleigh: " + QString::number(d->Rayleigh) + "\n"+
       "Reemission: " + QString::number(d->Reemission) + "\n";
  s += "----\n";
  s += "Overrides, loss: " + QString::number(d->OverrideLoss) + "\n";
  s += "Overrides, back: " + QString::number(d->OverrideBack) + "\n";
  s += "Overrides, forward: " + QString::number(d->OverrideForward) + "\n";
  if (d->OverrideSimplisticAbsorption>0 || d->OverrideSimplisticReflection>0 || d->OverrideSimplisticScatter>0)
  {
       s += "----\n";
       s += "Override simplistic, absorption: " + QString::number(d->OverrideSimplisticAbsorption) + "\n";
       s += "Override simplistic, specular: " + QString::number(d->OverrideSimplisticReflection) + "\n";
       s += "Override simplistic, lambertian: " + QString::number(d->OverrideSimplisticScatter) + "\n";
  }
  if (d->OverrideFSNPabs>0 || d->OverrideFSNlambert>0 || d->OverrideFSNPspecular>0)
  {
       s += "----\n";
       s += "Override FSNP, absorption: " + QString::number(d->OverrideFSNPabs) + "\n";
       s += "Override FSNP, specular: " + QString::number(d->OverrideFSNPspecular) + "\n";
       s += "Override FSNP, lambertian: " + QString::number(d->OverrideFSNlambert) + "\n";
  }
  if (d->OverrideMetalAbs>0 || d->OverrideMetalReflection>0)
  {
       s += "----\n";
       s += "Override OnMetal, absorption: " + QString::number(d->OverrideMetalAbs) + "\n";
       s += "Override OnMetal, specular: " + QString::number(d->OverrideMetalReflection) + "\n";
  }
  if (d->OverrideClaudioAbs>0 || d->OverrideClaudioLamb>0 || d->OverrideClaudioSpec>0)
  {
       s += "----\n";
       s += "Override Clauido's model', absorption: " + QString::number(d->OverrideClaudioAbs) + "\n";
       s += "Override Claudio's model', specular: " + QString::number(d->OverrideClaudioSpec) + "\n";
       s += "Override Claudio's model', lambertian: " + QString::number(d->OverrideClaudioLamb) + "\n";
  }
  if (d->OverrideWLSabs>0 || d->OverrideWLSshift>0)
  {
       s += "----\n";
       s += "Override WLS, absorption: " + QString::number(d->OverrideWLSabs) + "\n";
       s += "Override WLS, reemission: " + QString::number(d->OverrideWLSshift) + "\n";
  }
  s += "=====================";
  //OutText(s);
  //SetTab(0);
  ui->pteOut->appendPlainText(s);
}

void OutputWindow::on_pbShowPhotonLossLog_clicked()
{
    ShowPhotonLossLog();
}

void OutputWindow::ShowPhotonLossLog()
{
  ASimulationStatistics* d = EventsDataHub->SimStat;
  if (d->isEmpty())
  {
      message("There are no data!", this);
      return;
  }

  int sum = d->Absorbed + d->OverrideLoss + d->HitPM + d->HitDummy + d->Escaped + d->LossOnGrid + d->TracingSkipped + d->MaxCyclesReached + d->GeneratedOutsideGeometry + d->KilledByMonitor;
  QString s = "\n=====================\n";
  s += "Photon tracing ended:\n";
  s +=        "---------------------\n";
  s += "Absorbed: "+QString::number(d->Absorbed)+"\n"+
      "Override loss: "+QString::number(d->OverrideLoss)+"\n"+
      "Hit PM: "+QString::number(d->HitPM)+"\n"+
      "Hit dummy PM: "+QString::number(d->HitDummy)+"\n"+
      "Escaped: "+QString::number(d->Escaped)+"\n"+
      "Loss on optical grids: "+QString::number(d->LossOnGrid)+"\n"+
      "Tracing skipped (QE accelerator): "+QString::number(d->TracingSkipped)+"\n"+
      "Max tracing cycles reached: "+QString::number(d->MaxCyclesReached)+"\n"+
      "Generated outside defined geometry: "+QString::number(d->GeneratedOutsideGeometry)+"\n"+
      "Stopped by a monitor: "+QString::number(d->KilledByMonitor)+"\n"+
      "---------------------\n"+
      "Total: "+QString::number(sum)+"\n"+
      "=====================";
  ui->pteOut->appendPlainText(s);
}

void OutputWindow::UpdateMaterials()
{
   int old = ui->cobShowMaterial->currentIndex();

   ui->cobShowMaterial->clear();
   for (int i=0; i<MW->MpCollection->countMaterials(); i++)
      ui->cobShowMaterial->addItem( (*MW->MpCollection)[i]->name );

   if (old < ui->cobShowMaterial->count()) ui->cobShowMaterial->setCurrentIndex(old);
}

void OutputWindow::UpdateParticles()
{
  int old = ui->cobShowParticle->currentIndex();

  ui->cobShowParticle->clear();
  for (int i=0; i<MW->Detector->MpCollection->countParticles(); i++)
      ui->cobShowParticle->addItem( MW->Detector->MpCollection->getParticleName(i) );

  if (old < ui->cobShowParticle->count()) ui->cobShowParticle->setCurrentIndex(old);
}

void OutputWindow::on_pbShowSelected_clicked()
{
  if (EventsDataHub->EventHistory.isEmpty())
    {
      message("No data available!", this);
      return;
    }

  int particleId = ui->cobShowParticle->currentIndex();
  int materialId = ui->cobShowMaterial->currentIndex();
  bool fSelectiveParticleType = ui->cobShowParticleType->currentIndex() == 0 ? false : true;
  //int particleType = ui->cobShowParticleType->currentIndex() - 1;  // 0-primary, 1-secondary  //cob:0-all,1-prim,2-sec
  bool fUseSecondary = ( ui->cobShowParticleType->currentIndex() == 2);
  ui->pteOut->clear();

  int counter=0;
  TH1D* hist1 = 0;
  switch (ui->cobWhatToShow->currentIndex())
  {
  case (0):
    {
      //Statistics: energy deposition
      hist1 = new TH1D("DepEnergyHist","Deposited energy",ui->sbShowBins->value(),0,0);
      hist1->GetXaxis()->SetTitle("Deposited energy, keV");

      for (int i=0; i<EventsDataHub->EventHistory.size(); i++)
      {
          if (ui->cbCheckerParticle->isChecked() && particleId != EventsDataHub->EventHistory[i]->ParticleId) continue;  //wrong particle

          if (MW->Detector->MpCollection->getParticleType( EventsDataHub->EventHistory[i]->ParticleId ) == AParticle::_neutron_ ) continue; //to prevent overflow, we know neutrons cannot deposit energy anyway
          if (fSelectiveParticleType)
              //if (particleType != EventsDataHub->EventHistory[i]->SecondaryOf) continue; //wrong type
              if (fUseSecondary != EventsDataHub->EventHistory[i]->isSecondary()) continue; //wrong type
          for (int m=0; m < EventsDataHub->EventHistory[i]->Deposition.size(); m++)
          {
              if (ui->cbCheckerMaterial->isChecked() && materialId != EventsDataHub->EventHistory[i]->Deposition[m].MaterialId) continue; //wrong material
              //all filters OK
              if (EventsDataHub->EventHistory[i]->Deposition[m].DepositedEnergy>0) hist1->Fill(EventsDataHub->EventHistory[i]->Deposition[m].DepositedEnergy);
              counter++;
          }
      }
      break; //draw below
    }

  case (1):
    {
      //Statistics: travelled distance
      hist1 = new TH1D("TravDistHist","Travelled distance",ui->sbShowBins->value(),0,0);
      hist1->GetXaxis()->SetTitle("Travelled distance, mm");

      for (int i=0; i < EventsDataHub->EventHistory.size(); i++)
      {
          if (ui->cbCheckerParticle->isChecked() && particleId != EventsDataHub->EventHistory[i]->ParticleId) continue;  //wrong particle
          if (fSelectiveParticleType)
              //if (particleType != EventsDataHub->EventHistory[i]->SecondaryOf) continue; //wrong type
              if (fUseSecondary != EventsDataHub->EventHistory[i]->isSecondary()) continue; //wrong type
          for (int m=0; m < EventsDataHub->EventHistory[i]->Deposition.size(); m++)
          {
              if (ui->cbCheckerMaterial->isChecked() && materialId != EventsDataHub->EventHistory[i]->Deposition[m].MaterialId) continue; //wrong material
              //all filters OK
              hist1->Fill(EventsDataHub->EventHistory[i]->Deposition[m].Distance);
              counter++;
          }
      }
      break; //draw below
    }
  case (2):
      {
        //Statistics: angle vs Z axis
        hist1 = new TH1D("AnglZHist","Angle distribution",ui->sbShowBins->value(),0,0);
        hist1->GetXaxis()->SetTitle("Angle with Z, degrees");

        //qDebug() << EnergyVector.size() << EventsDataHub->EventHistory.size();
        for (int i=0; i < EventsDataHub->EventHistory.size(); i++)
        {
            if (ui->cbCheckerParticle->isChecked() && particleId != EventsDataHub->EventHistory[i]->ParticleId) continue;  //wrong particle
            if (fSelectiveParticleType)
                //if (particleType != EventsDataHub->EventHistory[i]->SecondaryOf) continue; //wrong type
                if (fUseSecondary != EventsDataHub->EventHistory[i]->isSecondary()) continue; //wrong type
            for (int m=0; m < EventsDataHub->EventHistory[i]->Deposition.size(); m++)
            {
                if (ui->cbCheckerMaterial->isChecked() && materialId != EventsDataHub->EventHistory[i]->Deposition[m].MaterialId) continue; //wrong material
                //all filters OK
                float& dx = EventsDataHub->EventHistory[i]->dx;
                float& dy = EventsDataHub->EventHistory[i]->dy;
                float& dz = EventsDataHub->EventHistory[i]->dz;
                float trans2 = dx*dx + dy*dy;
                float angle =  ( fabs(dz) < 1e-10 ? 0 : atan( sqrt(trans2)/dz )*180.0/3.1415926 );

                hist1->Fill(angle);
                counter++;
            }
        }
        break; //draw below
      }
  case (3):
    {
      //interaction probability in this material for this particle
      int total = 0;
      int interactions = 0;

      for (int i=0; i<EventsDataHub->EventHistory.size(); i++)
      {
          if (ui->cbCheckerParticle->isChecked() && particleId != EventsDataHub->EventHistory[i]->ParticleId) continue;  //wrong particle
          if (fSelectiveParticleType)
              //if (particleType != EventsDataHub->EventHistory[i]->SecondaryOf) continue; //wrong type
              if (fUseSecondary != EventsDataHub->EventHistory[i]->isSecondary()) continue; //wrong type
          bool flagFound = false;
          bool flagInteraction = false;
          for (int m=0; m < EventsDataHub->EventHistory[i]->Deposition.size(); m++)
          {
              //could be several layers with the same material, so make a flag
              if (ui->cbCheckerMaterial->isChecked() && materialId != EventsDataHub->EventHistory[i]->Deposition[m].MaterialId) continue; //wrong material
              //all filters OK
              flagFound = true;
              if (EventsDataHub->EventHistory[i]->Deposition[m].DepositedEnergy > 0) flagInteraction = true; //takes care of all but neutron capture - they do not deposit energy directly
          }
          if (flagFound) total++; //this particle was in this material
          if (flagInteraction) interactions++; //this particle had intreaction inside this material

          //if (Detector->ParticleCollection.at( EventsDataHub->EventHistory[i]->ParticleId )->type == AParticle::_neutron_ )
          if (MW->Detector->MpCollection->getParticleType( EventsDataHub->EventHistory[i]->ParticleId ) == AParticle::_neutron_ )
          {
              if (EventsDataHub->EventHistory[i]->Termination != EventHistoryStructure::NeutronAbsorption) continue;
              //last medium?
              if (EventsDataHub->EventHistory[i]->Deposition[EventsDataHub->EventHistory[i]->Deposition.size()-1].MaterialId != materialId) continue;
              interactions++;
          }
      }

      double InterProbability = 0;
      if (total != 0 ) InterProbability = 1.0*interactions / total;
      QString str, str1;
      str = "Total: ";
      str1.setNum(total);
      str += str1 + " Interacted: ";
      str1.setNum(interactions);
      str += str1;
      ui->pteOut->appendPlainText(str);
      str.setNum(InterProbability*100.0, 'g', 4);
      ui->pteOut->appendPlainText("Interaction probability: "+str+ " %");
      return;
      //break;
    }
    case (4):
      {
        //deposited energy in this material for this particle more than a certain value
        int total = 0;
        int interactions = 0;
        double threshold = ui->ledShowThreshold->text().toDouble();

        for (int i=0; i < EventsDataHub->EventHistory.size(); i++)
          {
            if (ui->cbCheckerParticle->isChecked() && particleId != EventsDataHub->EventHistory[i]->ParticleId) continue;  //wrong particle
            if (fSelectiveParticleType)
                //if (particleType != EventsDataHub->EventHistory[i]->SecondaryOf) continue; //wrong type
                if (fUseSecondary != EventsDataHub->EventHistory[i]->isSecondary()) continue; //wrong type
            bool flagFound = false;
            bool flagInteraction = false;
            for (int m=0; m < EventsDataHub->EventHistory[i]->Deposition.size(); m++)
              {
                //could be several layers with the same material, so make a flag
                if (ui->cbCheckerMaterial->isChecked() && materialId != EventsDataHub->EventHistory[i]->Deposition[m].MaterialId) continue; //wrong material
                //all filters OK
                flagFound = true;
                if (EventsDataHub->EventHistory[i]->Deposition[m].DepositedEnergy > threshold) flagInteraction = true; //neutrons do not deposit directly anyway
              }
            if (flagFound) total++; //this particle was in this material
            if (flagInteraction) interactions++; //this particle had interaction inside this material
          }

        double Fraction = 0;
        if (total != 0 ) Fraction = 1.0*interactions / total;
        QString str, str1;
        str = "Total: ";
        str1.setNum(total);
        str += str1 + " Deposited above threshold (" + QString::number(threshold) + " keV): ";
        str1.setNum(interactions);
        str += str1;
        ui->pteOut->appendPlainText(str);
        str.setNum(Fraction*100.0, 'g', 4);
        ui->pteOut->appendPlainText("Fraction: "+str+ " %");
        return;
      }
  }


  if (counter == 0)
  {
      message("No data was found for the given selection", this);
      return;
  }
  //drawing histogram:
  MW->GraphWindow->Draw(hist1);
}

void OutputWindow::on_cobWhatToShow_currentIndexChanged(int index)
{
  switch (index)
    {
    case 0:
    case 1:
    case 2:
      ui->swShowPrPData->setCurrentIndex(0);
      break;
    case 3:
      ui->swShowPrPData->setCurrentIndex(1);
    break;
    case 4:
      ui->swShowPrPData->setCurrentIndex(2);
    }
}

void OutputWindow::on_pbNextEvent_clicked()
{
    ui->sbEvent->setValue(ui->sbEvent->value()+1);
}

void OutputWindow::on_tabwinDiagnose_tabBarClicked(int index)
{
    if (index==1)
    {
        QTimer::singleShot(50, this, SLOT(RefreshPMhitsTable()));
    }
}

void OutputWindow::on_pbMonitorShowXY_clicked()
{
    int imon = ui->cobMonitor->currentIndex();
    if (imon >= EventsDataHub->SimStat->Monitors.size()) return;

    MW->GraphWindow->ShowAndFocus();
    TH2D* h = new TH2D(*EventsDataHub->SimStat->Monitors[imon]->getXY());
    MW->GraphWindow->Draw(h, "colz", true, true);
}

void OutputWindow::on_pbMonitorShowTime_clicked()
{
    int imon = ui->cobMonitor->currentIndex();
    if (imon >= EventsDataHub->SimStat->Monitors.size()) return;

    MW->GraphWindow->ShowAndFocus();
    TH1D* h = new TH1D(*EventsDataHub->SimStat->Monitors[imon]->getTime());
    MW->GraphWindow->Draw(h, "", true, true);
}

void OutputWindow::on_pbMonitorShowAngle_clicked()
{
    int imon = ui->cobMonitor->currentIndex();
    if (imon >= EventsDataHub->SimStat->Monitors.size()) return;

    MW->GraphWindow->ShowAndFocus();
    TH1D* h = new TH1D(*EventsDataHub->SimStat->Monitors[imon]->getAngle());
    MW->GraphWindow->Draw(h, "", true, true);
}

void OutputWindow::on_pbMonitorShowWave_clicked()
{
    int imon = ui->cobMonitor->currentIndex();
    if (imon >= EventsDataHub->SimStat->Monitors.size()) return;

    MW->GraphWindow->ShowAndFocus();
    if (!MW->EventsDataHub->LastSimSet.fWaveResolved)
    {
        TH1D* h = new TH1D(*EventsDataHub->SimStat->Monitors[imon]->getWave());
        MW->GraphWindow->Draw(h, "", true, true);
    }
    else
    {
        TH1D* h = EventsDataHub->SimStat->Monitors[imon]->getWave();
        int nbins = h->GetXaxis()->GetNbins();

        double WaveFrom = MW->EventsDataHub->LastSimSet.WaveFrom;
        double WaveTo = MW->EventsDataHub->LastSimSet.WaveTo;
        double WaveBins = MW->EventsDataHub->LastSimSet.WaveNodes;

        TH1D *hnew = new TH1D("", "", nbins, WaveFrom, WaveTo);
        for (int i=1; i <= nbins; i++)
        {
            double y = h->GetBinContent(i);
            double index = h->GetXaxis()->GetBinCenter(i);
            double x = (WaveTo-WaveFrom)*index/(WaveBins-1) + WaveFrom;
            hnew->Fill(x, y);
        }
        hnew->SetXTitle("Wavelength, nm");
        MW->GraphWindow->Draw(hnew, "", true, true);
    }
}

void OutputWindow::on_pbMonitorShowEnergy_clicked()
{
    int imon = ui->cobMonitor->currentIndex();
    if (imon >= EventsDataHub->SimStat->Monitors.size()) return;

    MW->GraphWindow->ShowAndFocus();
    TH1D* h = new TH1D(*EventsDataHub->SimStat->Monitors[imon]->getEnergy());
    MW->GraphWindow->Draw(h, "", true, true);
}

void OutputWindow::on_pbShowProperties_clicked()
{
    MW->DAwindow->showNormal();
    MW->DAwindow->ShowTab(0);
    //MW->DAwindow->raise();
    MW->DAwindow->UpdateGeoTree(ui->cobMonitor->currentText());
}

void OutputWindow::on_cobMonitor_activated(int)
{
    updateMonitors();
}

void OutputWindow::on_pbShowAverageOverAll_clicked()
{
    if (EventsDataHub->isEmpty()) return;

    QVector<float> sums(MW->PMs->count(), 0);
    double f = 1.0/EventsDataHub->Events.size();

    for (int iev=0; iev<EventsDataHub->Events.size(); iev++)
        for (int ipm=0; ipm<MW->PMs->count(); ipm++)
            sums[ipm] += f * EventsDataHub->Events.at(iev).at(ipm);

    scene->clear();
    double MaxSignal = 1.0e-10;
    for (int i=0; i<MW->PMs->count(); i++)
       if ( sums.at(i) > MaxSignal) MaxSignal = sums.at(i);
    if (MaxSignal<=1.0e-25) MaxSignal = 1.0;

    updateSignalLabels(MaxSignal);
    addPMitems(&sums, MaxSignal, 0); //add icons with PMs to the scene
    if (ui->cbShowPMsignals->isChecked())
      addTextitems(&sums, MaxSignal, 0); //add icons with signal text to the scene
    updateSignalScale();
}
