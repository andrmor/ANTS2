#include "TCanvas.h"
#include "geometrywindowclass.h"
#include "ui_geometrywindowclass.h"
#include "mainwindow.h"
#include "apmhub.h"
#include "apmtype.h"
#include "windownavigatorclass.h"
#include "detectorclass.h"
#include "rasterwindowbaseclass.h"
#include "aglobalsettings.h"
#include "ajsontools.h"
#include "amessage.h"
#include "asandwich.h"
#include "anetworkmodule.h"
#include "ageomarkerclass.h"
#include "ageoshape.h"
#include "acameracontroldialog.h"

#include <QStringList>
#include <QDebug>
#include <QFileDialog>
#include <QDesktopServices>
#include <QVBoxLayout>

#ifdef __USE_ANTS_JSROOT__
    #include <QWebEngineView>
    #include <QWebEnginePage>
    #include <QWebEngineProfile>
    #include <QWebEngineDownloadItem>
    #include "aroothttpserver.h"
#endif

#include "TView3D.h"
#include "TView.h"
#include "TGeoManager.h"
#include "TVirtualGeoTrack.h"

GeometryWindowClass::GeometryWindowClass(QWidget *parent, MainWindow *mw) :
  AGuiWindow("geometry", parent), MW(mw),
  ui(new Ui::GeometryWindowClass)
{    
    ui->setupUi(this);

    Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
    windowFlags |= Qt::WindowCloseButtonHint;
    windowFlags |= Qt::WindowMinimizeButtonHint;
    windowFlags |= Qt::WindowMaximizeButtonHint;
    //windowFlags |= Qt::Tool;
    this->setWindowFlags( windowFlags );

    this->setMinimumWidth(200);

    RasterWindow = new RasterWindowBaseClass(this);
    //centralWidget()->layout()->addWidget(RasterWindow);
    connect(RasterWindow, &RasterWindowBaseClass::userChangedWindow, this, &GeometryWindowClass::onRasterWindowChange);

    QVBoxLayout * layV = new QVBoxLayout();
    layV->setContentsMargins(0,0,0,0);
    layV->addWidget(RasterWindow);
    ui->swViewers->widget(0)->setLayout(layV);

#ifdef __USE_ANTS_JSROOT__
    WebView = new QWebEngineView(this);
    layV = new QVBoxLayout();
        layV->setContentsMargins(0,0,0,0);
        layV->addWidget(WebView);
    ui->swViewers->widget(1)->setLayout(layV);
    //WebView->load(QUrl("http://localhost:8080/?nobrowser&item=Objects/GeoWorld/world&opt=dray;all;tracks;transp50"));
    QWebEngineProfile::defaultProfile()->connect(QWebEngineProfile::defaultProfile(), &QWebEngineProfile::downloadRequested,
                                                 this, &GeometryWindowClass::onDownloadPngRequested);
#endif

    QActionGroup* group = new QActionGroup( this );
    ui->actionSmall_dot->setActionGroup(group);
    ui->actionLarge_dot->setActionGroup(group);
    ui->actionSmall_cross->setActionGroup(group);
    ui->actionLarge_cross->setActionGroup(group);

    ui->cbWireFrame->setVisible(false);

    generateSymbolMap();

    CameraControl = new ACameraControlDialog(RasterWindow, MW->Detector->Sandwich->World, this);
    CameraControl->setModal(false);
}

GeometryWindowClass::~GeometryWindowClass()
{
  delete ui;
}

void GeometryWindowClass::adjustGeoAttributes(TGeoVolume * vol, int Mode, int transp, bool adjustVis, int visLevel, int currentLevel)
{
    const int totNodes = vol->GetNdaughters();
    for (int i=0; i<totNodes; i++)
    {
        TGeoNode* thisNode = (TGeoNode*)vol->GetNodes()->At(i);
        TGeoVolume * v = thisNode->GetVolume();
        v->SetTransparency(Mode == 0 ? 0 : transp);
        if (Mode != 0)
        {
            if (adjustVis)
                v->SetAttBit(TGeoAtt::kVisOnScreen, (currentLevel < visLevel) );
            else
                v->SetAttBit(TGeoAtt::kVisOnScreen, true );
        }

        adjustGeoAttributes(v, Mode, transp, adjustVis, visLevel, currentLevel+1);
    }
}

void GeometryWindowClass::prepareGeoManager(bool ColorUpdateAllowed)
{
    if (!MW->Detector->top) return;

    int Mode = ui->cobViewer->currentIndex(); // 0 - standard, 1 - jsroot

    //root segments for roundish objects
    MW->Detector->GeoManager->SetNsegments(MW->GlobSet.NumSegments);

    //control of visibility of inner volumes
    int level = ui->sbLimitVisibility->value();
    if (!ui->cbLimitVisibility->isChecked()) level = -1;
    MW->Detector->GeoManager->SetVisLevel(level);

    if (ColorUpdateAllowed)
    {
        if (ColorByMaterial) MW->Detector->colorVolumes(1);
        else MW->Detector->colorVolumes(0);
    }

    MW->Detector->GeoManager->SetTopVisible(ui->cbShowTop->isChecked());
    //in this way jsroot have no problem to update visibility of top:
    //MW->Detector->GeoManager->SetTopVisible(true);
    //MW->Detector->top->SetVisibility(ui->cbShowTop->isChecked());
    MW->Detector->top->SetAttBit(TGeoAtt::kVisOnScreen, ui->cbShowTop->isChecked());

    int transp = ui->sbTransparency->value();
    MW->Detector->top->SetTransparency(Mode == 0 ? 0 : transp);
    adjustGeoAttributes(MW->Detector->top, Mode, transp, ui->cbLimitVisibility->isChecked(), level, 0);

    //making contaners visible
    MW->Detector->top->SetVisContainers(true);
}

void GeometryWindowClass::ShowGeometry(bool ActivateWindow, bool SAME, bool ColorUpdateAllowed)
{
    //qDebug()<<"  ----Showing geometry----" << MW->GeometryDrawDisabled;
    if (MW->GeometryDrawDisabled) return;

    prepareGeoManager(ColorUpdateAllowed);

    int Mode = ui->cobViewer->currentIndex(); // 0 - standard, 1 - jsroot

    if (Mode == 0)
    {
        if (ActivateWindow) ShowAndFocus(); //window is activated (focused)
        else SetAsActiveRootWindow(); //no activation in this mode

        //DRAW
        setHideUpdate(true);
        ClearRootCanvas();
        if (SAME) MW->Detector->top->Draw("SAME");
        else      MW->Detector->top->Draw("");
        PostDraw();

        //drawing dots
        MW->ShowGeoMarkers();
        UpdateRootCanvas();
    }
    else
    {
#ifdef __USE_ANTS_JSROOT__
        //qDebug() << "Before:" << gGeoManager->GetListOfTracks()->GetEntriesFast() << "markers: "<< MW->GeoMarkers.size();

        //deleting old markers
        TObjArray * Arr = gGeoManager->GetListOfTracks();
        const int numObj = Arr->GetEntriesFast();
        int iObj = 0;
        for (; iObj<numObj; iObj++)
            if (!dynamic_cast<TVirtualGeoTrack*>(Arr->At(iObj))) break;
        if (iObj < numObj)
        {
            //qDebug() << "First non-track object:"<<iObj;
            for (int iMarker=iObj; iMarker<numObj; iMarker++)
            {
                delete Arr->At(iMarker);
                (*Arr)[iMarker] = nullptr;
            }
            Arr->Compress();
        }
        //qDebug() << "After filtering markers:"<<gGeoManager->GetListOfTracks()->GetEntriesFast();

        if (!MW->GeoMarkers.isEmpty())
        {
            for (int i=0; i<MW->GeoMarkers.size(); i++)
            {
                GeoMarkerClass* gm = MW->GeoMarkers[i];
                //overrides
                if (gm->Type == "Recon" || gm->Type == "Scan" || gm->Type == "Nodes")
                {
                    gm->SetMarkerStyle(GeoMarkerStyle);
                    gm->SetMarkerSize(GeoMarkerSize);
                }

                TPolyMarker3D * mark = new TPolyMarker3D(*gm);
                gGeoManager->GetListOfTracks()->Add(mark);
            }
        }
        //qDebug() << "After:" << gGeoManager->GetListOfTracks()->GetEntriesFast();

        MW->NetModule->onNewGeoManagerCreated();

        QWebEnginePage * page = WebView->page();
        QString js = "var painter = JSROOT.GetMainPainter(\"onlineGUI_drawing\");";
        js += QString("painter.setAxesDraw(%1);").arg(ui->cbShowAxes->isChecked());
        js += QString("painter.setWireFrame(%1);").arg(ui->cbWireFrame->isChecked());
        js += QString("JSROOT.GEO.GradPerSegm = %1;").arg(ui->cbWireFrame->isChecked() ? 360 / MW->GlobSet.NumSegments : 6);
        js += QString("painter.setShowTop(%1);").arg(ui->cbShowTop->isChecked() ? "true" : "false");
        js += "if (JSROOT.hpainter) JSROOT.hpainter.updateAll();";
        page->runJavaScript(js);
#endif
    }

    CameraControl->updateGui();
}

/*
page->runJavaScript("JSROOT.GetMainPainter(\"onlineGUI_drawing\").produceCameraUrl(6)", [page](const QVariant &v)
{
    QString reply = v.toString();
    qDebug() << reply;
    QStringList sl = reply.split(',', QString::SkipEmptyParts); //quick parse just for now
    if (sl.size() > 2)
    {
        QString s;
        //s += "roty" + ui->leY->text() + ",";
        s += sl.at(0) + ",";
        //s += "rotz" + ui->leZ->text() + ",";
        s += sl.at(1) + ",";
        //s += "zoom" + ui->leZoom->text() + ",";
        s += sl.at(2) + ",";
        s += "dray,nohighlight,all,tracks,transp50";
        qDebug() << s;

        page->runJavaScript("JSROOT.redraw(\"onlineGUI_drawing\", JSROOT.GetMainPainter(\"onlineGUI_drawing\").GetObject(), \"" + s + "\");");
    }
});
*/


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
    RasterWindow->UpdateRootCanvas();
}

void GeometryWindowClass::SaveAs(const QString filename)
{
    RasterWindow->SaveAs(filename);
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

  if (!fRecallWindow) Zoom();

  if (ModePerspective)
  {
     if (!v->IsPerspective()) v->SetPerspective();
  }
  else
  {
      if (v->IsPerspective()) v->SetParallel();
  }

  if (fRecallWindow) RasterWindow->setWindowProperties();

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

/*
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
*/

bool GeometryWindowClass::IsWorldVisible()
{
    return ui->cbShowTop->isChecked();
}

bool GeometryWindowClass::event(QEvent *event)
{
  if (event->type() == QEvent::WindowActivate)
      RasterWindow->UpdateRootCanvas();

  return AGuiWindow::event(event);
}

#include <QCloseEvent>
void GeometryWindowClass::closeEvent(QCloseEvent * event)
{
    //qDebug() << "Geometry window close event";

    //fix for bug with root reported for Qt 5.14: close then restore results in resize of the canvas to huge size, and nothing is shown on the widow
    event->ignore();
    hide();
}

#include "anetworkmodule.h"
void GeometryWindowClass::ShowPMnumbers()
{
   QVector<QString> tmp;
   for (int i=0; i<MW->PMs->count(); i++)
       tmp.append( QString::number(i) );
   ShowText(tmp, kBlack, true);

   MW->NetModule->onNewGeoManagerCreated();
}

void GeometryWindowClass::ShowMonitorIndexes()
{
    QVector<QString> tmp;
    const QVector<const AGeoObject*> & MonitorsRecords = MW->Detector->Sandwich->MonitorsRecords;
    for (int i=0; i<MonitorsRecords.size(); i++)
        tmp.append( QString::number(i) );
    ShowText(tmp, kBlue, false);
}

#include "ageoobject.h"
#include "TGeoNode.h"
bool findMotherNodeFor(const TGeoNode * node, const TGeoNode * startNode, const TGeoNode* & foundNode)
{
    TGeoVolume * startVol = startNode->GetVolume();
    //qDebug() << "    Starting from"<<startVol->GetName();
    TObjArray * nList = startVol->GetNodes();
    if (!nList) return false;
    int numNodes = nList->GetEntries();
    //qDebug() << "    Num nodes:"<< numNodes;
    for (int inod=0; inod<numNodes; inod++)
    {
        TGeoNode * n = (TGeoNode*)nList->At(inod);
        //qDebug() << "    Checking "<< n->GetName();
        if (n == node)
        {
            //qDebug() << "    Match!";
            foundNode = startNode;
            return true;
        }
        //qDebug() << "    Sending down the line";
        bool bFound = findMotherNodeFor(node, n, foundNode);
        //qDebug() << "    Found?"<<bFound;
        if (bFound) return true;
    }
    return false;
}

void findMotherNode(const TGeoNode * node, const TGeoNode* & motherNode)
{
    //qDebug() << "--- search for " << node->GetName();
    TObjArray* allNodes = gGeoManager->GetListOfNodes();
    //qDebug() << allNodes->GetEntries();
    if (allNodes->GetEntries() != 1) return; // should be only World
    TGeoNode * worldNode = (TGeoNode*)allNodes->At(0);
    //qDebug() << worldNode->GetName();

    motherNode = worldNode;
    if (node == worldNode) return; //already there

    findMotherNodeFor(node, worldNode, motherNode); //bool OK = ...
//    if (bOK)
//        qDebug() << "--- found mother node:"<<motherNode->GetName();
//    else qDebug() << "--- search failed!";
}

void GeometryWindowClass::generateSymbolMap()
{
    //0
    SymbolMap << "0";
    numbersX.append({-1,1,1,-1,-1});
    numbersY.append({1.62,1.62,-1.62,-1.62,1.62});
    //1
    SymbolMap << "1";
    numbersX.append({-0.3,0.3,0.3});
    numbersY.append({0.42,1.62,-1.62});
    //2
    SymbolMap << "2";
    numbersX.append({-1,1,1,-1,-1,1});
    numbersY.append({1.62,1.62,0,0,-1.62,-1.62});
    //3
    SymbolMap << "3";
    numbersX.append({-1,1,1,-1,1,1,-1});
    numbersY.append({1.62,1.62,0,0,0,-1.62,-1.62});
    //4
    SymbolMap << "4";
    numbersX.append({-1,-1,1,1,1});
    numbersY.append({1.62,0,0,1.62,-1.62});
    //5
    SymbolMap << "5";
    numbersX.append({1,-1,-1,1,1,-1});
    numbersY.append({1.62,1.62,0,0,-1.62,-1.62});
    //6
    SymbolMap << "6";
    numbersX.append({1,-1,-1,1,1,-1});
    numbersY.append({1.62,1.62,-1.62,-1.62,0,0});
    //7
    SymbolMap << "7";
    numbersX.append({-1,1,1});
    numbersY.append({1.62,1.62,-1.62});
    //8
    SymbolMap << "8";
    numbersX.append({-1,1,1,-1,-1,1,1});
    numbersY.append({0,0,-1.62,-1.62,1.62,1.62,0});
    //9
    SymbolMap << "9";
    numbersX.append({-1   , 1   ,   1,  -1, -1,1});
    numbersY.append({-1.62,-1.62,1.62,1.62,  0,0});
    //.
    SymbolMap << ".";
    numbersX.append({-0.2, 0.2, 0.2,-0.2,-0.2});
    numbersY.append({-1.2,-1.2,-1.6,-1.6,-1.2});
    //-
    SymbolMap << "-";
    numbersX.append({-1, 1});
    numbersY.append({ 0, 0});
}

void GeometryWindowClass::ShowText(const QVector<QString> &strData, Color_t color, bool onPMs, bool bFullCycle)
{
    const APmHub & PMs = *MW->PMs;
    const QVector<const AGeoObject*> & Mons = MW->Detector->Sandwich->MonitorsRecords;

    int numObj = ( onPMs ? PMs.count() : Mons.size() );
    if (strData.size() != numObj)
    {
        message("Show text: mismatch in vector size", this);
        return;
    }
    //qDebug() << "Objects:"<<numObj;

    if (bFullCycle) MW->Detector->GeoManager->ClearTracks();
    if (!isVisible()) showNormal();

    //font size
      //checking minimum size
    double minSize = 1e10;
    for (int i = 0; i < numObj; i++)
    {
        if (onPMs)
        {
            int typ = MW->PMs->at(i).type;
            APmType tp = *MW->PMs->getType(typ);
            if (tp.SizeX<minSize) minSize = tp.SizeX;
            int shape = tp.Shape; //0 box, 1 round, 2 hexa
            if (shape == 0)
              if (tp.SizeY<minSize) minSize = tp.SizeY;
        }
        else
        {
            double msize = Mons.at(i)->Shape->minSize();
            if (msize < minSize) minSize = msize;
        }
    }

      //max number of digits
    int symbols = 0;
    for (int i=0; i<numObj; i++)
        if (strData[i].size() > symbols) symbols = strData[i].size();
    if (symbols == 0) symbols++;
//        qDebug()<<"Max number of symbols"<<symbols;
    double size = minSize/5.0/(0.5+0.5*symbols);

    for (int iObj = 0; iObj < numObj; iObj++)
    {
        double Xcenter = 0;
        double Ycenter = 0;
        double Zcenter = 0;
        if (onPMs)
        {
            Xcenter = PMs.at(iObj).x;
            Ycenter = PMs.at(iObj).y;
            Zcenter = PMs.at(iObj).z;
        }
        else
        {
            const TGeoNode * n = MW->Detector->Sandwich->MonitorNodes.at(iObj);
            //qDebug() << "\nProcessing monitor"<<n->GetName();

            double pos[3], master[3];
            pos[0] = 0;
            pos[1] = 0;
            pos[2] = 0;

            TGeoVolume * motherVol = n->GetMotherVolume();
            while (motherVol)
            {
                //qDebug() << "  Mother vol is:"<< motherVol->GetName();
                n->LocalToMaster(pos, master);
                pos[0] = master[0];
                pos[1] = master[1];
                pos[2] = master[2];
                //qDebug() << "  Position:"<<pos[0]<<pos[1]<<pos[2];

                const TGeoNode * motherNode = nullptr;
                findMotherNode(n, motherNode);
                if (!motherNode)
                {
                    //qDebug() << "  Mother node not found!";
                    break;
                }
                if (motherNode == n)
                {
                    //qDebug() << "  strange - world passed";
                    break;
                }

                n = motherNode;

                motherVol = n->GetMotherVolume();
                //qDebug() << "  Continue search: current node:"<<n->GetName();
            }
            Xcenter = pos[0];
            Ycenter = pos[1];
            Zcenter = pos[2];
        }

        QString str = strData[iObj];
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

    if (bFullCycle)
    {
        ShowGeometry(false);
        MW->Detector->GeoManager->DrawTracks();
        UpdateRootCanvas();
    }
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

#include "aeventtrackingrecord.h"
#include "asimulationmanager.h"
#include "amaterialparticlecolection.h"
#include "TGeoTrack.h"
#include "atrackrecords.h"
void GeometryWindowClass::ShowEvent_Particles(size_t iEvent, bool withSecondaries)
{
    if (iEvent < MW->SimulationManager->TrackingHistory.size())
    {
        const AEventTrackingRecord * er = MW->SimulationManager->TrackingHistory.at(iEvent);
        er->makeTracks(MW->SimulationManager->Tracks, MW->MpCollection->getListOfParticleNames(), MW->SimulationManager->TrackBuildOptions, withSecondaries);

        for (int iTr=0; iTr < (int)MW->SimulationManager->Tracks.size(); iTr++)
        {
            const TrackHolderClass* th = MW->SimulationManager->Tracks.at(iTr);
            TGeoTrack* track = new TGeoTrack(1, th->UserIndex);
            track->SetLineColor(th->Color);
            track->SetLineWidth(th->Width);
            track->SetLineStyle(th->Style);
            for (int iNode=0; iNode<th->Nodes.size(); iNode++)
                track->AddPoint(th->Nodes[iNode].R[0], th->Nodes[iNode].R[1], th->Nodes[iNode].R[2], th->Nodes[iNode].Time);
            if (track->GetNpoints()>1)
                MW->Detector->GeoManager->AddTrack(track);
            else delete track;
        }
    }

    DrawTracks();
}

#include "eventsdataclass.h"
void GeometryWindowClass::ShowPMsignals(int iEvent, bool bFullCycle)
{
    if (iEvent < 0 || iEvent >= MW->EventsDataHub->countEvents()) return;

    QVector<QString> tmp;
    for (int i=0; i<MW->PMs->count(); i++)
        tmp.append( QString::number(MW->EventsDataHub->Events.at(iEvent).at(i)) );
    ShowText(tmp, kBlack, true, bFullCycle);
}

void GeometryWindowClass::ShowGeoMarkers()
{
    if (!MW->GeoMarkers.isEmpty())
    {
        int Mode = ui->cobViewer->currentIndex(); // 0 - standard, 1 - jsroot
        if (Mode == 0)
        {
            SetAsActiveRootWindow();
            for (int i=0; i<MW->GeoMarkers.size(); i++)
            {
                GeoMarkerClass* gm = MW->GeoMarkers[i];
                //overrides
                if (gm->Type == "Recon" || gm->Type == "Scan" || gm->Type == "Nodes")
                {
                    gm->SetMarkerStyle(GeoMarkerStyle);
                    gm->SetMarkerSize(GeoMarkerSize);
                }
                gm->Draw("same");
            }
            UpdateRootCanvas();
        }
        else
        ShowGeometry(false);
    }
}

void GeometryWindowClass::ShowTracksAndMarkers()
{
    int Mode = ui->cobViewer->currentIndex(); // 0 - standard, 1 - jsroot
    if (Mode == 0)
    {
        DrawTracks();
        ShowGeoMarkers();
    }
    else
    {
        ShowGeometry(true, false);
    }
}

void GeometryWindowClass::ClearTracks(bool bRefreshWindow)
{
    MW->Detector->GeoManager->ClearTracks();
    if (bRefreshWindow)
    {
        int Mode = ui->cobViewer->currentIndex(); // 0 - standard, 1 - jsroot
        if (Mode == 0)
        {
            SetAsActiveRootWindow();
            UpdateRootCanvas();
        }
        else ShowGeometry(false);
    }
}

void GeometryWindowClass::on_pbShowGeometry_clicked()
{
    //qDebug() << "Redraw triggered!";
    ShowAndFocus();

    int Mode = ui->cobViewer->currentIndex(); // 0 - standard, 1 - jsroot
    if (Mode == 0)
    {
        RasterWindow->ForceResize();
        fRecallWindow = false;
    }

    ShowGeometry(true, false); //not doing "same" option!
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

void GeometryWindowClass::on_pbShowMonitorIndexes_clicked()
{
    ShowMonitorIndexes();
}

void GeometryWindowClass::on_pbShowTracks_clicked()
{
    DrawTracks();
}

void GeometryWindowClass::DrawTracks()
{
    if (MW->GeometryDrawDisabled) return;

    int Mode = ui->cobViewer->currentIndex(); // 0 - standard, 1 - jsroot
    if (Mode == 0)
    {
        SetAsActiveRootWindow();
        MW->Detector->GeoManager->DrawTracks();
        UpdateRootCanvas();
    }
    else ShowGeometry(false);
}

#include "ageomarkerclass.h"
void GeometryWindowClass::ShowPoint(double * r, bool keepTracks)
{
    MW->clearGeoMarkers();

    GeoMarkerClass* marks = new GeoMarkerClass("Source", 3, 10, kBlack);
    marks->SetNextPoint(r[0], r[1], r[2]);
    MW->GeoMarkers.append(marks);
    GeoMarkerClass* marks1 = new GeoMarkerClass("Source", 4, 3, kRed);
    marks1->SetNextPoint(r[0], r[1], r[2]);
    MW->GeoMarkers.append(marks1);

    ShowGeometry(false);
    if (keepTracks) DrawTracks();
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

void GeometryWindowClass::on_pbTop_clicked()
{
    if (ui->cobViewer->currentIndex() == 0)
    {
        SetAsActiveRootWindow();
        TView *v = RasterWindow->fCanvas->GetView();
        v->Top();
        RasterWindow->fCanvas->Modified();
        RasterWindow->fCanvas->Update();
        readRasterWindowProperties();
    }
    else
    {
#ifdef __USE_ANTS_JSROOT__
        QWebEnginePage * page = WebView->page();
        QString js = ""
        "var painter = JSROOT.GetMainPainter(\"onlineGUI_drawing\");"
        "painter.setCameraPosition(90,0);";
        page->runJavaScript(js);

        /*
        page->runJavaScript("JSROOT.GetMainPainter(\"onlineGUI_drawing\").produceCameraUrl()", [page](const QVariant &v)
        {
            QString reply = v.toString();
            qDebug() << reply; // let's ask Sergey to make JSON with this data
            QStringList sl = reply.split(',', QString::SkipEmptyParts); //quick parse just for now
            if (sl.size() > 2)
            {
                QString s;
                //s += "roty" + ui->leY->text() + ",";
                s += "roty90,";
                //s += "rotz" + ui->leZ->text() + ",";
                s += "rotz0,";
                //s += "zoom" + ui->leZoom->text() + ",";
                s += sl.at(2) + ",";
                s += "dray,nohighlight,all,tracks,transp50";
                page->runJavaScript("JSROOT.redraw(\"onlineGUI_drawing\", JSROOT.GetMainPainter(\"onlineGUI_drawing\").GetObject(), \"" + s + "\");");
            }
        });
        */
#endif
    }
}

void GeometryWindowClass::on_pbFront_clicked()
{
    if (ui->cobViewer->currentIndex() == 0)
    {
        SetAsActiveRootWindow();
        TView *v = RasterWindow->fCanvas->GetView();
        v->Front();
        RasterWindow->fCanvas->Modified();
        RasterWindow->fCanvas->Update();
        readRasterWindowProperties();
    }
    else
    {
#ifdef __USE_ANTS_JSROOT__
        QWebEnginePage * page = WebView->page();
        QString js = ""
        "var painter = JSROOT.GetMainPainter(\"onlineGUI_drawing\");"
        "painter.setCameraPosition(90,90);";
        page->runJavaScript(js);
#endif
    }
}

void GeometryWindowClass::onRasterWindowChange()
{
    fRecallWindow = true;
    CameraControl->updateGui();
}

void GeometryWindowClass::readRasterWindowProperties()
{
    fRecallWindow = true;
    RasterWindow->ViewParameters.read(RasterWindow->fCanvas);   // !*! method
}

void GeometryWindowClass::on_pbSide_clicked()
{
    if (ui->cobViewer->currentIndex() == 0)
    {
        SetAsActiveRootWindow();
        TView *v = RasterWindow->fCanvas->GetView();
        v->Side();
        RasterWindow->fCanvas->Modified();
        RasterWindow->fCanvas->Update();
        readRasterWindowProperties();
    }
    else
    {
#ifdef __USE_ANTS_JSROOT__
        QWebEnginePage * page = WebView->page();
        QString js = ""
                     "var painter = JSROOT.GetMainPainter(\"onlineGUI_drawing\");"
                     "painter.setCameraPosition(0.001,0.01);";
        page->runJavaScript(js);
#endif
    }
}

void GeometryWindowClass::on_cbShowAxes_toggled(bool /*checked*/)
{
    if (ui->cobViewer->currentIndex() == 0)
    {
        TView *v = RasterWindow->fCanvas->GetView();
        v->ShowAxis(); //it actually toggles show<->hide
    }
    else ShowGeometry(true, false);
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
      //fRecallWindow = false;
      UpdateRootCanvas();
  }
}

void GeometryWindowClass::FocusVolume(const QString & name)
{
    CameraControl->setFocus(name);
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

//#include <QElapsedTimer>
void GeometryWindowClass::showWebView()
{
#ifdef __USE_ANTS_JSROOT__
    //WebView->load(QUrl("http://localhost:8080/?nobrowser&item=[Objects/GeoWorld/WorldBox_1,Objects/GeoTracks/TObjArray]&opt=nohighlight;dray;all;tracks;transp50"));
    //WebView->load(QUrl("http://localhost:8080/?item=[Objects/GeoWorld/WorldBox_1,Objects/GeoTracks/TObjArray]&opt=nohighlight;dray;all;tracks;transp50"));
    //WebView->load(QUrl("http://localhost:8080/?item=[Objects/GeoWorld/world,Objects/GeoTracks/TObjArray]&opt=nohighlight;dray;all;tracks;transp50"));

    QString s = "http://localhost:8080/?nobrowser&item=Objects/GeoWorld/world&opt=nohighlight;dray;all;tracks";
    //QString s = "http://localhost:8080/?item=Objects/GeoWorld/world&opt=nohighlight;dray;all;tracks";
    if (ui->cbShowTop->isChecked())
        s += ";showtop";
    if (ui->cobViewType->currentIndex() == 1)
        s += ";ortho_camera_rotate";
    if (ui->cbWireFrame->isChecked())
        s += ";wireframe";
    s += QString(";transp%1").arg(ui->sbTransparency->value());

    prepareGeoManager(true);

    WebView->load(QUrl(s));
    WebView->show();

    /*
    QWebEnginePage * page = WebView->page();
    QString js = ""
    "var wait = true;"
    "if ((typeof JSROOT != \"undefined\") && JSROOT.GetMainPainter)"
    "{"
    "   var p = JSROOT.GetMainPainter(\"onlineGUI_drawing\");"
    "   if (p && p.isDrawingReady()) wait = false;"
    "}"
    "wait";

    bool bWait = true;
    QElapsedTimer timer;
    timer.start();
    do
    {
        qApp->processEvents();

        page->runJavaScript(js, [&bWait](const QVariant &v)
        {
            bWait = v.toBool();
        });
        //qDebug() << bWait << timer.elapsed();
    }
    while (bWait && timer.elapsed() < 2000);

    qDebug() << "exit!";
    */

    //ShowGeometry(true, false);
#endif
}

#include "globalsettingswindowclass.h"
void GeometryWindowClass::on_cobViewer_currentIndexChanged(int index)
{
#ifdef __USE_ANTS_JSROOT__
    if (index == 0)
    {
        ui->swViewers->setCurrentIndex(0);
        on_pbShowGeometry_clicked();
    }
    else
    {
        if (!MW->NetModule->isRootServerRunning())
        {
            bool bOK = MW->NetModule->StartRootHttpServer();
            if (!bOK)
            {
                ui->cobViewer->setCurrentIndex(0);
                message("Failed to start root http server. Check if another server is running at the same port", this);
                MW->GlobSetWindow->ShowNetSettings();
                return;
            }
        }

        ui->swViewers->setCurrentIndex(1);
        showWebView();
    }
    ui->cbWireFrame->setVisible(index == 1);
#else
    if (index != 0)
    {
        ui->cobViewer->setCurrentIndex(0);
        message("Enable ants2_jsroot in ants2.pro and rebuild ants2", this);
    }
#endif
}

void GeometryWindowClass::on_actionOpen_GL_viewer_triggered()
{
    int tran = ui->sbTransparency->value();
    TObjArray * list = MW->Detector->GeoManager->GetListOfVolumes();
    int numVolumes = list->GetEntries();

    for (int i = 0; i < numVolumes; i++)
    {
        TGeoVolume * tv = (TGeoVolume*)list->At(i);
        tv->SetTransparency(tran);
    }
    OpenGLview();
}

void GeometryWindowClass::OpenGLview()
{
    RasterWindow->fCanvas->GetViewer3D("ogl");
}

void GeometryWindowClass::on_actionJSROOT_in_browser_triggered()
{
#ifdef USE_ROOT_HTML
    if (MW->NetModule->isRootServerRunning())
    {
        //QString t = "http://localhost:8080/?nobrowser&item=[Objects/GeoWorld/WorldBox_1,Objects/GeoTracks/TObjArray]&opt=dray;all;tracks";
        QString t = "http://localhost:8080/?nobrowser&item=Objects/GeoWorld/world&opt=dray;all;tracks";
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

/*
    QWebEnginePage * page = WebView->page();
    page->runJavaScript(

//    "JSROOT.NewHttpRequest(\"http://localhost:8080/Objects/GeoWorld/WorldBox_1/root.json\", \"object\","
//    //"JSROOT.NewHttpRequest(\"http://localhost:8080/Objects/GeoTracks/TObjArray/root.json\", \"object\","
//    "function(res) {"
//        //"if (res) console.log('Retrieve object', res._typename);"
//        //    "else console.error('Fail to get object');"
//        //  "JSROOT.cleanup();"

//          //"JSROOT.redraw(\"onlineGUI_drawing\", res, \"transp50,tracks\");"
//          //"JSROOT.redraw(\"onlineGUI_drawing\", res, \"transp50,tracks\");"
//          "JSROOT.draw(\"onlineGUI_drawing\", res, \"transp50,tracks\");"
//    "}).send();");



                "var xhr = JSROOT.NewHttpRequest('http://localhost:8080/multi.json?number=2', 'multi', function(res)"
                                                        "{"
                                                            "if (!res) return;"
                                                            "JSROOT.redraw('onlineGUI_drawing', res[0], 'transp50;tracks');"
                                                        "}"
                                                ");"
                "xhr.send('Objects/GeoWorld/WorldBox_1/root.json\\nObjects/GeoTracks/TObjArray/root.json\\n');"
                //"xhr.send('/Objects/GeoWorld/WorldBox_1/root.json\n');"
                );
*/

void GeometryWindowClass::on_cbWireFrame_toggled(bool)
{
    ShowGeometry(true, false);
}

void GeometryWindowClass::on_cbLimitVisibility_clicked()
{
    int Mode = ui->cobViewer->currentIndex(); // 0 - standard, 1 - jsroot
    if (Mode == 0)
        ShowGeometry(true, false);
    else
    {
#ifdef __USE_ANTS_JSROOT__
        int level = ui->sbLimitVisibility->value();
        if (!ui->cbLimitVisibility->isChecked()) level = -1;
        MW->Detector->GeoManager->SetVisLevel(level);
        MW->NetModule->onNewGeoManagerCreated();

        prepareGeoManager();
        showWebView();
#endif
    }
}

void GeometryWindowClass::on_sbLimitVisibility_editingFinished()
{
    on_cbLimitVisibility_clicked();
}

void GeometryWindowClass::on_cbShowTop_toggled(bool)
{
    ShowGeometry(true, false);
/*
    int Mode = ui->cobViewer->currentIndex(); // 0 - standard, 1 - jsroot
    if (Mode == 0)
        ShowGeometry(true, false);
    else
    {
#ifdef __USE_ANTS_JSROOT__
        ShowGeometry(true, false);
        QWebEnginePage * page = WebView->page();
        QString js = "var painter = JSROOT.GetMainPainter(\"onlineGUI_drawing\");";
        //js += QString("painter.options.showtop = %1;").arg(checked ? "true" : "false");
        js += QString("painter.setShowTop(%1);").arg(checked ? "true" : "false");
        js += "painter.startDrawGeometry();";
        page->runJavaScript(js);
#endif
    }
*/
}

void GeometryWindowClass::on_cobViewType_currentIndexChanged(int index)
{
    if (TMPignore) return;

    int Mode = ui->cobViewer->currentIndex(); // 0 - standard, 1 - jsroot
    if (Mode == 0)
    {
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
    else
    {
        prepareGeoManager();
        showWebView();
       // ShowGeometry(true, false);
    }
}

void GeometryWindowClass::on_pbSaveAs_clicked()
{
    int Mode = ui->cobViewer->currentIndex(); // 0 - standard, 1 - jsroot
    if (Mode == 0)
    {
        QFileDialog *fileDialog = new QFileDialog;
        fileDialog->setDefaultSuffix("png");
        QString fileName = fileDialog->getSaveFileName(this, "Save image as file", MW->GlobSet.LastOpenDir, "png (*.png);;gif (*.gif);;Jpg (*.jpg)");
        if (fileName.isEmpty()) return;
        MW->GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

        QFileInfo file(fileName);
        if(file.suffix().isEmpty()) fileName += ".png";
        GeometryWindowClass::SaveAs(fileName);
        if (MW->GlobSet.fOpenImageExternalEditor) QDesktopServices::openUrl(QUrl("file:"+fileName, QUrl::TolerantMode));
    }
    else
    {
#ifdef __USE_ANTS_JSROOT__
        QWebEnginePage * page = WebView->page();
        QString js = "var painter = JSROOT.GetMainPainter(\"onlineGUI_drawing\");";
        js += QString("painter.createSnapshot('dummy.png')");
        page->runJavaScript(js);
#endif
    }
}

void GeometryWindowClass::onDownloadPngRequested(QWebEngineDownloadItem *item)
{
#ifdef __USE_ANTS_JSROOT__
    QString fileName = QFileDialog::getSaveFileName(this, "Select file name to safe image");
    if (fileName.isEmpty())
    {
        item->cancel();
        return;
    }
    item->setPath(fileName);
    item->accept();
#endif
}

void GeometryWindowClass::on_pbCameraDialog_clicked()
{
    CameraControl->showAndUpdate();
}
