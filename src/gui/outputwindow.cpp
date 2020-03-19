//ANTS2
#include "outputwindow.h"
#include "ui_outputwindow.h"
#include "mainwindow.h"
#include "graphwindowclass.h"
#include "apmhub.h"
#include "apmtype.h"
#include "windownavigatorclass.h"
#include "guiutils.h"
#include "amaterialparticlecolection.h"
#include "eventsdataclass.h"
#include "detectorclass.h"
#include "dynamicpassiveshandler.h"
#include "aglobalsettings.h"
#include "apositionenergyrecords.h"
#include "amessage.h"
#include "apmgroupsmanager.h"
#include "amonitor.h"
#include "asandwich.h"
#include "ageoobject.h"
#include "detectoraddonswindow.h"
#include "ajsontools.h"

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
#include <QFileDialog>

OutputWindow::OutputWindow(QWidget *parent, MainWindow *mw, EventsDataClass *eventsDataHub) :
    AGuiWindow("out", parent),
    ui(new Ui::OutputWindow)
{
    MW = mw;
    EventsDataHub = eventsDataHub;
    GVscale = 10.0;
    ui->setupUi(this);
    bForbidUpdate = false;

    this->setWindowTitle("Results/Output");

    Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
    windowFlags |= Qt::WindowCloseButtonHint;
    //windowFlags |= Qt::Tool;
    this->setWindowFlags( windowFlags );

    modelPMhits = 0;

    QVector<QWidget*> vecDis;
    vecDis << ui->pbSiPMpixels << ui->sbTimeBin
           << ui->lePTHistParticle << ui->cobPTHistVolMat << ui->lePTHistVolVolume << ui->sbPTHistVolIndex
           << ui->cobPTHistVolMatFrom << ui->cobPTHistVolMatTo << ui->lePTHistVolVolumeFrom << ui->lePTHistVolVolumeTo
           << ui->sbPTHistVolIndexFrom << ui->sbPTHistVolIndexTo
           << ui->leEVlimitToProc << ui->cbEVlimitToProcPrim << ui->leEVexcludeProc << ui->cbEVexcludeProcPrim;
    for (QWidget * w : vecDis) w->setEnabled(false);

    QVector<QWidget*> vecInv;
    vecInv << ui->cobPTHistVolPlus << ui->pbRefreshViz << ui->frPTHistX << ui->frPTHistY
           << ui->pbEventView_ShowTree << ui->pbEVgeo << ui->frEventFilters << ui->frTimeAware;
    for (QWidget * w : vecInv) w->setVisible(false);

    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);

    //Graphics view
    scaleScene = new QGraphicsScene(this);
    ui->gvScale->setScene(scaleScene);

    scene = new QGraphicsScene(this);
    //qDebug() << "This scene pointer:"<<scene;
    gvOut = new myQGraphicsView(ui->tabPmHitViz);
    gvOut->setGeometry(0,0,325,325);
    gvOut->setScene(scene);

    gvOut->setDragMode(QGraphicsView::ScrollHandDrag); //if zoomed, can use hand to center needed area
    gvOut->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    gvOut->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gvOut->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gvOut->setRenderHints(QPainter::Antialiasing);  

    ui->tabwinDiagnose->setCurrentIndex(0);
    updatePTHistoryBinControl();
    SetTab(2);

    ui->cobEVkin->setCurrentIndex(1);
    ui->cobEVdepo->setCurrentIndex(1);
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

void OutputWindow::OutText(const QString & str)
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

    const APmType *typ = MW->PMs->getTypeForPM(ipm);
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

void OutputWindow::addParticleHistoryLogLine(int iRec, int level)
{
    for (int i=0; i<secs.at(iRec).size(); i++)
        addParticleHistoryLogLine(secs.at(iRec).at(i), level+1);
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

void OutputWindow::resizeEvent(QResizeEvent * event)
{
    AGuiWindow::resizeEvent(event);

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
  for (int ipm = 0; ipm < MW->PMs->count(); ipm++)
  {
      const double & x =    MW->PMs->at(ipm).x;
      double         y =   -MW->PMs->at(ipm).y;
      const int & type =    MW->PMs->at(ipm).type;
      const double & size = MW->PMs->getType(type)->SizeX;

      if (x - size < Xmin) Xmin = x - size;
      if (x + size > Xmax) Xmax = x + size;
      if (y - size < Ymin) Ymin = y - size;
      if (y + size > Ymax) Ymax = y + size;
  }

  double Xdelta = Xmax - Xmin;
  double Ydelta = Ymax - Ymin;

  double frac = 1.05;
  scene->setSceneRect( (Xmin - frac*Xdelta)*GVscale,
                       (Ymin - frac*Ydelta)*GVscale,
                       ( (2.0*frac + 1.0) * Xdelta)*GVscale,
                       ( (2.0*frac + 1.0) * Ydelta)*GVscale );

  gvOut->fitInView( (Xmin - 0.01*Xdelta)*GVscale,
                    (Ymin - 0.01*Ydelta)*GVscale,
                    (1.02*Xdelta)*GVscale,
                    (1.02*Ydelta)*GVscale,
                    Qt::KeepAspectRatio);
}

void OutputWindow::InitWindow()
{
  bool shown = this->isVisible();
  this->showNormal();
  //SetTab(2);
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
  float MaxSignal = 0.0;
  if (!fHaveData) MaxSignal = 1.0;
  else
    {
      for (int i=0; i<MW->PMs->count(); i++)
        if (Passives->isActive(i))
        {
          if ( EventsDataHub->Events[CurrentEvent][i] > MaxSignal)
              MaxSignal = EventsDataHub->Events[CurrentEvent][i];
        }
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

  EV_showTree();

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
            ui->cobMonitor->addItem( QString("%1   index=%2").arg(obj->Name).arg(i));
        }
        if (oldNum>-1 && oldNum<numMonitors)
        {
            ui->cobMonitor->setCurrentIndex(oldNum);
            ui->sbMonitorIndex->setValue(oldNum);
        }
        else ui->sbMonitorIndex->setValue(0);

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

            bool bPhotonMode = (mon->config.PhotonOrParticle == 0);
            ui->pbMonitorShowWave->setVisible(bPhotonMode);
            ui->pbShowWavelength->setVisible(bPhotonMode);
            ui->pbMonitorShowEnergy->setVisible(!bPhotonMode);
        }
        else
        {
            ui->frMonitors->setEnabled(false);
            qWarning() << "Something is wrong: this is not a monitor object!";
        }
    }
}

void OutputWindow::addPMitems(const QVector<float> *vector, float MaxSignal, DynamicPassivesHandler *Passives)
{
  for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
      //pen
      QPen pen(Qt::black);
      int size = 6.0 * MW->PMs->SizeX(ipm) / 30.0;
      pen.setWidth(size);

      //brush
      QBrush brush(Qt::white);

      float sig = ( vector ? vector->at(ipm) : 0 );

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
      const APmType* tp = MW->PMs->getType(PM.type);
      if (tp->Shape == 0)
        {
          double sizex = tp->SizeX*GVscale;
          double sizey = tp->SizeY*GVscale;
          tmp = scene->addRect(-0.5*sizex, -0.5*sizey, sizex, sizey, pen, brush);
        }
      else if (tp->Shape == 1 || tp->Shape == 3)
        {
          double diameter = ( tp->Shape == 1 ? tp->SizeX*GVscale : 2.0*tp->getProjectionRadiusSpherical()*GVscale );
          tmp = scene->addEllipse( -0.5*diameter, -0.5*diameter, diameter, diameter, pen, brush);
        }
      else
        {
          double radius = 0.5*tp->SizeX*GVscale;
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

      if (PM.phi != 0) tmp->setRotation(-PM.phi);
        else if (PM.psi != 0) tmp->setRotation(-PM.psi);
      tmp->setTransform(QTransform().translate(PM.x*GVscale, -PM.y*GVscale)); //minus!!!!
    }
}

void OutputWindow::addTextitems(const QVector<float> *vector, float MaxSignal, DynamicPassivesHandler *Passives)
{
    const int prec = ui->sbDecimalPrecision->value();
  for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
      const APm &PM = MW->PMs->at(ipm);
      double size = 0.5*MW->PMs->getType( PM.type )->SizeX;
      //io->setTextWidth(40);

      float sig = ( vector ? vector->at(ipm) : 0 );
      //qDebug()<<"sig="<<sig;
      QString text = QString::number(sig, 'g', prec);
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
      double x = ( PM.x -wid*size*0.125 -0.15*size )*GVscale;
      double y = (-PM.y - 0.36*size  ) *GVscale; //minus y to invert the scale!!!
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

void OutputWindow::updateSignalLabels(float MaxSignal)
{
  QString str;
  str.setNum(MaxSignal, 'g', 4);
  ui->labHitsMax->setText(str);

  float lowmid = 0.25f * MaxSignal;
  if (lowmid>0)
    {
      str.setNum(lowmid, 'g', 4);
      ui->labLowMid->setText(str);
    }
  else ui->labLowMid->setText("");

  float mid = 0.5f * MaxSignal;
  if (mid > 0 && mid !=lowmid)
    {
      str.setNum(mid, 'g', 4);
      ui->labMid->setText(str);
    }
  else ui->labMid->setText("");

  float highmid = 0.75f * MaxSignal;
  if (highmid > 0 && highmid !=mid)
    {
      str.setNum(highmid, 'g', 4);
      ui->labHighMid->setText(str);
    }
  else ui->labHighMid->setText("");
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
              "Before starting a simulation check that\n"
              "'Statistics on detected photons' is activated:\n"
              "use the 'logs' button on the right side of the 'Simulate' button.", this);
      return;
    }

  //converting to wavelength
  int nBins = spec->GetNbinsX();
  //qDebug() << nBins << MW->WaveNodes;
  if (MW->EventsDataHub->LastSimSet.fWaveResolved)
    {
      auto WavelengthSpectrum = new TH1D("","Wavelength of detected photons", nBins, MW->WaveFrom, MW->WaveTo);
      for (int i=1; i<nBins+1; i++) //0 - underflow, n+1 - overflow
      {
          //qDebug() << i << spec->GetBinCenter(i)<< spec->GetBinContent(i);
          WavelengthSpectrum->SetBinContent(i, spec->GetBinContent(i));
      }
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
              "'Statistics on detected photons' has to be activated before simulation!\n"
              "Use the 'logs' button on the right side of the 'Simulate' button.", this);
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
      message("Angle of incidence data are empty!\n\n"
              "Before starting a simulation, check that:\n"
              "1) Simulation_options/Angle/Take_into_account_PM_angular_response is checked;\n"
              "2) 'Statistics on detected photons' is activated:\n"
              "   use the 'logs' button on the right side of the 'Simulate' button.", this);
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
               "'Statistics on detected photons' has to be activated before simulation!\n"
               "Use the 'logs' button on the right side of the 'Simulate' button.", this);
       return;
     }

   spec->GetXaxis()->SetTitle("Number of cycles in tracking");
   spec->SetTitle("Number of tracking cycles for detected photons");
   MW->GraphWindow->Draw(spec, "", true, false);
}

void OutputWindow::on_pbResetViewport_clicked()
{
    OutputWindow::RefreshData();
    OutputWindow::ResetViewport();
}

void OutputWindow::ShowOneEventLog(int iev)
{
  if (!isVisible()) show();
  raise();

  int precision = MW->GlobSet.TextLogPrecision;
  QString str = "Event# " + QString::number(iev);
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

    auto hist1D = new TH1F("SumSig","Distribution of sum signal", MW->GlobSet.BinsX, 0,0);
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
    auto hist1D = new TH1F("Sig", title, MW->GlobSet.BinsX, 0,0);
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

  auto hist1D = new TH1F("MaxSigDist","Distribution of max signal", MW->GlobSet.BinsX, 0,0);
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
                "Activate 'Photon generation log' before starting a simulation!\n"
                "Use the button on the right side of the 'Simulate' button.\n"
                "\nNote that simulations in the 'Only photons' mode do not generate this log!", this);
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

    ui->pteOut->appendPlainText(s);
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
    QVector<QComboBox*> vec;
    vec << ui->cobPTHistVolMat << ui->cobPTHistVolMatFrom << ui->cobPTHistVolMatTo;

    QStringList mats = MW->MpCollection->getListOfMaterialNames();
    for (QComboBox * c : vec)
    {
        int old = c->currentIndex();
        c->clear();
        c->addItems(mats);

        if (old < 0) old = 0;
        if (old < c->count()) c->setCurrentIndex(old);
    }
}

void OutputWindow::UpdateParticles()
{
//  int old = ui->cobShowParticle->currentIndex();

//  ui->cobShowParticle->clear();
//  for (int i=0; i<MW->Detector->MpCollection->countParticles(); i++)
//      ui->cobShowParticle->addItem( MW->Detector->MpCollection->getParticleName(i) );

    //  if (old < ui->cobShowParticle->count()) ui->cobShowParticle->setCurrentIndex(old);
}

void OutputWindow::SaveGuiToJson(QJsonObject &json) const
{
    QJsonObject js;
    saveEventViewerSettings(js);
    json["EventViewer"] = js;
}

void OutputWindow::LoadGuiFromJson(const QJsonObject &json)
{
    QJsonObject js;
    parseJson(json, "EventViewer", js);
    if (!js.isEmpty()) loadEventViewerSettings(js);
}

void OutputWindow::saveEventViewerSettings(QJsonObject & json) const
{
    json["HideAllTransport"] = ui->cbEVhideTrans->isChecked();
    json["HidePrimTransport"] = ui->cbEVhideTransPrim->isChecked();
    json["Precision"] = ui->sbEVprecision->value();
    json["Expansion"] = ui->sbEVexpansionLevel->value();
    json["Position"] = ui->cbEVpos->isChecked();
    json["Step"] = ui->cbEVstep->isChecked();
    json["KinEn"] = ui->cbEVkin->isChecked();
    json["DepoEn"] = ui->cbEVdepo->isChecked();
    json["Volume"] = ui->cbEVvol->isChecked();
    json["VolIndex"] = ui->cbEVvi->isChecked();
    json["Material"] = ui->cbEVmat->isChecked();
    json["Time"] = ui->cbEVtime->isChecked();
    json["ShowTracks"] = ui->cbEVtracks->isChecked();
    json["SuppressSec"] = ui->cbEVsupressSec->isChecked();
    json["ShowPMsig"] = ui->cbEVpmSig->isChecked();

    json["ShowFilters"] = ui->cbEVshowFilters->isChecked();

    json["LimToProcActive"] = ui->cbEVlimToProc->isChecked();
    json["LimToProc"] = ui->leEVlimitToProc->text();
    json["LimToProcPrim"] = ui->cbEVlimitToProcPrim->isChecked();

    json["ExclProcActive"] = ui->cbEVexcludeProc->isChecked();
    json["ExclToProc"] = ui->leEVexcludeProc->text();
    json["ExclToProcPrim"] = ui->cbEVexcludeProcPrim->isChecked();
}

void OutputWindow::loadEventViewerSettings(const QJsonObject & json)
{
    JsonToCheckbox(json, "HideAllTransport", ui->cbEVhideTrans);
    JsonToCheckbox(json, "HidePrimTransport", ui->cbEVhideTransPrim);
    JsonToSpinBox (json, "Precision", ui->sbEVprecision);
    JsonToSpinBox (json, "Expansion", ui->sbEVexpansionLevel);
    JsonToCheckbox(json, "Position", ui->cbEVpos);
    JsonToCheckbox(json, "Step", ui->cbEVstep);
    JsonToCheckbox(json, "KinEn", ui->cbEVkin);
    JsonToCheckbox(json, "DepoEn", ui->cbEVdepo);
    JsonToCheckbox(json, "Volume", ui->cbEVvol);
    JsonToCheckbox(json, "VolIndex", ui->cbEVvi);
    JsonToCheckbox(json, "Material", ui->cbEVmat);
    JsonToCheckbox(json, "Time", ui->cbEVtime);
    JsonToCheckbox(json, "ShowTracks", ui->cbEVtracks);
    JsonToCheckbox(json, "SuppressSec", ui->cbEVsupressSec);
    JsonToCheckbox(json, "ShowPMsig", ui->cbEVpmSig);

    JsonToCheckbox(json, "ShowFilters", ui->cbEVshowFilters);

    JsonToCheckbox    (json, "LimToProcActive", ui->cbEVlimToProc);
    JsonToLineEditText(json, "LimToProc", ui->leEVlimitToProc);
    JsonToCheckbox    (json, "LimToProcPrim", ui->cbEVlimitToProcPrim);

    JsonToCheckbox    (json, "ExclProcActive", ui->cbEVexcludeProc);
    JsonToLineEditText(json, "ExclToProc", ui->leEVexcludeProc);
    JsonToCheckbox    (json, "ExclToProcPrim", ui->cbEVexcludeProcPrim);
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
    MW->GraphWindow->Draw(h, "hist", true, true);
}

void OutputWindow::on_pbMonitorShowAngle_clicked()
{
    int imon = ui->cobMonitor->currentIndex();
    if (imon >= EventsDataHub->SimStat->Monitors.size()) return;

    MW->GraphWindow->ShowAndFocus();
    TH1D* h = new TH1D(*EventsDataHub->SimStat->Monitors[imon]->getAngle());
    MW->GraphWindow->Draw(h, "hist", true, true);
}

void OutputWindow::on_pbMonitorShowWave_clicked()
{
    int imon = ui->cobMonitor->currentIndex();
    if (imon >= EventsDataHub->SimStat->Monitors.size()) return;

    MW->GraphWindow->ShowAndFocus();
    TH1D* h = new TH1D(*EventsDataHub->SimStat->Monitors[imon]->getWave());
    MW->GraphWindow->Draw(h, "hist", true, true);
}

void OutputWindow::on_pbShowWavelength_clicked()
{
    if (!MW->EventsDataHub->LastSimSet.fWaveResolved)
    {
        on_pbMonitorShowWave_clicked();
        return;
    }

    int imon = ui->cobMonitor->currentIndex();
    if (imon >= EventsDataHub->SimStat->Monitors.size()) return;

    MW->GraphWindow->ShowAndFocus();

        TH1D* h = EventsDataHub->SimStat->Monitors[imon]->getWave();
        int nbins = h->GetXaxis()->GetNbins();

        double gsWaveFrom = MW->EventsDataHub->LastSimSet.WaveFrom;
        double gsWaveTo = MW->EventsDataHub->LastSimSet.WaveTo;
        double gsWaveBins = MW->EventsDataHub->LastSimSet.WaveNodes;
        if (gsWaveBins > 1) gsWaveBins--;
        double wavePerBin = (gsWaveTo - gsWaveFrom) / gsWaveBins;

        double binFrom = h->GetBinLowEdge(1);
        double waveFrom = gsWaveFrom + (binFrom - 0.5) * wavePerBin;
        double binTo = h->GetBinLowEdge(nbins+1);
        double waveTo = gsWaveFrom + (binTo - 0.5) * wavePerBin;

//        TH1D *hnew = new TH1D(*h);
//        double* new_bins = new double[nbins+1];
//        for (int i=0; i <= nbins; i++)
//            new_bins[i] = WaveFrom + wavePerBin * h->GetBinLowEdge(i+1);
//        hnew->SetBins(nbins, new_bins);

        TH1D *hnew = new TH1D("", "", nbins, waveFrom, waveTo);
        for (int i=1; i <= nbins; i++)
        {
            double y = h->GetBinContent(i);
            //double index = h->GetXaxis()->GetBinCenter(i);
            //double x = (WaveTo-WaveFrom)*index/(WaveBins-1) + WaveFrom;
            //hnew->Fill(x, y);
            hnew->SetBinContent(i, y);
        }
        hnew->SetXTitle("Wavelength, nm");
        MW->GraphWindow->Draw(hnew, "hist", true, true);
}

void OutputWindow::on_pbMonitorShowEnergy_clicked()
{
    int imon = ui->cobMonitor->currentIndex();
    if (imon >= EventsDataHub->SimStat->Monitors.size()) return;

    MW->GraphWindow->ShowAndFocus();
    TH1D* h = new TH1D(*EventsDataHub->SimStat->Monitors[imon]->getEnergy());
    MW->GraphWindow->Draw(h, "hist", true, true);
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

// --------- particle tracking history ----------

#include "asimulationmanager.h"
#include "aeventtrackingrecord.h"
#include "atrackinghistorycrawler.h"

void OutputWindow::on_pbPTHistRequest_clicked()
{
    if (MW->SimulationManager->TrackingHistory.empty())
    {
        message("Tracking history is empty!", this);
        return;
    }

    AFindRecordSelector Opt;
    ATrackingHistoryCrawler Crawler(MW->SimulationManager->TrackingHistory);

    Opt.bParticle = ui->cbPTHistParticle->isChecked();
    Opt.Particle = ui->lePTHistParticle->text();
    Opt.bPrimary = ui->cbPTHistOnlyPrim->isChecked();
    Opt.bSecondary = ui->cbPTHistOnlySec->isChecked();
    Opt.bLimitToFirstInteractionOfPrimary = ui->cbPTHistLimitToFirst->isChecked();

    int bins = ui->sbPTHistBinsX->value();
    double from = ui->ledPTHistFromX->text().toDouble();
    double to   = ui->ledPTHistToX  ->text().toDouble();

    int Selector = ui->twPTHistType->currentIndex(); // 0 - Vol, 1 - Boundary
    if (Selector == 0)
    {
        Opt.bMaterial = ui->cbPTHistVolMat->isChecked();
        Opt.Material = ui->cobPTHistVolMat->currentIndex();
        Opt.bVolume = ui->cbPTHistVolVolume->isChecked();
        Opt.Volume = ui->lePTHistVolVolume->text().toLocal8Bit().data();
        Opt.bVolumeIndex = ui->cbPTHistVolIndex->isChecked();
        Opt.VolumeIndex = ui->sbPTHistVolIndex->value();

        int What = ui->cobPTHistVolRequestWhat->currentIndex();
        switch (What)
        {
        case 0:
          {
            AHistorySearchProcessor_findParticles p;
            Crawler.find(Opt, p);
            QMap<QString, int>::const_iterator it = p.FoundParticles.constBegin();
            ui->ptePTHist->clear();
            ui->ptePTHist->appendPlainText("Particles found:\n");
            while (it != p.FoundParticles.constEnd())
            {
                ui->ptePTHist->appendPlainText(QString("%1   %2 times").arg(it.key()).arg(it.value()));
                ++it;
            }
            break;
          }
        case 1:
          {
            AHistorySearchProcessor_findProcesses p;
            Crawler.find(Opt, p);

            QMap<QString, int>::const_iterator it = p.FoundProcesses.constBegin();
            ui->ptePTHist->clear();
            ui->ptePTHist->appendPlainText("Processes found:\n");
            while (it != p.FoundProcesses.constEnd())
            {
                ui->ptePTHist->appendPlainText(QString("%1   %2 times").arg(it.key()).arg(it.value()));
                ++it;
            }
          }
            break;
        case 2:
          {
            AHistorySearchProcessor_findTravelledDistances p(bins, from, to);
            Crawler.find(Opt, p);

            if (p.Hist->GetEntries() == 0)
                message("No trajectories found", this);
            else
            {
                MW->GraphWindow->Draw(p.Hist);
                p.Hist = nullptr;
            }
            binsDistance = bins;
            fromDistance = from;
            toDistance = to;

            break;
          }
        case 3:
          {
            int mode = ui->cobPTHistVolPlus->currentIndex();
            if (mode <0 || mode >2)
            {
                message("Unknown energy deposition collection mode", this);
                return;
            }
            AHistorySearchProcessor_findDepositedEnergy::CollectionMode edm = static_cast<AHistorySearchProcessor_findDepositedEnergy::CollectionMode>(mode);

            AHistorySearchProcessor_findDepositedEnergy p(edm, bins, from, to);
            Crawler.find(Opt, p);

            if (p.Hist->GetEntries() == 0)
                message("No deposition detected", this);
            else
            {
                MW->GraphWindow->Draw(p.Hist);
                p.Hist = nullptr;
            }
            binsEnergy = bins;
            fromEnergy = from;
            toEnergy = to;
            selectedModeForEnergyDepo = mode;

            break;
          }
        case 4:
         {
            AHistorySearchProcessor_getDepositionStats * p = nullptr;
            if (ui->cbLimitTimeWindow->isChecked())
            {
                p = new AHistorySearchProcessor_getDepositionStatsTimeAware(ui->ledTimeFrom->text().toFloat(), ui->ledTimeTo->text().toFloat());
                Crawler.find(Opt, *p);
            }
            else
            {
                p = new AHistorySearchProcessor_getDepositionStats();
                Crawler.find(Opt, *p);
            }

            ui->ptePTHist->clear();
            ui->ptePTHist->appendPlainText("Deposition statistics:\n");
            QMap<QString, AParticleDepoStat>::const_iterator it = p->DepoData.constBegin();
            QVector< QPair<QString, AParticleDepoStat> > vec;
            double sum = 0;
            while (it != p->DepoData.constEnd())
            {
                vec << QPair<QString, AParticleDepoStat>(it.key(), it.value());
                sum += it.value().sum;
                ++it;
            }
            double sumInv = (sum > 0 ? 100.0/sum : 1.0);

            std::sort(vec.begin(), vec.end(), [](const QPair<QString, AParticleDepoStat> & a, const QPair<QString, AParticleDepoStat> & b)->bool{return a.second.sum > b.second.sum;});

            for (const auto & el : vec)
            {
                const AParticleDepoStat & rec = el.second;
                const double mean = rec.sum / rec.num;
                const double sigma = sqrt( (rec.sumOfSquares - 2.0*mean*rec.sum)/rec.num + mean*mean );

                QString str = QString("%1\t%2 keV (%3%)\t#: %4").arg(el.first).arg(rec.sum).arg( QString::number(rec.sum*sumInv, 'g', 4) ).arg(rec.num);

                if (rec.num > 1)  str += QString("\tmean: %1 keV").arg(mean);
                if (rec.num > 10) str += QString("\tsigma: %1 keV").arg(sigma);

                ui->ptePTHist->appendPlainText(str);
            }
            ui->ptePTHist->appendPlainText("\n---------\n");
            ui->ptePTHist->appendPlainText(QString("sum of all listed depositions: %1 keV").arg(sum));
            delete p;
            break;
         }
        default:
            qWarning() << "Unknown type of volume request";
        }
    }
    else
    {
        //Border
        Opt.bFromMat = ui->cbPTHistVolMatFrom->isChecked();
        Opt.FromMat = ui->cobPTHistVolMatFrom->currentIndex();
        Opt.bFromVolume = ui->cbPTHistVolVolumeFrom->isChecked();
        Opt.FromVolume = ui->lePTHistVolVolumeFrom->text().toLocal8Bit().data();
        Opt.bFromVolIndex = ui->cbPTHistVolIndexFrom->isChecked();
        Opt.FromVolIndex = ui->sbPTHistVolIndexFrom->value();

        Opt.bToMat = ui->cbPTHistVolMatTo->isChecked();
        Opt.ToMat = ui->cobPTHistVolMatTo->currentIndex();
        Opt.bToVolume = ui->cbPTHistVolVolumeTo->isChecked();
        Opt.ToVolume = ui->lePTHistVolVolumeTo->text().toLocal8Bit().data();
        Opt.bToVolIndex = ui->cbPTHistVolIndexTo->isChecked();
        Opt.ToVolIndex = ui->sbPTHistVolIndexTo->value();

        QString what = ui->lePTHistBordWhat->text();
        QString vsWhat = ui->lePTHistBordVsWhat->text();
        QString andVsWhat = ui->lePTHistBordAndVsWhat->text();
        QString cuts = ui->lePTHistBordCuts->text();

        bool bVs = ui->cbPTHistBordVs->isChecked();
        bool bVsVs = ui->cbPTHistBordAndVs->isChecked();
        bool bAveraged = ui->cbPTHistBordAsStat->isChecked();

        int bins2 = ui->sbPTHistBinsY->value();
        double from2 = ui->ledPTHistFromY->text().toDouble();
        double to2   = ui->ledPTHistToY  ->text().toDouble();

        if (!bVs)
        {
            //1D stat
            AHistorySearchProcessor_Border p(what, cuts, bins, from, to);
            Crawler.find(Opt, p);

            if (p.Hist1D->GetEntries() == 0)
                message("No data", this);
            else
            {
                MW->GraphWindow->Draw(p.Hist1D, "hist");
                p.Hist1D = nullptr;
            }
        }
        else
        {
            // "vs" is activated
            if (!bVsVs && bAveraged)
            {
                //1D vs
                AHistorySearchProcessor_Border p(what, vsWhat, cuts, bins, from, to);
                Crawler.find(Opt, p);

                if (p.Hist1D->GetEntries() == 0)
                    message("No data", this);
                else
                {
                    MW->GraphWindow->Draw(p.Hist1D, "hist");
                    p.Hist1D = nullptr;
                }
            }
            else if (!bVsVs && !bAveraged)
            {
                //2D stat
                AHistorySearchProcessor_Border p(what, vsWhat, cuts, bins, from, to, bins2, from2, to2);
                Crawler.find(Opt, p);

                if (p.Hist2D->GetEntries() == 0)
                    message("No data", this);
                else
                {
                    MW->GraphWindow->Draw(p.Hist2D, "colz");
                    p.Hist2D = nullptr;
                }
                binsB2 = bins2;
                fromB2 = from2;
                toB2 = to2;
            }
            else if (bVsVs)
            {
                //2D vsvs
                AHistorySearchProcessor_Border p(what, vsWhat, andVsWhat, cuts, bins, from, to, bins2, from2, to2);
                Crawler.find(Opt, p);

                if (p.Hist2D->GetEntries() == 0)
                    message("No data", this);
                else
                {
                    MW->GraphWindow->Draw(p.Hist2D, "colz");
                    p.Hist2D = nullptr;
                }
                binsB2 = bins2;
                fromB2 = from2;
                toB2 = to2;
            }
            else message("Unexpected mode!", this);
        }
        binsB1 = bins;
        fromB1 = from;
        toB1 = to;
    }
}

void OutputWindow::on_cbPTHistOnlyPrim_clicked(bool checked)
{
    if (checked) ui->cbPTHistOnlySec->setChecked(false);
}

void OutputWindow::on_cbPTHistOnlySec_clicked(bool checked)
{
    if (checked) ui->cbPTHistOnlyPrim->setChecked(false);
}

void OutputWindow::on_cobPTHistVolRequestWhat_currentIndexChanged(int index)
{
    updatePTHistoryBinControl();

    if (index == 2)
    {
        ui->sbPTHistBinsX->setValue(binsDistance);
        ui->ledPTHistFromX->setText(QString::number(fromDistance));
        ui->ledPTHistToX->setText(QString::number(toDistance));
    }
    else if (index == 3)
    {
        ui->sbPTHistBinsX->setValue(binsEnergy);
        ui->ledPTHistFromX->setText(QString::number(fromEnergy));
        ui->ledPTHistToX->setText(QString::number(toEnergy));

        ui->cobPTHistVolPlus->clear();
        ui->cobPTHistVolPlus->addItems(QStringList() << "Individual"<<"With secondaries"<<"Over event");
        ui->cobPTHistVolPlus->setCurrentIndex(selectedModeForEnergyDepo);
    }
    ui->cobPTHistVolPlus->setVisible(index == 3);

    ui->frTimeAware->setVisible(index == 4);
}

void OutputWindow::on_twPTHistType_currentChanged(int index)
{
    if (index == 0)
        on_cobPTHistVolRequestWhat_currentIndexChanged(ui->cobPTHistVolRequestWhat->currentIndex());
    else
    {
        ui->sbPTHistBinsX->setValue(binsB1);
        ui->ledPTHistFromX->setText(QString::number(fromB1));
        ui->ledPTHistToX->setText(QString::number(toB1));
        ui->sbPTHistBinsY->setValue(binsB2);
        ui->ledPTHistFromY->setText(QString::number(fromB2));
        ui->ledPTHistToY->setText(QString::number(toB2));
    }

    updatePTHistoryBinControl();
}

void OutputWindow::updatePTHistoryBinControl()
{
    if (ui->twPTHistType->currentIndex() == 0)
    {
        //Volume
        ui->frPTHistX->setVisible( ui->cobPTHistVolRequestWhat->currentIndex() > 1  && ui->cobPTHistVolRequestWhat->currentIndex() != 4);
        ui->frPTHistY->setVisible(false);
    }
    else
    {
        //Border
        bool bVs = ui->cbPTHistBordVs->isChecked();
        ui->lePTHistBordVsWhat->setEnabled(bVs);
        ui->cbPTHistBordAndVs->setEnabled(bVs);
        if (!bVs) ui->cbPTHistBordAndVs->setChecked(false);

        bool bVsVs = ui->cbPTHistBordAndVs->isChecked();
        if (bVsVs) ui->cbPTHistBordAsStat->setChecked(true);
        ui->lePTHistBordAndVsWhat->setEnabled(bVs && bVsVs);
        ui->cbPTHistBordAsStat->setEnabled(bVs && !bVsVs);
        bool bAveraged = ui->cbPTHistBordAsStat->isChecked();

        ui->frPTHistX->setVisible(true);
        ui->frPTHistY->setVisible(bVsVs || (bVs && !bAveraged));
    }
}

void OutputWindow::on_cbPTHistBordVs_toggled(bool)
{
    updatePTHistoryBinControl();
}

void OutputWindow::on_cbPTHistBordAndVs_toggled(bool)
{
    updatePTHistoryBinControl();
}

void OutputWindow::on_cbPTHistBordAsStat_toggled(bool)
{
    updatePTHistoryBinControl();
}

void OutputWindow::fillEvTabViewRecord(QTreeWidgetItem * item, const AParticleTrackingRecord * pr, int ExpansionLevel) const
{
    item->setText(0, pr->ParticleName);
    qlonglong poi = reinterpret_cast<qlonglong>(pr);
    item->setText(1, QString("%1").arg(poi));
    //item->setFlags(w->flags() & ~Qt::ItemIsDragEnabled);// & ~Qt::ItemIsSelectable);

    if (ExpansionLevel > 0) ui->trwEventView->expandItem(item);
    ExpansionLevel--;

    int precision = ui->sbEVprecision->value();
    bool bHideTransp = ui->cbEVhideTrans->isChecked();
    bool bHideTranspPrim = ui->cbEVhideTransPrim->isChecked();

    bool bPos = ui->cbEVpos->isChecked();
    bool bStep = ui->cbEVstep->isChecked();
    bool bTime = ui->cbEVtime->isChecked();
    double timeUnits = 1.0;
    switch (ui->cobEVtime->currentIndex())
    {
    case 0: break;
    case 1: timeUnits *= 0.001; break;
    case 2: timeUnits *= 1.0e-6; break;
    }
    bool bVolume = ui->cbEVvol->isChecked();
    bool bKin = ui->cbEVkin->isChecked();
    bool bDepo = ui->cbEVdepo->isChecked();
    double kinUnits = 1.0;
    switch (ui->cobEVkin->currentIndex())
    {
    case 0: kinUnits *= 1.0e6;
    case 1: break;
    case 2: kinUnits *= 1.0e-3; break;
    }
    double depoUnits = 1.0;
    switch (ui->cobEVdepo->currentIndex())
    {
    case 0: depoUnits *= 1.0e6;
    case 1: break;
    case 2: depoUnits *= 1.0e-3; break;
    }
    bool bIndex = ui->cbEVvi->isChecked();
    bool bMat = ui->cbEVmat->isChecked();

    QString curVolume;
    int     curVolIndex;
    int     curMat;

    for (size_t iStep = 0; iStep < pr->getSteps().size(); iStep++)
    {
        ATrackingStepData * step = pr->getSteps().at(iStep);

        QString s = step->Process;

        if (step->Process == "C")
        {
            ATransportationStepData * trStep = static_cast<ATransportationStepData*>(step);
            curVolume = trStep->VolName;
            curVolIndex = trStep->VolIndex;
            curMat = trStep->iMaterial;
        }
        else if (step->Process == "T")
        {
            ATransportationStepData * trStep = dynamic_cast<ATransportationStepData*>(step);
            if (bHideTransp || (bHideTranspPrim && pr->isPrimary()) )
            {
                curVolume   = trStep->VolName;
                curVolIndex = trStep->VolIndex;
                curMat      = trStep->iMaterial;
                continue;
            }

            s += QString("  %1 (#%2, %3) -> %4 (#%5, %6)").arg(curVolume)
                                                          .arg(curVolIndex)
                                                          .arg(MW->MpCollection->getMaterialName(curMat))
                                                          .arg(trStep->VolName)
                                                          .arg(trStep->VolIndex)
                                                          .arg(MW->MpCollection->getMaterialName(trStep->iMaterial));
            //cannot set currents yet - the indication should still show the "from" values - remember about energy deposition during "T" step!
        }

        if (bPos) s += QString("  (%1, %2, %3)").arg(step->Position[0], 0, 'g', precision).arg(step->Position[1], 0, 'g', precision).arg(step->Position[2], 0, 'g', precision);
        if (bStep)
        {
            double delta = 0;
            if (iStep != 0)
            {
                ATrackingStepData * prev = pr->getSteps().at(iStep-1);
                for (int i=0; i<3; i++)
                    delta += (step->Position[i] - prev->Position[i]) * (step->Position[i] - prev->Position[i]);
                delta = sqrt(delta);
            }
            s += QString("  %1mm").arg(delta, 0, 'g', precision);
        }

        if (step->Process != "O" && step->Process != "T")
        {
            if (bVolume) s += QString("  %1").arg(curVolume);
            if (bIndex)  s += QString("  %1").arg(curVolIndex);
            if (bMat)    s += QString("  %1").arg( MW->MpCollection->getMaterialName( curMat ));
        }

        if (bTime)   s += QString("  t=%1").arg(step->Time * timeUnits, 0, 'g', precision);
        if (bDepo)   s += QString("  depo=%1").arg(step->DepositedEnergy * depoUnits, 0, 'g', precision);
        if (bKin)    s += QString("  E=%1").arg(step->Energy * kinUnits, 0, 'g', precision);

        QTreeWidgetItem * it = new QTreeWidgetItem(item);
        it->setText(0, s);
        qlonglong poi = reinterpret_cast<qlonglong>(pr);
        it->setText(1, QString("%1").arg(poi));
        poi = reinterpret_cast<qlonglong>(step);
        it->setText(2, QString("%1").arg(poi));

        if (ExpansionLevel > 0) ui->trwEventView->expandItem(it);

        for (int iSec : step->Secondaries)
        {
            QTreeWidgetItem * subItem = new QTreeWidgetItem(it);
            fillEvTabViewRecord(subItem, pr->getSecondaries().at(iSec), ExpansionLevel-1);
        }

        if (step->Process == "T")
        {
            ATransportationStepData * trStep = dynamic_cast<ATransportationStepData*>(step);
            curVolume = trStep->VolName;
            curVolIndex = trStep->VolIndex;
            curMat = trStep->iMaterial;
        }
    }
}

void OutputWindow::EV_showTree()
{
    ui->trwEventView->clear();

    std::vector<AEventTrackingRecord *> & TH = MW->SimulationManager->TrackingHistory;
    if (TH.empty()) return;

    int iEv = ui->sbEvent->value();
    if (iEv >= (int)TH.size()) return;

    int ExpLevel = ui->sbEVexpansionLevel->value();

    AEventTrackingRecord * er = TH.at(iEv);
    for (AParticleTrackingRecord* pr : er->getPrimaryParticleRecords())
    {
        QTreeWidgetItem * item = new QTreeWidgetItem(ui->trwEventView);
        fillEvTabViewRecord(item, pr, ExpLevel);
    }
}

void OutputWindow::EV_show()
{
    EV_showTree();
    EV_showGeo();
}

#include "geometrywindowclass.h"
void OutputWindow::EV_showGeo()
{
    MW->SimulationManager->clearTracks();
    MW->GeometryWindow->ClearTracks(false);

    int iEv = ui->sbEvent->value();
    if (ui->cbEVtracks->isChecked()) MW->GeometryWindow->ShowEvent_Particles(iEv, !ui->cbEVsupressSec->isChecked());

    if (ui->cbEVpmSig->isChecked()) MW->GeometryWindow->ShowPMsignals(iEv, false);

    MW->GeometryWindow->DrawTracks();
}

int OutputWindow::findEventWithFilters(int currentEv, bool bUp)
{
    std::vector<AEventTrackingRecord *> & TH = MW->SimulationManager->TrackingHistory;
    if (TH.empty()) return -1;
    if (currentEv == 0 && !bUp) return -1;
    if (currentEv >= (int)TH.size() && bUp) return -1;

    const QRegularExpression rx = QRegularExpression("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'

    bool bLimProc = ui->cbEVlimToProc->isChecked();
    bool bLimProc_prim = ui->cbEVlimitToProcPrim->isChecked();

    bool bExclProc = ui->cbEVexcludeProc->isChecked();
    bool bExclProc_prim = ui->cbEVexcludeProcPrim->isChecked();

    bool bLimVols = ui->cbLimitToVolumes->isChecked();

    if (currentEv > (int)TH.size()) currentEv = (int)TH.size();

    bUp ? currentEv++ : currentEv--;
    while (currentEv >= 0 && currentEv < (int)TH.size())
    {
        const AEventTrackingRecord * er = TH.at(currentEv);

        bool bGood = true;
        if (bLimProc)
        {
            QStringList LimProc = ui->leEVlimitToProc->text().split(rx, QString::SkipEmptyParts);
            bGood = er->isHaveProcesses(LimProc, bLimProc_prim);
        }
        if (bGood && bExclProc)
        {
            QStringList ExclProc = ui->leEVexcludeProc->text().split(rx, QString::SkipEmptyParts);
            bGood = !er->isHaveProcesses(ExclProc, bExclProc_prim);
        }
        if (bGood && bLimVols)
        {
            QStringList LimVols = ui->leLimitToVolumes->text().split(rx, QString::SkipEmptyParts);
            QStringList LimVolStartWith;
            for (int i=LimVols.size()-1; i >= 0; i--)
            {
                const QString & s = LimVols.at(i);
                if (s.endsWith('*'))
                {
                    LimVolStartWith << s.mid(0, s.size()-1);
                    LimVols.removeAt(i);
                }
            }
            bGood = er->isTouchedVolumes(LimVols, LimVolStartWith);
        }

        if (bGood) return currentEv;

        bUp ? currentEv++ : currentEv--;
    };
    return -1;
}

void OutputWindow::on_pbNextEvent_clicked()
{
    QWidget * cw = ui->tabwinDiagnose->currentWidget();
    int i = ui->sbEvent->value();
    if (cw == ui->tabEventViewer)
    {
        if (MW->SimulationManager->TrackingHistory.empty())
        {
            message("Tracking history is empty!", this);
            return;
        }
        int newi = findEventWithFilters(i, true);
        if (newi == -1 && i != MW->EventsDataHub->countEvents()-1)
            message("There are no events according to the selected criteria", this);
        else i = newi;
    }
    else i++;

    if (i >= 0 && i < MW->EventsDataHub->countEvents()) ui->sbEvent->setValue(i);
}

void OutputWindow::on_pbPreviousEvent_clicked()
{
    QWidget * cw = ui->tabwinDiagnose->currentWidget();
    int i = ui->sbEvent->value();
    if (cw == ui->tabEventViewer)
    {
        if (MW->SimulationManager->TrackingHistory.empty())
        {
            message("Tracking history is empty!", this);
            return;
        }
        int newi = findEventWithFilters(i, false);
        if (newi == -1 && i != 0)
            message("There are no events according to the selected criteria", this);
        else i = newi;
    }
    else i--;
    if (i >= 0) ui->sbEvent->setValue(i);
}

void OutputWindow::on_sbEvent_valueChanged(int i)
{
    if (EventsDataHub->Events.isEmpty())
        ui->sbEvent->setValue(0);
    else if (i >= EventsDataHub->Events.size())
        ui->sbEvent->setValue(EventsDataHub->Events.size()-1); //will retrigger this method
    else
    {
        QWidget * cw = ui->tabwinDiagnose->currentWidget();

        if (cw == ui->tabText && !bForbidUpdate) ShowOneEventLog(i);
        else if (cw == ui->tabPMhits || cw == ui->tabPmHitViz) on_pbRefreshViz_clicked();
        else if (cw == ui->tabEventViewer) EV_show();
    }
}

void OutputWindow::on_tabwinDiagnose_currentChanged(int)
{
    QWidget * cw = ui->tabwinDiagnose->currentWidget();
    bool bShowEventNum = false;

    if (cw == ui->tabText)
    {
        bShowEventNum = true;
    }
    else if (cw == ui->tabPMhits)
    {
        bShowEventNum = true;
    }
    else if (cw == ui->tabPmHitViz)
    {
        bShowEventNum = true;
        gvOut->update();
    }
    else if (cw == ui->tabEventViewer)
    {
        bShowEventNum = true;
        //EV_showTree();
    }
    else if (cw == ui->tabParticleLog)
    {

    }
    else if (cw == ui->tabPhotonLog)
    {

    }
    else if (cw == ui->tabMonitors)
    {

    }

    ui->frEventNumber->setVisible( bShowEventNum );
}

void OutputWindow::on_pbEventView_ShowTree_clicked()
{
    ExpandedItems.clear();
    int counter = 0;
    for (int i=0; i<ui->trwEventView->topLevelItemCount(); i++)
    {
        QTreeWidgetItem * item = ui->trwEventView->topLevelItem(i);
        doProcessExpandedStatus(item, counter, true);
    }

    EV_showTree();

    for (int i=0; i<ui->trwEventView->topLevelItemCount(); i++)
    {
        QTreeWidgetItem * item = ui->trwEventView->topLevelItem(i);
        doProcessExpandedStatus(item, counter, false);
    }
}

void OutputWindow::doProcessExpandedStatus(QTreeWidgetItem * item, int & counter, bool bStore)
{
    if (bStore)
    {
        ExpandedItems << item->isExpanded();
        for (int i=0; i<item->childCount(); i++)
            doProcessExpandedStatus(item->child(i), counter, bStore);
    }
    else
    {
        if (counter >= ExpandedItems.size()) return; // not expected
        if (ExpandedItems.at(counter)) ui->trwEventView->expandItem(item);
        else ui->trwEventView->collapseItem(item);
        counter++;
        for (int i=0; i<item->childCount(); i++)
            doProcessExpandedStatus(item->child(i), counter, bStore);
    }
}

void OutputWindow::on_pbEVgeo_clicked()
{
    EV_showGeo();
}

void OutputWindow::on_sbEVexpansionLevel_valueChanged(int)
{
    EV_showTree();
}

#include <QMenu>
#include "TGraph.h"
void OutputWindow::on_trwEventView_customContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem * item = ui->trwEventView->currentItem();
    if (!item) return;

    AParticleTrackingRecord * pr = nullptr;
    QString s = item->text(1);
    if (!s.isEmpty())
    {
        qlonglong sp = s.toLongLong();
        pr = reinterpret_cast<AParticleTrackingRecord*>(sp);
    }
    ATrackingStepData * st = nullptr;
    s = item->text(2);
    if (!s.isEmpty())
    {
        qlonglong sp = s.toLongLong();
        st = reinterpret_cast<ATrackingStepData*>(sp);
    }

    if (!pr) return;

    QMenu Menu;
    QAction * showPosition;
    if (st) showPosition = Menu.addAction("Show position");
    Menu.addSeparator();
    QAction * showELDD = Menu.addAction("Show energy linear deposition density");
    QAction* selectedItem = Menu.exec(ui->trwEventView->mapToGlobal(pos));
    if (!selectedItem) return; //nothing was selected
    if (selectedItem == showELDD)
    {
        std::vector<float> dist;
        std::vector<float> ELDD;
        pr->fillELDD(st, dist, ELDD);

        if (!dist.empty())
        {
            TGraph * g = MW->GraphWindow->ConstructTGraph(dist, ELDD, "Deposited energy: linear density", "Distance, mm", "Linear density, keV/mm", 4, 20, 1, 4);
            MW->GraphWindow->Draw(g, "APL");
            //MW->GraphWindow->UpdateRootCanvas();
        }
    }
    else if (selectedItem == showPosition)
    {
        double pos[3];
        for (int i=0; i<3; i++) pos[i] = st->Position[i];
        MW->GeometryWindow->ShowPoint(pos, true);
    }
}

void OutputWindow::on_cbEVhideTrans_clicked()
{
    EV_showTree();
}

void OutputWindow::on_cbEVhideTransPrim_clicked()
{
    EV_showTree();
}

void OutputWindow::on_sbMonitorIndex_editingFinished()
{
    int mon = ui->sbMonitorIndex->value();
    if (mon >= ui->cobMonitor->count()) mon = 0;
    ui->sbMonitorIndex->setValue(mon);
    if (mon < ui->cobMonitor->count()) ui->cobMonitor->setCurrentIndex(mon); //protection: can be empty
    updateMonitors();
}

void OutputWindow::on_pbNextMonitor_clicked()
{
    int mon = ui->cobMonitor->currentIndex();
    mon++;
    if (mon >= ui->cobMonitor->count()) mon = 0;
    ui->sbMonitorIndex->setValue(mon);
    if (mon < ui->cobMonitor->count()) ui->cobMonitor->setCurrentIndex(mon); //protection: can be empty
    updateMonitors();
}
