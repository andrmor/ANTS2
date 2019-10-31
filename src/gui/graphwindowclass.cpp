//has to be on top!!
#include "TCanvas.h"

//ANTS2
#include "graphwindowclass.h"
#include "ui_graphwindowclass.h"
#include "mainwindow.h"
#include "rasterwindowgraphclass.h"
#include "windownavigatorclass.h"
#include "aglobalsettings.h"
#include "amessage.h"
#include "afiletools.h"
#include "shapeablerectitem.h"
#include "graphicsruler.h"
#include "arootlineconfigurator.h"
#include "arootmarkerconfigurator.h"
#include "atoolboxscene.h"
#include "abasketmanager.h"
#include "adrawexplorerwidget.h"

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
#include <QVariant>
#include <QVariantList>
#include <QSet>

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
#include "TTree.h"
#include "TPave.h"
#include "TPaveLabel.h"
#include "TPavesText.h"

GraphWindowClass::GraphWindowClass(QWidget *parent, MainWindow* mw) :
  AGuiWindow(parent), MW(mw),
  ui(new Ui::GraphWindowClass)
{
    Basket = new ABasketManager();

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
    windowFlags |= Qt::Tool;
    this->setWindowFlags( windowFlags );

    //DrawListWidget init
    Explorer = new ADrawExplorerWidget(DrawObjects);
    ui->layExplorer->insertWidget(1, Explorer);
    ui->splitter->setSizes({200,600});
    connect(Explorer, &ADrawExplorerWidget::requestRedraw, this, &GraphWindowClass::RedrawAll);
    connect(Explorer, &ADrawExplorerWidget::requestMakeCopy, this, &GraphWindowClass::onRequestMakeCopy);
    connect(Explorer, &ADrawExplorerWidget::requestInvalidateCopy, this, &GraphWindowClass::onRequestInvalidateCopy);
    connect(Explorer, &ADrawExplorerWidget::requestRegister, this, &GraphWindowClass::onRequestRegister);
    ui->pbBackToLast->setVisible(false);

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
//  QWinContainer = QWidget::createWindowContainer(RasterWindow, this);

//  QMargins margins = RasterWindow->frameMargins();
//  QWinContainer->setGeometry(ui->fUIbox->x() + ui->fUIbox->width() + 3 + margins.left(), 3 + margins.top(), 600, 600);
//  QWinContainer->setVisible(true);

  QHBoxLayout* l = dynamic_cast<QHBoxLayout*>(centralWidget()->layout());
  if (l) l->insertWidget(1, RasterWindow);
  else message("Unexpected layout!", this);

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
}

GraphWindowClass::~GraphWindowClass()
{
  ClearBasket();

  delete ui;

  clearTmpTObjects();

  delete scene; scene =  nullptr;
  delete gvOver; gvOver = nullptr;

  delete Basket; Basket = nullptr;
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

TGraph *GraphWindowClass::ConstructTGraph(const QVector<double> &x, const QVector<double> &y) const
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

TGraph *GraphWindowClass::ConstructTGraph(const std::vector<float> &x, const std::vector<float> &y) const
{
    int numEl = (int)x.size();
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

TGraph *GraphWindowClass::ConstructTGraph(const QVector<double> &x, const QVector<double> &y,
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

TGraph *GraphWindowClass::ConstructTGraph(const QVector<double> &x, const QVector<double> &y, const QString &Title, const QString &XTitle, const QString &YTitle, Color_t MarkerColor, int MarkerStyle, int MarkerSize, Color_t LineColor, int LineStyle, int LineWidth) const
{
    TGraph* gr = ConstructTGraph(x,y);
    gr->SetTitle(Title.toLatin1().data()); gr->GetXaxis()->SetTitle(XTitle.toLatin1().data()); gr->GetYaxis()->SetTitle(YTitle.toLatin1().data());
    gr->SetMarkerStyle(MarkerStyle); gr->SetMarkerColor(MarkerColor); gr->SetMarkerSize(MarkerSize);
    gr->SetEditable(false); gr->GetYaxis()->SetTitleOffset((Float_t)1.30);
    gr->SetLineWidth(LineWidth); gr->SetLineColor(LineColor); gr->SetLineStyle(LineStyle);
    return gr;
}

TGraph *GraphWindowClass::ConstructTGraph(const std::vector<float> &x, const std::vector<float> &y, const char *Title, const char *XTitle, const char *YTitle, Color_t MarkerColor, int MarkerStyle, int MarkerSize, Color_t LineColor, int LineStyle, int LineWidth) const
{
    TGraph* gr = ConstructTGraph(x,y);
    gr->SetTitle(Title); gr->GetXaxis()->SetTitle(XTitle); gr->GetYaxis()->SetTitle(YTitle);
    gr->SetMarkerStyle(MarkerStyle); gr->SetMarkerColor(MarkerColor); gr->SetMarkerSize(MarkerSize);
    gr->SetEditable(false); gr->GetYaxis()->SetTitleOffset((Float_t)1.30);
    gr->SetLineWidth(LineWidth); gr->SetLineColor(LineColor); gr->SetLineStyle(LineStyle);
    return gr;
}

TGraph2D *GraphWindowClass::ConstructTGraph2D(const QVector<double> &x, const QVector<double> &y, const QVector<double> &z) const
{
    int numEl = x.size();
    TGraph2D* gr = new TGraph2D(numEl, (double*)x.data(), (double*)y.data(), (double*)z.data());
    gr->SetFillStyle(0);
    gr->SetFillColor(0);
    return gr;
}

TGraph2D *GraphWindowClass::ConstructTGraph2D(const QVector<double>& x, const QVector<double>& y, const QVector<double>& z,
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

void GraphWindowClass::AddLine(double x1, double y1, double x2, double y2, int color, int width, int style)
{
        TLine* l = new TLine(x1, y1, x2, y2);
        l->SetLineColor(color);
        l->SetLineWidth(width);
        l->SetLineStyle(style);

        DrawWithoutFocus(l, "SAME");
}

void GraphWindowClass::ShowAndFocus()
{
    RasterWindow->fCanvas->cd();
    this->show();
    this->activateWindow();
    this->raise();
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
  RasterWindow->UpdateRootCanvas();
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

double GraphWindowClass::getMinX(bool *ok)
{
    return ui->ledXfrom->text().toDouble(ok);
}

double GraphWindowClass::getMaxX(bool *ok)
{
    return ui->ledXto->text().toDouble(ok);
}

double GraphWindowClass::getMinY(bool *ok)
{
    return ui->ledYfrom->text().toDouble(ok);
}

double GraphWindowClass::getMaxY(bool *ok)
{
    return ui->ledYto->text().toDouble(ok);
}

double GraphWindowClass::getMinZ(bool *ok)
{
    return ui->ledZfrom->text().toDouble(ok);
}

double GraphWindowClass::getMaxZ(bool *ok)
{
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

void GraphWindowClass::Draw(TObject *obj, const char *options, bool DoUpdate, bool TransferOwnership)
{
    ShowAndFocus();
    DrawWithoutFocus(obj, options, DoUpdate, TransferOwnership);
}

void GraphWindowClass::DrawWithoutFocus(TObject *obj, const char *options, bool DoUpdate, bool TransferOwnership)
{
    ui->lwBasket->clearSelection();
    setBasketItemUpdateAllowed(false);
    const QString opt = options;

    if (opt.contains("same", Qt::CaseInsensitive))
        DrawObjects.append(ADrawObject(obj, options));
    else
    {
        //this is new main object
        clearTmpTObjects(); //delete all TObjects previously drawn
        DrawObjects.clear();
        DrawObjects.append(ADrawObject(obj, options));
    }

    doDraw(obj, options, DoUpdate);

    if (TransferOwnership) RegisterTObject(obj);
    onRequestInvalidateCopy();
    EnforceOverlayOff();
    UpdateControls();
}

void GraphWindowClass::UpdateGuiControlsForMainObject(const QString & ClassName, const QString & options)
{
    //3D control
    bool flag3D = false;
    if (ClassName.startsWith("TH3") || ClassName.startsWith("TProfile2D") || ClassName.startsWith("TH2") || ClassName.startsWith("TF2") || ClassName.startsWith("TGraph2D"))
        flag3D = true;
    if ((ClassName.startsWith("TH2") || ClassName.startsWith("TProfile2D")) && ( options.contains("col",Qt::CaseInsensitive) || options.contains("prof", Qt::CaseInsensitive) || (options.isEmpty())) )
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

void GraphWindowClass::RegisterTObject(TObject *obj)
{
    //qDebug() << "Registered:"<<obj<<obj->ClassName();
    tmpTObjects.append(obj);
}

void GraphWindowClass::doDraw(TObject *obj, const char *opt, bool DoUpdate)
{
    //qDebug() << "-+-+ DoDraw";
    SetAsActiveRootWindow();

    TH1* h = dynamic_cast<TH1*>(obj);
    if (h) h->SetStats(ui->cbShowLegend->isChecked());

    obj->Draw(opt);
    if (DoUpdate) RasterWindow->fCanvas->Update();

    Explorer->updateGui();
    QString options(opt);
    if (!options.contains("same", Qt::CaseInsensitive))
        UpdateGuiControlsForMainObject(obj->ClassName(), options);
}

void GraphWindowClass::startOverlayMode()
{
    if(gvOver->isVisible())
        return;

    QPixmap map = qApp->screens().first()->grabWindow(RasterWindow->winId());//QApplication::desktop()->winId());
    gvOver->resize(RasterWindow->width(), RasterWindow->height());
    //gvOver->move(RasterWindow->x(), RasterWindow->y());
    gvOver->move(RasterWindow->x(), menuBar()->height());
    //gvOver->setGeometry(RasterWindow->geometry());
    scene->setSceneRect(0, 0, RasterWindow->width(), RasterWindow->height());
    scene->setBackgroundBrush(map);
    //RasterWindow->setVisible(false);// map.save("TestMap.png");

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
    //RasterWindow->setVisible(true);
    setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    RasterWindow->fCanvas->Update();
}

void GraphWindowClass::OnBusyOn()
{
    ui->fUIbox->setEnabled(false);
    ui->fBasket->setEnabled(false);
}

void GraphWindowClass::OnBusyOff()
{
    ui->fUIbox->setEnabled(true);
    ui->fBasket->setEnabled(true);
}

void GraphWindowClass::mouseMoveEvent(QMouseEvent *event)
{
    if(RasterWindow->isVisible())
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
  if (event->type() == QEvent::WindowActivate)
      RasterWindow->UpdateRootCanvas();

  if (event->type() == QEvent::Show)
      if (ColdStart)
      {
          //first time this window is shown
          ColdStart = false;
          this->resize(width()+1, height());
          this->resize(width()-1, height());
      }

  return AGuiWindow::event(event);
}

void GraphWindowClass::closeEvent(QCloseEvent *)
{
  ExtractionCanceled = true;
  RasterWindow->setExtractionComplete(true);
  DrawObjects.clear();
  PreviousDrawObjects.clear();
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
  UpdateControls();
}

void GraphWindowClass::on_cbLogY_toggled(bool checked)
{
  if (TMPignore) return;
  RasterWindow->fCanvas->SetLogy(checked);
  RasterWindow->fCanvas->Update();
  UpdateControls();
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

  return DrawObjects.first().Pointer;
}

void GraphWindowClass::Reshape()
{    
    //qDebug() << "Reshape triggered";
    qApp->processEvents();
    //ui->pbHideBar->setFocus(); //remove focus
//    qDebug()<<"GraphWindow  -> Reshape triggered; objects:"<<DrawObjects.size();

    //if (DrawObjects.isEmpty()) return;
    if (DrawObjects.isEmpty()) return;

    TObject * tobj = DrawObjects.first().Pointer;

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
    //QString PlotType = DrawObjects.first().Pointer->ClassName();
    QString PlotType = tobj->ClassName();
//    QString PlotOptions = DrawObjects.first().Options;
//    qDebug()<<"  main object name/options:"<<PlotType<<PlotOptions;

    if (PlotType.startsWith("TH1"))
      {
        //its 1D hist!
        //TH1* h = (TH1*) DrawObjects.first().Pointer;
        TH1* h = (TH1*)tobj;
        h->GetXaxis()->SetRangeUser(xmin, xmax);
        h->SetMinimum(ymin);
        h->SetMaximum(ymax);
      }
    else if (PlotType == "TProfile")
      {
        //its 1d profile
        //TProfile* h = (TProfile*) DrawObjects.first().Pointer;
        TProfile* h = (TProfile*)tobj;
        h->GetXaxis()->SetRangeUser(xmin, xmax);
        h->SetMinimum(ymin);
        h->SetMaximum(ymax);
      }
    else if (PlotType.startsWith("TH2"))
      {
        //its 2D hist!
        //TH2* h = (TH2*) DrawObjects.first().Pointer;
        TH2* h = (TH2*)tobj;
        h->GetXaxis()->SetRangeUser(xmin, xmax);
        h->GetYaxis()->SetRangeUser(ymin, ymax);

        if (OKzmin) h->SetMinimum(zmin);
        if (OKzmax) h->SetMaximum(zmax);
      }
    else if (PlotType == "TProfile2D")
      {
        //its 2D profile!
        //TProfile2D* h = (TProfile2D*) DrawObjects.first().Pointer;
        TProfile2D* h = (TProfile2D*)tobj;
        h->GetXaxis()->SetRangeUser(xmin, xmax);
        h->GetYaxis()->SetRangeUser(ymin, ymax);
      //  h->SetMinimum(zmin);
      //  h->SetMaximum(zmax);
      }
    else if (PlotType.startsWith("TF1"))
      {
        //its 1D function!
        //TF1* f = (TF1*) DrawObjects.first().Pointer;
        TF1* f = (TF1*)tobj;
        f->SetRange(xmin, xmax);
        f->SetMinimum(ymin);
        f->SetMaximum(ymax);
      }
    else if (PlotType.startsWith("TF2"))
      {
        //its 2D function!
        //TF2* f = (TF2*) DrawObjects.first().Pointer;
        TF2* f = (TF2*)tobj;
        f->SetRange(xmin, ymin, xmax, ymax);
        //f->SetRange(xmin, ymin, zmin, xmax, ymax, zmax);
        f->SetMaximum(zmax/1.05);
        f->SetMinimum(zmin);
      }
    else if (PlotType == "TGraph" || PlotType == "TGraphErrors")
      {
        //its 1D graph!
        TGraph* gr = (TGraph*)tobj;
        gr->GetXaxis()->SetLimits(xmin, xmax);
        gr->SetMinimum(ymin);
        gr->SetMaximum(ymax);
      }
    else if (PlotType == "TMultiGraph")
      {
        //its a collection of (here) 1D graphs
        TMultiGraph* gr = (TMultiGraph*)tobj;

        gr->GetXaxis()->SetLimits(xmin, xmax);
        gr->SetMinimum(ymin);
        gr->SetMaximum(ymax);
      }
    else if (PlotType == "TGraph2D")
      {
        //its 2D graph!        
        TGraph2D* gr = (TGraph2D*)tobj;
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
    //qDebug()<<"---Redraw all triggered"
    EnforceOverlayOff();

    if (DrawObjects.isEmpty())
    {
        ClearRootCanvas();
        UpdateRootCanvas();
        return;
    }

    for (ADrawObject & obj : DrawObjects)
    {
        QString opt = obj.Options;
        QByteArray ba = opt.toLocal8Bit();
        const char* options = ba.data();

        if (obj.bEnabled) doDraw(obj.Pointer, options, false);
    }

    qApp->processEvents();
    RasterWindow->fCanvas->Update();
    UpdateControls();
}

void GraphWindowClass::clearTmpTObjects()
{
    for (int i=0; i<tmpTObjects.size(); i++) delete tmpTObjects[i];
    tmpTObjects.clear();

    delete hProjection; hProjection = 0;
}

void GraphWindowClass::on_cbShowLegend_toggled(bool checked)
{
    if (checked)
        gStyle->SetOptStat(LastOptStat);
    else
    {
        LastOptStat = gStyle->GetOptStat();
        gStyle->SetOptStat("");
    }

    RedrawAll();
}

void GraphWindowClass::on_pbZoom_clicked()
{
  if (DrawObjects.isEmpty()) return;

  //qDebug()<<"Zoom clicked";
  TObject* obj = DrawObjects.first().Pointer;
  QString PlotType = obj->ClassName();  
  QString opt = DrawObjects.first().Options;
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

  TObject* obj = DrawObjects.first().Pointer;

  TH1 * h = dynamic_cast<TH1*>(obj);
  if (h)
  {
      h->GetXaxis()->UnZoom();
      h->GetYaxis()->UnZoom();
  }
  else
  {
      TGraph * gr = dynamic_cast<TGraph*>(obj);
      if (gr)
      {
          gr->GetXaxis()->UnZoom(); //does not work!
          gr->GetYaxis()->UnZoom();
      }
  }

  /*
  else if (PlotType == "TGraph2D")
    {
      if (RasterWindow->fCanvas->GetView())
        {
          RasterWindow->fCanvas->GetView()->UnZoom();
          RasterWindow->fCanvas->GetView()->Modify();
        }
    }
  else if (PlotType == "TGraph")// || PlotType == "TMultiGraph" || PlotType == "TF1" || PlotType == "TF2")
  {
  }
  else
    {
      qDebug() << "Unzoom is not implemented for this object type:"<<PlotType;
      return;
    }
  */

  RasterWindow->fCanvas->Modified();
  RasterWindow->fCanvas->Update();
  UpdateControls();
}

void GraphWindowClass::on_leOptions_editingFinished()
{   
   ui->pbUnzoom->setFocus();
   QString newOptions = ui->leOptions->text();
   //preventing redraw just because of refocus
   //if (old_option == newOptions) return;
   old_option = newOptions;

   if (DrawObjects.isEmpty()) return;
   DrawObjects.first().Options = newOptions;

   GraphWindowClass::RedrawAll();
}

void GraphWindowClass::SaveGraph(QString fileName)
{
  //QFileInfo file(fileName);
  //if(file.suffix().isEmpty()) fileName += ".png";
  //qDebug() << "Saving graph:" << fileName;
  RasterWindow->SaveAs(fileName);
}

void GraphWindowClass::UpdateControls()
{
  if (MW->ShutDown) return;
  if (DrawObjects.isEmpty()) return;

  //qDebug()<<"  GraphWindow: updating indication of ranges";
  TMPignore = true;

  TCanvas* c = RasterWindow->fCanvas;
  ui->cbLogX->setChecked(c->GetLogx());
  ui->cbLogY->setChecked(c->GetLogy());
  ui->cbGridX->setChecked(c->GetGridx());
  ui->cbGridY->setChecked(c->GetGridy());

  TObject* obj = DrawObjects.first().Pointer;
  if (!obj)
  {
      qWarning() << "Cannot update graph window rang controls - object does not exist";
      return;
  }
  QString PlotType = obj->ClassName();
  QString opt = DrawObjects.first().Options;
  //qDebug() << "PlotType:"<< PlotType << "Opt:"<<opt;

  zmin = 0; zmax = 0;
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
  else if (PlotType.startsWith("TH3"))
  {
          ui->ledZfrom->setText( "" );   //   ui->ledZfrom->setText( QString::number(zmin, 'g', 4) );
          ui->ledZto->setText( "" ); // ui->ledZto->setText( QString::number(zmax, 'g', 4) );
  }
  else if (PlotType.startsWith("TProfile2D"))
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
  else if (PlotType.startsWith("TF1") )
  {
      //cannot use GetRange - y is reported 0 always
//      xmin = ((TF1*) obj)->GetXmin();
//      xmax = ((TF1*) obj)->GetXmax();
//      ymin = ((TF1*) obj)->GetMinimum();
//      ymax = ((TF1*) obj)->GetMaximum();
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
  else if (PlotType.startsWith("TF2"))
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
  else if (PlotType == "TGraph" || PlotType == "TGraphErrors" || PlotType == "TMultiGraph")
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
       //   qDebug()<<"---Ymin:"<<ymin;
  }
  else if (PlotType == "TGraph2D")
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

  //else ui->leOptions->setEnabled(true);

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

//  if (fFirstTime)
//  {
//      xmin0 = xmin; xmax0 = xmax;
//      ymin0 = ymin; ymax0 = ymax;
//      zmin0 = zmin; zmax0 = zmax;
//      //qDebug() << "minmax0 XYZ"<<xmin0<<xmax0<<ymin0<<ymax0<<zmin0<<zmax0;
//  }

  TMPignore = false;
  //qDebug()<<"  GraphWindow: updating toolbar done";
}

void GraphWindowClass::DoSaveGraph(QString name)
{  
  GraphWindowClass::SaveGraph(MW->GlobSet.LastOpenDir + "/" + name);
}

void GraphWindowClass::DrawStrOpt(TObject *obj, QString options, bool DoUpdate)
{
    if (!obj)
    {
        //TGraph is bad, it needs update to show the title axes :)
        RedrawAll();
        return;
    }
    Draw(obj, options.toLatin1().data(), DoUpdate, true); // changed to register - now hist/graph scripts make a copy to draw
}

void GraphWindowClass::onDrawRequest(TObject *obj, const QString options, bool transferOwnership, bool focusWindow)
{
    if (focusWindow)
        Draw(obj, options.toLatin1().data(), true, transferOwnership);
    else
        DrawWithoutFocus(obj, options.toLatin1().data(), true, transferOwnership);
}

void SetMarkerAttributes(TAttMarker* m, const QVariantList& vl)
{
    m->SetMarkerColor(vl.at(0).toInt());
    m->SetMarkerStyle(vl.at(1).toInt());
    m->SetMarkerSize (vl.at(2).toDouble());
}
void SetLineAttributes(TAttLine* l, const QVariantList& vl)
{
    l->SetLineColor(vl.at(0).toInt());
    l->SetLineStyle(vl.at(1).toInt());
    l->SetLineWidth(vl.at(2).toDouble());
}

bool GraphWindowClass::DrawTree(TTree *tree, const QString& what, const QString& cond, const QString& how,
                                const QVariantList binsAndRanges, const QVariantList markersAndLines,
                                QString* result)
{
    if (what.isEmpty())
    {
        if (result) *result = "\"What\" string is empty!";
        return false;
    }

    QStringList Vars = what.split(":", QString::SkipEmptyParts);
    int num = Vars.size();
    if (num > 3)
    {
        if (result) *result = "Invalid \"What\" string - there should be 1, 2 or 3 fields separated with \":\" character!";
        return false;
    }

    QString howProc = how;
    QVector<QString> vDisreguard;
    vDisreguard << "func" << "same" << "pfc" << "plc" << "pmc" << "lego" << "col" << "candle" << "violin" << "cont" << "list" << "cyl" << "pol" << "scat";
    for (const QString& s : vDisreguard) howProc.remove(s, Qt::CaseInsensitive);
    bool bHistToGraph = ( num == 2 && ( howProc.contains("L") || howProc.contains("C") ) );
    qDebug() << "Graph instead of hist?"<< bHistToGraph;

    QVariantList defaultBR;
    defaultBR << (int)100 << (double)0 << (double)0;
    QVariantList defaultMarkerLine;
    defaultMarkerLine << (int)602 << (int)1 << (double)1.0;

    //check ups
    QVariantList vlBR;
    for (int i=0; i<3; i++)
    {
        if (i >= binsAndRanges.size())
             vlBR.push_back(defaultBR);
        else vlBR.push_back(binsAndRanges.at(i));

        QVariantList vl = vlBR.at(i).toList();
        if (vl.size() != 3)
        {
            if (result) *result = "Error in BinsAndRanges argument (bad size)";
            return false;
        }
        bool bOK0, bOK1, bOK2;
        vl.at(0).toInt(&bOK0);
        vl.at(0).toDouble(&bOK1);
        vl.at(0).toDouble(&bOK2);
        if (!bOK0 || !bOK1 || !bOK2)
        {
            if (result) *result = "Error in BinsAndRanges argument (conversion problem)";
            return false;
        }
    }
    //  qDebug() << "binsranges:" << vlBR;

    QVariantList vlML;
    for (int i=0; i<2; i++)
    {
        if (i >= markersAndLines.size())
             vlML.push_back(defaultMarkerLine);
        else vlML.push_back(markersAndLines.at(i));

        QVariantList vl = vlML.at(i).toList();
        if (vl.size() != 3)
        {
            if (result) *result = "Error in MarkersAndLines argument (bad size)";
            return false;
        }
        bool bOK0, bOK1, bOK2;
        vl.at(0).toInt(&bOK0);
        vl.at(0).toInt(&bOK1);
        vl.at(0).toDouble(&bOK2);
        if (!bOK0 || !bOK1 || !bOK2)
        {
            if (result) *result = "Error in MarkersAndLines argument (conversion problem)";
            return false;
        }
    }
    //  qDebug() << "markersLine:"<<vlML;

    QString str = what + ">>htemp(";
    for (int i = 0; i < num; i++)
    {
        if (i == 1 && bHistToGraph) break;

        QVariantList br = vlBR.at(i).toList();
        int    bins = br.at(0).toInt();
        double from = br.at(1).toDouble();
        double to   = br.at(2).toDouble();
        str += QString::number(bins) + "," + QString::number(from) + "," + QString::number(to) + ",";
    }
    str.chop(1);
    str += ")";

    TString What = str.toLocal8Bit().data();
    //TString Cond = ( cond.isEmpty() ? "" : cond.toLocal8Bit().data() );
    TString Cond = cond.toLocal8Bit().data();
    //TString How  = (  how.isEmpty() ? "" :  how.toLocal8Bit().data() );
    TString How  = how.toLocal8Bit().data();

    QString howAdj = how;   //( how.isEmpty() ? "goff" : "goff,"+how );
    if (!bHistToGraph) howAdj = "goff," + how;
    TString HowAdj = howAdj.toLocal8Bit().data();

    // -------------Delete old tmp hist if exists---------------
    TObject* oldObj = gDirectory->FindObject("htemp");
    if (oldObj)
    {
          qDebug() << "Old htemp found: "<<oldObj->GetName() << " -> deleting!";
        gDirectory->RecursiveRemove(oldObj);
    }

    // --------------DRAW--------------
    qDebug() << "TreeDraw -> what:" << What << "cuts:" << Cond << "opt:"<<HowAdj;

    GraphWindowClass* tmpWin = 0;
    if (bHistToGraph)
    {
        tmpWin = new GraphWindowClass(this, MW);
        tmpWin->SetAsActiveRootWindow();
    }

    TH1::AddDirectory(true);
    tree->Draw(What, Cond, HowAdj);
    TH1::AddDirectory(false);

    // --------------Checks------------
    TH1* tmpHist = dynamic_cast<TH1*>(gDirectory->Get("htemp"));
    if (!tmpHist)
    {
        qDebug() << "No histogram was generated: check input!";
        if (result) *result = "No histogram was generated: check input!";
        delete tmpWin;
        return false;
    }

    // -------------Formatting-----------
    if (bHistToGraph)
    {
        TGraph *g = dynamic_cast<TGraph*>(gPad->GetPrimitive("Graph"));
        if (!g)
        {
            qDebug() << "Graph was not generated: check input!";
            if (result) *result = "No graph was generated: check input!";
            delete tmpWin;
            return false;
        }

        TGraph* clone = new TGraph(*g);
        if (clone)
        {
            if (clone->GetN() > 0)
            {
                const QVariantList xx = vlBR.at(0).toList();
                double min = xx.at(1).toDouble();
                double max = xx.at(2).toDouble();
                if (max > min)
                    clone->GetXaxis()->SetLimits(min, max);
                const QVariantList yy = vlBR.at(1).toList();
                min = yy.at(1).toDouble();
                max = yy.at(2).toDouble();
                if (max > min)
                {
                    clone->SetMinimum(min);
                    clone->SetMaximum(max);
                }

                clone->SetTitle(tmpHist->GetTitle());
                SetMarkerAttributes(static_cast<TAttMarker*>(clone), vlML.at(0).toList());
                SetLineAttributes(static_cast<TAttLine*>(clone), vlML.at(1).toList());

                if ( !How.Contains("same", TString::kIgnoreCase) ) How = "A," + How;
                SetAsActiveRootWindow();
                Draw(clone, How);
            }
            else
            {
                qDebug() << "Empty graph was generated!";
                if (result) *result = "Empty graph was generated!";
                delete tmpWin;
                return false;
            }
        }
    }
    else
    {
        if (tmpHist->GetEntries() == 0)
        {
            qDebug() << "Empty histogram was generated!";
            if (result) *result = "Empty histogram was generated!";
            return false;
        }

        TH1* h = dynamic_cast<TH1*>(tmpHist->Clone(""));

        switch (num)
        {
            case 1:
                h->GetXaxis()->SetTitle(Vars.at(0).toLocal8Bit().data());
                break;
            case 2:
                h->GetYaxis()->SetTitle(Vars.at(0).toLocal8Bit().data());
                h->GetXaxis()->SetTitle(Vars.at(1).toLocal8Bit().data());
                break;
            case 3:
                h->GetZaxis()->SetTitle(Vars.at(0).toLocal8Bit().data());
                h->GetYaxis()->SetTitle(Vars.at(1).toLocal8Bit().data());
                h->GetXaxis()->SetTitle(Vars.at(2).toLocal8Bit().data());
        }

        SetMarkerAttributes(static_cast<TAttMarker*>(h), vlML.at(0).toList());
        SetLineAttributes(static_cast<TAttLine*>(h), vlML.at(1).toList());
        Draw(h, How, true, false);
    }

    if (result) *result = "";
    delete tmpWin;
    return true;
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
    ui->actionToggle_Explorer_Basket->setEnabled(!checked);
    ui->actionToggle_toolbar->setEnabled(!checked);

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
    scene->update(scene->sceneRect());
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
    SelBox->setTrueRectangle(trueW, trueH);
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
    SelBox->setTrueRectangle(dx, dy);      //-0.5*DX, -0.5*DY, DX, DY);

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
  TObject* obj = DrawObjects.first().Pointer;
  QString PlotType = obj->ClassName();
  //  qDebug()<<"  Class name/PlotOptions/opt:" << PlotType << DrawObjects.first().Options;
  if (PlotType != "TH2D" && PlotType != "TH2F") return;

  MW->WindowNavigator->BusyOn(); // -->

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
  else
    {
      MW->WindowNavigator->BusyOff(); // <--
      return;
    }

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
   DrawObjects.append(ADrawObject(hProjection, ""));
   RedrawAll();

   delete hWeights;

   MW->WindowNavigator->BusyOff(); // <--
}

void GraphWindowClass::EnforceOverlayOff()
{
   ui->cbToolBox->setChecked(false); //update is in on_toggle
}

void GraphWindowClass::ExportData(bool fUseBinCenters)
{
  TObject *obj = DrawObjects.first().Pointer;
  if (!obj)
    {
      message("Data are no longer available", this);
      return;
    }

  QString cn = obj->ClassName();
  //qDebug() << "Class name:"<<cn;
  if (cn.startsWith("TH2"))
    {
      TObject *obj = DrawObjects.first().Pointer;
      TH2* h = static_cast<TH2*>(obj);
      exportTextForTH2(h);
      return;
    }
  else if (cn.startsWith("TF2"))
    {
      TObject *obj = DrawObjects.first().Pointer;
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
  QString fileName = fileDialog->getSaveFileName(this, "Export data to ascii file", MW->GlobSet.LastOpenDir+"/"+obj->GetName(), "Text files(*.txt)");
  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
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
  QString fileName = fileDialog->getSaveFileName(this, "Export data to ascii file", MW->GlobSet.LastOpenDir+"/"+h->GetTitle(), "Text files(*.txt)");
  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
  if (QFileInfo(fileName).suffix().isEmpty()) fileName += ".txt";
  SaveDoubleVectorsToFile(fileName, &x, &y, &f);
}

void GraphWindowClass::on_actionSave_root_object_triggered()
{
  TObject *obj = DrawObjects.first().Pointer;
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
          QString fileName = fileDialog->getSaveFileName(this, "Save TH1 histogram", MW->GlobSet.LastOpenDir, "Root files(*.root)");
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
          QString fileName = fileDialog->getSaveFileName(this, "Save TH2 histogram", MW->GlobSet.LastOpenDir, "Root files(*.root)");
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

void GraphWindowClass::on_pbAddToBasket_clicked()
{   
    if (DrawObjects.isEmpty()) return;

    bool ok;
    int row = Basket->size();
    QString name = "Item"+QString::number(row);
    QString text = QInputDialog::getText(this, "New basket item",
                                         "Enter name:", QLineEdit::Normal,
                                         name, &ok);
    if (!ok || text.isEmpty()) return;

    AddCurrentToBasket(text);
}

void GraphWindowClass::AddCurrentToBasket(const QString & name)
{
    if (DrawObjects.isEmpty()) return;

    Basket->add(name.simplified(), DrawObjects);

    ui->actionToggle_Explorer_Basket->setChecked(true);
    UpdateBasketGUI();
}

void GraphWindowClass::AddLegend(double x1, double y1, double x2, double y2, QString title)
{
    TLegend* leg = RasterWindow->fCanvas->BuildLegend(x1, y1, x2, y2, title.toLatin1());

    RegisterTObject(leg);
    DrawObjects.append(ADrawObject(leg, "same"));

    RedrawAll();
}

void GraphWindowClass::SetLegendBorder(int color, int style, int size)
{
    for (int i=0; i<DrawObjects.size(); i++)
    {
        QString cn = DrawObjects[i].Pointer->ClassName();
        //qDebug() << cn;
        if (cn == "TLegend")
        {
            TLegend* le = dynamic_cast<TLegend*>(DrawObjects[i].Pointer);
            le->SetLineColor(color);
            le->SetLineStyle(style);
            le->SetLineWidth(size);

            RedrawAll();
            return;
        }
    }
    qDebug() << "Legend object was not found!";
}

void GraphWindowClass::AddText(QString text, bool bShowFrame, int Alignment_0Left1Center2Right)
{
  if (!text.isEmpty()) ShowTextPanel(text, bShowFrame, Alignment_0Left1Center2Right);
}

void GraphWindowClass::ExportTH2AsText(QString fileName)
{
    TObject *obj = DrawObjects.first().Pointer;
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
    TObject *obj = DrawObjects.first().Pointer;
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
    ui->lwBasket->clear();
    ui->lwBasket->addItems(Basket->getItemNames());
}

void GraphWindowClass::on_lwBasket_itemDoubleClicked(QListWidgetItem *)
{
    //qDebug() << "Row double clicked:"<<ui->lwBasket->currentRow();
    switchToBasket(ui->lwBasket->currentRow());
}

void GraphWindowClass::onRequestMakeCopy()
{
    PreviousDrawObjects = DrawObjects;
    setBasketItemUpdateAllowed(false);
    ui->pbBackToLast->setVisible(true);
}

void GraphWindowClass::onRequestInvalidateCopy()
{
    PreviousDrawObjects.clear();
    ui->pbBackToLast->setVisible(false);
}

void GraphWindowClass::onRequestRegister(TObject *tobj)
{
    tmpTObjects.append(tobj);
}

void GraphWindowClass::on_lwBasket_customContextMenuRequested(const QPoint &pos)
{
    QMenu BasketMenu;
    int row = -1;

    QListWidgetItem* temp = ui->lwBasket->itemAt(pos);

    QAction* switchToThis = 0;
    QAction* onTop = 0;
    QAction* del = 0;
    QAction* rename = 0;

    if (temp)
    {
        //menu triggered at a valid item
        row = ui->lwBasket->row(temp);

        BasketMenu.addSeparator();
        onTop = BasketMenu.addAction("Show on top of the main draw");
        switchToThis = BasketMenu.addAction("Switch to this");
        BasketMenu.addSeparator();
        rename = BasketMenu.addAction("Rename");
        BasketMenu.addSeparator();
        del = BasketMenu.addAction("Delete");
        BasketMenu.addSeparator();
    }
    BasketMenu.addSeparator();
    QAction* append = BasketMenu.addAction("Append basket file");
    QAction* appendTxt = BasketMenu.addAction("Append graph from text file");
    QAction* appendTxtEr = BasketMenu.addAction("Append graph with error bars from text file");
    QAction* appendRootHistsAndGraphs = BasketMenu.addAction("Append graphs / histograms from a ROOT file");
    BasketMenu.addSeparator();
    QAction* save = BasketMenu.addAction("Save basket to file");
    BasketMenu.addSeparator();
    QAction* clear = BasketMenu.addAction("Clear basket");

    //------

    QAction* selectedItem = BasketMenu.exec(ui->lwBasket->mapToGlobal(pos));
    if (!selectedItem) return; //nothing was selected

    if (selectedItem == switchToThis)
        switchToBasket(row);
    else if (selectedItem == clear)
    {
        QMessageBox msgBox;
        msgBox.setText("Clear basket cannot be undone!");
        msgBox.setInformativeText("Clear basket?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Yes)
            ClearBasket();
    }
    else if (selectedItem == save)
    {
        const QString fileName = QFileDialog::getSaveFileName(this, "Save basket to a file", MW->GlobSet.LastOpenDir, "Root files (*.root)");
        if (!fileName.isEmpty())
        {
            MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
            Basket->saveAll(fileName);
        }
    }
    else if (selectedItem == append)
    {
        const QString fileName = QFileDialog::getOpenFileName(this, "Append all from a basket file", MW->GlobSet.LastOpenDir, "Root files (*.root)");
        if (!fileName.isEmpty())
        {
            MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
            QString err = Basket->appendBasket(fileName);
            if (!err.isEmpty()) message(err, this);
            UpdateBasketGUI();
        }
    }
    else if (selectedItem == appendRootHistsAndGraphs)
    {
        const QString fileName = QFileDialog::getOpenFileName(this, "Append hist and graph objects from ROOT file", MW->GlobSet.LastOpenDir, "Root files (*.root)");
        if (!fileName.isEmpty())
        {
            MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
            Basket->appendRootHistGraphs(fileName);
            UpdateBasketGUI();
        }
    }
    else if (selectedItem == appendTxt)
    {
        QString fileName = QFileDialog::getOpenFileName(this, "Append graph from ascii file to basket", MW->GlobSet.LastOpenDir, "Data files (*.txt *.dat); All files (*.*)");
        if (fileName.isEmpty()) return;
        MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
        const QString res = Basket->appendTxtAsGraph(fileName);
        if (!res.isEmpty()) message(res, this);
        else
        {
            UpdateBasketGUI();
            switchToBasket(Basket->size() - 1);
        }
    }
    else if (selectedItem == appendTxtEr)
    {
        QString fileName = QFileDialog::getOpenFileName(this, "Append graph with errors from ascii file to basket", MW->GlobSet.LastOpenDir, "Data files (*.txt *.dat); All files (*.*)");
        if (fileName.isEmpty()) return;
        MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();
        const QString res = Basket->appendTxtAsGraphErrors(fileName);
        if (!res.isEmpty()) message(res, this);
        else
        {
            UpdateBasketGUI();
            switchToBasket(Basket->size() - 1);
        }
    }
    else if (selectedItem == del)
    {
        Basket->remove(row);
        UpdateBasketGUI();
    }
    else if (selectedItem == rename)
    {
        if (row == -1) return; //protection
        bool ok;
        QString text = QInputDialog::getText(this, "Rename basket item",
                                             "Enter new name:", QLineEdit::Normal,
                                             Basket->getName(row), &ok);
        if (ok && !text.isEmpty())
            Basket->rename(row, text.simplified());
        UpdateBasketGUI();
    }
    else if (selectedItem == onTop)
        Basket_DrawOnTop(row);
}

void GraphWindowClass::ClearBasket()
{
    Basket->clear();
    UpdateBasketGUI();
}

void GraphWindowClass::on_actionSave_image_triggered()
{
  QFileDialog *fileDialog = new QFileDialog;
  fileDialog->setDefaultSuffix("png");
  QString fileName = fileDialog->getSaveFileName(this, "Save image as file", MW->GlobSet.LastOpenDir, "png (*.png);;gif (*.gif);;Jpg (*.jpg)");
  if (fileName.isEmpty()) return;
  MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

  QFileInfo file(fileName);
  if (file.suffix().isEmpty()) fileName += ".png";

  GraphWindowClass::SaveGraph(fileName);
  if (MW->GlobSet.fOpenImageExternalEditor) QDesktopServices::openUrl(QUrl("file:"+fileName, QUrl::TolerantMode));
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

void GraphWindowClass::Basket_DrawOnTop(int row)
{
    if (row == -1) return;
    if (DrawObjects.isEmpty()) return;

    //qDebug() << "Basket item"<<row<<"was requested to be drawn on top of the current draw";
    QVector<ADrawObject> & BasketDrawObjects = Basket->getDrawObjects(row);

    for (int i=0; i<BasketDrawObjects.size(); i++)
    {
        TString CName = BasketDrawObjects[i].Pointer->ClassName();
        if ( CName== "TLegend" || CName == "TPaveText") continue;
        //qDebug() << CName;
        QString options = BasketDrawObjects[i].Options;
        options.replace("same", "", Qt::CaseInsensitive);
        options.replace("a", "", Qt::CaseInsensitive);
        TString safe = "same";
        safe += options.toLatin1().data();
        //qDebug() << "New options:"<<safe;
        DrawObjects.append(ADrawObject(BasketDrawObjects[i].Pointer, safe));
    }
    setBasketItemUpdateAllowed(false);
    RedrawAll();
}

void GraphWindowClass::on_pbSmooth_clicked()
{
  if (DrawObjects.isEmpty())
    {
      message("Object already does not exist!", this);
      return;
    }
  TObject *obj = DrawObjects.first().Pointer;
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
    TObject * obj = ( DrawObjects.isEmpty() ? nullptr : DrawObjects.first().Pointer );

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

void GraphWindowClass::on_actionToggle_toolbar_triggered(bool checked)
{
    ui->fUIbox->setVisible(checked);
}

void GraphWindowClass::on_actionEqualize_scale_XY_triggered()
{
   MW->WindowNavigator->BusyOn();
   //qDebug() << "Before-> X and Y size per pixel:" << RasterWindow->getXperPixel() << RasterWindow->getYperPixel();

   double XperP = fabs(RasterWindow->getXperPixel());
   double YperP = fabs(RasterWindow->getYperPixel());
   double CanvasWidth = RasterWindow->width();
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
}

void GraphWindowClass::on_pbAddLegend_clicked()
{
    TLegend* leg = RasterWindow->fCanvas->BuildLegend();

    RegisterTObject(leg);
    DrawObjects.append(ADrawObject(leg, "same"));

    RedrawAll();
}

void GraphWindowClass::on_pbRemoveLegend_clicked()
{
    for (int i=0; i<DrawObjects.size(); i++)
    {
        QString cn = DrawObjects[i].Pointer->ClassName();
        //qDebug() << cn;
        if (cn == "TLegend")
        {
            DrawObjects.remove(i);
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
  TPaveText* la = new TPaveText(0.15, 0.75, 0.5, 0.85, "NDC");
  la->SetFillColor(0);
  la->SetBorderSize(bShowFrame ? 1 : 0);
  la->SetLineColor(1);
  la->SetTextAlign( (AlignLeftCenterRight + 1) * 10 + 2);

  QStringList sl = Text.split("\n");
  for (QString s : sl) la->AddText(s.toLatin1());

  DrawWithoutFocus(la, "same", true, false); //it seems the Paveltext is owned by drawn object - registration causes crash if used with non-registered object (e.g. script)
}

void GraphWindowClass::SetStatPanelVisible(bool flag)
{
    ui->cbShowLegend->setChecked(flag);
}

void GraphWindowClass::on_pbRemoveText_clicked()
{
    for (int i=0; i<DrawObjects.size(); i++)
    {
        QString cn = DrawObjects[i].Pointer->ClassName();
        //qDebug() << cn;
        if (cn == "TPaveText")
        {
            DrawObjects.remove(i);
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
    if (DrawObjects.isEmpty())
    {
        message("No data", this);
        return;
    }

    const QString cn = DrawObjects.first().Pointer->ClassName();
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

    TH1* h =   static_cast<TH1*>(DrawObjects.first().Pointer);
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

        //QVector<ADrawObject> CopyMasterDrawObjects;
        //if (CurrentBasketItem == -1) CopyMasterDrawObjects = MasterDrawObjects;

        //drawing the fit
        Draw(f, "same");

        //draw base line
        if (ui->cbShowBaseLine->isChecked())
        {
            TF1 *fl = new TF1("line", "pol2", startX, stopX);
            fl->SetLineStyle(2);
            fl->SetParameters(c/b, -a/b);
            Draw(fl, "same");
        }

        //draw panel with results
        ShowTextPanel("fwhm = " + QString::number(FWHM) + "\nmean = " + QString::number(mid) + "\nfwhm/mean = "+QString::number(rel));

        //MasterDrawObjects = CopyMasterDrawObjects;
    }
    else message("Fit failed!", this);
}

void GraphWindowClass::on_ledAngle_customContextMenuRequested(const QPoint &pos)
{
    QMenu Menu;

    QAction* alignXWithRuler =Menu.addAction("Align X axis with the Ruler tool");
    QAction* alignYWithRuler =Menu.addAction("Align Y axis with the Ruler tool");

    QAction* selectedItem = Menu.exec(ui->ledAngle->mapToGlobal(pos));
    if (!selectedItem) return; //nothing was selected

    double angle = scene->getRuler()->getAngle() *180.0/M_PI;

    if (selectedItem == alignXWithRuler)
      {
        ui->ledAngle->setText( QString::number(angle, 'g', 4) );
        selBoxControlsUpdated();
      }
    else if (selectedItem == alignYWithRuler)
      {
        ui->ledAngle->setText( QString::number(angle - 90.0, 'g', 4) );
        selBoxControlsUpdated();
      }
}

void GraphWindowClass::on_pbBackToLast_clicked()
{
    DrawObjects = PreviousDrawObjects;
    setBasketItemUpdateAllowed(false);
    RedrawAll();
    ui->pbBackToLast->setVisible(false);
}

void GraphWindowClass::on_actionToggle_Explorer_Basket_toggled(bool arg1)
{
    int w = ui->fBasket->width();
    if (!arg1) w = -w;
    this->resize(this->width()+w, this->height());

    ui->fBasket->setVisible(arg1);
}

void GraphWindowClass::switchToBasket(int index)
{
    if (index < 0 || index >= Basket->size()) return;

    DrawObjects = Basket->getCopy(index);
    RedrawAll();

    ActiveBasketItem = index;
    setBasketItemUpdateAllowed(true);
}

void GraphWindowClass::setBasketItemUpdateAllowed(bool flag)
{
    if (ActiveBasketItem < 0 || ActiveBasketItem >= Basket->size()) flag = false;

    if (!flag) ActiveBasketItem = -1;
    ui->pbUpdateInBasket->setEnabled(flag);

    for (int i=0; i < ui->lwBasket->count(); i++)
    {
        QListWidgetItem * item = ui->lwBasket->item(i);
        if (i == ActiveBasketItem)
        {
            item->setForeground(QBrush(Qt::cyan));
            item->setBackground(QBrush(Qt::lightGray));
        }
        else
        {
            item->setForeground(QBrush(Qt::black));
            item->setBackground(QBrush(Qt::white));
        }
    }
    ui->lwBasket->clearSelection();
}

void GraphWindowClass::on_pbUpdateInBasket_clicked()
{
    if (ActiveBasketItem < 0 || ActiveBasketItem >= Basket->size()) return;
    Basket->update(ActiveBasketItem, DrawObjects);
}
