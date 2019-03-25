#include "interfacetoglobscript.h"
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "tmpobjhubclass.h"
#include "apmhub.h"
#include "apositionenergyrecords.h"
#include "atrackrecords.h"
#include "sensorlrfs.h"
#include "alrfmoduleselector.h"
#include "ajsontools.h"
#include "apmtype.h"
#include "aconfiguration.h"
#include "apreprocessingsettings.h"
#include "areconstructionmanager.h"
#include "apmgroupsmanager.h"
#include "modules/lrf_v3/arepository.h"
#include "modules/lrf_v3/asensor.h"
#include "modules/lrf_v3/ainstructioninput.h"
#include "amonitor.h"

#ifdef SIM
#include "simulationmanager.h"
#endif

#ifdef GUI
#include "mainwindow.h"
#include "exampleswindow.h"
#include "reconstructionwindow.h"
#include "reconstructionwindow.h"
#include "lrfwindow.h"
#include "outputwindow.h"
#include "geometrywindowclass.h"
#include "graphwindowclass.h"
#endif

#include <QFile>
#include <QThread>
#include <QDateTime>
#include <QtWidgets/QApplication>
#include <QtGui/QVector3D>
#include <QJsonDocument>
#include <QDebug>

#include "TRandom2.h"
#include "TGeoManager.h"
#include "TGeoTrack.h"
#include "TColor.h"
#include "TAxis.h"
#include "TH1D.h"
#include "TH2D.h"


//----------------------------------
AInterfaceToLRF::AInterfaceToLRF(AConfiguration *Config, EventsDataClass *EventsDataHub)
  : Config(Config), EventsDataHub(EventsDataHub) //,f2d(0)
{
  SensLRF = Config->GetDetector()->LRFs->getOldModule();

  Description = "Access to LRFs (B-spline module)";

  H["Make"] = "Calculates new LRFs";

  H["CountIterations"] = "Returns the number of LRF iterations in history.";
  H["GetCurrent"] = "Returns the index of the current LRF iteration.";
  //H["ShowVsXY"] = "Plots a 2D histogram of the LRF. Does not work for 3D LRFs!";
}

QString AInterfaceToLRF::Make()
{
  QJsonObject jsR = Config->JSON["ReconstructionConfig"].toObject();
  SensLRF->LRFmakeJson = jsR["LRFmakeJson"].toObject();
  bool ok = SensLRF->makeLRFs(SensLRF->LRFmakeJson, EventsDataHub, Config->GetDetector()->PMs);
  Config->AskForLRFGuiUpdate();
  if (!ok) return SensLRF->getLastError();
  else return "";
}

double AInterfaceToLRF::GetLRF(int ipm, double x, double y, double z)
{
    //qDebug() << ipm<<x<<y<<z;
    //qDebug() << SensLRF->getIteration()->countPMs();
    if (!SensLRF->isAllLRFsDefined()) return 0;
    if (ipm<0 || ipm >= SensLRF->getIteration()->countPMs()) return 0;
    return SensLRF->getLRF(ipm, x, y, z);
}

double AInterfaceToLRF::GetLRFerror(int ipm, double x, double y, double z)
{
    if (!SensLRF->isAllLRFsDefined()) return 0;
    if (ipm<0 || ipm >= SensLRF->getIteration()->countPMs()) return 0;
    return SensLRF->getLRFErr(ipm, x, y, z);
}

QVariant AInterfaceToLRF::GetAllLRFs(double x, double y, double z)
{
    if (!SensLRF->isAllLRFsDefined())
    {
        abort("Not all LRFs are defined!");
        return 0;
    }

    QVariantList arr;
    const int numPMs = SensLRF->getIteration()->countPMs(); //Config->Detector->PMs->count();
    for (int ipm=0; ipm<numPMs; ipm++)
        arr << QVariant( SensLRF->getLRF(ipm, x, y, z) );
    return arr;
}

//void InterfaceToLRF::ShowVsXY(int ipm, int PointsX, int PointsY)
//{
//  int iterIndex = -1;
//  if (!getValidIteration(iterIndex)) return;
//  if (ipm < 0 || ipm >= Config->GetDetector()->PMs->count())
//    return abort("Invalid sensor number "+QString::number(ipm)+"\n");

//  double minmax[4];
//  const PMsensor *sensor = SensLRF->getIteration(iterIndex)->sensor(ipm);
//  sensor->getGlobalMinMax(minmax);

//  if (f2d) delete f2d;
//  f2d = new TF2("f2d", SensLRF, &SensorLRFs::getFunction2D,
//                minmax[0], minmax[1], minmax[2], minmax[3], 2,
//                "SensLRF", "getFunction2D");

//  f2d->SetParameter(0, ipm);
//  f2d->SetParameter(1, 0);//z0);

//  f2d->SetNpx(PointsX);
//  f2d->SetNpy(PointsY);
//  MW->GraphWindow->DrawWithoutFocus(f2d, "surf");
//}

int AInterfaceToLRF::CountIterations()
{
  return SensLRF->countIterations();
}

int AInterfaceToLRF::GetCurrent()
{
  return SensLRF->getCurrentIterIndex();
}

void AInterfaceToLRF::SetCurrent(int iterIndex)
{
  if(!getValidIteration(iterIndex)) return;
  SensLRF->setCurrentIter(iterIndex);
  Config->AskForLRFGuiUpdate();
}

void AInterfaceToLRF::SetCurrentName(QString name)
{
    SensLRF->setCurrentIterName(name);
    Config->AskForLRFGuiUpdate();
}

void AInterfaceToLRF::DeleteCurrent()
{
  int iterIndex = -1;
  if(!getValidIteration(iterIndex)) return;
  SensLRF->deleteIteration(iterIndex);
  Config->AskForLRFGuiUpdate();
}

QString AInterfaceToLRF::Save(QString fileName)
{
  int iterIndex = -1;
  if (!getValidIteration(iterIndex)) return "No data to save";

  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
      return "Cannot open file "+fileName+" to save LRFs!";

  QJsonObject json;
  if (!SensLRF->saveIteration(iterIndex, json))
    {
      saveFile.close();
      return SensLRF->getLastError();
    }

  QJsonDocument saveDoc(json);
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return "";
}

int AInterfaceToLRF::Load(QString fileName)
{
  QFile loadFile(fileName);
  if (!loadFile.open(QIODevice::ReadOnly)) {
    abort("Cannot open save file\n");
    return -1;
  }

  QByteArray saveData = loadFile.readAll();
  loadFile.close();

  QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
  QJsonObject json = loadDoc.object();
  if(!SensLRF->loadAll(json)) {
    abort("Failed to load LRFs: "+SensLRF->getLastError()+"\n");
    return -1;
  }

  //update GUI
  Config->AskForLRFGuiUpdate();

  return SensLRF->getCurrentIterIndex();
}

bool AInterfaceToLRF::getValidIteration(int &iterIndex)
{
  if (iterIndex < -1) {
      abort(QString::number(iterIndex)+" &lt; -1, therefore invalid iteration index.\n");
      return false;
  }
  if (iterIndex == -1)
    iterIndex = SensLRF->getCurrentIterIndex();
  if (iterIndex == -1) {
    abort("There is no LRF data!\n");
    return false;
  }

  int countIter = SensLRF->countIterations();
  if (iterIndex >= countIter) {
    abort("Invalid iteration index "+QString::number(iterIndex)+
          ". There are only "+QString::number(countIter)+" iteration(s).\n");
    return false;
  }

  return true;
}

#ifdef GUI
//----------------------------------
AGeoWin_SI::AGeoWin_SI(MainWindow *MW, ASimulationManager *SimManager)
 : MW(MW), SimManager(SimManager)
{
  Description = "Access to the Geometry window of GUI";

  Detector = MW->Detector;
  H["SaveImage"] = "Save image currently shown on the geometry window to an image file.\nTip: use .png extension";
}

double AGeoWin_SI::GetPhi()
{
    return MW->GeometryWindow->Phi;
}

double AGeoWin_SI::GetTheta()
{
  return MW->GeometryWindow->Theta;
}

void AGeoWin_SI::SetPhi(double phi)
{
  MW->GeometryWindow->Phi = phi;
}

void AGeoWin_SI::SetTheta(double theta)
{
  MW->GeometryWindow->Theta = theta;
}

void AGeoWin_SI::Rotate(double Theta, double Phi, int Steps, int msPause)
{
  if (Steps <= 0) return;
  double stepT = Theta/Steps;
  double stepP = Phi/Steps;
  double T0 = GetTheta();
  double P0 = GetPhi();

  QTime time;
  MW->GeometryWindow->fNeedZoom = false;
  MW->GeometryWindow->fRecallWindow = true;
  for (int i=0; i<Steps; i++)
    {
      qApp->processEvents();
      time.restart();
      MW->GeometryWindow->Theta = T0 + stepT*(i+1);
      MW->GeometryWindow->Phi   = P0 + stepP*(i+1);
      MW->GeometryWindow->PostDraw();

      int msPassed = time.elapsed();
      if (msPassed<msPause) QThread::msleep(msPause-msPassed);
    }
}

void AGeoWin_SI::SetZoom(int level)
{
  MW->GeometryWindow->ZoomLevel = level;
  MW->GeometryWindow->Zoom(true);
  MW->GeometryWindow->on_pbShowGeometry_clicked();
  MW->GeometryWindow->readRasterWindowProperties();
}

void AGeoWin_SI::UpdateView()
{
  MW->GeometryWindow->fRecallWindow = true;
  MW->GeometryWindow->PostDraw();
  MW->GeometryWindow->UpdateRootCanvas();
}

void AGeoWin_SI::SetParallel(bool on)
{
  MW->GeometryWindow->ModePerspective = !on;
}

void AGeoWin_SI::Show()
{
  MW->GeometryWindow->showNormal();
  MW->GeometryWindow->raise();
}

void AGeoWin_SI::Hide()
{
  MW->GeometryWindow->hide();
}

void AGeoWin_SI::BlockUpdates(bool on)
{
  MW->DoNotUpdateGeometry = on;
  MW->GeometryDrawDisabled = on;
}

int AGeoWin_SI::GetX()
{
  return MW->GeometryWindow->x();
}

int AGeoWin_SI::GetY()
{
  return MW->GeometryWindow->y();
}

int AGeoWin_SI::GetW()
{
  return MW->GeometryWindow->width();
}

int AGeoWin_SI::GetH()
{
    return MW->GeometryWindow->height();
}

QVariant AGeoWin_SI::GetWindowGeometry()
{
    QVariantList vl;
    vl << MW->GeometryWindow->x() << MW->GeometryWindow->y() << MW->GeometryWindow->width() << MW->GeometryWindow->height();
    return vl;
}

void AGeoWin_SI::SetWindowGeometry(QVariant xywh)
{
    if (xywh.type() != QVariant::List)
    {
        abort("Array [X Y Width Height] is expected");
        return;
    }
    QVariantList vl = xywh.toList();
    if (vl.size() != 4)
    {
        abort("Array [X Y Width Height] is expected");
        return;
    }

    int x = vl[0].toInt();
    int y = vl[1].toInt();
    int w = vl[2].toInt();
    int h = vl[3].toInt();
    //MW->GeometryWindow->setGeometry(x, y, w, h);
    MW->GeometryWindow->move(x, y);
    MW->GeometryWindow->resize(w, h);
}

void AGeoWin_SI::ShowGeometry()
{
  MW->GeometryWindow->readRasterWindowProperties();
  MW->GeometryWindow->ShowGeometry(false);
}

void AGeoWin_SI::ShowPMnumbers()
{
  MW->GeometryWindow->on_pbShowPMnumbers_clicked();
}

void AGeoWin_SI::ShowReconstructedPositions()
{
  //MW->Rwindow->ShowReconstructionPositionsIfWindowVisible();
  MW->Rwindow->ShowPositions(0, true);
}

void AGeoWin_SI::SetShowOnlyFirstEvents(bool fOn, int number)
{
  MW->Rwindow->SetShowFirst(fOn, number);
}

void AGeoWin_SI::ShowTruePositions()
{
  MW->Rwindow->DotActualPositions();
  MW->GeometryWindow->ShowGeometry(false, false);
}

void AGeoWin_SI::ShowTracks(int num, int OnlyColor)
{
  Detector->GeoManager->ClearTracks();
  if (SimManager->Tracks.isEmpty()) return;

  for (int iTr=0; iTr<SimManager->Tracks.size() && iTr<num; iTr++)
  {
      TrackHolderClass* th = SimManager->Tracks.at(iTr);
      TGeoTrack* track = new TGeoTrack(1, th->UserIndex);
      track->SetLineColor(th->Color);
      track->SetLineWidth(th->Width);
      track->SetLineStyle(th->Style);
      for (int iNode=0; iNode<th->Nodes.size(); iNode++)
        track->AddPoint(th->Nodes[iNode].R[0], th->Nodes[iNode].R[1], th->Nodes[iNode].R[2], th->Nodes[iNode].Time);

      if (track->GetNpoints()>1)
      {
          if (OnlyColor == -1  || OnlyColor == th->Color)
            Detector->GeoManager->AddTrack(track);
      }
      else delete track;
  }
  MW->GeometryWindow->DrawTracks();
}

void AGeoWin_SI::ShowSPS_position()
{
  MW->on_pbSingleSourceShow_clicked();
}

void AGeoWin_SI::ClearTracks()
{
  MW->GeometryWindow->on_pbClearTracks_clicked();
}

void AGeoWin_SI::ClearMarkers()
{
  MW->GeometryWindow->on_pbClearDots_clicked();
}

void AGeoWin_SI::SaveImage(QString fileName)
{
  MW->GeometryWindow->SaveAs(fileName);
}

void AGeoWin_SI::ShowTracksMovie(int num, int steps, int msDelay, double dTheta, double dPhi, double rotSteps, int color)
{
  int tracks = SimManager->Tracks.size();
  if (tracks == 0) return;
  if (num > tracks) num = tracks;

  int toDo;
  int thisNodes = 2;
  do //finished when all nodes of the longest track are shown
    {
      //this iteration will show track nodes < thisNode
      toDo = num; //every track when finished results in toDo--

      // cycle by indication step with interpolation between the nodes
      for (int iStep=0; iStep<steps; iStep++)
        {
          MW->Detector->GeoManager->ClearTracks();

          //cycle by tracks
          for (int iTr=0; iTr<num; iTr++)
            {
              TrackHolderClass* th = SimManager->Tracks.at(iTr);
              int ThisTrackNodes = th->Nodes.size();
              if (ThisTrackNodes <= thisNodes && iStep == steps-1) toDo--; //last node of this track

              TGeoTrack* track = new TGeoTrack(1, th->UserIndex);
              track->SetLineWidth(th->Width+1);
              if (color == -1) track->SetLineColor(15); // during tracking
              else track->SetLineColor(color); // during tracking
              int lim = std::min(thisNodes, th->Nodes.size());
              for (int iNode=0; iNode<lim; iNode++)
                {
                  double x, y, z;
                  if (iNode == thisNodes-1)
                    {
                      //here track is in interpolation mode
                      x = th->Nodes[iNode-1].R[0] + (th->Nodes[iNode].R[0]-th->Nodes[iNode-1].R[0])*iStep/(steps-1);
                      y = th->Nodes[iNode-1].R[1] + (th->Nodes[iNode].R[1]-th->Nodes[iNode-1].R[1])*iStep/(steps-1);
                      z = th->Nodes[iNode-1].R[2] + (th->Nodes[iNode].R[2]-th->Nodes[iNode-1].R[2])*iStep/(steps-1);
                      if (color == -1) track->SetLineColor(15); //black during tracking
                    }
                  else
                    {
                      x = th->Nodes[iNode].R[0];
                      y = th->Nodes[iNode].R[1];
                      z = th->Nodes[iNode].R[2];
                      if (color == -1) track->SetLineColor( th->Color );
                    }
                  track->AddPoint(x, y, z, th->Nodes[iNode].Time);
                }
              if (color == -1)
                if (ThisTrackNodes <= thisNodes && iStep == steps-1) track->SetLineColor( th->Color );

              //adding track to GeoManager
              if (track->GetNpoints()>1)
                Detector->GeoManager->AddTrack(track);
              else delete track;
            }
          MW->GeometryWindow->DrawTracks();
          Rotate(dTheta, dPhi, rotSteps);
          QThread::msleep(msDelay);
        }
      thisNodes++;
    }
  while (toDo>0);
}

void AGeoWin_SI::ShowEnergyVector()
{
  MW->Rwindow->UpdateSimVizData(0);
}
#endif
//----------------------------------

//----------------------------------
#ifdef GUI
InterfaceToGraphWin::InterfaceToGraphWin(MainWindow *MW)
  : MW(MW)
{
  Description = "Access to the Graph window of GUI";

  H["SaveImage"] = "Save image currently shown on the graph window to an image file.\nTip: use .png extension";
  H["GetAxis"] = "Returns an object with the values presented to user in 'Range' boxes.\n"
                 "They can be accessed with min/max X/Y/Z (e.g. grwin.GetAxis().maxY).\n"
                 "The values can be 'undefined'";
  H["AddLegend"] = "Adds a temporary (not savable yet!) legend to the graph.\n"
      "x1,y1 and x2,y2 are the bottom-left and top-right corner coordinates (0..1)";
}

void InterfaceToGraphWin::Show()
{
  MW->GraphWindow->showNormal();
}

void InterfaceToGraphWin::Hide()
{
  MW->GraphWindow->hide();
}

void InterfaceToGraphWin::PlotDensityXY()
{
  MW->Rwindow->DoPlotXY(0);
}

void InterfaceToGraphWin::PlotEnergyXY()
{
  MW->Rwindow->DoPlotXY(2);
}

void InterfaceToGraphWin::PlotChi2XY()
{
    MW->Rwindow->DoPlotXY(3);
}

void InterfaceToGraphWin::ConfigureXYplot(int binsX, double X0, double X1, int binsY, double Y0, double Y1)
{
   MW->Rwindow->ConfigurePlotXY(binsX, X0, X1, binsY, Y0, Y1);
   MW->Rwindow->updateGUIsettingsInConfig();
}

void InterfaceToGraphWin::ConfigureXYplotExtra(bool suppress0, bool plotVsTrue, bool showPMs, bool showManifest, bool invertX, bool invertY)
{
    MW->Rwindow->ConfigurePlotXYextra(suppress0, plotVsTrue, showPMs, showManifest, invertX, invertY);
    MW->Rwindow->updateGUIsettingsInConfig();
}

void InterfaceToGraphWin::SetLog(bool Xaxis, bool Yaxis)
{
    MW->GraphWindow->SetLog(Xaxis, Yaxis);
}

void InterfaceToGraphWin::SetStatPanelVisible(bool flag)
{
    MW->GraphWindow->SetStatPanelVisible(flag);
}

void InterfaceToGraphWin::AddLegend(double x1, double y1, double x2, double y2, QString title)
{
    MW->GraphWindow->AddLegend(x1, y1, x2, y2, title);
}

void InterfaceToGraphWin::SetLegendBorder(int color, int style, int size)
{
    MW->GraphWindow->SetLegendBorder(color, style, size);
}

void InterfaceToGraphWin::AddText(QString text, bool Showframe, int Alignment_0Left1Center2Right)
{
    MW->GraphWindow->AddText(text, Showframe, Alignment_0Left1Center2Right);
}

void InterfaceToGraphWin::AddLine(double x1, double y1, double x2, double y2, int color, int width, int style)
{
    MW->GraphWindow->AddLine(x1, y1, x2, y2, color, width, style);
}

void InterfaceToGraphWin::AddToBasket(QString Title)
{
  MW->GraphWindow->AddCurrentToBasket(Title);
}

void InterfaceToGraphWin::ClearBasket()
{
  MW->GraphWindow->ClearBasket();

}

void InterfaceToGraphWin::SaveImage(QString fileName)
{
    MW->GraphWindow->SaveGraph(fileName);
}

void InterfaceToGraphWin::ExportTH2AsText(QString fileName)
{
    MW->GraphWindow->ExportTH2AsText(fileName);
}

QVariant InterfaceToGraphWin::GetProjection()
{
    QVector<double> vec = MW->GraphWindow->Get2DArray();
    QJsonArray arr;
    for (auto v : vec) arr << v;
    QJsonValue jv = arr;
    QVariant res = jv.toVariant();
    return res;
}

QVariant InterfaceToGraphWin::GetAxis()
{
  bool ok;
  QJsonObject result;

  result["minX"] = MW->GraphWindow->getMinX(&ok);
  if (!ok) result["minX"] = QJsonValue();
  result["maxX"] = MW->GraphWindow->getMaxX(&ok);
  if (!ok) result["maxX"] = QJsonValue();

  result["minY"] = MW->GraphWindow->getMinY(&ok);  
  if (!ok) result["minY"] = QJsonValue();
  result["maxY"] = MW->GraphWindow->getMaxY(&ok);
  if (!ok) result["maxY"] = QJsonValue();

  result["minZ"] = MW->GraphWindow->getMinZ(&ok);
  if (!ok) result["minZ"] = QJsonValue();
  result["maxZ"] = MW->GraphWindow->getMaxZ(&ok);
  if (!ok) result["maxZ"] = QJsonValue();

  return QJsonValue(result).toVariant();
}
#endif

AInterfaceToPMs::AInterfaceToPMs(AConfiguration *Config) : Config(Config)
{
    Description = "Access to PM positions / add PMs or remove all PMs from the configuration";

    PMs = Config->GetDetector()->PMs;

    H["SetPMQE"] = "Sets the QE of PM. Forces a call to UpdateAllConfig().";
    H["SetElGain"]  = "Set the relative strength for the electronic channel of ipm PMs.\nForces UpdateAllConfig()";

    H["GetElGain"]  = "Get the relative strength for the electronic channel of ipm PMs";

    H["GetRelativeGains"]  = "Get array with relative gains (max scaled to 1.0) of PMs. The factors take into account both QE and electronic channel gains (if activated)";

    H["GetPMx"] = "Return x position of PM.";
    H["GetPMy"] = "Return y position of PM.";
    H["GetPMz"] = "Return z position of PM.";

    H["SetPassivePM"] = "Set this PM status as passive.";
    H["SetActivePM"] = "Set this PM status as active.";

    H["CountPMs"] = "Return number of PMs";
}


bool AInterfaceToPMs::checkValidPM(int ipm)
{
  if (ipm<0 || ipm>PMs->count()-1)
    {
      abort("Wrong PM number!");
      return false;
    }
  return true;
}

bool AInterfaceToPMs::checkAddPmCommon(int UpperLower, int type)
{
    if (UpperLower<0 || UpperLower>1)
      {
        qWarning() << "Wrong UpperLower parameter: 0 - upper PM array, 1 - lower";
        return false;
      }
    if (type<0 || type>PMs->countPMtypes()-1)
      {
        qWarning() << "Attempting to add PM of non-existing type.";
        return false;
      }
    return true;
}

int AInterfaceToPMs::CountPM() const
{
    return PMs->count();
}

double AInterfaceToPMs::GetPMx(int ipm)
{
  if (!checkValidPM(ipm)) return std::numeric_limits<double>::quiet_NaN();
  return PMs->X(ipm);
}

double AInterfaceToPMs::GetPMy(int ipm)
{
  if (!checkValidPM(ipm)) return std::numeric_limits<double>::quiet_NaN();
  return PMs->Y(ipm);
}

double AInterfaceToPMs::GetPMz(int ipm)
{
  if (!checkValidPM(ipm)) return std::numeric_limits<double>::quiet_NaN();
  return PMs->Z(ipm);
}

bool AInterfaceToPMs::IsPmCenterWithin(int ipm, double x, double y, double distance_in_square)
{
  if (!checkValidPM(ipm)) return false;
  double dx = x - PMs->at(ipm).x;
  double dy = y - PMs->at(ipm).y;
  return ( (dx*dx + dy*dy) < distance_in_square );
}

bool AInterfaceToPMs::IsPmCenterWithinFast(int ipm, double x, double y, double distance_in_square) const
{
  double dx = x - PMs->at(ipm).x;
  double dy = y - PMs->at(ipm).y;
  return ( (dx*dx + dy*dy) < distance_in_square );
}

QVariant AInterfaceToPMs::GetPMtypes()
{
   QJsonObject obj;
   PMs->writePMtypesToJson(obj);
   QJsonArray ar = obj["PMtypes"].toArray();

   QVariant res = ar.toVariantList();
   return res;
}

QVariant AInterfaceToPMs::GetPMpositions() const
{
    QVariantList arr;
    const int numPMs = PMs->count();
    for (int ipm=0; ipm<numPMs; ipm++)
    {
        QVariantList el;
        el << QVariant(PMs->at(ipm).x) << QVariant(PMs->at(ipm).y) << QVariant(PMs->at(ipm).z);
        arr << QVariant(el);
    }
    //  qDebug() << QVariant(arr);
    return arr;
}

void AInterfaceToPMs::RemoveAllPMs()
{
   Config->GetDetector()->PMarrays[0] = APmArrayData();
   Config->GetDetector()->PMarrays[1] = APmArrayData();
   Config->GetDetector()->writeToJson(Config->JSON);
   Config->GetDetector()->BuildDetector();
}

bool AInterfaceToPMs::AddPMToPlane(int UpperLower, int type, double X, double Y, double angle)
{
  if (!checkAddPmCommon(UpperLower, type)) return false;

  APmArrayData& ArrData = Config->GetDetector()->PMarrays[UpperLower];
  //qDebug() << "Size:"<<ArrData.PositionsAnglesTypes.size()<<"Reg:"<<ArrData.Regularity;

  if (ArrData.PositionsAnglesTypes.isEmpty())
    {
      //this is the first PM, can define array regularity
      ArrData.Regularity = 1;
      ArrData.PMtype = type;
    }
  else
    {
      if (ArrData.Regularity != 1)
      {
          qWarning() << "Attempt to add auto-z PM to a regular or full-custom PM array";
          return false;
      }
    }

  ArrData.PositionsAnglesTypes.append(APmPosAngTypeRecord(X, Y, 0, 0,0,angle, type));
  ArrData.PositioningScript.clear();
  ArrData.fActive = true;

  Config->GetDetector()->writeToJson(Config->JSON);
  Config->GetDetector()->BuildDetector();
  return true;
}

bool AInterfaceToPMs::AddPM(int UpperLower, int type, double X, double Y, double Z, double phi, double theta, double psi)
{
    if (!checkAddPmCommon(UpperLower, type)) return false;

    APmArrayData& ArrData = Config->GetDetector()->PMarrays[UpperLower];
    //qDebug() << "Size:"<<ArrData.PositionsAnglesTypes.size()<<"Reg:"<<ArrData.Regularity;

    if (ArrData.PositionsAnglesTypes.isEmpty())
      {
        //this is the first PM, can define array regularity
        ArrData.Regularity = 2;
        ArrData.PMtype = type;
      }
    else
      {
        if (ArrData.Regularity != 2)
        {
            qWarning() << "Attempt to add full-custom position PM to a auto-z PM array";
            return false;
        }
      }

    ArrData.PositionsAnglesTypes.append(APmPosAngTypeRecord(X, Y, Z, phi,theta,psi, type));
    ArrData.PositioningScript.clear();
    ArrData.fActive = true;

    Config->GetDetector()->writeToJson(Config->JSON);
    Config->GetDetector()->BuildDetector();
    return true;
}

void AInterfaceToPMs::SetAllArraysFullyCustom()
{
    for (int i=0; i<Config->GetDetector()->PMarrays.size(); i++)
        Config->GetDetector()->PMarrays[i].Regularity = 2;
    Config->GetDetector()->writeToJson(Config->JSON);
}

#ifdef GUI
AInterfaceToOutputWin::AInterfaceToOutputWin(MainWindow *MW) :
    MW(MW)
{
    Description = "Access to the Output window of GUI";
}

void AInterfaceToOutputWin::ShowOutputWindow(bool flag, int tab)
{
    if (flag)
      {
        MW->Owindow->showNormal();
        MW->Owindow->raise();
      }
    else MW->Owindow->hide();

    if (tab>-1 && tab<4) MW->Owindow->SetTab(tab);
    qApp->processEvents();
}

void AInterfaceToOutputWin::Show()
{
    MW->Owindow->show();
}

void AInterfaceToOutputWin::Hide()
{
    MW->Owindow->hide();
}

void AInterfaceToOutputWin::SetWindowGeometry(QVariant xywh)
{
    if (xywh.type() != QVariant::List)
    {
        abort("Array [X Y Width Height] is expected");
        return;
    }
    QVariantList vl = xywh.toList();
    if (vl.size() != 4)
    {
        abort("Array [X Y Width Height] is expected");
        return;
    }

    int x = vl[0].toInt();
    int y = vl[1].toInt();
    int w = vl[2].toInt();
    int h = vl[3].toInt();
    MW->Owindow->move(x, y);
    MW->Owindow->resize(w, h);
}
#endif

// ------------- New LRF module interface ------------

ALrfScriptInterface::ALrfScriptInterface(DetectorClass *Detector, EventsDataClass *EventsDataHub) :
  Detector(Detector), EventsDataHub(EventsDataHub), repo(Detector->LRFs->getNewModule())
{
    Description = "Access to new LRF module";
}

QString ALrfScriptInterface::Make(QString name, QVariantList instructions, bool use_scan_data, bool fit_error, bool scale_by_energy)
{
  if(instructions.length() < 1)
    return "No instructions were given";

  if(EventsDataHub->Events.isEmpty())
    return "There are no events loaded";

  if(use_scan_data) {
    if(EventsDataHub->Scan.isEmpty())
      return "Scan data is not setup!";
  } else if(!EventsDataHub->isReconstructionReady())
    return "Reconstruction data is not setup!";

  std::vector<LRF::AInstructionID> current_instructions;
  for(const QVariant &variant : instructions) {
    QJsonObject json = QJsonObject::fromVariantMap(variant.toMap());
    LRF::AInstruction *instruction = LRF::AInstruction::fromJson(json);
    if(instruction == nullptr)
      return "Failed to process instruction: "+QJsonDocument(json).toJson();
    current_instructions.push_back(repo->addInstruction(std::unique_ptr<LRF::AInstruction>(instruction)));
  }

  LRF::ARecipeID recipe;
  bool is_new_recipe = !repo->findRecipe(current_instructions, &recipe);
  if(is_new_recipe)
    recipe = repo->addRecipe(name.toLocal8Bit().data(), current_instructions);

  std::vector<APoint> sensorPos;
  APmHub *PMs = Detector->PMs;
  for(int i = 0; i < PMs->count(); i++) {
    APm &PM = PMs->at(i);
    sensorPos.push_back(APoint(PM.x, PM.y, PM.z));
  }

  Detector->Config->AskChangeGuiBusy(true);

  LRF::AInstructionInput data(repo, sensorPos, Detector->PMgroups,
                         EventsDataHub, use_scan_data, fit_error, scale_by_energy);

  if(!repo->updateRecipe(data, recipe)) {
    if(is_new_recipe) {
      repo->remove(recipe);
      repo->removeUnusedInstructions();
    }
    return "Failed to update recipe!";
  }
  else repo->setCurrentRecipe(recipe);

  Detector->Config->AskChangeGuiBusy(false);

  return "";  //empty = no error message
}

QString ALrfScriptInterface::Make(int recipe_id, bool use_scan_data, bool fit_error, bool scale_by_energy)
{
  LRF::ARecipeID rid(recipe_id);
  if(!repo->hasRecipe(rid))
    return "Non-existing recipe id";

  if(EventsDataHub->Events.isEmpty())
    return "There are no events loaded";

  if(use_scan_data) {
    if(EventsDataHub->Scan.isEmpty())
      return "Scan data is not setup";
  } else if(!EventsDataHub->isReconstructionReady())
    return "Reconstruction data is not setup";

  std::vector<APoint> sensorPos;
  APmHub *PMs = Detector->PMs;
  for(int i = 0; i < PMs->count(); i++) {
    APm &PM = PMs->at(i);
    sensorPos.push_back(APoint(PM.x, PM.y, PM.z));
  }

  Detector->Config->AskChangeGuiBusy(true);

  LRF::AInstructionInput data(repo, sensorPos, Detector->PMgroups,
                         EventsDataHub, use_scan_data, fit_error, scale_by_energy);

  if(!repo->updateRecipe(data, rid))
    return "Failed to update recipe!";
  else repo->setCurrentRecipe(rid);

  Detector->Config->AskChangeGuiBusy(false);

  return "";  //empty = no error message
}

int ALrfScriptInterface::GetCurrentRecipeId()
{
  return repo->getCurrentRecipeID().val();
}

int ALrfScriptInterface::GetCurrentVersionId()
{
  return repo->getCurrentRecipeVersionID().val();
}

bool ALrfScriptInterface::SetCurrentRecipeId(int rid)
{
  return repo->setCurrentRecipe(LRF::ARecipeID(rid));
}

bool ALrfScriptInterface::SetCurrentVersionId(int vid, int rid)
{
  if(rid == -1)
    return repo->setCurrentRecipe(repo->getCurrentRecipeID(), LRF::ARecipeVersionID(vid));
  else
    return repo->setCurrentRecipe(LRF::ARecipeID(rid), LRF::ARecipeVersionID(vid));
}

void ALrfScriptInterface::DeleteCurrentRecipe()
{
  repo->remove(repo->getCurrentRecipeID());
}

void ALrfScriptInterface::DeleteCurrentRecipeVersion()
{
  repo->removeVersion(repo->getCurrentRecipeID(), repo->getCurrentRecipeVersionID());
}

double ALrfScriptInterface::GetLRF(int ipm, double x, double y, double z)
{
  const LRF::ASensor *sensor = repo->getCurrentLrfs().getSensor(ipm);
  return sensor != nullptr ? sensor->eval(APoint(x, y, z)) : 0;
}

QList<int> ALrfScriptInterface::GetListOfRecipes()
{
  QList<int> recipes;
  for(auto rid : repo->getRecipeList())
    recipes.append(rid.val());
  return recipes;
}

bool ALrfScriptInterface::SaveRepository(QString fileName)
{
  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
    return false;

  QJsonDocument saveDoc(repo->toJson());
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return true;
}

bool ALrfScriptInterface::SaveCurrentRecipe(QString fileName)
{
  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
    return false;

  QJsonDocument saveDoc(repo->toJson(repo->getCurrentRecipeID()));
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return true;
}

bool ALrfScriptInterface::SaveCurrentVersion(QString fileName)
{
  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
    return false;

  QJsonDocument saveDoc(repo->toJson(repo->getCurrentRecipeID(), repo->getCurrentRecipeVersionID()));
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return true;
}

QList<int> ALrfScriptInterface::Load(QString fileName)
{
  QFile loadFile(fileName);
  if (!loadFile.open(QIODevice::ReadOnly)) {
    abort("Cannot open save file\n");
    return QList<int>();
  }
  QJsonObject json = QJsonDocument::fromJson(loadFile.readAll()).object();
  loadFile.close();

  LRF::ARepository new_repo(json);
  QList<int> new_recipes;
  for(auto rid : new_repo.getRecipeList())
    new_recipes.append(rid.val());
  if(repo->mergeRepository(new_repo))
    return new_recipes;
  else return QList<int>();
}

// ------------- End of New LRF module interface ------------

