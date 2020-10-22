#ifndef GEOMETRYWINDOWCLASS_H
#define GEOMETRYWINDOWCLASS_H

#include <QVector>

#include "aguiwindow.h"
#include "TMathBase.h"

class DetectorClass;
class ASimulationManager;
class RasterWindowBaseClass;
class QWebEngineView;
class QWebEngineDownloadItem;
class TGeoVolume;
class ACameraControlDialog;
class GeoMarkerClass;

namespace Ui {
  class GeometryWindowClass;
}

class GeometryWindowClass : public AGuiWindow
{
  Q_OBJECT

public:
  explicit GeometryWindowClass(QWidget *parent, DetectorClass & Detector, ASimulationManager & SimulationManager);
  ~GeometryWindowClass();

  bool ModePerspective = true;
  int  ZoomLevel       = 0;
  bool fRecallWindow   = false;
  bool bDisableDraw    = false;

  QVector<GeoMarkerClass*> GeoMarkers;

  void ShowAndFocus();
  void FocusVolume(const QString & name);
  void SetAsActiveRootWindow();
  void ClearRootCanvas();
  void UpdateRootCanvas();

  void SaveAs(const QString & filename);
  void OpenGLview();

  void ResetView();
  void setHideUpdate(bool flag);
  void PostDraw();
  void Zoom(bool update = false);

  void AddLineToGeometry(QPointF &start, QPointF &end, Color_t color = 1, int width = 1);
  void AddPolygonfToGeometry(QPolygonF &poly, Color_t color, int width);

  void onBusyOn();
  void onBusyOff();

  bool isColorByMaterial() {return ColorByMaterial;}

  void writeToJson(QJsonObject & json) const;
  void readFromJson(const QJsonObject & json);

  bool IsWorldVisible();

  void ShowGeometry(bool ActivateWindow = true, bool SAME = true, bool ColorUpdateAllowed = true);
  void ShowEvent_Particles(size_t iEvent, bool withSecondaries);
  void ShowPMsignals(const QVector<float> &Event, bool bFullCycle = true);
  void ShowGeoMarkers();
  void ShowTracksAndMarkers();
  void ShowCustomNodes(int firstN);

  void ClearTracks(bool bRefreshWindow = true);
  void ClearGeoMarkers(int All_Rec_True = 0);

protected:
    bool event(QEvent *event);
    void closeEvent(QCloseEvent * event) override;

public slots:
    void DrawTracks();
    void ShowPoint(double * r, bool keepTracks = false);
    void ShowPMnumbers();
    void ShowMonitorIndexes();
    void ShowText(const QVector<QString> & strData, Color_t color, bool onPMs = true, bool bFullCycle = true); //onPMs=false -> srawing on monitors
    void on_pbTop_clicked();
    void on_pbFront_clicked();
    void onRasterWindowChange();
    void readRasterWindowProperties();   // !*!

    void on_pbShowPMnumbers_clicked();
    void on_pbShowTracks_clicked();
    void on_pbClearTracks_clicked();
    void on_pbClearDots_clicked();

private slots:
    void onDownloadPngRequested(QWebEngineDownloadItem *item);

private slots:
    void on_pbShowGeometry_clicked();
    void on_cbShowTop_toggled(bool checked);
    void on_cbColor_toggled(bool checked);
    void on_pbSaveAs_clicked();
    void on_pbSide_clicked();
    void on_cobViewType_currentIndexChanged(int index);
    void on_cbShowAxes_toggled(bool checked);
    void on_actionSmall_dot_toggled(bool arg1);
    void on_actionLarge_dot_triggered(bool arg1);
    void on_actionSmall_cross_toggled(bool arg1);
    void on_actionLarge_cross_toggled(bool arg1);
    void on_actionSize_1_triggered();
    void on_actionSize_2_triggered();
    void on_actionDefault_zoom_1_triggered();
    void on_actionDefault_zoom_2_triggered();
    void on_actionDefault_zoom_to_0_triggered();
    void on_actionSet_line_width_for_objects_triggered();
    void on_actionDecrease_line_width_triggered();
    void on_pbShowMonitorIndexes_clicked();
    void on_cobViewer_currentIndexChanged(int index);
    void on_actionOpen_GL_viewer_triggered();
    void on_actionJSROOT_in_browser_triggered();
    void on_cbWireFrame_toggled(bool checked);
    void on_cbLimitVisibility_clicked();
    void on_sbLimitVisibility_editingFinished();
    void on_pbCameraDialog_clicked();

private:
  DetectorClass      & Detector;
  ASimulationManager & SimulationManager;

  Ui::GeometryWindowClass * ui;
  RasterWindowBaseClass * RasterWindow = nullptr;

  ACameraControlDialog * CameraControl = nullptr;

#ifdef __USE_ANTS_JSROOT__
    QWebEngineView * WebView = nullptr;
#endif

  int GeoMarkerSize  = 2;
  int GeoMarkerStyle = 6;

  bool TMPignore = false;
  bool BarShown = true;
  bool ShowTop = false;
  bool ColorByMaterial = false;

  //draw on PMs/Monitors related
  QVector<QString> SymbolMap;
  QVector< QVector < double > > numbersX;
  QVector< QVector < double > > numbersY;

private:
  void doChangeLineWidth(int deltaWidth);
  void showWebView();
  void prepareGeoManager(bool ColorUpdateAllowed = true);
  void adjustGeoAttributes(TGeoVolume * vol, int Mode, int transp, bool adjustVis, int visLevel, int currentLevel);
  void generateSymbolMap();

signals:
  void requestUpdateRegisteredGeoManager();
  void requestUpdateMaterialListWidget();
  void requestShowNetSettings();
};

#endif // GEOMETRYWINDOWCLASS_H
