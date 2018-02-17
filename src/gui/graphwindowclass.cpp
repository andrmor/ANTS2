//has to be on top!!
#include "TCanvas.h"

//ANTS2
#include "graphwindowclass.h"
#include "ui_graphwindowclass.h"
#include "mainwindow.h"
#include "rasterwindowgraphclass.h"
#include "windownavigatorclass.h"
#include "globalsettingsclass.h"
#include "amessage.h"
#include "afiletools.h"
#include "shapeablerectitem.h"
#include "graphicsruler.h"
#include "arootlineconfigurator.h"
#include "arootmarkerconfigurator.h"
#include "atoolboxscene.h"

//Qt
#include <QtGui>
#include <QFileDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QColorDialog>
#include <QListWidget>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QGraphicsSceneMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>

//Root
#include "TGraph.h"
#include "TGraph2D.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "TH1D.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TList.h"
#include "TF1.h"
#include "TF2.h"
#include "TMath.h"
#include "TView.h"
#include "TFrame.h"
#include "TMultiGraph.h"
#include "TGraphErrors.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TStyle.h"
#include "TEllipse.h"
#include "TPolyLine.h"
#include "TLine.h"
#include "TFile.h"
#include "TKey.h"
#include "TAxis.h"
#include "TView3D.h"
#include "TViewer3DPad.h"
#include "TAttLine.h"
#include "TLegend.h"
#include "TVectorD.h"
//#include "TROOT.h"

#include "TPave.h"
#include "TPaveLabel.h"
#include "TPavesText.h"

GraphWindowClass::GraphWindowClass(QWidget *parent, MainWindow* mw) :
  QMainWindow(parent),
  ui(new Ui::GraphWindowClass)
{ 
  //inits
  RasterWindow = 0;
  QWinContainer = 0;
  TG_X0 = 0, TG_Y0 = 0;   
  TMPignore = false;
  DrawObjects.clear();
  MW = mw;
  ColdStart = true;
  ExtractionCanceled = false;
  gvOver = 0;  
  CurrentBasketItem = -1;
  BasketMode = 0;
  fFirstTime = false;
  LastOptStat = 1111;

  hProjection = 0;

  //setting UI
  ui->setupUi(this);
  this->setMinimumWidth(200);
  ui->swToolBox->setVisible(false);
  ui->swToolBox->setCurrentIndex(0);
  ui->sProjBins->setEnabled(false);

  //window flags
  Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
  windowFlags |= Qt::WindowCloseButtonHint;
  windowFlags |= Qt::WindowMinimizeButtonHint;
  windowFlags |= Qt::WindowMaximizeButtonHint;
  this->setWindowFlags( windowFlags );

  //input boxes format validators
  QDoubleValidator* dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);
  QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
  foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);
    //
  //QIntValidator* iv  = new QIntValidator(this);
  //iv->setBottom(1);
  //ui->leiBinsX->setValidator(iv);

  //starting QWindow
  RasterWindow = new RasterWindowGraphClass(this);
  RasterWindow->resize(400, 400);
  RasterWindow->ForceResize();

  //creating QWindow container and placing the raster window in it
  QWinContainer = QWidget::createWindowContainer(RasterWindow, this);

  QMargins margins = RasterWindow->frameMargins();
  QWinContainer->setGeometry(ui->fUIbox->x() + ui->fUIbox->width() + 3 + margins.left(), 3 + margins.top(), 600, 600);
  QWinContainer->setVisible(true);

  //connecting signals-slots
  connect(RasterWindow, &RasterWindowGraphClass::LeftMouseButtonReleased, this, &GraphWindowClass::UpdateControls);

  //overlay to show selection box, later scale tool too
  gvOver = new QGraphicsView(this);
  gvOver->setFrameStyle(0);
  gvOver->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  gvOver->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  scene = new AToolboxScene(this);
  gvOver->setScene(scene);
  gvOver->hide();

  //toolbox graphics scene
  connect(scene->getSelBox(), &ShapeableRectItem::geometryChanged, this, &GraphWindowClass::selBoxGeometryChanged);
  connect(scene->getSelBox(), &ShapeableRectItem::requestResetGeometry, this, &GraphWindowClass::selBoxResetGeometry);
  connect(ui->cbSelBoxShowBG, &QCheckBox::toggled, scene->getSelBox(), &ShapeableRectItem::setShowContrast);
  connect(scene->getRuler(), &GraphicsRuler::geometryChanged, this, &GraphWindowClass::rulerGeometryChanged);
  connect(ui->cbRulerTicksLength, &QCheckBox::toggled, scene->getRuler(), &GraphicsRuler::setShowTicks);
  connect(ui->cbRulerShowBG, &QCheckBox::toggled, scene->getRuler(), &GraphicsRuler::setShowContrast);

  ui->fBasket->setVisible(false);
}

GraphWindowClass::~GraphWindowClass()
{
  ClearBasket();

  delete ui;

  clearTmpTObjects();

  delete scene;
  delete gvOver;

  //RasterWindow->setParent(0);
  //delete RasterWindow;
  //delete QWinContainer;
}

TGraph* GraphWindowClass::MakeGraph(const QVector<double> *x, const QVector<double> *y,
                                    Color_t color, const char *XTitle, const char *YTitle,
                                    int MarkerStyle, int MarkerSize,
                                    int LineStyle, int LineWidth,
                                    const char* options,
                                    bool OnlyBuild)
{
    int numEl = x->size();
    TVectorD xx(numEl);
    TVectorD yy(numEl);
    for (int i=0; i < numEl; i++)
    {
        xx[i] = x->at(i);
        yy[i] = y->at(i);
    }

    TGraph* gr = new TGraph(xx,yy);
    gr->SetTitle(""); gr->GetXaxis()->SetTitle(XTitle); gr->GetYaxis()->SetTitle(YTitle);
    gr->SetMarkerStyle(MarkerStyle); gr->SetMarkerColor(color); gr->SetMarkerSize(MarkerSize);
    gr->SetEditable(false); gr->GetYaxis()->SetTitleOffset((Float_t)1.30);
    gr->SetLineWidth(LineWidth); gr->SetLineColor(color); gr->SetLineStyle(LineStyle);
    gr->SetFillStyle(0);
    gr->SetFillColor(0);

    //in the case of OnlyBuild return the graph (no drawing)
    if (OnlyBuild) return gr; //gr is not registered!

    //drawing the graph
    QString opts = options;
    if (opts.contains("same",Qt::CaseInsensitive))
    {
        if (LineWidth == 0) GraphWindowClass::Draw(gr, "P");
        else GraphWindowClass::Draw(gr, "PL");
    }
    else
    {
        if (LineWidth == 0) GraphWindowClass::Draw(gr, "AP");
        else GraphWindowClass::Draw(gr, "APL");
    }

    return 0;
}

TGraph *GraphWindowClass::ConstructTGraph(const QVector<double> x, const QVector<double> y) const
{
  int numEl = x.size();
  TVectorD xx(numEl);
  TVectorD yy(numEl);
  for (int i=0; i < numEl; i++)
  {
      xx[i] = x.at(i);
      yy[i] = y.at(i);
  }

  TGraph* gr = new TGraph(xx,yy);
  gr->SetFillStyle(0);
  gr->SetFillColor(0);
  return gr;
}

TGraph *GraphWindowClass::ConstructTGraph(const QVector<double> x, const QVector<double> y,
                                          const char *Title, const char *XTitle, const char *YTitle,
                                          Color_t MarkerColor, int MarkerStyle, int MarkerSize,
                                          Color_t LineColor, int LineStyle, int LineWidth) const
{
    TGraph* gr = ConstructTGraph(x,y);
    gr->SetTitle(Title); gr->GetXaxis()->SetTitle(XTitle); gr->GetYaxis()->SetTitle(YTitle);
    gr->SetMarkerStyle(MarkerStyle); gr->SetMarkerColor(MarkerColor); gr->SetMarkerSize(MarkerSize);
    gr->SetEditable(false); gr->GetYaxis()->SetTitleOffset((Float_t)1.30);
    gr->SetLineWidth(LineWidth); gr->SetLineColor(LineColor); gr->SetLineStyle(LineStyle);
    return gr;
}

TGraph2D *GraphWindowClass::ConstructTGraph2D(const QVector<double> x, const QVector<double> y, const QVector<double> z) const
{
    int numEl = x.size();
    TGraph2D* gr = new TGraph2D(numEl, (double*)x.data(), (double*)y.data(), (double*)z.data());
    gr->SetFillStyle(0);
    gr->SetFillColor(0);
    return gr;
}

TGraph2D *GraphWindowClass::ConstructTGraph2D(const QVector<double> x, const QVector<double> y, const QVector<double> z,
                                            const char *Title, const char *XTitle, const char *YTitle, const char *ZTitle,
                                            Color_t MarkerColor, int MarkerStyle, int MarkerSize,
                                            Color_t LineColor, int LineStyle, int LineWidth)
{
    TGraph2D* gr = ConstructTGraph2D(x,y,z);
    gr->SetTitle(Title); gr->GetXaxis()->SetTitle(XTitle); gr->GetYaxis()->SetTitle(YTitle); gr->GetZaxis()->SetTitle(ZTitle);
    gr->SetMarkerStyle(MarkerStyle); gr->SetMarkerColor(MarkerColor); gr->SetMarkerSize(MarkerSize);
    gr->GetYaxis()->SetTitleOffset((Float_t)1.30);
    gr->SetLineWidth(LineWidth); gr->SetLineColor(LineColor); gr->SetLineStyle(LineStyle);
    return gr;
}

void GraphWindowClass::ShowAndFocus()
{
  RasterWindow->fCanvas->cd();
  this->show();
  this->activateWindow();
  this->raise();

  if (ColdStart)
    {
      //first time this window is shown
      ColdStart = false;
      this->resize(width()+1, height());
      this->resize(width()-1, height());
    }
}

void GraphWindowClass::SetAsActiveRootWindow()
{
  RasterWindow->fCanvas->cd();
}

void GraphWindowClass::ClearRootCanvas()
{
  RasterWindow->fCanvas->Clear();
}

void GraphWindowClass::UpdateRootCanvas()
{
  //RasterWindow->fCanvas->Modified();
  RasterWindow->fCanvas->Update();
}

void GraphWindowClass::SetModifiedFlag()
{
  RasterWindow->fCanvas->Modified();
}

void GraphWindowClass::SetLog(bool X, bool Y)
{
  ui->cbLogX->setChecked(X);
  ui->cbLogY->setChecked(Y);
}

void GraphWindowClass::ClearDrawObjects_OnShutDown()
{
  DrawObjects.clear();  
 // ui->pbHideBar->setFocus(); //to avoid triggering "on edit finished"
  RasterWindow->fCanvas->Clear();
}

double GraphWindowClass::getCanvasMinX()
{
  return RasterWindow->getCanvasMinX();
}

double GraphWindowClass::getCanvasMaxX()
{
  return RasterWindow->getCanvasMaxX();
}

double GraphWindowClass::getCanvasMinY()
{
  return RasterWindow->getCanvasMinY();
}

double GraphWindowClass::getCanvasMaxY()
{
    return RasterWindow->getCanvasMaxY();
}

double GraphWindowClass::getMinX(bool *ok = 0)
{
    if (!ui->ledXfrom->isEnabled()) {
        if(ok) *ok = false;
        return 0.0;
    }
    return ui->ledXfrom->text().toDouble(ok);
}

double GraphWindowClass::getMaxX(bool *ok = 0)
{
    if (!ui->ledXto->isEnabled()) {
        if(ok) *ok = false;
        return 0.0;
    }
    return ui->ledXto->text().toDouble(ok);
}

double GraphWindowClass::getMinY(bool *ok = 0)
{
    if (!ui->ledYfrom->isEnabled()) {
        if(ok) *ok = false;
        return 0.0;
    }
    return ui->ledYfrom->text().toDouble(ok);
}

double GraphWindowClass::getMaxY(bool *ok = 0)
{
    if (!ui->ledYto->isEnabled()) {
        if(ok) *ok = false;
        return 0.0;
    }
    return ui->ledYto->text().toDouble(ok);
}

double GraphWindowClass::getMinZ(bool *ok = 0)
{
    if (!ui->ledZfrom->isEnabled()) {
        if(ok) *ok = false;
        return 0.0;
    }
    return ui->ledZfrom->text().toDouble(ok);
}

double GraphWindowClass::getMaxZ(bool *ok = 0)
{
    if (!ui->ledZto->isEnabled()) {
        if(ok) *ok = false;
        return 0.0;
    }
    return ui->ledZto->text().toDouble(ok);
}

bool GraphWindowClass::IsExtractionComplete()
{
  return RasterWindow->IsExtractionComplete();
}

void GraphWindowClass::ExtractX()
{
  ExtractionCanceled = false;
  RasterWindow->ExtractX();
}

void GraphWindowClass::Extract2DLine()
{
  ExtractionCanceled = false;
  RasterWindow->Extract2DLine();
}

void GraphWindowClass::Extract2DEllipse()
{
  ExtractionCanceled = false;
  RasterWindow->Extract2DEllipse();
}

void GraphWindowClass::Extract2DBox()
{
  ExtractionCanceled = false;
  RasterWindow->Extract2DBox();
}

void GraphWindowClass::Extract2DPolygon()
{
  ExtractionCanceled = false;
  RasterWindow->Extract2DPolygon();
}

double GraphWindowClass::extractedX()
{
  return RasterWindow->extractedX;
}

double GraphWindowClass::extracted2DLineA()
{
  return RasterWindow->extracted2DLineA;
}

double GraphWindowClass::extracted2DLineB()
{
  return RasterWindow->extracted2DLineB;
}

double GraphWindowClass::extracted2DLineC()
{
    return RasterWindow->extracted2DLineC;
}

double GraphWindowClass::extracted2DLineXstart()
{
    return RasterWindow->Line2DstartX;
}

double GraphWindowClass::extracted2DLineXstop()
{
    return RasterWindow->Line2DstopX;
}

double GraphWindowClass::extracted2DEllipseX()
{
  return RasterWindow->extracted2DEllipseX;
}

double GraphWindowClass::extracted2DEllipseY()
{
  return RasterWindow->extracted2DEllipseY;
}

double GraphWindowClass::extracted2DEllipseR1()
{
  return RasterWindow->extracted2DEllipseR1;
}

double GraphWindowClass::extracted2DEllipseR2()
{
  return RasterWindow->extracted2DEllipseR2;
}

double GraphWindowClass::extracted2DEllipseTheta()
{
  return RasterWindow->extracted2DEllipseTheta;
}

double GraphWindowClass::extractedX1()
{
  return RasterWindow->extractedX1;
}

double GraphWindowClass::extractedY1()
{
  return RasterWindow->extractedY1;
}

double GraphWindowClass::extractedX2()
{
  return RasterWindow->extractedX2;
}

double GraphWindowClass::extractedY2()
{
  return RasterWindow->extractedY2;
}

QList<double> GraphWindowClass::extractedPolygon()
{
  return RasterWindow->extractedPolygon;
}

void GraphWindowClass::Draw(TObject *obj, const char *options, bool DoUpdate, bool RegisterObject)
{
  if (!RasterWindow) return;
  if (!RasterWindow->fCanvas) return;
  GraphWindowClass::ShowAndFocus();

  DrawWithoutFocus(obj, options, DoUpdate, RegisterObject);
}

void GraphWindowClass::DrawWithoutFocus(TObject *obj, const char *options, bool DoUpdate, bool RegisterObject)
{
  if (!RasterWindow) return;
  if (!RasterWindow->fCanvas) return;

  CurrentBasketItem = -1;
  ui->lwBasket->clearSelection();
  fFirstTime = true;

  QString opt = options;
  if (options == QString("")) opt = "";

  QString ClassName = obj->ClassName();
    //qDebug()<<"      -->class_name:"<<ClassName<<" object name:"<<obj->GetName()<<" options (char):"<<options<<" options (QStr):"<<opt;

  if (opt.contains("same", Qt::CaseInsensitive))
    {
      // not the new main object!
//      qDebug()<<"same found!";
      DrawObjects.append(DrawObjectStructure(obj, options));
    }
  else
    {
      //This is Draw of the new main object!
        //delete all TObjects previously drawn
      clearTmpTObjects();
        //clear old record
      DrawObjects.clear();
        //register as the main
      DrawObjects.append(DrawObjectStructure(obj, options));

      //3D control
      bool flag3D = false;
      if (ClassName.startsWith("TH3") || ClassName.startsWith("TProfile2D") || ClassName.startsWith("TH2") || ClassName.startsWith("TF2") || ClassName.startsWith("TGraph2D"))
        flag3D = true;
      if ((ClassName.startsWith("TH2") || ClassName.startsWith("TProfile2D")) && ( opt.contains("col",Qt::CaseInsensitive) || opt.contains("prof", Qt::CaseInsensitive) || (opt == "")) )
        flag3D = false;
//      qDebug()<<"3D flag:"<<flag3D;

      ui->fZrange->setEnabled(flag3D);
      RasterWindow->setShowCursorPosition(!flag3D);
      ui->leOptions->setText(options);

      if ( ClassName.startsWith("TH1") || ClassName == "TF1" )
        {
          //enable toolbox; only the ruler
          ui->fToolBox->setEnabled(true);
          ui->fZrange->setEnabled(false);
          ui->cbRulerTicksLength->setChecked(false);
        }
      else if ( ClassName.startsWith("TH2") )
        {
          //enable toolbox - both ruler and projection box
          ui->fToolBox->setEnabled(true);
          ui->fZrange->setEnabled(true);
        }
      else
        {
          //hide toolbox
          ui->fToolBox->setEnabled(false);
        }

      //export setup
      if (ClassName == "TGraph" || ClassName.startsWith("TF") || ClassName.startsWith("TH2") )
        {
          ui->actionExport_data_as_text->setText("Export data as text");
          ui->actionExport_data_using_bin_start_positions_TH1->setText("--");
          ui->actionExport_data_as_text->setEnabled(true);
          ui->actionExport_data_using_bin_start_positions_TH1->setVisible(false);
        }
      else if (ClassName.startsWith("TH1"))
        {
          ui->actionExport_data_as_text->setText("Export data as text: bin center positions");
          ui->actionExport_data_using_bin_start_positions_TH1->setText("Export data as text: bin start positions");
          ui->actionExport_data_as_text->setEnabled(true);
          ui->actionExport_data_using_bin_start_positions_TH1->setVisible(true);
        }
      else
        {
          ui->actionExport_data_as_text->setText("Export data as text");
          ui->actionExport_data_using_bin_start_positions_TH1->setText("--");
          ui->actionExport_data_as_text->setEnabled(false);
          ui->actionExport_data_using_bin_start_positions_TH1->setVisible(false);
        }
      //Equalize XY
      if (ClassName.startsWith("TH2") || ClassName.startsWith("TF2") || ClassName.startsWith("TGraph2D"))
        ui->actionEqualize_scale_XY->setEnabled(true);
      else
        ui->actionEqualize_scale_XY->setEnabled(false);
    }

  GraphWindowClass::EnforceOverlayOff(); //maybe drawing was triggered when overlay is on and root window is invisible

  if (RegisterObject) RegisterTObject(obj);  //should be skipped only for scripts!

  GraphWindowClass::doDraw(obj, options, DoUpdate);

  if (CurrentBasketItem == -1)
    {
      if (RegisterObject) MasterDrawObjects = DrawObjects; //pointers are copied!
      else MasterDrawObjects.clear();
    }
  fFirstTime = false;

  //update range indication etc
  GraphWindowClass::UpdateControls();
}

void GraphWindowClass::RegisterTObject(TObject *obj)
{
    //qDebug() << "Registered:"<<obj<<obj->ClassName();
    tmpTObjects.append(obj);
}

void GraphWindowClass::doDraw(TObject *obj, const char *options, bool DoUpdate)
{
    //qDebug() << "-+-+ DoDraw";
  GraphWindowClass::SetAsActiveRootWindow();

  obj->Draw(options);

  if (DoUpdate) RasterWindow->fCanvas->Update();
}

void GraphWindowClass::updateLegendVisibility()
{  
  if (DrawObjects.isEmpty()) return;

  //qDebug() << "Updating legend";
  TObject* obj = DrawObjects.first().getPointer();
  QString PlotType = obj->ClassName();
  if (PlotType.startsWith("TH1")) ((TH1*) obj)->SetStats(ui->cbShowLegend->isChecked());
  if (PlotType.startsWith("TH2")) ((TH2*) obj)->SetStats(ui->cbShowLegend->isChecked());

  //RasterWindow->fCanvas->Modified();
  //RasterWindow->fCanvas->Update();
  //qDebug() << "update legend done";
}

void GraphWindowClass::startOverlayMode()
{
    if(gvOver->isVisible())
        return;

    QPixmap map = qApp->screens().first()->grabWindow(RasterWindow->winId());//QApplication::desktop()->winId());
    gvOver->setGeometry(QWinContainer->geometry());
    scene->setSceneRect(0, 0, QWinContainer->width(), QWinContainer->height());
    scene->setBackgroundBrush(map);
    QWinContainer->setVisible(false);// map.save("TestMap.png");

    QPointF origin;
    RasterWindow->PixelToXY(0, 0, origin.rx(), origin.ry());
    GraphicsRuler *ruler = scene->getRuler();
    ruler->setOrigin(origin);
    ruler->setScale(RasterWindow->getXperPixel(), RasterWindow->getYperPixel());

    scene->moveToolToVisible();
    setFixedSize(this->size());
    gvOver->show();
}

void GraphWindowClass::endOverlayMode()
{
    if (gvOver->isHidden())// || ui->cbShowRuler->isChecked() || ui->cbProjectionTool->isChecked())
        return;

    gvOver->hide();
    QWinContainer->setVisible(true);
    setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

void GraphWindowClass::OnBusyOn()
{
  ui->fUIbox->setEnabled(false);
  RasterWindow->setBlockEvents(true);
}

void GraphWindowClass::OnBusyOff()
{
  ui->fUIbox->setEnabled(true);
  RasterWindow->setBlockEvents(false);
}

void GraphWindowClass::switchOffBasket()
{
  ui->cbShowBasket->setChecked(false);
}

void GraphWindowClass::resizeEvent(QResizeEvent *)
{
  //tool bar box height and basket fit the window
  ui->fUIbox->resize(ui->fUIbox->width(), this->height() - 24 - 3);
  ui->fBasket->resize(ui->fBasket->width(), this->height());
  ui->lwBasket->resize(ui->lwBasket->width(), this->height()-ui->lwBasket->y()-3);

  int deltaBasket = 0;
  if (ui->cbShowBasket->isChecked()) deltaBasket = ui->fBasket->width();

  int width = this->width() - (3 + ui->fUIbox->width()) - deltaBasket;
  int height = this->height() - (3 + 3);
//  qDebug()<<width<<height;

  int mh = 0;
  if (ui->menuBar) mh =  ui->menuBar->height();
  if (QWinContainer) QWinContainer->setGeometry(ui->fUIbox->x() + ui->fUIbox->width()+3, mh, width, height);
  if (RasterWindow) RasterWindow->ForceResize();

  if (ui->cbShowBasket->isChecked()) ui->fBasket->move(this->width()-3-ui->fBasket->width(), 0);

//  if (QWinContainer && RasterWindow && gvOver)
  //    if (ui->cbProjectionTool->isChecked()) GraphWindowClass::on_pbPrepareOverlay_clicked();
}

void GraphWindowClass::mouseMoveEvent(QMouseEvent *event)
{
    if(QWinContainer->isVisible())
    {
        QMainWindow::mouseMoveEvent(event);
        return;
    }

    double x, y;
    QPoint mouseInGV = event->pos() - gvOver->pos();
    RasterWindow->PixelToXY(mouseInGV.x(), mouseInGV.y(), x, y);
    QString str = "Cursor coordinates: " + QString::number(x, 'g', 4);
    str += " : " + QString::number(y, 'g', 4);
    setWindowTitle(str);
    QMainWindow::mouseMoveEvent(event);
}

bool GraphWindowClass::event(QEvent *event)
{
  if (MW->WindowNavigator)
    {
      if (event->type() == QEvent::Hide) MW->WindowNavigator->HideWindowTriggered("graph");
      else if (event->type() == QEvent::Show) MW->WindowNavigator->ShowWindowTriggered("graph");
    }

  return QMainWindow::event(event);
}

void GraphWindowClass::closeEvent(QCloseEvent *)
{
  ExtractionCanceled = true;
  RasterWindow->setExtractionComplete(true);
  DrawObjects.clear();
  MasterDrawObjects.clear();
  RedrawAll();
  RasterWindow->setShowCursorPosition(false);
  LastDistributionShown = "";
}

void GraphWindowClass::on_cbGridX_toggled(bool checked)
{
  if (TMPignore) return;
  RasterWindow->fCanvas->SetGridx(checked);
  RasterWindow->fCanvas->Update();
}

void GraphWindowClass::on_cbGridY_toggled(bool checked)
{
  if (TMPignore) return;
  RasterWindow->fCanvas->SetGridy(checked);
  RasterWindow->fCanvas->Update();
}

void GraphWindowClass::on_cbLogX_toggled(bool checked)
{
  if (TMPignore) return;
  RasterWindow->fCanvas->SetLogx(checked);
  RasterWindow->fCanvas->Update();
}

void GraphWindowClass::on_cbLogY_toggled(bool checked)
{
  if (TMPignore) return;
  RasterWindow->fCanvas->SetLogy(checked);
  RasterWindow->fCanvas->Update();
}

void GraphWindowClass::on_ledXfrom_editingFinished()
{
  //ui->pbUnzoom->setFocus();
  if (xmin == ui->ledXfrom->text().toDouble()) return;
  GraphWindowClass::Reshape();
}

void GraphWindowClass::on_ledXto_editingFinished()
{
  //ui->pbUnzoom->setFocus();
  if (xmax == ui->ledXto->text().toDouble()) return;
  GraphWindowClass::Reshape();
}

void GraphWindowClass::on_ledYfrom_editingFinished()
{
  //ui->pbUnzoom->setFocus();
  if (ymin == ui->ledYfrom->text().toDouble()) return;
  GraphWindowClass::Reshape();
}

void GraphWindowClass::on_ledYto_editingFinished()
{
  //ui->pbUnzoom->setFocus();
  if (ymax == ui->ledYto->text().toDouble()) return;
  GraphWindowClass::Reshape();
}

void GraphWindowClass::on_ledZfrom_editingFinished()
{
  //ui->pbUnzoom->setFocus();
  if (zmin == ui->ledZfrom->text().toDouble()) return;
  GraphWindowClass::Reshape();
}

void GraphWindowClass::on_ledZto_editingFinished()
{
  //ui->pbUnzoom->setFocus();
  if (zmax == ui->ledZto->text().toDouble()) return;
  GraphWindowClass::Reshape();
}

TObject* GraphWindowClass::GetMainPlottedObject()
{
  if (DrawObjects.isEmpty()) return 0;

  return DrawObjects.first().getPointer();
}

void GraphWindowClass::Reshape()
{    
    //qDebug() << "Reshape triggered";
    qApp->processEvents();
    //ui->pbHideBar->setFocus(); //remove focus
//    qDebug()<<"GraphWindow  -> Reshape triggered; objects:"<<DrawObjects.size();

    //if (DrawObjects.isEmpty()) return;
    if (getCurrentDrawObjects()->isEmpty()) return;

    //double xmin, xmax, ymin, ymax, zmin, zmax;
    xmin = ui->ledXfrom->text().toDouble();
    xmax = ui->ledXto->text().toDouble();
    ymin = ui->ledYfrom->text().toDouble();
    ymax = ui->ledYto->text().toDouble();
    bool OKzmin;
    zmin = ui->ledZfrom->text().toDouble(&OKzmin);
    bool OKzmax;
    zmax = ui->ledZto->text().toDouble(&OKzmax);

    //Reshaping the main (first) object
    //QString PlotType = DrawObjects.first().getPointer()->ClassName();
    QString PlotType = getCurrentDrawObjects()->first().getPointer()->ClassName();
//    QString PlotOptions = DrawObjects.first().getOptions();
//    qDebug()<<"  main object name/options:"<<PlotType<<PlotOptions;

    if (PlotType.startsWith("TH1"))
      {
        //its 1D hist!
        //TH1* h = (TH1*) DrawObjects.first().getPointer();
        TH1* h = (TH1*) getCurrentDrawObjects()->first().getPointer();
        h->GetXaxis()->SetRangeUser(xmin, xmax);
        h->SetMinimum(ymin);
        h->SetMaximum(ymax);
      }
    else if (PlotType == "TProfile")
      {
        //its 1d profile
        //TProfile* h = (TProfile*) DrawObjects.first().getPointer();
        TProfile* h = (TProfile*) getCurrentDrawObjects()->first().getPointer();
        h->GetXaxis()->SetRangeUser(xmin, xmax);
        h->SetMinimum(ymin);
        h->SetMaximum(ymax);
      }
    else if (PlotType.startsWith("TH2"))
      {
        //its 2D hist!
        //TH2* h = (TH2*) DrawObjects.first().getPointer();
        TH2* h = (TH2*) getCurrentDrawObjects()->first().getPointer();
        h->GetXaxis()->SetRangeUser(xmin, xmax);
        h->GetYaxis()->SetRangeUser(ymin, ymax);

        if (OKzmin) h->SetMinimum(zmin);
        if (OKzmax) h->SetMaximum(zmax);
      }
    else if (PlotType == "TProfile2D")
      {
        //its 2D profile!
        //TProfile2D* h = (TProfile2D*) DrawObjects.first().getPointer();
        TProfile2D* h = (TProfile2D*) getCurrentDrawObjects()->first().getPointer();
        h->GetXaxis()->SetRangeUser(xmin, xmax);
        h->GetYaxis()->SetRangeUser(ymin, ymax);
      //  h->SetMinimum(zmin);
      //  h->SetMaximum(zmax);
      }
    else if (PlotType.startsWith("TF1"))
      {
        //its 1D function!
        //TF1* f = (TF1*) DrawObjects.first().getPointer();
        TF1* f = (TF1*) getCurrentDrawObjects()->first().getPointer();
        f->SetRange(xmin, xmax);
        f->SetMinimum(ymin);
        f->SetMaximum(ymax);
      }
    else if (PlotType.startsWith("TF2"))
      {
        //its 2D function!
        //TF2* f = (TF2*) DrawObjects.first().getPointer();
        TF2* f = (TF2*) getCurrentDrawObjects()->first().getPointer();
        f->SetRange(xmin, ymin, xmax, ymax);
        //f->SetRange(xmin, ymin, zmin, xmax, ymax, zmax);
        f->SetMaximum(zmax/1.05);
        f->SetMinimum(zmin);
      }
    else if (PlotType == "TGraph" || PlotType == "TGraphErrors")
      {
        //its 1D graph!
        TGraph* gr = (TGraph*) getCurrentDrawObjects()->first().getPointer();
        gr->GetXaxis()->SetLimits(xmin, xmax);
        gr->SetMinimum(ymin);
        gr->SetMaximum(ymax);
      }
    else if (PlotType == "TMultiGraph")
      {
        //its a collection of (here) 1D graphs
        TMultiGraph* gr = (TMultiGraph*) getCurrentDrawObjects()->first().getPointer();

        gr->GetXaxis()->SetLimits(xmin, xmax);
        gr->SetMinimum(ymin);
        gr->SetMaximum(ymax);
      }
    else if (PlotType == "TGraph2D")
      {
        //its 2D graph!        
        TGraph2D* gr = (TGraph2D*) getCurrentDrawObjects()->first().getPointer();
        //gr->GetXaxis()->SetLimits(xmin, xmax);
        gr->GetHistogram()->GetXaxis()->SetRangeUser(xmin, xmax);
        //gr->GetYaxis()->SetLimits(ymin, ymax);
        gr->GetHistogram()->GetYaxis()->SetRangeUser(ymin, ymax);
        //gr->GetZaxis()->SetLimits(zmin, zmax);//SetRangeUser(zmin, zmax);
        //gr->GetHistogram()->SetRange(xmin, ymin, xmax, ymax);

        gr->SetMinimum(zmin);
        gr->SetMaximum(zmax);
      }

  qApp->processEvents();
  GraphWindowClass::RedrawAll(); 
  //qDebug() << "reshape done";
}

void GraphWindowClass::RedrawAll()
{  
  //qDebug()<<"---Redraw all triggered." << " Current basket item:"<<CurrentBasketItem;
  GraphWindowClass::EnforceOverlayOff();

  //QVector<DrawObjectStructure> OldDrawObjects;

  if (CurrentBasketItem >= 0) //-1 - Basket is off; -2 -basket is Off, using tmp drawing (e.g. overlap of two histograms)
      DrawObjects = Basket[CurrentBasketItem].DrawObjects;

  if (DrawObjects.isEmpty())
    {
      ClearRootCanvas();
      UpdateRootCanvas();
      return;
    }

  for (int i=0; i<DrawObjects.size(); i++)
    {      
      QString opt = DrawObjects[i].getOptions();
      QByteArray ba = opt.toLocal8Bit();
      const char* options = ba.data();

      //qDebug()<<"   object #"<<i<<" Class name:"<<OldDrawObjects[i].getPointer()->ClassName()<<" options (QStr)"<<opt<<"-> options (chars):"<<options;
      doDraw(DrawObjects[i].getPointer(), options, false);
    }

  qApp->processEvents();
  //qDebug() << "----Mod+";
  //RasterWindow->fCanvas->Modified();
  //qDebug() << "----Upd+";
  RasterWindow->fCanvas->Update();  
  //qDebug() << "----Contr+";
  GraphWindowClass::UpdateControls();

  //ui->leOptions->setText(getCurrentDrawObjects()->first().getOptions()); 
  //qDebug() << "---redraw done";
}

void GraphWindowClass::clearTmpTObjects()
{
    for (int i=0; i<tmpTObjects.size(); i++)
    {
        //qDebug() << "Diagnostics - deleting tmpTObject:" << tmpTObjects[i];
        //qDebug() << "...Class name:" << tmpTObjects[i]->ClassName();
        delete tmpTObjects[i];
    }
    tmpTObjects.clear();

    delete hProjection; hProjection = 0;
}

void GraphWindowClass::on_cbShowLegend_toggled(bool checked)
{
  qApp->processEvents();
  if (checked)
  {
      //qDebug() << LastOptStat << "-> OptStyle";
      gStyle->SetOptStat(LastOptStat);
      qApp->processEvents();
      GraphWindowClass::updateLegendVisibility();
      RasterWindow->fCanvas->Modified();
      RasterWindow->fCanvas->Update();
  }
  else
  {
      LastOptStat = gStyle->GetOptStat();
      //qDebug() << "OptStyle ->" <<LastOptStat;
      gStyle->SetOptStat("");
      qApp->processEvents();
      RasterWindow->fCanvas->Modified();
      RasterWindow->fCanvas->Update();
  }

  //qDebug() << gStyle->GetOptStat();
  //RasterWindow->fCanvas->Modified();
  //RasterWindow->fCanvas->Update();
  //RedrawAll();
}

void GraphWindowClass::on_pbZoom_clicked()
{
  if (DrawObjects.isEmpty()) return;

  //qDebug()<<"Zoom clicked";
  TObject* obj = DrawObjects.first().getPointer();
  QString PlotType = obj->ClassName();  
  QString opt = DrawObjects.first().getOptions();
  //qDebug()<<"  Class name/PlotOptions/opt:"<<PlotType<<opt;

  if (
      PlotType == "TGraph" || PlotType == "TMultiGraph" ||
      PlotType == "TF1" ||
      PlotType.startsWith("TH1") || PlotType == "TProfile" ||
      (PlotType.startsWith("TH2") || PlotType == "TProfile2D") && (opt == "" || opt.contains("col", Qt::CaseInsensitive) || opt.contains("prof", Qt::CaseInsensitive))
      )
    {
      MW->WindowNavigator->BusyOn();
      GraphWindowClass::Extract2DBox();
      do
        {
          qApp->processEvents();
          if (ExtractionCanceled)
            {
              MW->WindowNavigator->BusyOff(false);
              return;
            }
        }
      while (!IsExtractionComplete());
      MW->WindowNavigator->BusyOff(false);

      ui->ledXfrom->setText(QString::number(extractedX1(), 'g', 4));
      ui->ledXto->setText(QString::number(extractedX2(), 'g', 4));
      ui->ledYfrom->setText(QString::number(extractedY1(), 'g', 4));
      ui->ledYto->setText(QString::number(extractedY2(), 'g', 4));

      GraphWindowClass::Reshape();
    }

  /*
  if (PlotType.startsWith("TH2") || PlotType == "TProfile2D")
   if (opt == "" || opt.contains("col", Qt::CaseInsensitive) || opt.contains("prof", Qt::CaseInsensitive))
     {
      MW->WindowNavigator->BusyOn();
      GraphWindowClass::Extract2DBox();
      do
        {
          qApp->processEvents();
          if (ExtractionCanceled)
            {
              MW->WindowNavigator->BusyOff();
              return;
            }
        }
      while (!MW->GraphWindow->IsExtractionComplete());
      MW->WindowNavigator->BusyOff();

      ui->ledXfrom->setText(QString::number(extractedX1(), 'g', 4));
      ui->ledXto->setText(QString::number(extractedX2(), 'g', 4));
      ui->ledYfrom->setText(QString::number(extractedY1(), 'g', 4));
      ui->ledYto->setText(QString::number(extractedY2(), 'g', 4));

      GraphWindowClass::Reshape();
    }
  if (PlotType == "TGraph" || PlotType.startsWith("TMultiGraph"))
    {
      MW->WindowNavigator->BusyOn();
      GraphWindowClass::Extract2DBox();
      do
        {
          qApp->processEvents();
          if (ExtractionCanceled)
            {
              MW->WindowNavigator->BusyOff();
              return;
            }
        }
      while (!MW->GraphWindow->IsExtractionComplete());
      MW->WindowNavigator->BusyOff();

      ui->ledXfrom->setText(QString::number(extractedX1(), 'g', 4));
      ui->ledXto->setText(QString::number(extractedX2(), 'g', 4));
      ui->ledYfrom->setText(QString::number(extractedY1(), 'g', 4));
      ui->ledYto->setText(QString::number(extractedY2(), 'g', 4));

      GraphWindowClass::Reshape();
    }
    */

  GraphWindowClass::UpdateControls();
}

void GraphWindowClass::on_pbUnzoom_clicked()
{
  if (DrawObjects.isEmpty()) return;

  TObject* obj = DrawObjects.first().getPointer();
  QString PlotType = obj->ClassName();

  if (PlotType.startsWith("TH1"))
    {
      ((TH1*) obj)->GetXaxis()->UnZoom();
      ((TH1*) obj)->GetYaxis()->UnZoom();
    }
  else if (PlotType == "TProfile")
    {
      ((TProfile*) obj)->GetXaxis()->UnZoom();
      ((TProfile*) obj)->GetYaxis()->UnZoom();
    }
  else if (PlotType.startsWith("TH2"))
    {
      ((TH2*) obj)->GetXaxis()->UnZoom();
      ((TH2*) obj)->GetYaxis()->UnZoom();
    }
  else if (PlotType == "TProfile2D")
    {
      ((TProfile2D*) obj)->GetXaxis()->UnZoom();
      ((TProfile2D*) obj)->GetYaxis()->UnZoom();
    }
  else if (PlotType == "TGraph2D")
    {
      //((TGraph*) obj)->GetXaxis()->UnZoom();
      //((TGraph*) obj)->GetYaxis()->UnZoom();
      if (RasterWindow->fCanvas->GetView())
        {
          RasterWindow->fCanvas->GetView()->UnZoom();
          RasterWindow->fCanvas->GetView()->Modify();
        }
    }
  else if (PlotType == "TGraph" || PlotType == "TMultiGraph" || PlotType == "TF1" || PlotType == "TF2")
    { //using values stored on first draw of this object
      //qDebug() << xmin0<<xmax0<<ymin0<<ymax0<<zmin0<<zmax0;
      ui->ledXfrom->setText( QString::number(xmin0, 'g', 4) );
      ui->ledXto->setText( QString::number(xmax0, 'g', 4) );
      ui->ledYfrom->setText( QString::number(ymin0, 'g', 4) );
      ui->ledYto->setText( QString::number(ymax0, 'g', 4) );
      ui->ledZfrom->setText( QString::number(zmin0, 'g', 4) );
      ui->ledZto->setText( QString::number(zmax0, 'g', 4) );
      Reshape();
      return;
    }  
  else
    {
      qDebug() << "Unzoom is not implemented for this object type:"<<PlotType;
      return;
    }

  RasterWindow->fCanvas->Modified();
  RasterWindow->fCanvas->Update();
  GraphWindowClass::UpdateControls();
}

void GraphWindowClass::on_leOptions_editingFinished()
{   
    ui->pbUnzoom->setFocus();
   QString newOptions = ui->leOptions->text();
   //preventing redraw just because of refocus
   if (old_option == newOptions) return;
   old_option = newOptions;

   if (DrawObjects.isEmpty()) return;
   getCurrentDrawObjects()->first().setOptions(newOptions);

   GraphWindowClass::RedrawAll();
}

QVector<DrawObjectStructure>* GraphWindowClass::getCurrentDrawObjects()
{
  if (CurrentBasketItem < 0) //-1 - Basket is off; -2 -basket is Off, using tmp drawing (e.g. overlap two histograms)
     return &DrawObjects;
  else
     return &Basket[CurrentBasketItem].DrawObjects;
}

void GraphWindowClass::SaveGraph(QString fileName)
{
  QFileInfo file(fileName);
  if(file.suffix().isEmpty()) fileName += ".png";

  //qDebug() << "Saving graph:" << fileName;
  RasterWindow->SaveAs(fileName);
}

void GraphWindowClass::UpdateControls()
{
  if (MW->ShutDown) return;
  if (getCurrentDrawObjects()->isEmpty()) return;
  //qDebug()<<"  GraphWindow: updating indication of ranges";
  TMPignore = true;

  TCanvas* c = RasterWindow->fCanvas;
  ui->cbLogX->setChecked(c->GetLogx());
  ui->cbLogY->setChecked(c->GetLogy());
  ui->cbGridX->setChecked(c->GetGridx());
  ui->cbGridY->setChecked(c->GetGridy());

  //TObject* obj = DrawObjects.first().getPointer();
  TObject* obj = getCurrentDrawObjects()->first().getPointer();
  QString PlotType = obj->ClassName();
  //const char* PlotOptions = DrawObjects.first().Options;

  zmin = 0; zmax = 0;

  //QString opt = DrawObjects.first().getOptions();
  QString opt = getCurrentDrawObjects()->first().getOptions();
//  qDebug()<<"here opt = "<<opt;

  //histograms
  if (PlotType.startsWith("TH1") || PlotType.startsWith("TH2") || PlotType =="TProfile")
    {
      c->GetRangeAxis(xmin, ymin, xmax, ymax);
      if (c->GetLogx())
        {
          xmin = TMath::Power(10.0, xmin);
          xmax = TMath::Power(10.0, xmax);
        }
      if (c->GetLogy())
        {
          ymin = TMath::Power(10.0, ymin);
          ymax = TMath::Power(10.0, ymax);
        }

      if (PlotType.startsWith("TH2") )
        {
           if (ui->leOptions->text().startsWith("col"))
             {
               //it is color contour - 2D plot
               zmin = ((TH2*) obj)->GetMinimum();
               zmax = ((TH2*) obj)->GetMaximum();
               ui->ledZfrom->setText( QString::number(zmin, 'g', 4) );
               ui->ledZto->setText( QString::number(zmax, 'g', 4) );
             }
           else
             {
               //3D plot
               float min[3], max[3];
               TView* v = c->GetView();
               if (v && !MW->ShutDown)
                 {
                   v->GetRange(min, max);                   
                   ui->ledZfrom->setText( QString::number(min[2], 'g', 4) );
                   ui->ledZto->setText( QString::number(max[2], 'g', 4) );
                 }
               else
                 {
                   ui->ledZfrom->setText("");
                   ui->ledZto->setText("");
                 }
             }
        }
    }
  if (PlotType.startsWith("TH3"))
    {
          ui->ledZfrom->setText( "" );   //   ui->ledZfrom->setText( QString::number(zmin, 'g', 4) );
          ui->ledZto->setText( "" ); // ui->ledZto->setText( QString::number(zmax, 'g', 4) );
    }

  if (PlotType.startsWith("TProfile2D"))
    {
        if (opt == "" || opt == "prof" || opt.contains("col") || opt.contains("colz"))
        {
            c->GetRangeAxis(xmin, ymin, xmax, ymax);
            if (c->GetLogx())
              {
                xmin = TMath::Power(10.0, xmin);
                xmax = TMath::Power(10.0, xmax);
              }
            if (c->GetLogy())
              {
                ymin = TMath::Power(10.0, ymin);
                ymax = TMath::Power(10.0, ymax);
              }
        }
          ui->ledZfrom->setText( "" );   //   ui->ledZfrom->setText( QString::number(zmin, 'g', 4) );
          ui->ledZto->setText( "" ); // ui->ledZto->setText( QString::number(zmax, 'g', 4) );
    }

  //functions
  if (PlotType.startsWith("TF1") )
    {
      //cannot use GetRange - y is reported 0 always
//      xmin = ((TF1*) obj)->GetXmin();
//      xmax = ((TF1*) obj)->GetXmax();
//      ymin = ((TF1*) obj)->GetMinimum();
//      ymax = ((TF1*) obj)->GetMaximum();
      c->GetRangeAxis(xmin, ymin, xmax, ymax);
    }
  if (PlotType.startsWith("TF2"))
    {
      ((TF2*) obj)->GetRange(xmin, ymin, xmax, ymax);
      //  zmin = ((TF2*) obj)->GetMinimum();  -- too slow, it involves minimizer!
      //  zmax = ((TF2*) obj)->GetMaximum();
      float min[3], max[3];
      TView* v = c->GetView();
      if (v)// && !MW->ShutDown)
        {
          v->GetRange(min, max);
          ui->ledZfrom->setText( QString::number(min[2], 'g', 4) );
          ui->ledZto->setText( QString::number(max[2], 'g', 4) );
        }
      else
        {
          ui->ledZfrom->setText("");
          ui->ledZto->setText("");
        }      
    }

  //graph
  if (PlotType == "TGraph" || PlotType == "TGraphErrors" || PlotType == "TMultiGraph")
    {
      c->GetRangeAxis(xmin, ymin, xmax, ymax);

 //     qDebug()<<"---Ymin:"<<ymin;

      if (c->GetLogx())
        {
          xmin = TMath::Power(10.0, xmin);
          xmax = TMath::Power(10.0, xmax);
        }
      if (c->GetLogy())
        {
          ymin = TMath::Power(10.0, ymin);
          ymax = TMath::Power(10.0, ymax);
        }
    }

  if (PlotType == "TGraph2D")
    {
      //xmin = ((TGraph2D*) obj)->GetHistogram()->GetXaxis()->GetXmin();
      //xmax = ((TGraph2D*) obj)->GetHistogram()->GetXaxis()->GetXmax();
       //xmin = ((TGraph2D*) obj)->GetXmin();
       //xmax = ((TGraph2D*) obj)->GetXmax();
       //ymin = ((TGraph2D*) obj)->GetYmin();
       //ymax = ((TGraph2D*) obj)->GetYmax();
       //zmin = ((TGraph2D*) obj)->GetZmin();
       //zmax = ((TGraph2D*) obj)->GetZmax();

       float min[3], max[3];
       TView* v = c->GetView();
       if (v)// && !MW->ShutDown)
         {
           v->GetRange(min, max);
           xmin = min[0]; xmax = max[0];
           ymin = min[1]; ymax = max[1];
           zmin = min[2]; zmax = max[2];
           //qDebug() << "minmax XYZ"<<xmin<<xmax<<ymin<<ymax<<zmin<<zmax;
         }

       ui->ledZfrom->setText( QString::number(zmin, 'g', 4) );
       ui->ledZto->setText( QString::number(zmax, 'g', 4) );

//      qDebug()<<"from object:"<<xmin<<xmax<<ymin<<ymax<<zmin<<zmax;
      //ui->leOptions->setEnabled(false);
    }
  else ui->leOptions->setEnabled(true);

  ui->ledXfrom->setText( QString::number(xmin, 'g', 4) );
  xmin = ui->ledXfrom->text().toDouble();  //to have consistent rounding
  ui->ledXto->setText( QString::number(xmax, 'g', 4) );
  xmax = ui->ledXto->text().toDouble();
  ui->ledYfrom->setText( QString::number(ymin, 'g', 4) );
  ymin = ui->ledYfrom->text().toDouble();
  ui->ledYto->setText( QString::number(ymax, 'g', 4) );
  ymax = ui->ledYto->text().toDouble();

  zmin = ui->ledZfrom->text().toDouble();
  zmax = ui->ledZto->text().toDouble();

  if (fFirstTime)
    {
      xmin0 = xmin; xmax0 = xmax;
      ymin0 = ymin; ymax0 = ymax;
      zmin0 = zmin; zmax0 = zmax;
      //qDebug() << "minmax0 XYZ"<<xmin0<<xmax0<<ymin0<<ymax0<<zmin0<<zmax0;
    }

  TMPignore = false;
  //qDebug()<<"  GraphWindow: updating toolbar done";
}

void GraphWindowClass::DoSaveGraph(QString name)
{  
  GraphWindowClass::SaveGraph(MW->GlobSet->LastOpenDir + "/" + name);
}

void GraphWindowClass::DrawStrOpt(TObject *obj, QString options, bool DoUpdate)
{
  if (!obj)
    {
      //TGraph is bad, it needs update to show the title axes :)
      RedrawAll();
      return;
    }
  Draw(obj, options.toLatin1().data(), DoUpdate, false);
}

void GraphWindowClass::on_cbToolBox_toggled(bool checked)
{
    //qDebug()<< "cbToolBox state togged";
    ui->swToolBar->setCurrentIndex((int)checked);

    ui->pbToolboxDragMode->setEnabled(checked);
    ui->cobToolBox->setEnabled(checked);
    ui->swToolBox->setVisible(checked);

    ui->fRange->setEnabled(!checked);
    ui->fGrid->setEnabled(!checked);
    ui->fLog->setEnabled(!checked);
    ui->cbShowLegend->setEnabled(!checked);
    ui->leOptions->setEnabled(!checked);
    ui->menuPalette->setEnabled(!checked);
    ui->pbAddToBasket->setEnabled(!checked);
    ui->cbShowBasket->setEnabled(!checked);

    int imode = ui->cobToolBox->currentIndex();
    if(checked)
    {
      scene->setActiveTool((AToolboxScene::Tool)imode);
      startOverlayMode();
    }
    else
    {
        endOverlayMode();
    }
    gvOver->update();
}

void GraphWindowClass::on_pbToolboxDragMode_clicked()
{
  scene->activateItemDrag();
}

void GraphWindowClass::on_pbToolboxDragMode_2_clicked()
{
  GraphWindowClass::on_pbToolboxDragMode_clicked();
}

void GraphWindowClass::on_cobToolBox_currentIndexChanged(int index)
{
  if (ui->cbToolBox->isChecked())
  {
    scene->setActiveTool((AToolboxScene::Tool)index);
    scene->moveToolToVisible();

    scene->update(scene->sceneRect());
    gvOver->update();
  }
}

void GraphWindowClass::selBoxGeometryChanged()
{
    //qDebug() << "selBoxGeometryChanged";
    ShapeableRectItem *SelBox = scene->getSelBox();

    double scaleX = RasterWindow->getXperPixel();
    double scaleY = RasterWindow->getYperPixel();
    SelBox->setScale(scaleX, scaleY);

    ui->ledAngle->setText(QString::number( SelBox->getTrueAngle(), 'f', 2 ));
    ui->ledWidth->setText(QString::number( SelBox->getTrueWidth(), 'f', 2 ));
    ui->ledHeight->setText(QString::number( SelBox->getTrueHeight(), 'f', 2 ));

    double x0, y0;
    RasterWindow->PixelToXY(SelBox->pos().x(), SelBox->pos().y(), x0, y0);
    ui->ledXcenter->setText(QString::number(x0, 'f', 2));
    ui->ledYcenter->setText(QString::number(y0, 'f', 2));

    //SelBox->update(SelBox->boundingRect());
    gvOver->update();
}

void GraphWindowClass::selBoxResetGeometry(double halfW, double halfH)
{
    double xc, yc; //center
    RasterWindow->PixelToXY(halfW, halfH, xc, yc);
    double x0, y0; //corner
    RasterWindow->PixelToXY(0, 0, x0, y0);

    double trueW = 0.5 * fabs(x0 - xc);
    double trueH = 0.5 * fabs(y0 - yc);

    ShapeableRectItem *SelBox = scene->getSelBox();
    SelBox->setTrueRect(trueW, trueH);
    SelBox->setPos(halfW, halfH);
    SelBox->setTrueAngle(0);

    selBoxGeometryChanged();
    selBoxControlsUpdated();
}

void GraphWindowClass::selBoxControlsUpdated()
{
    double x0 = ui->ledXcenter->text().toDouble();
    double y0 = ui->ledYcenter->text().toDouble();
    double dx = ui->ledWidth->text().toDouble();
    double dy = ui->ledHeight->text().toDouble();
    double angle = ui->ledAngle->text().toDouble();

  //  qDebug() << RasterWindow->getCanvasMinX() << RasterWindow->getCanvasMaxX() << RasterWindow->getCanvasMinY() << RasterWindow->getCanvasMaxY();

    double scaleX = RasterWindow->getXperPixel();
    double scaleY = RasterWindow->getYperPixel();
  //  qDebug() << "Scales: " << scaleX << scaleY;

    //  double DX = dx/scaleX;
    //  double DY = dy/scaleY;
  //  qDebug() << "width/height in pixels:"<< DX << DY;

    ShapeableRectItem *SelBox = scene->getSelBox();
    SelBox->setScale(scaleX, scaleY);
    SelBox->setTrueAngle(angle);
    SelBox->setTrueRect(dx, dy);      //-0.5*DX, -0.5*DY, DX, DY);

    int ix, iy;
    RasterWindow->XYtoPixel(x0, y0, ix, iy);
  //  qDebug() << "position" << ix << iy;
    SelBox->setPos(ix, iy);
    //SelBox->setTransform(QTransform().translate(ix, iy));

    scene->update(scene->sceneRect());
    gvOver->update();
}

void GraphWindowClass::on_pbSelBoxToCenter_clicked()
{
  scene->resetTool(AToolboxScene::ToolSelBox);
}

void GraphWindowClass::on_pbSelBoxFGColor_clicked()
{
  ShapeableRectItem *selbox = scene->getSelBox();
  QColor fg = QColorDialog::getColor(selbox->getForegroundColor(), this, "Choose the projection box's foreground color", QColorDialog::ShowAlphaChannel);
  if(fg.isValid())
      selbox->setForegroundColor(fg);
}

void GraphWindowClass::on_pbSelBoxBGColor_clicked()
{
  ShapeableRectItem *selbox = scene->getSelBox();
  QColor bg = QColorDialog::getColor(selbox->getBackgroundColor(), this, "Choose the projection box's background color", QColorDialog::ShowAlphaChannel);
  if(bg.isValid())
      selbox->setBackgroundColor(bg);
}


void GraphWindowClass::rulerGeometryChanged()
{
    const GraphicsRuler *ruler = scene->getRuler();
    QPointF p1 = ruler->getP1();
    QPointF p2 = ruler->getP2();

    ui->ledRulerX->setText(QString::number(p1.x(), 'g', 4));
    ui->ledRulerY->setText(QString::number(p1.y(), 'g', 4));
    ui->ledRulerX2->setText(QString::number(p2.x(), 'g', 4));
    ui->ledRulerY2->setText(QString::number(p2.y(), 'g', 4));
    ui->ledRulerDX->setText(QString::number(p2.x()-p1.x(), 'g', 4));
    ui->ledRulerDY->setText(QString::number(p2.y()-p1.y(), 'g', 4));
    ui->ledRulerLen->setText(QString::number(ruler->getLength(), 'g', 4));
    ui->ledRulerAngle->setText(QString::number(ruler->getAngle()*180.0/M_PI, 'g', 4));
    ui->ledRulerTicksLength->setText(QString::number(ruler->getTickLength(), 'g', 4));
}

void GraphWindowClass::rulerControlsP1Updated()
{
    scene->getRuler()->setP1(QPointF(ui->ledRulerX->text().toDouble(), ui->ledRulerY->text().toDouble()));
}

void GraphWindowClass::rulerControlsP2Updated()
{
    scene->getRuler()->setP2(QPointF(ui->ledRulerX2->text().toDouble(), ui->ledRulerY2->text().toDouble()));
}

void GraphWindowClass::rulerControlsLenAngleUpdated()
{    
    GraphicsRuler *ruler = scene->getRuler();
      double l = ui->ledRulerLen->text().toDouble();
    ruler->setAngle(ui->ledRulerAngle->text().toDouble()*M_PI/180);    
    //ruler->setLength(ui->ledRulerLen->text().toDouble()); //haha Raimundo
      ruler->setLength(l);
}

void GraphWindowClass::on_ledRulerTicksLength_editingFinished()
{
    scene->getRuler()->setTickLength(ui->ledRulerTicksLength->text().toDouble());
}

void GraphWindowClass::on_pbRulerFGColor_clicked()
{
    GraphicsRuler *ruler = scene->getRuler();
    QColor fg = QColorDialog::getColor(ruler->getForegroundColor(), this, "Choose the ruler's foreground color", QColorDialog::ShowAlphaChannel);
    if(fg.isValid())
        ruler->setForegroundColor(fg);
}

void GraphWindowClass::on_pbRulerBGColor_clicked()
{
    GraphicsRuler *ruler = scene->getRuler();
    QColor bg = QColorDialog::getColor(ruler->getBackgroundColor(), this, "Choose the ruler's background color", QColorDialog::ShowAlphaChannel);
    if(bg.isValid())
        ruler->setBackgroundColor(bg);
}

void GraphWindowClass::on_pbResetRuler_clicked()
{
  scene->resetTool(AToolboxScene::ToolRuler);
}

void GraphWindowClass::on_pbXprojection_clicked()
{
   GraphWindowClass::ShowProjection("x");
}

void GraphWindowClass::on_pbYprojection_clicked()
{
   GraphWindowClass::ShowProjection("y");
}

void GraphWindowClass::on_pbDensityDistribution_clicked()
{
   GraphWindowClass::ShowProjection("dens");
}

void GraphWindowClass::on_pbXaveraged_clicked()
{
   GraphWindowClass::ShowProjection("xAv");
}

void GraphWindowClass::on_pbYaveraged_clicked()
{
   GraphWindowClass::ShowProjection("yAv");
}

void GraphWindowClass::ShowProjection(QString type)
{
  ui->cbToolBox->setChecked(false);
  if (DrawObjects.isEmpty()) return;

  selBoxControlsUpdated();

  //  qDebug()<<"ShowProjection clicked: "<< type;
  TObject* obj = DrawObjects.first().getPointer();
  QString PlotType = obj->ClassName();

  //  qDebug()<<"  Class name/PlotOptions/opt:" << PlotType << DrawObjects.first().getOptions();

  if (PlotType != "TH2D" && PlotType != "TH2F") return;

  TH2* h = static_cast<TH2*>(obj);

  int nBinsX = h->GetXaxis()->GetNbins();
  int nBinsY = h->GetYaxis()->GetNbins();
  //  qDebug() << "Bins in X and Y" << nBinsX << nBinsY;

  double x0 = ui->ledXcenter->text().toDouble();
  double y0 = ui->ledYcenter->text().toDouble();  
  double dx = 0.5*ui->ledWidth->text().toDouble();
  double dy = 0.5*ui->ledHeight->text().toDouble();
  //  qDebug() << "Center:"<<x0<<y0<<"dx, dy:"<<dx<<dy;

  const ShapeableRectItem *SelBox = scene->getSelBox();
  //double scaleX = RasterWindow->getXperPixel();
  //double scaleY = RasterWindow->getYperPixel();
  double angle = SelBox->getTrueAngle();
  //    qDebug() << "True angle"<<angle;
  angle *= 3.1415926535/180.0;

  double cosa = cos(angle);
  double sina = sin(angle);

  if (hProjection) delete hProjection;
  TH1D* hWeights = 0;
  if (type=="x" || type=="xAv")
    {
      int nn;
      if (ui->cbProjBoxAutobin->isChecked())
      {
          int n = h->GetXaxis()->GetNbins();
          double binLength = (h->GetXaxis()->GetBinCenter(n) - h->GetXaxis()->GetBinCenter(1))/(n-1);
          nn = 2.0*dx / binLength;
          ui->sProjBins->setValue(nn);
      }
      else nn = ui->sProjBins->value();
      if (type == "x")
        hProjection = new TH1D("X-Projection","X1 projection", nn, -dx, +dx);
      else
        {
          hProjection = new TH1D("X-Av","X1 averaged", nn, -dx, +dx);
          hWeights    = new TH1D("X-W", "",            nn, -dx, +dx);
        }
    }  
  else if (type=="y" || type=="yAv")
    {
      int nn;
      if (ui->cbProjBoxAutobin->isChecked())
      {
          int n = h->GetYaxis()->GetNbins();
          double binLength = (h->GetYaxis()->GetBinCenter(n) - h->GetYaxis()->GetBinCenter(1))/(n-1);
          nn = 2.0*dy / binLength;
          ui->sProjBins->setValue(nn);
      }
      else nn = ui->sProjBins->value();
      if (type == "y")
        hProjection = new TH1D("Y-Projection","Y1 projection", nn, -dy, +dy);
      else
        {
          hProjection = new TH1D("Y-Av","Y1 averaged", nn, -dy, +dy);
          hWeights    = new TH1D("Y-W", "",            nn, -dy, +dy);
        }
    }
  else if (type == "dens")
    {
      //qDebug() << "Doing density distribution";
      hProjection = new TH1D("DensDistr","Density distribution", ui->sProjBins->value(), 0, 0);
    }
  else return;

  for (int iy = 1; iy<nBinsY+1; iy++)
    for (int ix = 1; ix<nBinsX+1; ix++)
      {
        double x = h->GetXaxis()->GetBinCenter(ix);
        double y = h->GetYaxis()->GetBinCenter(iy);
        //    qDebug() << "ix, x" << ix << x << "  iy, y " << iy << y;

        //transforming to the selection box coordinates
        x -= x0;
        y -= y0;

        // ooposite direction!
        double nx =  x*cosa + y*sina;
        double ny = -x*sina + y*cosa;
        //    qDebug () << "new coords: "<< nx << ny;

        //is it within the borders?
        if (  nx < -dx || nx > dx || ny < -dy || ny > dy  )
          {
            //outside!
            //h->SetBinContent(ix, iy, 0);
          }
        else
          {
            double w = h->GetBinContent(ix, iy);
            if (type == "x") hProjection->Fill(nx, w);
            if (type == "xAv")
              {
                hProjection->Fill(nx, w);
                hWeights->Fill(nx, 1);
              }
            else if (type == "y") hProjection->Fill(ny, w);
            if (type == "yAv")
              {
                hProjection->Fill(ny, w);
                hWeights->Fill(ny, 1);
              }
            else if (type == "dens") hProjection->Fill(w, 1);
          }
      }

   if (type == "x" || type == "y") hProjection->GetXaxis()->SetTitle("Distance, mm");
   else if (type == "dens") hProjection->GetXaxis()->SetTitle("Density, counts");   
   if (type == "xAv" || type == "yAv") *hProjection = *hProjection / *hWeights;

   DrawObjects.clear();
   DrawObjects.append(DrawObjectStructure(hProjection, ""));
   RedrawAll();

   delete hWeights;
}

double GraphWindowClass::runScaleDialog()
{
  QDialog* D = new QDialog(this);

  QDoubleValidator* vali = new QDoubleValidator(D);
  QVBoxLayout* l = new QVBoxLayout(D);
  QHBoxLayout* l1 = new QHBoxLayout();
    QLabel* lab1 = new QLabel("Multiply by ");
    QLineEdit* leM = new QLineEdit("1.0");
    leM->setValidator(vali);
    l1->addWidget(lab1);
    l1->addWidget(leM);
    QLabel* lab2 = new QLabel(" and divide by ");
    QLineEdit* leD = new QLineEdit("1.0");
    leD->setValidator(vali);
    l1->addWidget(lab2);
    l1->addWidget(leD);
  l->addLayout(l1);
    QPushButton* pb = new QPushButton("Scale");
    connect(pb, &QPushButton::clicked, D, &QDialog::accept);
  l->addWidget(pb);

  int ret = D->exec();
  double res = 1.0;
  if (ret == QDialog::Accepted)
    {
      double Mult = leM->text().toDouble();
      double Div = leD->text().toDouble();
      if (Div == 0)
        {
          message("Cannot divide by 0!", this);
        }
      else
        {
          res = Mult / Div;
        }
    }

  delete D;
  return res;
}

void GraphWindowClass::EnforceOverlayOff()
{
   ui->cbToolBox->setChecked(false); //update is in on_toggle
}

void GraphWindowClass::ExportData(bool fUseBinCenters)
{
  TObject *obj = DrawObjects.first().getPointer();
  if (!obj)
    {
      message("Data are no longer available", this);
      return;
    }

  QString cn = obj->ClassName();
  //qDebug() << "Class name:"<<cn;
  if (cn.startsWith("TH2"))
    {
      TObject *obj = DrawObjects.first().getPointer();
      TH2* h = static_cast<TH2*>(obj);
      exportTextForTH2(h);
      return;
    }
  else if (cn.startsWith("TF2"))
    {
      TObject *obj = DrawObjects.first().getPointer();
      TF2* f = static_cast<TF2*>(obj);
      TH2* h = dynamic_cast<TH2*>(f->GetHistogram());
      exportTextForTH2(h);
      return;
    }
  else if (!cn.startsWith("TH1") && cn!="TGraph" && cn!="TF1")
    {
      message("Object type not supported!", this);
      return;
    }

  QVector<double> x,y;
  if (cn.startsWith("TH1") || cn == "TF1")
    {
      //1D histogram
      TH1* h;
      if (cn.startsWith("TH1")) h = static_cast<TH1*>(obj);
      else
        {
          TF1* f = static_cast<TF1*>(obj);
          h = f->GetHistogram();
        }

      //qDebug() << "Histogram name:"<<h->GetName()<<"Entries"<<h->GetEntries()<<"Bins"<<h->GetNbinsX();
      if (fUseBinCenters)
        { //bin centers
          for (int i=1; i<h->GetNbinsX()+1; i++)
            {
              x.append(h->GetBinCenter(i));
              y.append(h->GetBinContent(i));
            }
        }
      else
        { //bin starts
          for (int i=1; i<h->GetNbinsX()+2; i++)
            {
              x.append(h->GetBinLowEdge(i));
              y.append(h->GetBinContent(i));
            }
        }
    }
  else if (cn == "TGraph")
    {
      //Graph
      TGraph* g = static_cast<TGraph*>(obj);
      //qDebug() << "Graph name:"<<g->GetName()<<"Entries"<<g->GetN();
      for (int i=0; i<g->GetN(); i++)
        {
          double xx, yy;
          int ok = g->GetPoint(i, xx, yy);
          if (ok != -1)
            {
              x.append(xx);
              y.append(yy);
            }
        }
    }
  else
    {
      qWarning() << "Unsupported type:"<<cn;
      return;
    }

  QFileDialog *fileDialog = new QFileDialog;
  fileDialog->setDefaultSuffix("txt");
  QString fileName = fileDialog->getSaveFileName(this, "Export data to ascii file", MW->GlobSet->LastOpenDir+"/"+obj->GetName(), "Text files(*.txt)");
  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
  if (QFileInfo(fileName).suffix().isEmpty()) fileName += ".txt";
  SaveDoubleVectorsToFile(fileName, &x, &y);
}

void GraphWindowClass::exportTextForTH2(TH2* h)
{
  if (!h) return;
  qDebug() << "Data size:"<< h->GetNbinsX() << "by" << h->GetNbinsY();

  QVector<double> x, y, f;
  for (int iX=1; iX<h->GetNbinsX()+1; iX++)
    for (int iY=1; iY<h->GetNbinsX()+1; iY++)
    {
      double X = h->GetXaxis()->GetBinCenter(iX);
      double Y = h->GetYaxis()->GetBinCenter(iY);
      x.append(X);
      y.append(Y);

      int iBin = h->GetBin(iX, iY);
      double F = h->GetBinContent(iBin);
      f.append(F);
      //qDebug() << iX<<iY<<iBin << "coords:" << X << Y << "val:" << F;
    }

  QFileDialog *fileDialog = new QFileDialog;
  fileDialog->setDefaultSuffix("txt");
  QString fileName = fileDialog->getSaveFileName(this, "Export data to ascii file", MW->GlobSet->LastOpenDir+"/"+h->GetTitle(), "Text files(*.txt)");
  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
  if (QFileInfo(fileName).suffix().isEmpty()) fileName += ".txt";
  SaveDoubleVectorsToFile(fileName, &x, &y, &f);
}

void GraphWindowClass::on_actionSave_root_object_triggered()
{
  TObject *obj = DrawObjects.first().getPointer();
  QString cn = obj->ClassName();
  if (!obj)
    {
      message("Object no longer exists!", this);
      return;
    }

  //qDebug() << "Class name:"<<cn;
  if (cn.startsWith("TH1") || cn.startsWith("TF1"))
    {
      TH1* hist = 0;

      if (cn.startsWith("TH1")) hist = static_cast<TH1*>(obj);
      else
        {
          TF1* fun = static_cast<TF1*>(obj);
          hist = fun->GetHistogram();
        }

      if (hist)
        {
          QFileDialog *fileDialog = new QFileDialog;
          fileDialog->setDefaultSuffix("root");
          QString fileName = fileDialog->getSaveFileName(this, "Save TH1 histogram", MW->GlobSet->LastOpenDir, "Root files(*.root)");
          if (fileName.isEmpty()) return;
          hist->SaveAs(fileName.toLatin1().data());
        }
      else message("Histogram does not exist!",this);
    }
  else if (cn.startsWith("TH2") || cn.startsWith("TF2"))
    {
      TH2* hist = 0;

      if (cn.startsWith("TH2")) hist = static_cast<TH2*>(obj);
      else
        {
          TF2* fun = static_cast<TF2*>(obj);
          hist = dynamic_cast<TH2*>(fun->GetHistogram());
        }

      if (hist)
        {
          QFileDialog *fileDialog = new QFileDialog;
          fileDialog->setDefaultSuffix("root");
          QString fileName = fileDialog->getSaveFileName(this, "Save TH2 histogram", MW->GlobSet->LastOpenDir, "Root files(*.root)");
          if (fileName.isEmpty()) return;
          hist->SaveAs(fileName.toLatin1().data());
        }
      else message("Histogram does not exist!",this);
    }
  else
    {
      message("Object type not supported!", this);
      return;
    }
}

void GraphWindowClass::on_cbShowBasket_toggled(bool checked)
{
   int w = ui->fBasket->width();
   if (!checked) w = -w;
   this->resize(this->width()+w, this->height());
}

void GraphWindowClass::on_pbAddToBasket_clicked()
{   
   if (DrawObjects.isEmpty()) return;

     //qDebug() << "Add item to basket triggered";
     //for (int i=0; i<DrawObjects.size(); i++)
     //  qDebug() << i << DrawObjects[i].getPointer()->ClassName();

   bool ok;
   int row = Basket.size();
   QString name = "Item"+QString::number(row);
   QString text = QInputDialog::getText(this, "New basket item",
                                              "Enter name:", QLineEdit::Normal,
                                              name, &ok);
   if (!ok || text.isEmpty()) return;

   AddCurrentToBasket(text);
}

void GraphWindowClass::AddCurrentToBasket(QString name)
{
  if (DrawObjects.isEmpty()) return;

  name = name.simplified();
  Basket.append(BasketItemClass(name, &DrawObjects));

  ui->cbShowBasket->setChecked(true);
  UpdateBasketGUI();
}

void GraphWindowClass::AddLegend(double x1, double y1, double x2, double y2, QString title)
{
  RasterWindow->fCanvas->BuildLegend(x1, y1, x2, y2, title.toLatin1());
  UpdateRootCanvas();
}

void GraphWindowClass::AddText(QString text, bool bShowFrame, int Alignment_0Left1Center2Right)
{
  if (!text.isEmpty()) ShowTextPanel(text, bShowFrame, Alignment_0Left1Center2Right);
}

void GraphWindowClass::ExportTH2AsText(QString fileName)
{
    TObject *obj = DrawObjects.first().getPointer();
    if (!obj) return;

    QString cn = obj->ClassName();
    if (!cn.startsWith("TH2")) return;
    TH2* h = static_cast<TH2*>(obj);

    //qDebug() << "Data size:"<< h->GetNbinsX() << "by" << h->GetNbinsY();
    QVector<double> x, y, f;
    for (int iY=1; iY<h->GetNbinsY()+1; iY++)
      for (int iX=1; iX<h->GetNbinsX()+1; iX++)
      {
        double X = h->GetXaxis()->GetBinCenter(iX);
        double Y = h->GetYaxis()->GetBinCenter(iY);
        x.append(X);
        y.append(Y);

        int iBin = h->GetBin(iX, iY);
        double F = h->GetBinContent(iBin);
        f.append(F);
        //qDebug() << iX<<iY<<iBin << "coords:" << X << Y << "val:" << F;
      }

    SaveDoubleVectorsToFile(fileName, &x, &y, &f);
}

QVector<double> GraphWindowClass::Get2DArray()
{
    TObject *obj = DrawObjects.first().getPointer();
    if (!obj) return QVector<double>();

    QString cn = obj->ClassName();
    if (!cn.startsWith("TH2")) return QVector<double>();
    TH2* h = static_cast<TH2*>(obj);

    //qDebug() << "Data size:"<< h->GetNbinsX() << "by" << h->GetNbinsY();
    QVector<double> arr;
    for (int iY=1; iY<h->GetNbinsX()+1; iY++)
      for (int iX=1; iX<h->GetNbinsX()+1; iX++)
      {
        int iBin = h->GetBin(iX, iY);
        double F = h->GetBinContent(iBin);
        arr << F;
      }  
    return arr;
}

void GraphWindowClass::UpdateBasketGUI()
{
    //qDebug() << "Update basket triggered";
  ui->lwBasket->clear();
  for (int i=0; i<Basket.size(); i++)
    {
      QString str = Basket[i].Name + " " + Basket[i].Type;
      ui->lwBasket->addItem(str);
    }
}

TGraph* HistToGraph(TH1* h)
{
  if (!h) return 0;
  QVector<double> x, f;
  for (int i=1; i<h->GetXaxis()->GetNbins()-1; i++)
    {
      x.append(h->GetBinCenter(i));
      f.append(h->GetBinContent(i));
    }
  return new TGraph(x.size(), x.data(), f.data());
}

BasketItemClass::BasketItemClass(QString name, QVector<DrawObjectStructure> *drawObjects)
{
    //qDebug() << "...basket item constructor triggered";
  Name = name;
  Type = drawObjects->first().getPointer()->ClassName();
    //qDebug() << "...name/type:"<<Name<<Type;

    //qDebug() << "...objects in the item:"<<drawObjects->size();
  for (int i=0; i<drawObjects->size(); i++)
    {
        //qDebug() << "......"<<i;
      TObject* obj = 0;
      QString type = (*drawObjects)[i].getPointer()->ClassName();
        //qDebug() << "......Class name:"<<type;
      QString options = (*drawObjects)[i].getOptions();
        //qDebug() << "......options:"<<options;

      if      (type == "TH1D")
      {
          obj = new TH1D( *(TH1D*)(*drawObjects)[i].getPointer());
          //TH1D* d = (TH1D*)obj;
          //d->SetName("blabla");
          //gROOT->RecursiveRemove(obj);
      }
      else if (type == "TLegend") obj = (TLegend*)(*drawObjects)[i].getPointer()->Clone("new");
      else if (type == "TPaveLabel") obj = (TPaveLabel*)(*drawObjects)[i].getPointer()->Clone("new");
      else if (type == "TPaveText") obj = (TPaveText*)(*drawObjects)[i].getPointer()->Clone("new");
      else if (type == "TH1I") obj = new TH1I( *(TH1I*)(*drawObjects)[i].getPointer());
      else if (type == "TH1F") obj = new TH1F( *(TH1F*)(*drawObjects)[i].getPointer());
      else if (type == "TH2D") obj = new TH2D( *(TH2D*)(*drawObjects)[i].getPointer());
      else if (type == "TH2F") obj = new TH2F( *(TH2F*)(*drawObjects)[i].getPointer());
      else if (type == "TH2I") obj = new TH2I( *(TH2I*)(*drawObjects)[i].getPointer());
      else if (type == "TH2C") obj = new TH2C( *(TH2C*)(*drawObjects)[i].getPointer());
      else if (type == "TH3F") obj = (TH3F*)(*drawObjects)[i].getPointer()->Clone();
      else if (type == "TH3D") obj = (TH3D*)(*drawObjects)[i].getPointer()->Clone();
      else if (type == "TH3I") obj = (TH3I*)(*drawObjects)[i].getPointer()->Clone();
      else if (type == "TProfile") obj = new TProfile( *(TProfile*)(*drawObjects)[i].getPointer());
      else if (type == "TProfile2D") obj = new TProfile2D( *(TProfile2D*)(*drawObjects)[i].getPointer());

      else if (type == "TEllipse") obj = new TEllipse( *(TEllipse*)(*drawObjects)[i].getPointer());
      else if (type == "TBox") obj = new TBox( *(TBox*)(*drawObjects)[i].getPointer());
      else if (type == "TPolyLine") obj = new TPolyLine( *(TPolyLine*)(*drawObjects)[i].getPointer());
      else if (type == "TLine") obj = new TLine( *(TLine*)(*drawObjects)[i].getPointer());

      //else if (type == "TF1") obj = new TF1( *(TF1*)(*drawObjects)[i].getPointer());
      else if (type == "TF1")
        {
          //does not work normal way - e.g. recalculated LRFs will replace old ones even in the copied object
          TF1* f = (TF1*)(*drawObjects)[i].getPointer();
          TGraph* g = HistToGraph( f->GetHistogram() );
          g->GetXaxis()->SetTitle( f->GetHistogram()->GetXaxis()->GetTitle() );
          g->GetYaxis()->SetTitle( f->GetHistogram()->GetYaxis()->GetTitle() );
          g->SetLineStyle( f->GetLineStyle());
          g->SetLineColor(f->GetLineColor());
          g->SetLineWidth(f->GetLineWidth());
          //g->SetMarkerStyle(f->GetMarkerStyle());
          //g->SetMarkerColor(f->GetMarkerColor());
          //g->SetMarkerSize(f->GetMarkerSize());
          g->SetFillColor(0);
          g->SetFillStyle(0);
          obj = g;
          if (!options.contains("same", Qt::CaseInsensitive))
              if (!options.contains("AL", Qt::CaseInsensitive))
                  options += "AL";
          Type = g->ClassName();
        }
      else if (type == "TF2")
        {
          //does not work normal way - e.g. recalculated LRFs will replace old ones even in the copied object
          //obj = new TF2( *(TF2*)(*drawObjects)[i].getPointer());

          //does not work this way too - conflict 1D-2D histograms
          /*
          TF2* f = (TF2*)(*drawObjects)[i].getPointer();
          TGraph* g = HistToGraph( f->GetHistogram() );
          qDebug() << g;
          g->SetLineStyle( f->GetLineStyle());
          g->SetLineColor(f->GetLineColor());
          g->SetLineWidth(f->GetLineWidth());
          obj = g;
          //options += "AL";
          Type = g->ClassName();
          */
          TF2* f = (TF2*)(*drawObjects)[i].getPointer();
          TH1* h = f->CreateHistogram();
          obj = h;
          Type = h->ClassName();
        }

      else if (type == "TGraph2D")    obj = new TGraph2D( *(TGraph2D*)(*drawObjects)[i].getPointer());     
      //else if (type == "TGraph")      obj = new TGraph( *(TGraph*)(*drawObjects)[i].getPointer());
      else if (type == "TGraph")      obj = (TGraph*)(*drawObjects)[i].getPointer()->Clone("new");
      else if (type == "TGraphErrors")      obj = new TGraphErrors( *(TGraphErrors*)(*drawObjects)[i].getPointer());
      else if (type == "TMultiGraph")
        {
          //obj = new TMultiGraph( *(TMultiGraph*)(*drawObjects)[i].getPointer());
          //line above does not compiles - ROOT bug in copy constructor implementation
          TMultiGraph* newm = new TMultiGraph();

          TMultiGraph* m = dynamic_cast<TMultiGraph*>((*drawObjects)[i].getPointer());
          TList* l = m->GetListOfGraphs();
          for (int i=0; i<l->GetEntries(); i++)
            {
                //qDebug() << i;
              const TGraph* gOld = dynamic_cast<const TGraph*>(l->At(i));
              if (gOld)
                {
                  TGraph* g = new TGraph(*gOld);
                  g->SetFillColor(0);
                  g->SetFillStyle(0);
                  if (g) newm->Add(g);
                }
            }
          obj = newm;
        }      
      else
        {
          message("Non-implemented object type: "+type);
          exit(-777);
        }

      TNamed* nn = dynamic_cast<TNamed*>(obj);
      if (nn) nn->SetTitle(name.toLocal8Bit().constData());

        //qDebug() << "......copied object class name:"<<obj->ClassName();
      this->DrawObjects.append(DrawObjectStructure(obj, options));
        //qDebug() << "......";
    }

  //qDebug() << "...basked item created";
}

BasketItemClass::~BasketItemClass()
{
    //cannot delete using pointers here
}

void BasketItemClass::clearObjects()
{
   for (int i=0; i<DrawObjects.size(); i++)
       delete DrawObjects[i].getPointer();
   DrawObjects.clear();
}

void GraphWindowClass::on_lwBasket_itemDoubleClicked(QListWidgetItem *)
{
   //qDebug() << "Row double clicked:"<<ui->lwBasket->currentRow();
   CurrentBasketItem = ui->lwBasket->currentRow();
   RedrawAll();
}

void GraphWindowClass::on_pbBasketBackToLast_clicked()
{
   DrawObjects = MasterDrawObjects;
   CurrentBasketItem = -1;
   BasketMode = 0;
   RedrawAll();
   ui->lwBasket->clearSelection();
}

void GraphWindowClass::on_lwBasket_customContextMenuRequested(const QPoint &pos)
{
  QMenu BasketMenu;
  int row = -1;

  //QString shownItemType;
  QListWidgetItem* temp = ui->lwBasket->itemAt(pos);

  QAction* switchToThis = 0;
  QAction* onTop = 0;
  QAction* del = 0;
  QAction* rename = 0;
  QAction* scale = 0;
  QAction* uniMap = 0;
  QAction* gaussFit = 0;
  QAction* setLine = 0;
  QAction* setMarker = 0;
  QAction* drawMenu = 0;
  QAction* drawIntegral = 0;

  if (temp)
    {
      //menu triggered at a valid item
      row = ui->lwBasket->row(temp);

      BasketMenu.addSeparator();      
      onTop = BasketMenu.addAction("Show on top of the main draw");
      switchToThis = BasketMenu.addAction("Switch to this");
      BasketMenu.addSeparator();
      setLine = BasketMenu.addAction("Set line attributes");
      setMarker = BasketMenu.addAction("Set marker attributes");
      drawMenu = BasketMenu.addAction("Root menu");
      BasketMenu.addSeparator();
      rename = BasketMenu.addAction("Rename");
      scale = BasketMenu.addAction("Scale");
      if (!MasterDrawObjects.isEmpty())
          if ( QString(MasterDrawObjects.first().getPointer()->ClassName()) == "TH2D")
              if (Basket.at(row).Type == "TH2D")
                 uniMap = BasketMenu.addAction("Use as unif. correction map");
      if (Basket.at(row).Type.startsWith("TH1"))
      {
             gaussFit = BasketMenu.addAction("Fit with Gauss");
             drawIntegral = BasketMenu.addAction("Draw integral");
      }
      BasketMenu.addSeparator();      
      del = BasketMenu.addAction("Delete");
      BasketMenu.addSeparator();
    }  
  BasketMenu.addSeparator();
  QAction* save = BasketMenu.addAction("Save basket to file");
  BasketMenu.addSeparator();
  QAction* append = BasketMenu.addAction("Append basket file");
  QAction* appendTxt = BasketMenu.addAction("Append graph from text file");
  QAction* appendTxtEr = BasketMenu.addAction("Append graph+errorbars from text file");
  QAction* appendRootHistsAndGraphs = BasketMenu.addAction("Append graphs and histograms from ROOT file");

  BasketMenu.addSeparator();
  QAction* clear = BasketMenu.addAction("Clear basket");

  QAction* selectedItem = BasketMenu.exec(ui->lwBasket->mapToGlobal(pos));
  if (!selectedItem) return; //nothing was selected

  //gStyle->SetOptTitle(1);
  if (selectedItem == switchToThis)
    {
      CurrentBasketItem = row;
      RedrawAll();
    }
  else if (selectedItem == drawMenu)
    {
      TObject* obj = Basket[row].DrawObjects.first().getPointer();

      TH1* h = dynamic_cast<TH1*>(obj);
      if (h)
        {
          h->DrawPanel();
          return;
        }
      TGraph* g = dynamic_cast<TGraph*>(obj);
      if (g)
        {
          g->DrawPanel();
          return;
        }

      message("Not supported for this object type", this);
    }
  else if (selectedItem == setMarker)
  {
      TObject* obj = Basket[row].DrawObjects.first().getPointer();
      TAttMarker* la = dynamic_cast<TAttMarker*>(obj);      
      if (la)
      {
         int color = la->GetMarkerColor();
         int siz = la->GetMarkerSize();
         int style = la->GetMarkerStyle();
         ARootMarkerConfigurator* rlc = new ARootMarkerConfigurator(&color, &siz, &style);
         int res = rlc->exec();
         if (res != 0)
         {
             la->SetMarkerColor(color);
             la->SetMarkerSize(siz);
             la->SetMarkerStyle(style);
             la->Modify();
             SetModifiedFlag();
             UpdateRootCanvas();
         }
      }
      else  message("Not supported for this object type", this);
  }
  else if (selectedItem == setLine)
  {
      TObject* obj = Basket[row].DrawObjects.first().getPointer();
      TAttLine* la = dynamic_cast<TAttLine*>(obj);
      if (la)
      {
         int color = la->GetLineColor();
         int wid = la->GetLineWidth();
         int style = la->GetLineStyle();
         ARootLineConfigurator* rlc = new ARootLineConfigurator(&color, &wid, &style);
         int res = rlc->exec();
         if (res != 0)
         {
             la->SetLineColor(color);
             la->SetLineWidth(wid);
             la->SetLineStyle(style);
             la->Modify();
             SetModifiedFlag();
             UpdateRootCanvas();
         }
      }
      else  message("Not supported for this object type", this);
  }
  else if (selectedItem == clear)
    {
      QMessageBox msgBox;
      msgBox.setText("Clear basket cannot be undone!");
      msgBox.setInformativeText("Clear basket?");
      msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
      msgBox.setDefaultButton(QMessageBox::Cancel);
      int ret = msgBox.exec();
      if (ret == QMessageBox::Yes)
      {
          ClearBasket();
          //UpdateBasketGUI();
          on_pbBasketBackToLast_clicked();
      }
    }
  else if (selectedItem == save)
      SaveBasket();
  else if (selectedItem == append)
    {
      AppendBasket();
      UpdateBasketGUI();
    }
  else if (selectedItem == appendRootHistsAndGraphs)
  {
      AppendRootHistsOrGraphs();
      UpdateBasketGUI();
  }
  else if (selectedItem == appendTxt)
    {
      qDebug() << "Appending txt file as graph to basket";
      QString fileName = QFileDialog::getOpenFileName(this, "Append graph from ascii file to Basket", MW->GlobSet->LastOpenDir, "Data files (*.txt *.dat); All files (*.*)");
      if (fileName.isEmpty()) return;
      MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
      QString name(QFileInfo(fileName).baseName());
      QVector<double> x, y;
      int res = LoadDoubleVectorsFromFile(fileName, &x, &y);
      if (res == 0)
        {
          TGraph* gr = new TGraph(x.size(), x.data(), y.data());
          gr->SetMarkerStyle(20);
          QVector<DrawObjectStructure>* drawObjects = new QVector<DrawObjectStructure>;
          drawObjects->append(DrawObjectStructure(gr, "APL"));
          Basket.append(BasketItemClass(name, drawObjects));
        }
      else message("Format error: Should be columns of X Y data", this);
      UpdateBasketGUI();
    }
  else if (selectedItem == appendTxtEr)
    {
      qDebug() << "Appending txt file as graph+errors to basket";
      QString fileName = QFileDialog::getOpenFileName(this, "Append graph from ascii file to Basket", MW->GlobSet->LastOpenDir, "Data files (*.txt *.dat); All files (*.*)");
      if (fileName.isEmpty()) return;
      MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
      QString name(QFileInfo(fileName).baseName());
      QVector<double> x, y, err;
      int res = LoadDoubleVectorsFromFile(fileName, &x, &y, &err);
      if (res == 0)
        {
          TGraphErrors* gr = new TGraphErrors(x.size(), x.data(), y.data(), 0, err.data());
          gr->SetMarkerStyle(20);
          QVector<DrawObjectStructure>* drawObjects = new QVector<DrawObjectStructure>;
          drawObjects->append(DrawObjectStructure(gr, "APL"));
          Basket.append(BasketItemClass(name, drawObjects));
        }
      else message("Format error: Should be columns of data X Y and optional error", this);
      UpdateBasketGUI();
    }
  else if (selectedItem == del)
    {
      if (row == -1) return; //protection
      Basket[row].clearObjects();
      Basket.remove(row);
      UpdateBasketGUI();
      on_pbBasketBackToLast_clicked();
    }
  else if (selectedItem == rename)
    {
      if (row == -1) return; //protection
      bool ok;
      QString text = QInputDialog::getText(this, "Rename basket item",
                                                 "Enter new name:", QLineEdit::Normal,
                                                 Basket[row].Name, &ok);      
      if (ok && !text.isEmpty())
        {
          Basket[row].Name = text.simplified();
          TObject* obj = Basket[row].DrawObjects.first().getPointer();
          if (obj)
            {
              TNamed* nn = dynamic_cast<TNamed*>(obj);
              if (nn) nn->SetTitle(Basket[row].Name.toLocal8Bit().constData());
            }
        }
      UpdateBasketGUI();
    }
  else if (selectedItem == onTop)
    {
      if (row == -1) return; //protection
      if (DrawObjects.isEmpty()) return; //protection

      //gStyle->SetOptTitle(0);
      qDebug() << "Item"<<row<<"was requested to be drawn on top of currently draw";

      //TObject* secondPointer = Basket[row].DrawObjects.first().getPointer();
      //DrawObjects.append(DrawObjectStructure(secondPointer, "same"));  //adding only ther first
      for (int i=0; i<Basket[row].DrawObjects.size(); i++)
        {
          TString CName = Basket[row].DrawObjects[i].getPointer()->ClassName();          
          if ( CName== "TLegend" || CName == "TPaveText") continue;
          //qDebug() << CName;
          QString options = Basket[row].DrawObjects[i].getOptions();
          options.replace("same", "");
          options.replace("SAME", "");
          options.replace("a", "");
          options.replace("A", "");
          TString safe = "same";
          safe += options.toLatin1().data();
          DrawObjects.append(DrawObjectStructure(Basket[row].DrawObjects[i].getPointer(), safe));
        }
      CurrentBasketItem = -2; //forcing to "basket off, tmp graph" mode
      RedrawAll();     
    }
  else if (selectedItem == scale)
    {
      if (row == -1) return; //protection
      if (DrawObjects.isEmpty()) return; //protection
      TObject* obj = Basket[row].DrawObjects.first().getPointer();
      if (obj)
        {
          QString name = obj->ClassName();
          QList<QString> impl;
          impl << "TGraph" << "TGraphErrors"  << "TH1I" << "TH1D" << "TH1F" << "TH2I"<< "TH2D"<< "TH2D";
          if (!impl.contains(name))
           {
             message("Not implemented for this object", this);
             return;
           }

          //double sf = QInputDialog::getDouble(this, "Input dialog", "Scaling factor = ", 1.0, -2147483647, 2147483647, 5, &fIn);
          //if (!fIn) return;
          double sf = runScaleDialog();
          if (sf == 1.0) return;

          if (name == "TGraph")
            {
              TGraph* gr = dynamic_cast<TGraph*>(obj);
              TF2 f("aaa", "[0]*y",0,1);
              f.SetParameter(0, sf);
              gr->Apply(&f);
            }
          if (name == "TGraphErrors")
            {
              TGraphErrors* gr = dynamic_cast<TGraphErrors*>(obj);
              TF2 f("aaa", "[0]*y",0,1);
              f.SetParameter(0, sf);
              gr->Apply(&f);
            }
          if (name.startsWith("TH1"))
            {
              TH1* h = dynamic_cast<TH1*>(obj);
              //h->Sumw2();
              h->Scale(sf);
            }
          if (name.startsWith("TH2"))
            {
              TH2* h = dynamic_cast<TH2*>(obj);
              //h->Sumw2();
              h->Scale(sf);
            }
        }
      RedrawAll();
    } 
  else if (selectedItem == uniMap)
  {
      TH2D* map = static_cast<TH2D*>(Basket[row].DrawObjects.first().getPointer());
      TH2D* h =   static_cast<TH2D*>(MasterDrawObjects.first().getPointer());

      *h = *h / *map;

      DrawObjects = MasterDrawObjects;
      CurrentBasketItem = -1;
      BasketMode = 0;
      RedrawAll();
      ui->lwBasket->clearSelection();
  }
  else if (selectedItem == gaussFit)
  {
      TH1* h =   static_cast<TH1*>(Basket[row].DrawObjects.first().getPointer());
      TF1* f1 = new TF1("f1", "gaus");
      int status = h->Fit(f1, "+");
      if (status == 0)
      {
          ui->cbShowFitParameters->setChecked(true);
          ui->cbShowLegend->setChecked(true);
          RedrawAll();
      }
  }
  else if (selectedItem == drawIntegral)
  {
      TH1* h = dynamic_cast<TH1*>(Basket[row].DrawObjects.first().getPointer());
      if (!h)
      {
          message("This operation requires TH1 ROOT object", this);
          return;
      }
      int bins = h->GetNbinsX();

      double* edges = new double[bins+1];
      for (int i=0; i<bins+1; i++)
          edges[i] = h->GetBinLowEdge(i+1);

      //    for (int i=0; i<bins+1; i++) qDebug() << i << "->" << edges[i];

      QString title = "Integral of " + Basket[row].Name;
      TH1D* hi = new TH1D("integral", title.toLocal8Bit().data(), bins, edges);
      delete [] edges;

      QString Xtitle = h->GetXaxis()->GetTitle();
      if (!Xtitle.isEmpty()) hi->GetXaxis()->SetTitle(Xtitle.toLocal8Bit().data());

      double prev = 0;
      for (int i=1; i<bins+1; i++)
        {
          prev += h->GetBinContent(i);
          hi->SetBinContent(i, prev);
        }
      Draw(hi, "");
  }
}

void GraphWindowClass::ClearBasket()
{
   for (int ib=0; ib<Basket.size(); ib++)
       Basket[ib].clearObjects();

  Basket.clear();  
  on_pbBasketBackToLast_clicked();
  UpdateBasketGUI();
}

void GraphWindowClass::on_actionSave_image_triggered()
{
  QFileDialog *fileDialog = new QFileDialog;
  fileDialog->setDefaultSuffix("png");
  QString fileName = fileDialog->getSaveFileName(this, "Save image as file", MW->GlobSet->LastOpenDir, "png (*.png);;gif (*.gif);;Jpg (*.jpg)");
  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

  GraphWindowClass::SaveGraph(fileName);
  if (MW->GlobSet->fOpenImageExternalEditor) QDesktopServices::openUrl(QUrl("file:"+fileName, QUrl::TolerantMode));
}

void GraphWindowClass::on_actionExport_data_as_text_triggered()
{
   ExportData(true);
}

void GraphWindowClass::on_actionExport_data_using_bin_start_positions_TH1_triggered()
{
   ExportData(false);
}

void GraphWindowClass::on_actionBasic_ROOT_triggered()
{
  gStyle->SetPalette(1);
  GraphWindowClass::RedrawAll();
}

void GraphWindowClass::on_actionDeep_sea_triggered()
{
  gStyle->SetPalette(51);
  GraphWindowClass::RedrawAll();
}

void GraphWindowClass::on_actionGrey_scale_triggered()
{
  gStyle->SetPalette(52);
  GraphWindowClass::RedrawAll();
}

void GraphWindowClass::on_actionDark_body_radiator_triggered()
{
  gStyle->SetPalette(53);
  GraphWindowClass::RedrawAll();
}

void GraphWindowClass::on_actionTwo_color_hue_triggered()
{
  gStyle->SetPalette(54);
  GraphWindowClass::RedrawAll();
}

void GraphWindowClass::on_actionRainbow_triggered()
{
  gStyle->SetPalette(55);
  GraphWindowClass::RedrawAll();
}

void GraphWindowClass::on_actionInverted_dark_body_triggered()
{
  gStyle->SetPalette(56);
  GraphWindowClass::RedrawAll();
}

void GraphWindowClass::SaveBasket()
{
  qDebug() << "Saving basket";

  QString fileName = QFileDialog::getSaveFileName(this, "Save Basket objects to file", MW->GlobSet->LastOpenDir, "Root files (*.root)");
  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
  if(QFileInfo(fileName).suffix().isEmpty()) fileName += ".root";

  QString str;
  TFile f(fileName.toLocal8Bit(),"RECREATE");

  int index = 0;
  for (int ib=0; ib<Basket.size(); ib++)
    {
      str += Basket[ib].Name + "\n";
      str += QString::number( Basket[ib].DrawObjects.size() );

      for (int i=0; i<Basket[ib].DrawObjects.size(); i++)
        {
          TString name = "";
          name += index;
          index++;
          ((TNamed*)Basket[ib].DrawObjects[i].getPointer())->SetName(name);
          Basket[ib].DrawObjects[i].getPointer()->Write();
          str += "|"+Basket[ib].DrawObjects[i].getOptions();
        }

      str += "\n";
    }

  TNamed desc;
  desc.SetTitle(str.toLocal8Bit().data());
  desc.Write("BasketDescription");

  f.Close();
}

void GraphWindowClass::AppendBasket()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Append objects from Basket file", MW->GlobSet->LastOpenDir, "Root files (*.root)");
  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
  //if(QFileInfo(fileName).suffix().isEmpty()) fileName += ".root";

  QByteArray ba = fileName.toLocal8Bit();
  const char *c_str = ba.data();
  TFile* f = new TFile(c_str);

  TNamed* desc = (TNamed*)f->Get("BasketDescription");
  QString text = desc->GetTitle();
  //qDebug() << text;

  if (desc)
    {
      //qDebug()<<"Proper basket file!";
      QStringList sl = text.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);

      int numLines = sl.size();
      //qDebug() << "Lines: "<<numLines;

      bool ok = true;
      int indexFileObject = 0;
      if (numLines % 2 == 0 )
        {
          //qDebug() << "Ok, even number of lines";
          for (int iDrawObject=0; iDrawObject<numLines/2; iDrawObject++ )
            {
              //qDebug() << iDrawObject<<">-----";
              QString name = sl[iDrawObject*2];
              bool ok;
              QStringList fields = sl[iDrawObject*2+1].split("|");
              if (fields.size()<2)
                {
                  qWarning()<<"Too short descr line";
                  ok=false;
                  break;
                }
              int numObj = fields[0].toInt(&ok);
              if (!ok) break;
              if (numObj != fields.size()-1)
                {
                  qWarning()<<"Number of objects vs option strings mismatch:"<<numObj<<fields.size()-1;
                  ok=false;
                  break;
                }

              //qDebug() << "name:"<< name;
              //qDebug() << "objects:"<< numObj;

              QVector<DrawObjectStructure>* drawObjects = new QVector<DrawObjectStructure>;
              for (int i=0; i<numObj; i++)
                {
                  TKey *key = (TKey*)f->GetListOfKeys()->At(indexFileObject);
                  indexFileObject++;
                  QString type = key->GetClassName();
                  //TString objName = key->GetName();
                  //qDebug() << "-->"<< i<<"   "<<objName<<"  "<<type<<"   "<<fields[i+1];
                  TObject *p = 0;

                  if (type=="TH1D") p = (TH1D*)key->ReadObj();
                  if (type=="TH1I") p = (TH1I*)key->ReadObj();
                  if (type=="TH1F") p = (TH1F*)key->ReadObj();
                  if (type=="TH2D") p = (TH2D*)key->ReadObj();
                  if (type=="TH2F") p = (TH2F*)key->ReadObj();
                  if (type=="TH2I") p = (TH2I*)key->ReadObj();

                  if (type=="TProfile") p = (TProfile*)key->ReadObj();
                  if (type=="TProfile2D") p = (TProfile2D*)key->ReadObj();

                  if (type=="TEllipse") p = (TEllipse*)key->ReadObj();
                  if (type=="TBox") p = (TBox*)key->ReadObj();
                  if (type=="TPolyLine") p = (TPolyLine*)key->ReadObj();
                  if (type=="TLine") p = (TLine*)key->ReadObj();

                  if (type=="TF1") p = (TF1*)key->ReadObj();
                  if (type=="TF2") p = (TF2*)key->ReadObj();

                  if (type=="TGraph2D") p = (TGraph2D*)key->ReadObj();
                  if (type=="TGraph") p = (TGraph*)key->ReadObj();
                  if (type=="TGraphErrors") p = (TGraphErrors*)key->ReadObj();

                  if (type=="TLegend") p = (TLegend*)key->ReadObj();

                  if (p)
                    {
                      //qDebug() << p->GetName();
                      drawObjects->append(DrawObjectStructure(p, fields[i+1]));
                    }
                  else
                    {
                      qWarning() << "Unregistered object type" << type <<"for load basket from file!";
                    }
                }
              if (!drawObjects->isEmpty()) Basket.append(BasketItemClass(name, drawObjects));

            }
        }
      else ok = false;

      if (ok) qDebug() << "Append successful";
      else    qDebug() << "Corrupted basked file";
    }
  else
    {
      message("It is not a proper Basket file!", this);
    }
  f->Close();
}

void GraphWindowClass::AppendRootHistsOrGraphs()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Append objects from ROOT file", MW->GlobSet->LastOpenDir, "Root files (*.root)");
    if (fileName.isEmpty()) return;
    MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

    QByteArray ba = fileName.toLocal8Bit();
    const char *c_str = ba.data();
    TFile* f = new TFile(c_str);

    const int numKeys = f->GetListOfKeys()->GetEntries();
    qDebug() << "File contains" << numKeys << "TKeys";

    for (int i=0; i<numKeys; i++)
    {
        TKey *key = (TKey*)f->GetListOfKeys()->At(i);
        QString Type = key->GetClassName();
        QString Name = key->GetName();
        qDebug() << i << Type << Name;

        if (Type.startsWith("TH") || Type.startsWith("TProfile") || Type.startsWith("TGraph"))
        {
            QVector<DrawObjectStructure>* drawObjects = new QVector<DrawObjectStructure>;
            TObject *p = 0;

            if (Type=="TH1D") p = (TH1D*)key->ReadObj();
            else if (Type=="TH1I") p = (TH1I*)key->ReadObj();
            else if (Type=="TH1F") p = (TH1F*)key->ReadObj();
            else if (Type=="TH2D") p = (TH2D*)key->ReadObj();
            else if (Type=="TH2F") p = (TH2F*)key->ReadObj();
            else if (Type=="TH2I") p = (TH2I*)key->ReadObj();

            else if (Type=="TProfile") p = (TProfile*)key->ReadObj();
            else if (Type=="TProfile2D") p = (TProfile2D*)key->ReadObj();

            else if (Type=="TGraph2D") p = (TGraph2D*)key->ReadObj();
            else if (Type=="TGraph") p = (TGraph*)key->ReadObj();
            else if (Type=="TGraphErrors") p = (TGraphErrors*)key->ReadObj();

            if (p)
            {
                drawObjects->append(DrawObjectStructure(p, ""));
                Basket.append(BasketItemClass(Name, drawObjects));
                qDebug() << "  appended";
            }
            else qWarning() << "Unregistered object type" << Type <<"for load basket from file!";
        }
        else qDebug() << "  ignored";
    }

    f->Close();
}

void GraphWindowClass::on_pbSmooth_clicked()
{
  if (DrawObjects.isEmpty())
    {
      message("Object already does not exist!", this);
      return;
    }
  TObject *obj = DrawObjects.first().getPointer();
  QString cn = obj->ClassName();

  //qDebug() << "Class name:"<<cn;
  if (cn.startsWith("TH1"))
    {
      TH1* hist = dynamic_cast<TH1*>(obj);
      if (hist) hist->Smooth(ui->sbSmooth->value());
    }
  else if (cn.startsWith("TH2"))
    {
      TH2* hist = dynamic_cast<TH2*>(obj);
      if (hist) hist->Smooth(ui->sbSmooth->value());
    }
  else
    {
      message("Implemented only for TH1 and TH2 histograms", this);
    }

  GraphWindowClass::EnforceOverlayOff();
  RedrawAll();
}

void GraphWindowClass::on_actionTop_triggered()
{
  SetAsActiveRootWindow();
  TView* v = RasterWindow->fCanvas->GetView();
  if (v) v->TopView();
}

void GraphWindowClass::on_actionSide_triggered()
{
  SetAsActiveRootWindow();
  TView* v = RasterWindow->fCanvas->GetView();
  if (v) v->SideView();
}

void GraphWindowClass::on_actionFront_triggered()
{
  SetAsActiveRootWindow();
  TView* v = RasterWindow->fCanvas->GetView();
  if (v) v->FrontView();
}

void GraphWindowClass::on_pbAttributes_clicked()
{
  TObject* obj = 0;

  if (CurrentBasketItem < 0) //-1 - Basket is off; -2 -basket is Off, using tmp drawing (e.g. overlap of two histograms)
    {
      if (!DrawObjects.isEmpty() )
         obj = DrawObjects.first().getPointer();
    }
  else if (CurrentBasketItem<Basket.size()) //Basket is ON
    {
      if (!Basket[CurrentBasketItem].DrawObjects.isEmpty())
         obj = Basket[CurrentBasketItem].DrawObjects.first().getPointer();
    }

  if (obj)
    {
      TH1* h = dynamic_cast<TH1*>(obj);
      if (h)
        {
          h->DrawPanel();
          return;
        }
      TGraph* g = dynamic_cast<TGraph*>(obj);
      if (g)
        {
          g->DrawPanel();
          return;
        }
    }

  RasterWindow->fCanvas->SetLineAttributes();
}

void GraphWindowClass::on_actionToggle_toolbar_toggled(bool arg1)
{
   if (arg1)
     {
       BarShown = false;
       ui->fUIbox->resize(150,500);
     }
   else
     {
       BarShown = true;
       ui->fUIbox->resize(0,500);
     }
   GraphWindowClass::resizeEvent(0);
}

void GraphWindowClass::on_actionEqualize_scale_XY_triggered()
{
   MW->WindowNavigator->BusyOn();
   //qDebug() << "Before-> X and Y size per pixel:" << RasterWindow->getXperPixel() << RasterWindow->getYperPixel();

   double XperP = fabs(RasterWindow->getXperPixel());
   double YperP = fabs(RasterWindow->getYperPixel());
   double CanvasWidth = QWinContainer->width();
   double NewCanvasWidth = CanvasWidth * XperP/YperP;
   double delta = NewCanvasWidth - CanvasWidth;
   resize(width()+delta, height());

   //qDebug() << "After guess-> X and Y size per pixel:" << RasterWindow->getXperPixel() << RasterWindow->getYperPixel();
   XperP = fabs(RasterWindow->getXperPixel());
   YperP = fabs(RasterWindow->getYperPixel());
   if (XperP != YperP)
     {
       bool XlargerY = (XperP > YperP);
       do
         {
           if (XperP<YperP) this->resize(this->width()-1, this->height());
           else this->resize(this->width()+1, this->height());
           UpdateRootCanvas();
           qApp->processEvents();

           XperP = fabs(RasterWindow->getXperPixel());
           YperP = fabs(RasterWindow->getYperPixel());
           if (XperP == YperP) break;
           if ( (XperP > YperP) != XlargerY ) break;
         }
       while ( isVisible() && width()>200 && width()<2000);
       //qDebug() << "After fine tune-> X and Y size per pixel:" << RasterWindow->getXperPixel() << RasterWindow->getYperPixel();
     }
   MW->WindowNavigator->BusyOff();   
}

void GraphWindowClass::on_ledRulerDX_editingFinished()
{
   GraphicsRuler *ruler = scene->getRuler();
   ruler->setDX(ui->ledRulerDX->text().toDouble());
}

void GraphWindowClass::on_ledRulerDY_editingFinished()
{
    GraphicsRuler *ruler = scene->getRuler();
    ruler->setDY(ui->ledRulerDY->text().toDouble());
}

void GraphWindowClass::on_cbShowFitParameters_toggled(bool checked)
{
    if (checked) gStyle->SetOptFit(0111);
    else gStyle->SetOptFit(0000);
    //RasterWindow->fCanvas->Modified();
    //RasterWindow->fCanvas->Update();
}

void GraphWindowClass::on_pbAddLegend_clicked()
{
  TLegend* leg = RasterWindow->fCanvas->BuildLegend();

  if (CurrentBasketItem < 0) //-1 - Basket is off; -2 -basket is Off, using tmp drawing (e.g. overlap of two histograms)
  {
      RegisterTObject(leg);
      DrawObjects.append(DrawObjectStructure(leg, "same"));
  }
  else
  {
      //do not register for basket - they have their own system
      Basket[CurrentBasketItem].DrawObjects.append(DrawObjectStructure(leg, "same"));      
  }
  RedrawAll();
}

void GraphWindowClass::on_pbRemoveLegend_clicked()
{
    QVector<DrawObjectStructure> &DrObj = (CurrentBasketItem < 0) ? DrawObjects : Basket[CurrentBasketItem].DrawObjects;
    for (int i=0; i<DrObj.size(); i++)
      {
          QString cn = DrObj[i].getPointer()->ClassName();
            //qDebug() << cn;
          if (cn == "TLegend")
            {              
              DrObj.remove(i);
              RedrawAll();
              return;
            }
      }
    qDebug() << "Legend object was not found!";
}

void GraphWindowClass::on_pbAddText_clicked()
{
  QDialog D(this);
  D.setModal(true);
  QVBoxLayout* l = new QVBoxLayout(&D);

  QLabel* lab = new QLabel("Enter text", &D);
  l->addWidget(lab);

  QPlainTextEdit* te = new QPlainTextEdit(&D);
  l->addWidget(te);

  QCheckBox* cbFrame = new QCheckBox("Show frame", &D);
  l->addWidget(cbFrame);
  cbFrame->setChecked(true);

  QComboBox* cobAlign = new QComboBox(&D);
  cobAlign->addItem("Text alignment: Left");
  cobAlign->addItem("Text alignment: Center");
  cobAlign->addItem("Text alignment: Right");
  l->addWidget(cobAlign);

  QPushButton* pb = new QPushButton("Confirm", &D);
  l->addWidget(pb);
  connect(pb, SIGNAL(clicked(bool)), &D, SLOT(accept()));

  D.exec();
  if (D.result() == QDialog::Rejected) return;

  QString Text = te->toPlainText();  
  if (!Text.isEmpty()) ShowTextPanel(Text, cbFrame->isChecked(), cobAlign->currentIndex());
}

void GraphWindowClass::ShowTextPanel(const QString Text, bool bShowFrame, int AlignLeftCenterRight)
{
  double xc1, yc1, xc2, yc2;
  RasterWindow->fCanvas->GetRange(xc1, yc1, xc2, yc2);

  double deltaX = xc2-xc1;
  double deltaY = yc2-yc1;
  double x1 = xc1 + 0.15*deltaX;
  double y1 = yc1 + 0.75*deltaY;
  double x2 = xc1 + 0.5*deltaX;
  double y2 = yc1 + 0.85*deltaY;

  TPaveText* la = new TPaveText(x1, y1, x2, y2);
  la->SetFillColor(0);
  la->SetBorderSize(bShowFrame ? 1 : 0);
  la->SetLineColor(1);
  la->SetTextAlign( (AlignLeftCenterRight + 1) * 10 + 2);

  QStringList sl = Text.split("\n");
  for (QString s : sl) la->AddText(s.toLatin1());

  DrawWithoutFocus(la, "same", true, false); //it seems the Paveltext is owned by drawn object - registration causes crash if used with non-registered object (e.g. script)

//  if (CurrentBasketItem < 0) //-1 - Basket is off; -2 -basket is Off, using tmp drawing (e.g. overlap of two histograms)
//  {
//     RegisterTObject(la);
//     DrawObjects.append(DrawObjectStructure(la, "same"));
//  }
//  else
//  {
//     //do not register for basket - they have their own system
//     Basket[CurrentBasketItem].DrawObjects.append(DrawObjectStructure(la, "same"));
//  }

//  RedrawAll();
}

void GraphWindowClass::on_pbRemoveText_clicked()
{
  QVector<DrawObjectStructure> &DrObj = (CurrentBasketItem < 0) ? DrawObjects : Basket[CurrentBasketItem].DrawObjects;
  for (int i=0; i<DrObj.size(); i++)
    {
        QString cn = DrObj[i].getPointer()->ClassName();
          //qDebug() << cn;
        if (cn == "TPaveText")
          {            
            DrObj.remove(i);
            RedrawAll();
            return;
          }
    }
  qDebug() << "Text object was not found!";
}


bool GraphWindowClass::Extraction()
{
    do
      {
        qApp->processEvents();
        if (IsExtractionCanceled()) break;
      }
    while (!IsExtractionComplete() );

    MW->WindowNavigator->BusyOff(false);

    return !IsExtractionCanceled();  //returns false = canceled
}

Double_t GauseWithBase(Double_t *x, Double_t *par)
{
   return par[0]*exp(par[1]*(x[0]+par[2])*(x[0]+par[2])) + par[3]*x[0] + par[4];   // [0]*exp([1]*(x+[2])^2) + [3]*x + [4]
}

void GraphWindowClass::on_pbFWHM_clicked()
{
    QVector<DrawObjectStructure> &DrObj = (CurrentBasketItem < 0) ? DrawObjects : Basket[CurrentBasketItem].DrawObjects;
    if (DrObj.isEmpty())
    {
        message("No data", this);
        return;
    }

    const QString cn = DrObj.first().getPointer()->ClassName();
    if ( !cn.startsWith("TH1") && cn!="TProfile")
    {
        message("Can be used only with 1D histograms!", this);
        return;
    }

    MW->WindowNavigator->BusyOn();
    Extract2DLine();
    if (!Extraction()) return; //cancel

    double startX = extracted2DLineXstart();
    double stopX = extracted2DLineXstop();
    if (startX>stopX) std::swap(startX, stopX);
    //qDebug() << startX << stopX;

    double a = extracted2DLineA();
    double b = extracted2DLineB();
    double c = extracted2DLineC();
    if (fabs(b)<1.0e-10)
    {
        message("Bad base line, cannot fit", this);
        return;
    }

    TH1* h =   static_cast<TH1*>(DrObj.first().getPointer());
                                                                   //  S  * exp( -0.5/s2 * (x   -m )^2) +  A *x +  B
    TF1 *f = new TF1("myfunc", GauseWithBase, startX, stopX, 5);  //  [0] * exp(    [1]  * (x + [2])^2) + [3]*x + [4]

    double initMid = startX + 0.5*(stopX - startX);
    //qDebug() << "Initial mid:"<<initMid;
    double initSigma = (stopX - startX)/2.3548; //sigma
    double startPar1 = -0.5 / (initSigma * initSigma );
    //qDebug() << "Initial par1"<<startPar1;
    double midBinNum = h->GetXaxis()->FindBin(initMid);
    double valOnMid = h->GetBinContent(midBinNum);
    double baseAtMid = (c - a * initMid) / b;
    double gaussAtMid = valOnMid - baseAtMid;
    //qDebug() << "bin, valMid, baseMid, gaussmid"<<midBinNum<< valOnMid << baseAtMid << gaussAtMid;

    f->SetParameter(0, gaussAtMid);
    f->SetParameter(1, startPar1);
    f->SetParameter(2, -initMid);
    f->FixParameter(3, -a/b);  // fixed!
    f->FixParameter(4, c/b);   // fixed!

    int status = h->Fit(f, "R0");
    if (status == 0)
    {
        double mid = -f->GetParameter(2);
        double sigma = TMath::Sqrt(-0.5/f->GetParameter(1));
        //qDebug() << "sigma:"<<sigma;
        double FWHM = sigma * 2.0*TMath::Sqrt(2.0*TMath::Log(2.0));
        double rel = FWHM/mid;

        QVector<DrawObjectStructure> CopyMasterDrawObjects;
        if (CurrentBasketItem == -1) CopyMasterDrawObjects = MasterDrawObjects;

        //drawing the fit
        Draw(f, "same");

        //draw base line
        TF1 *fl = new TF1("line", "pol2", startX, stopX);
        fl->SetLineStyle(2);
        fl->SetParameters(c/b, -a/b);
        Draw(fl, "same");

        //draw panel with results
        ShowTextPanel("fwhm = " + QString::number(FWHM) + "\nmean = " + QString::number(mid) + "\nfwhm/mean = "+QString::number(rel));

        MasterDrawObjects = CopyMasterDrawObjects;
    }
    else message("Fit failed!", this);
}
