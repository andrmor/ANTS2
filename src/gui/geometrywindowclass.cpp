#include "TCanvas.h"
#include "geometrywindowclass.h"
#include "ui_geometrywindowclass.h"
#include "mainwindow.h"
#include "apmhub.h"
#include "apmtype.h"
#include "windownavigatorclass.h"
#include "detectorclass.h"
#include "rasterwindowbaseclass.h"
#include "globalsettingsclass.h"
#include "ajsontools.h"
#include "amessage.h"

#include <QDebug>
#include <QFileDialog>
#include <QDesktopServices>

#include "TView3D.h"
#include "TView.h"
#include "TGeoManager.h"
#include "TVirtualGeoTrack.h"

GeometryWindowClass::GeometryWindowClass(QWidget *parent, MainWindow *mw) :
  QMainWindow(parent), MW(mw),
  ui(new Ui::GeometryWindowClass)
{    
  ui->setupUi(this);

  Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
  windowFlags |= Qt::WindowCloseButtonHint;
  windowFlags |= Qt::WindowMinimizeButtonHint;
  windowFlags |= Qt::WindowMaximizeButtonHint;
  this->setWindowFlags( windowFlags );

  this->setMinimumWidth(200);
  RasterWindow = new RasterWindowBaseClass(this);
  //RasterWindow->resize(400, 400);
  centralWidget()->layout()->addWidget(RasterWindow);
  //RasterWindow->ForceResize();

  connect(RasterWindow, &RasterWindowBaseClass::UserChangedWindow, this, &GeometryWindowClass::onRasterWindowChange);

  QActionGroup* group = new QActionGroup( this );
  ui->actionSmall_dot->setActionGroup(group);
  ui->actionLarge_dot->setActionGroup(group);
  ui->actionSmall_cross->setActionGroup(group);
  ui->actionLarge_cross->setActionGroup(group);
}

GeometryWindowClass::~GeometryWindowClass()
{
  delete ui;

  //RasterWindow->setParent(0);
  //delete RasterWindow;
  //delete QWinContainer;
}

void GeometryWindowClass::ShowAndFocus()
{
  RasterWindow->fCanvas->cd();
  this->show();
  this->activateWindow();
  this->raise();
}

void GeometryWindowClass::SetAsActiveRootWindow()
{
  RasterWindow->fCanvas->cd();
}

void GeometryWindowClass::ClearRootCanvas()
{
  RasterWindow->fCanvas->Clear();
}

void GeometryWindowClass::UpdateRootCanvas()
{
  //RasterWindow->fCanvas->Modified();
  //RasterWindow->fCanvas->Update();
    RasterWindow->UpdateRootCanvas();
}

void GeometryWindowClass::SaveAs(const QString filename)
{
  RasterWindow->SaveAs(filename);
}

void GeometryWindowClass::OpenGLview()
{
  RasterWindow->fCanvas->GetViewer3D("ogl");
}

void GeometryWindowClass::ResetView()
{
  TView3D *v = (TView3D*)RasterWindow->fCanvas->GetView();

  v->SetPsi(0);
  v->SetLatitude(60.0);
  v->SetLongitude(-120.0);

  TMPignore = true;
  ui->cobViewType->setCurrentIndex(0);
  TMPignore = false;

  RasterWindow->UpdateRootCanvas();
}

void GeometryWindowClass::setHideUpdate(bool flag)
{
  RasterWindow->setVisible(!flag);
}

void GeometryWindowClass::PostDraw()
{  
  TView3D *v = dynamic_cast<TView3D*>(RasterWindow->fCanvas->GetView());
  if (!v) return;

  if (fNeedZoom) Zoom();
  fNeedZoom = false;

  if (ModePerspective)
    {
     if (!v->IsPerspective()) v->SetPerspective();
    }
  else
    {
      if (v->IsPerspective()) v->SetParallel();
    }

  if (fRecallWindow)
    {
      //recalling window properties
      //qDebug() << "Restoring window";
      RasterWindow->setWindowProperties(CenterX, CenterY, HWidth, HHeight, Phi, Theta);
    }
//  else
//    {
//      //saving window properties to use next draw
//      fRecallWindow = true;
//      RasterWindow->getWindowProperties(CenterX, CenterY, HWidth, HHeight, Phi, Theta);
//    }

  if (ui->cbShowAxes->isChecked()) v->ShowAxis();
  setHideUpdate(false);

  //canvas is updated in the caller
}

void GeometryWindowClass::onBusyOn()
{
   this->setEnabled(false);
   RasterWindow->setBlockEvents(true);
}

void GeometryWindowClass::onBusyOff()
{
  this->setEnabled(true);
  RasterWindow->setBlockEvents(false);
}

void GeometryWindowClass::writeWindowPropsToJson(QJsonObject &json)
{
  if (fRecallWindow)
    {
      QJsonObject js;
      js["CenterX"] = CenterX;
      js["CenterY"] = CenterY;
      js["HWidth"] = HWidth;
      js["HHeight"] = HHeight;
      js["Phi"] = Phi;
      js["Theta"] = Theta;
      json["GeometryWindow"] = js;
    }
}

void GeometryWindowClass::readWindowPropsFromJson(QJsonObject &json)
{
  QJsonObject js = json["GeometryWindow"].toObject();
  parseJson(js, "CenterX", CenterX);
  parseJson(js, "CenterY", CenterY);
  parseJson(js, "HWidth", HWidth);
  parseJson(js, "HHeight", HHeight);
  parseJson(js, "Phi", Phi);
  parseJson(js, "Theta", Theta);
}

bool GeometryWindowClass::IsWorldVisible()
{
    return ui->cbShowTop->isChecked();
}

void GeometryWindowClass::resizeEvent(QResizeEvent *)
{
  //if (RasterWindow) RasterWindow->ForceResize();
  //ShowGeometry(true, false);
}

bool GeometryWindowClass::event(QEvent *event)
{
  if (MW->WindowNavigator)
    {
      if (event->type() == QEvent::Hide) MW->WindowNavigator->HideWindowTriggered("geometry");
      else if (event->type() == QEvent::Show) MW->WindowNavigator->ShowWindowTriggered("geometry");
    }

  if (event->type() == QEvent::WindowActivate)
      RasterWindow->UpdateRootCanvas();

  return QMainWindow::event(event);
}

void GeometryWindowClass::ShowPMnumbers()
{
   QVector<QString> tmp(0);
   for (int i=0; i<MW->PMs->count(); i++) tmp.append( QString::number(i) );

   ShowTextOnPMs(tmp, kBlack);
}

void GeometryWindowClass::ShowTextOnPMs(QVector<QString> strData, Color_t color)
{
    //protection
    int numPMs = MW->PMs->count();
    if (strData.size() != numPMs)
      {
        message("Mismatch between the text vector and number of PMs!", this);
        return;
      }

    MW->Detector->GeoManager->ClearTracks();
    if (!isVisible()) show();

    //font size
      //checking minimum PM size
    double minSize = 1e10;
    for (int i=0; i<numPMs; i++)
      {
        int typ = MW->PMs->at(i).type;
        APmType tp = *MW->PMs->getType(typ);
        if (tp.SizeX<minSize) minSize = tp.SizeX;
        int shape = tp.Shape; //0 box, 1 round, 2 hexa
        if (shape == 0)
          if (tp.SizeY<minSize) minSize = tp.SizeY;
      }
      //max number of digits
    int symbols = 0;
    for (int i=0; i<numPMs; i++)
        if (strData[i].size() > symbols) symbols = strData[i].size();
    if (symbols == 0) symbols++;
//        qDebug()<<"Max number of symbols"<<symbols;
    double size = minSize/5.0/(0.5+0.5*symbols);

    //graphics ofr symbols
    QVector< QVector < double > > numbersX;
    QVector< QVector < double > > numbersY;
    QVector< double > tmp;
    numbersX.resize(0);
    numbersY.resize(0);

    //0
    tmp.resize(0);
    tmp<<-1<<1<<1<<-1<<-1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<-1.62<<-1.62<<1.62;
    numbersY.append(tmp);
    //1
    tmp.resize(0);
    //tmp<<0<<0;
    tmp<<-0.3<<0.3<<0.3;
    numbersX.append(tmp);
    tmp.resize(0);
    //tmp<<1.62<<-1.62;
    tmp<<0.42<<1.62<<-1.62;
    numbersY.append(tmp);
    //2
    tmp.resize(0);
    tmp<<-1<<1<<1<<-1<<-1<<1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<0<<0<<-1.62<<-1.62;
    numbersY.append(tmp);
    //3
    tmp.resize(0);
    tmp<<-1<<1<<1<<-1<<1<<1<<-1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<0<<0<<0<<-1.62<<-1.62;
    numbersY.append(tmp);
    //4
    tmp.resize(0);
    tmp<<-1<<-1<<1<<1<<1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<0<<0<<1.62<<-1.62;
    numbersY.append(tmp);
    //5
    tmp.resize(0);
    tmp<<1<<-1<<-1<<1<<1<<-1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<0<<0<<-1.62<<-1.62;
    numbersY.append(tmp);
    //6
    tmp.resize(0);
    tmp<<1<<-1<<-1<<1<<1<<-1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<-1.62<<-1.62<<0<<0;
    numbersY.append(tmp);
    //7
    tmp.resize(0);
    tmp<<-1<<1<<1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<-1.62;
    numbersY.append(tmp);
    //8
    tmp.resize(0);
    tmp<<-1<<1<<1<<-1<<-1<<1<<1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<0<<0<<-1.62<<-1.62<<1.62<<1.62<<0;
    numbersY.append(tmp);
    //9
    tmp.resize(0);
    tmp<<-1<<1<<1<<-1<<-1<<1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<-1.62<<-1.62<<1.62<<1.62<<0<<0;
    numbersY.append(tmp);
    //.
    tmp.resize(0);
    tmp<<-0.2<<0.2<<0.2<<-0.2<<-0.2;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<-1.2<<-1.2<<-1.6<<-1.6<<-1.2;
    numbersY.append(tmp);
    //-
    tmp.resize(0);
    tmp<<-1<<+1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<0<<0;
    numbersY.append(tmp);

    //mapping
    QVector<QString> SymbolMap(0);
    SymbolMap <<"0"<<"1"<<"2"<<"3"<<"4"<<"5"<<"6"<<"7"<<"8"<<"9"<<"."<<"-";

    for (int ipm=0; ipm<MW->PMs->count(); ipm++)
      {
        double Xcenter = MW->PMs->X(ipm);
        double Ycenter = MW->PMs->Y(ipm);
        double Zcenter = MW->PMs->Z(ipm);

        QString str = strData[ipm];
        if (str.isEmpty()) continue;
        int numDigits = str.size();
        if (str.right(1) == "F") numDigits--;
//        qDebug()<<"PM number: "<<ipm<<"    string="<<str<<"  digits="<<numDigits<<"size"<<size;

        int lineWidth = 2;
        if (size<2) lineWidth = 1;

        for (int idig=0; idig<numDigits; idig++)
          {
            QString str1 = str.mid(idig,1);

            int isymbol = -1;
            for (int i=0; i<SymbolMap.size(); i++)
              if (str1 == SymbolMap[i]) isymbol = i;

//            qDebug()<<"position="<<idig<<  "  To show: str="<<str1<<"index of mapping="<<isymbol;

            Int_t track_index = MW->Detector->GeoManager->AddTrack(2,22); //  Here track_index is the index of the newly created track in the array of primaries. One can get the pointer of this track and make it known as current track by the manager class:
            TVirtualGeoTrack *track = MW->Detector->GeoManager->GetTrack(track_index);
            if (str.right(1) == "F")
                track->SetLineColor(kRed);
            else
                track->SetLineColor(color);
            track->SetLineWidth(lineWidth);

            if (isymbol > -1)
             for (int i=0; i<numbersX[isymbol].size(); i++)
              {
                double x = Xcenter - 2.6*size*(0.5*(numDigits-1) - 1.0*idig) + size*numbersX[isymbol][i];
                double y = Ycenter + size*numbersY[isymbol][i];
                track->AddPoint(x, y, Zcenter, 0);
              }
          }
      }

    ShowGeometry(false);
    MW->Detector->GeoManager->DrawTracks();
    UpdateRootCanvas();
}

void GeometryWindowClass::AddLineToGeometry(QPointF& start, QPointF& end, Color_t color, int width)
{
  Int_t track_index = MW->Detector->GeoManager->AddTrack(2,22); //  Here track_index is the index of the newly created track in the array of primaries. One can get the pointer of this track and make it known as current track by the manager class:
  TVirtualGeoTrack *track = MW->Detector->GeoManager->GetTrack(track_index);
  track->SetLineColor(color);
  track->SetLineWidth(width);

  track->AddPoint(start.x(), start.y(), 0, 0);
  track->AddPoint(end.x(), end.y(), 0, 0);
}

void GeometryWindowClass::AddPolygonfToGeometry(QPolygonF& poly, Color_t color, int width)
{
  if (poly.size()<2) return;
  for (int i=0; i<poly.size()-1; i++)
    AddLineToGeometry(poly[i], poly[i+1], color, width);
}

void GeometryWindowClass::ShowGeometry(bool ActivateWindow, bool SAME, bool ColorUpdateAllowed)
// default:  ActivateWindow = true,  SAME = true,  ColorUpdateAllowed = true
{
    //qDebug()<<"  ----Showing geometry----"<<GeometryDrawDisabled;
    if (MW->GeometryDrawDisabled) return;

    //setting this window as active pad in root
    //with or without activation (focussing) of this window
    if (ActivateWindow) ShowAndFocus(); //window is activated (focused)
    else SetAsActiveRootWindow(); //no activation in this mode
    MW->Detector->GeoManager->SetNsegments(MW->GlobSet->NumSegments);
    int level = ui->sbLimitVisibility->value();
    if (!ui->cbLimitVisibility->isChecked()) level = -1;
    MW->Detector->GeoManager->SetVisLevel(level);

    //coloring volumes
    if (ColorUpdateAllowed)
      {
        if (ColorByMaterial) MW->Detector->colorVolumes(1);
        else MW->Detector->colorVolumes(0);
      }
    //top volume visibility
    if (ShowTop) MW->Detector->GeoManager->SetTopVisible(true); // the TOP is generally invisible
    else MW->Detector->GeoManager->SetTopVisible(false);

    //transparency setup
    int totNodes = MW->Detector->top->GetNdaughters();
    for (int i=0; i<totNodes; i++)
      {
        TGeoNode* thisNode = (TGeoNode*)MW->Detector->top->GetNodes()->At(i);
        thisNode->GetVolume()->SetTransparency(0);
      }

    //making contaners visible
    MW->Detector->top->SetVisContainers(true);

    //DRAW
    fNeedZoom = true;
    setHideUpdate(true);
    ClearRootCanvas();
    if (SAME)
      {
 //       qDebug()<<"keeping";
        MW->Detector->top->Draw("SAME");
      }
    else
      {
 //       qDebug()<<"new";
        //GeometryWindow->ResetView();
        MW->Detector->top->Draw("");
      }
    PostDraw();

    //drawing dots
    MW->ShowGeoMarkers();
    UpdateRootCanvas();
}

void GeometryWindowClass::on_pbShowGeometry_clicked()
{
  GeometryWindowClass::ShowAndFocus();
  RasterWindow->ForceResize();
  fRecallWindow = false;

  ShowGeometry(true, false); //not doing "same" option!
}

void GeometryWindowClass::on_cbShowTop_toggled(bool checked)
{
  ShowTop = checked;
  ShowGeometry(true, false);
}

void GeometryWindowClass::on_cbColor_toggled(bool checked)
{
  ColorByMaterial = checked;
  MW->UpdateMaterialListEdit();
  on_pbShowGeometry_clicked();
}

void GeometryWindowClass::on_pbShowPMnumbers_clicked()
{
  ShowPMnumbers();
}

void GeometryWindowClass::on_pbShowTracks_clicked()
{
    DrawTracks();
}

void GeometryWindowClass::DrawTracks()
{
  if (MW->GeometryDrawDisabled) return;

  SetAsActiveRootWindow();
  MW->Detector->GeoManager->DrawTracks();
  UpdateRootCanvas();
}

void GeometryWindowClass::on_pbClearTracks_clicked()
{
  MW->Detector->GeoManager->ClearTracks();
  ShowGeometry(true, false);
}

void GeometryWindowClass::on_pbClearDots_clicked()
{ 
  MW->clearGeoMarkers();
  ShowGeometry(true, false);
}

void GeometryWindowClass::on_pbSaveAs_clicked()
{
  QFileDialog *fileDialog = new QFileDialog;
  fileDialog->setDefaultSuffix("png");
  QString fileName = fileDialog->getSaveFileName(this, "Save image as file", MW->GlobSet->LastOpenDir, "png (*.png);;gif (*.gif);;Jpg (*.jpg)");
  if (fileName.isEmpty()) return;
  MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

  QFileInfo file(fileName);
  if(file.suffix().isEmpty()) fileName += ".png";

  GeometryWindowClass::SaveAs(fileName);

  if (MW->GlobSet->fOpenImageExternalEditor) QDesktopServices::openUrl(QUrl("file:"+fileName, QUrl::TolerantMode));
}

void GeometryWindowClass::on_pbShowGLview_clicked()
{
  //transparency setup
  int tran = ui->sbTransparency->value();

  TObjArray* list = MW->Detector->GeoManager->GetListOfVolumes();
  int numVolumes = list->GetEntries();
  for (int i=0; i<numVolumes; i++)
    {
      TGeoVolume* tv = (TGeoVolume*)list->At(i);
      tv->SetTransparency(tran);
    }
  GeometryWindowClass::OpenGLview();
}

void GeometryWindowClass::on_pbTop_clicked()
{
  SetAsActiveRootWindow();
  TView *v = RasterWindow->fCanvas->GetView();
  v->Top();

  RasterWindow->fCanvas->Modified();
  RasterWindow->fCanvas->Update();
  readRasterWindowProperties();
}

void GeometryWindowClass::on_pbFront_clicked()
{
  SetAsActiveRootWindow();
  TView *v = RasterWindow->fCanvas->GetView();
  v->Front();
  RasterWindow->fCanvas->Modified();
  RasterWindow->fCanvas->Update();
  readRasterWindowProperties();
}

void GeometryWindowClass::onRasterWindowChange(double centerX, double centerY, double hWidth, double hHeight, double phi, double theta)
{
  fRecallWindow = true;
  CenterX = centerX;
  CenterY = centerY;
  HWidth = hWidth;
  HHeight = hHeight;
  Phi = phi;
  Theta = theta;
}

void GeometryWindowClass::readRasterWindowProperties()
{
  fRecallWindow = true;
  RasterWindow->getWindowProperties(CenterX, CenterY, HWidth, HHeight, Phi, Theta);
}

void GeometryWindowClass::on_pbSide_clicked()
{
  SetAsActiveRootWindow();
  TView *v = RasterWindow->fCanvas->GetView();
  v->Side();
  RasterWindow->fCanvas->Modified();
  RasterWindow->fCanvas->Update();
  readRasterWindowProperties();
}

void GeometryWindowClass::on_cobViewType_currentIndexChanged(int index)
{
  if (TMPignore) return;

  TView *v = RasterWindow->fCanvas->GetView();
  if (index == 0)
    {
      ModePerspective = true;
      v->SetPerspective();
    }
  else
    {
      ModePerspective = false;
      v->SetParallel();
    }

  RasterWindow->fCanvas->Modified();
  RasterWindow->fCanvas->Update();
  readRasterWindowProperties();

#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
  RasterWindow->setInvertedXYforDrag( index==1 );
#endif
}

void GeometryWindowClass::on_cbShowAxes_toggled(bool /*checked*/)
{
  TView *v = RasterWindow->fCanvas->GetView();
  v->ShowAxis(); //it actually toggles show<->hide  
}


void GeometryWindowClass::on_actionSmall_dot_toggled(bool arg1)
{
  if (arg1)
    {
      GeoMarkerStyle = 1;
      ShowGeometry();
    }

  ui->actionSize_1->setEnabled(false);
  ui->actionSize_2->setEnabled(false);
}

void GeometryWindowClass::on_actionLarge_dot_triggered(bool arg1)
{
  if (arg1)
    {
      GeoMarkerStyle = 8;
      ShowGeometry();
    }

  ui->actionSize_1->setEnabled(true);
  ui->actionSize_2->setEnabled(true);
}

void GeometryWindowClass::on_actionSmall_cross_toggled(bool arg1)
{
  if (arg1)
    {
      GeoMarkerStyle = 6;
      ShowGeometry();
    }

  ui->actionSize_1->setEnabled(false);
  ui->actionSize_2->setEnabled(false);
}

void GeometryWindowClass::on_actionLarge_cross_toggled(bool arg1)
{
  if (arg1)
    {
      GeoMarkerStyle = 2;
      ShowGeometry();
    }

  ui->actionSize_1->setEnabled(true);
  ui->actionSize_2->setEnabled(true);
}

void GeometryWindowClass::on_actionSize_1_triggered()
{
  GeoMarkerSize++;
  ShowGeometry();

  ui->actionSize_2->setEnabled(true);
}

void GeometryWindowClass::on_actionSize_2_triggered()
{
  if (GeoMarkerSize>0) GeoMarkerSize--;

  if (GeoMarkerSize==0) ui->actionSize_2->setEnabled(false);
  else ui->actionSize_2->setEnabled(true);

  ShowGeometry();
}

void GeometryWindowClass::Zoom(bool update)
{
  TView3D *v = dynamic_cast<TView3D*>(RasterWindow->fCanvas->GetView());
  if (!v) return;

  double zoomFactor = 1.0;
  if (ZoomLevel>0) zoomFactor = pow(1.25, ZoomLevel);
  else if (ZoomLevel<0) zoomFactor = pow(0.8, -ZoomLevel);
  if (ZoomLevel != 0) v->ZoomView(0, zoomFactor);
  if (update)
    {
      RasterWindow->ForceResize();
      fRecallWindow = false;
      UpdateRootCanvas();
    }
}

void GeometryWindowClass::on_actionDefault_zoom_1_triggered()
{
    ZoomLevel++;
    on_pbShowGeometry_clicked();
}

void GeometryWindowClass::on_actionDefault_zoom_2_triggered()
{
    ZoomLevel--;
    on_pbShowGeometry_clicked();
}

void GeometryWindowClass::on_actionDefault_zoom_to_0_triggered()
{
    ZoomLevel = 0;
    on_pbShowGeometry_clicked();
}

void GeometryWindowClass::on_actionSet_line_width_for_objects_triggered()
{
    doChangeLineWidth(1);
}

void GeometryWindowClass::on_actionDecrease_line_width_triggered()
{
    doChangeLineWidth(-1);
}

void GeometryWindowClass::doChangeLineWidth(int deltaWidth)
{
    // for all WorldTree objects the following will be overriden. Still affects, e.g., PMs
    TObjArray* list = MW->Detector->GeoManager->GetListOfVolumes();
    int numVolumes = list->GetEntries();
    for (int i=0; i<numVolumes; i++)
      {
        TGeoVolume* tv = (TGeoVolume*)list->At(i);
        int LWidth = tv->GetLineWidth() + deltaWidth;
        if (LWidth <1) LWidth = 1;
        qDebug() << i << tv->GetName() << LWidth;
        tv->SetLineWidth(LWidth);
      }

    // for all WorldTree objects
    MW->Detector->changeLineWidthOfVolumes(deltaWidth);

    on_pbShowGeometry_clicked();
}

#include "anetworkmodule.h"
void GeometryWindowClass::on_pbWebViewer_clicked()
{
#ifdef USE_ROOT_HTML
    if (MW->NetModule->isRootServerRunning())
      {
        //QString t = "http://localhost:8080/?nobrowser&item=Objects/GeoWorld/World_1&opt=dray;all;tracks";
        //[Objects/GeoWorld/World_1,Objects/GeoTracks]
        QString t = "http://localhost:8080/?nobrowser&item=[Objects/GeoWorld/World_1,Objects/GeoTracks/TObjArray]&opt=dray;all;tracks";
        t += ";transp"+QString::number(ui->sbTransparency->value());
        if (ui->cbShowAxes->isChecked()) t += ";axis";

        QDesktopServices::openUrl(QUrl(t));
      }
    else
      {
        message("Root html server has to be started:"
                "\nUse MainWindow->Menu->Settings->Net->Run_CERN_ROOT_HTML_server", this);
      }
#else
    message("ANTS2 has to be compiled with the activated option in ants2.pro:"
            "\nCONFIG += ants2_RootServer\n", this);
#endif
}
