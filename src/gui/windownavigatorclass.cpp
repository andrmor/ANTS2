#include "windownavigatorclass.h"
#include "ui_windownavigatorclass.h"
#include "mainwindow.h"
#include "outputwindow.h"
#include "reconstructionwindow.h"
#include "materialinspectorwindow.h"
#include "lrfwindow.h"
#include "alrfwindow.h"
#include "graphwindowclass.h"
#include "geometrywindowclass.h"
#include "exampleswindow.h"
#include "gainevaluatorwindowclass.h"
#include "genericscriptwindowclass.h"
#include "globalsettingsclass.h"
#include "ascriptwindow.h"

#include <QDebug>
#include <QTime>

#ifdef Q_OS_WIN32
#include <QtWinExtras>
#endif


WindowNavigatorClass::WindowNavigatorClass(QWidget *parent, MainWindow *mw) :
  QMainWindow(parent),
  ui(new Ui::WindowNavigatorClass)
{
  ui->setupUi(this);
  this->setFixedSize(this->size());
  MW = mw;

  Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
  windowFlags |= Qt::WindowCloseButtonHint;
  windowFlags |= Qt::WindowStaysOnTopHint;
  this->setWindowFlags( windowFlags );

  MainOn = false;
  ReconOn = false;
  OutOn = false;
  MatOn = false;
  ExamplesOn = false;
  LRFon = false;
  newLRFon = false;
  GeometryOn = false;
  GraphOn = false;
  ScriptOn = false;
  MainChangeExplicitlyRequested = false;

  time = new QTime();

  DisableBSupdate = false;

//  this->setMinimumWidth(50);

// QDesktopWidget *desktop = QApplication::desktop();
//  int ix = desktop->availableGeometry(MW).width(); //use the same screen where is the main window (one can use screen index directly, -1=default)
//  this->move(ix - 120, 50);

#ifdef Q_OS_WIN32
  taskButton = 0;
  QTimer::singleShot(1000,this,SLOT(SetupWindowsTaskbar())); //without signal/slot it does not work somehow...

  QWinJumpList* jumplist = new QWinJumpList(this);
  //QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)+"/ants2";
  jumplist->tasks()->addLink(QString("Base dir"), QDir::toNativeSeparators(MW->GlobSet->AntsBaseDir));
  jumplist->tasks()->addLink(QString("Last working dir"), QDir::toNativeSeparators(MW->GlobSet->LastOpenDir));
  jumplist->tasks()->addLink(QString("Script dir"), QDir::toNativeSeparators(MW->GlobSet->LibScripts));
  //jumplist->tasks()->addSeparator();
  jumplist->tasks()->setVisible(true);
#endif
}

WindowNavigatorClass::~WindowNavigatorClass()
{
  if (time) delete time;
  delete ui;
}

void WindowNavigatorClass::SetupWindowsTaskbar()
{
#ifdef Q_OS_WIN32
  taskButton = new QWinTaskbarButton(MW);
  taskButton->setWindow(MW->windowHandle());
  taskProgress = taskButton->progress();
  taskProgress->setVisible(false);
  taskProgress->setValue(0);

  QWidget *widget = MW;
  QWinThumbnailToolBar *thumbbar = new QWinThumbnailToolBar(widget);
  thumbbar->setWindow(widget->windowHandle());

  QWinThumbnailToolButton *maxAll = new QWinThumbnailToolButton(thumbbar);
  maxAll->setToolTip("Show all active");
  maxAll->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
  maxAll->setDismissOnClick(true);
  connect(maxAll, SIGNAL(clicked()), this, SLOT(on_pbMaxAll_clicked()));

  QWinThumbnailToolButton *Main = new QWinThumbnailToolButton(thumbbar);
  Main->setToolTip("Main window");
  QPixmap pix(20, 20);
  pix.fill(Qt::transparent);
  QPainter painter( &pix );
  painter.setFont( QFont("Arial", 15) );
  painter.drawText( 2, 17, "M" );
  Main->setIcon(QIcon(pix));
  connect(Main, SIGNAL(clicked()), this, SLOT(on_pbMain_clicked()));

  QWinThumbnailToolButton *Rec = new QWinThumbnailToolButton(thumbbar);
  Rec->setToolTip("Reconstruction window");
  pix.fill(Qt::transparent);
  painter.drawText( 4, 17, "R" );
  Rec->setIcon(QIcon(pix));
  connect(Rec, SIGNAL(clicked()), this, SLOT(on_pbRecon_clicked()));

  QWinThumbnailToolButton *Out = new QWinThumbnailToolButton(thumbbar);
  Out->setToolTip("Output window");
  pix.fill(Qt::transparent);
  painter.drawText( 2, 17, "O" );
  Out->setIcon(QIcon(pix));
  connect(Out, SIGNAL(clicked()), this, SLOT(on_pbOut_clicked()));

  QWinThumbnailToolButton *Geo = new QWinThumbnailToolButton(thumbbar);
  Geo->setToolTip("Geometry window");
  pix.fill(Qt::transparent);
  painter.drawText( 2, 17, "G" );
  Geo->setIcon(QIcon(pix));
  connect(Geo, SIGNAL(clicked()), this, SLOT(on_pbGeometry_clicked()));

//  QWinThumbnailToolButton *lrf = new QWinThumbnailToolButton(thumbbar);
//  lrf->setToolTip("LRF window");
//  pix.fill(Qt::transparent);
//  painter.drawText( 4, 17, "L" );
//  lrf->setIcon(QIcon(pix));
//  connect(lrf, SIGNAL(clicked()), this, SLOT(on_pbLRF_clicked()));

  QWinThumbnailToolButton *scr = new QWinThumbnailToolButton(thumbbar);
  scr->setToolTip("Script window");
  pix.fill(Qt::transparent);
  painter.drawText( 4, 17, "S" );
  scr->setIcon(QIcon(pix));
  connect(scr, SIGNAL(clicked()), this, SLOT(on_pbScript_clicked()));

  QWinThumbnailToolButton *ex = new QWinThumbnailToolButton(thumbbar);
  ex->setToolTip("Examples/Load window");
  pix.fill(Qt::transparent);
  painter.drawText( 4, 17, "E" );
  ex->setIcon(QIcon(pix));
  connect(ex, SIGNAL(clicked()), this, SLOT(on_pbExamples_clicked()));

  thumbbar->addButton(maxAll);
  thumbbar->addButton(ex);
  thumbbar->addButton(Main);
  thumbbar->addButton(Rec);
  thumbbar->addButton(Geo);
  thumbbar->addButton(Out);  
//  thumbbar->addButton(lrf);
  thumbbar->addButton(scr);

#endif
}

void WindowNavigatorClass::setProgress(int percent)
{
  ui->pb->setValue(percent);

#ifdef Q_OS_WIN32
  if (taskButton)
    {
      taskProgress->setVisible(true);
      taskProgress->setValue(percent);
    }
#endif
}

void WindowNavigatorClass::HideWindowTriggered(QString w)
{
  if (DisableBSupdate) return;

  if (w == "main") MainOn = false;
  if (w == "recon") ReconOn = false;
  if (w == "out") OutOn = false;
  if (w == "mat") MatOn = false;
  if (w == "examples") ExamplesOn = false;
  if (w == "lrf") LRFon = false;
  if (w == "newLrf") newLRFon = false;
  if (w == "geometry") GeometryOn = false;
  if (w == "graph") GraphOn = false;
  if (w == "script") ScriptOn = false;

  ui->pbMain->setChecked(MainOn);
  ui->pbRecon->setChecked(ReconOn);
  ui->pbOut->setChecked(OutOn);
  ui->pbMaterials->setChecked(MatOn);
  ui->pbExamples->setChecked(ExamplesOn);
  ui->pbLRF->setChecked(LRFon);
  ui->pbNewLRF->setChecked(newLRFon);
  ui->pbGeometry->setChecked(GeometryOn);
  ui->pbGraph->setChecked(GraphOn);
  ui->pbScript->setChecked(ScriptOn);
}

void WindowNavigatorClass::ShowWindowTriggered(QString w)
{
  if (w == "main") MainOn = true;
  if (w == "recon") ReconOn = true;
  if (w == "out") OutOn = true;
  if (w == "mat") MatOn = true;
  if (w == "examples") ExamplesOn = true;
  if (w == "lrf") LRFon = true;
  if (w == "newLrf") newLRFon = true;
  if (w == "geometry") GeometryOn = true;
  if (w == "graph") GraphOn = true;
  if (w == "script") ScriptOn = true;

  ui->pbMain->setChecked(MainOn);
  ui->pbRecon->setChecked(ReconOn);
  ui->pbOut->setChecked(OutOn);
  ui->pbMaterials->setChecked(MatOn);
  ui->pbExamples->setChecked(ExamplesOn);
  ui->pbLRF->setChecked(LRFon);
  ui->pbNewLRF->setChecked(newLRFon);
  ui->pbGeometry->setChecked(GeometryOn);
  ui->pbGraph->setChecked(GraphOn);
  ui->pbScript->setChecked(ScriptOn);
}

void WindowNavigatorClass::ResetAllProgressBars()
{
  MW->SetProgress(0);
  setProgress(0);
  MW->Rwindow->SetProgress(0);
  MW->lrfwindow->SetProgress(0);

#ifdef Q_OS_WIN32
  if (taskButton)
    {
      taskProgress->setVisible(false);
      taskProgress->setValue(0);
    }
#endif
}

void WindowNavigatorClass::TriggerHideButton()
{
  WindowNavigatorClass::on_pbMinAll_clicked();
}

void WindowNavigatorClass::TriggerShowButton()
{
  WindowNavigatorClass::on_pbMaxAll_clicked();
}

void WindowNavigatorClass::BusyOn()
{  
  MW->stopRootUpdate(); //--//

  MW->onBusyOn();
  MW->Rwindow->onBusyOn();
  MW->lrfwindow->onBusyOn();
  MW->MIwindow->setEnabled(false);
  MW->ELwindow->setEnabled(false);
  if (MW->GainWindow) MW->GainWindow->setEnabled(false);
  if (MW->GraphWindow) MW->GraphWindow->OnBusyOn();
  if (MW->GeometryWindow) MW->GeometryWindow->onBusyOn();
  MW->Owindow->setEnabled(false);

  emit BusyStatusChanged(true);

  time->restart();
}

void WindowNavigatorClass::BusyOff(bool fShowTime)
{
  int runtime = time->elapsed(); //gets the runtime in ms
  if (fShowTime)
    {
      QString str;
      str.setNum(runtime);
      MW->Owindow->OutText("-=-=-=-=-=-=-=- done: elapsed time: "+str+" ms");
      MW->Owindow->OutText("");
    }

  //main window
  MW->onBusyOff();

  //reconstruction window
  //MW->Rwindow->setEnabled(true);
  MW->Rwindow->onBusyOff();

  //lrf window
  MW->lrfwindow->onBusyOff();

  //material inspector
  MW->MIwindow->setEnabled(true);

  //examples
  MW->ELwindow->setEnabled(true);

  //gain evaluator
  if (MW->GainWindow) MW->GainWindow->setEnabled(true);

  //geometry window
  if (MW->GraphWindow) MW->GraphWindow->OnBusyOff();

  if (MW->GeometryWindow) MW->GeometryWindow->onBusyOff();

  //output window
  MW->Owindow->setEnabled(true);

  emit BusyStatusChanged(false);


  //MW->DisableRootUpdate = false; //--//
  MW->startRootUpdate(); //--//
}

void WindowNavigatorClass::on_pbMaxAll_clicked()
{
  this->activateWindow();
  //qDebug()<<"---MAX ALL---";

  MainChangeExplicitlyRequested = true;

  if (MainOn)
    {
      MW->showNormal();
      MW->raise();
    }
  if (ReconOn)
    {
      MW->Rwindow->showNormal();
      MW->Rwindow->raise();
    }
  if (OutOn)
    {
      MW->Owindow->showNormal();
      MW->Owindow->raise();
    }
  if (MatOn)
    {
      MW->MIwindow->showNormal();
      MW->MIwindow->raise();
    }
  if (ExamplesOn)
    {
      MW->ELwindow->showNormal();
      MW->ELwindow->raise();
    }
  if (LRFon)
    {
      MW->lrfwindow->showNormal();
      MW->lrfwindow->raise();
    }
  if (newLRFon)
    {
      MW->newLrfWindow->showNormal();
      MW->newLrfWindow->raise();
    }
  if (GeometryOn)
    {
      MW->GeometryWindow->showNormal();
      MW->GeometryWindow->raise();
    }
  if (GraphOn)
    if (MW->GraphWindow)
    {
      MW->GraphWindow->showNormal();
      MW->GraphWindow->raise();
    }
  if (ScriptOn)
    if (MW->ScriptWindow)
    {
      MW->ScriptWindow->showNormal();
      MW->ScriptWindow->raise();
    }

  MainChangeExplicitlyRequested = false;
}

void WindowNavigatorClass::ChangeGuiBusyStatus(bool flag)
{
    if (flag) BusyOn();
    else BusyOff();
}

void WindowNavigatorClass::on_pbMinAll_clicked()
{
  MainChangeExplicitlyRequested = true;
  DisableBSupdate = true;

  QList<QMainWindow*> list;
  list.clear();
  list<<MW->Owindow<<MW->Rwindow<<MW->lrfwindow<<MW->newLrfWindow<<MW->MIwindow<<MW->ELwindow<<MW->GraphWindow<<MW->GeometryWindow<<MW->ScriptWindow;
  foreach (QMainWindow* win, list) win->hide();
  if (MW->GenScriptWindow) MW->GenScriptWindow->hide();
  if (MW->GainWindow) MW->GainWindow->hide();
  MW->showMinimized();

  DisableBSupdate = false;
  MainChangeExplicitlyRequested = false;
}

void WindowNavigatorClass::on_pbMain_clicked()
{
   MainChangeExplicitlyRequested = true;
   if ( MW->isHidden() || MW->isMinimized() || (!MainOn) )
     {
       MW->showNormal();
       MW->raise();
     }
   else MW->showMinimized();//MW->hide();
   MainChangeExplicitlyRequested = false;  
}

void WindowNavigatorClass::on_pbRecon_clicked()
{
  if ( MW->Rwindow->isHidden() || MW->Rwindow->isMinimized() || (!ReconOn) )
    {
      MW->Rwindow->showNormal();
      MW->Rwindow->raise();
    }
  else MW->Rwindow->hide();
}

void WindowNavigatorClass::on_pbOut_clicked()
{
  if ( MW->Owindow->isHidden() || MW->Owindow->isMinimized() || (!OutOn) )
    {
      MW->Owindow->showNormal();
      MW->Owindow->raise();
    }
  else MW->Owindow->hide();
}

void WindowNavigatorClass::on_pbLRF_clicked()
{
  if ( MW->lrfwindow->isHidden() || MW->lrfwindow->isMinimized() || (!LRFon) )
    {
      MW->lrfwindow->showNormal();
      MW->lrfwindow->raise();
    }
  else MW->lrfwindow->hide();
}

void WindowNavigatorClass::on_pbNewLRF_clicked()
{
  if ( MW->newLrfWindow->isHidden() || MW->newLrfWindow->isMinimized() || (!newLRFon) )
    {
        MW->newLrfWindow->showNormal();
        MW->newLrfWindow->raise();
    }
  else MW->newLrfWindow->hide();
}

void WindowNavigatorClass::on_pbGeometry_clicked()
{
  if ( MW->GeometryWindow->isHidden() || MW->GeometryWindow->isMinimized() || (!GeometryOn) )
    {
      MW->GeometryWindow->showNormal();
      MW->GeometryWindow->raise();
      MW->ShowGeometry();
      MW->ShowTracks();
    }
  else MW->GeometryWindow->hide();

  /*
  if (!MW->geometryRW) return;

 // if ( !MW->geometryRW->isVisible() || MW->geometryRW->windowState() == Qt::WindowMinimized || (!GeometryOn) )
  if ( !MW->geometryRW->isExposed() || (!GeometryOn) )
    {
      MW->geometryRW->showNormal();
      MW->geometryRW->raise();

      MW->GWaddon->showNormal();
      MW->GWaddon->raise();
      MW->GWaddon->MoveHome();
    }
  else MW->geometryRW->hide();
  */
}

void WindowNavigatorClass::on_pbGraph_clicked()
{
  if ( MW->GraphWindow->isHidden() || MW->GraphWindow->isMinimized() || (!GraphOn) )
    {
      MW->GraphWindow->showNormal();
      MW->GraphWindow->raise();
    }
  else MW->GraphWindow->hide();

  /*
  if (!MW->graphRW) return;

 // if ( !MW->graphRW->isVisible() || MW->graphRW->windowState() == Qt::WindowMinimized || (!GraphOn) )
  if ( !MW->graphRW->isExposed() || (!GraphOn) )
    {
      MW->graphRW->showNormal();
      MW->graphRW->raise();
    }
  else MW->graphRW->hide();
  */
}

void WindowNavigatorClass::on_pbMaterials_clicked()
{
  if ( MW->MIwindow->isHidden() || MW->MIwindow->isMinimized() || (!MatOn) )
    {
      MW->MIwindow->showNormal();
      MW->MIwindow->raise();
    }
  else MW->MIwindow->hide();  
}

void WindowNavigatorClass::on_pbExamples_clicked()
{
  if ( !MW->ELwindow->isVisible() || MW->ELwindow->isHidden() || MW->ELwindow->isMinimized() || (!ExamplesOn) )
    {
      MW->ELwindow->showNormal();
      MW->ELwindow->raise();
      MW->ELwindow->activateWindow();
    }
  else MW->ELwindow->hide();
}

void WindowNavigatorClass::on_pbScript_clicked()
{
    if ( MW->ScriptWindow->isHidden() || MW->ScriptWindow->isMinimized() || (!ScriptOn) )
      {
        MW->ScriptWindow->showNormal();
        MW->ScriptWindow->raise();
      }
    else MW->ScriptWindow->hide();
}

void WindowNavigatorClass::closeEvent(QCloseEvent *)
{
   MW->showNormal();
}

