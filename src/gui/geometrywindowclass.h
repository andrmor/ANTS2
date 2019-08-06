#ifndef GEOMETRYWINDOWCLASS_H
#define GEOMETRYWINDOWCLASS_H

#include "aguiwindow.h"
#include "TMathBase.h"

class MainWindow;
class RasterWindowBaseClass;
class QWebEngineView;
class QVariant;
class QWebEngineDownloadItem;
class TGeoVolume;

namespace Ui {
  class GeometryWindowClass;
}

class GeometryWindowClass : public AGuiWindow
{
  Q_OBJECT

public:
  explicit GeometryWindowClass(QWidget *parent, MainWindow *mw);
  ~GeometryWindowClass();

  bool ModePerspective = true;
  int  ZoomLevel = 0; //0 is fastest, +2 is typically the most "natural"
  bool fNeedZoom = true; //set to true by "ShowGeometry", which force-resets the view

  void ShowAndFocus();
  void SetAsActiveRootWindow(); //focus root on this Raster Window
  void ClearRootCanvas(); //clear root canvas
  void UpdateRootCanvas();

  void SaveAs(const QString filename);
  void OpenGLview();   //show current geometry using OpenGL viewer

  void ResetView();
  void setHideUpdate(bool flag);
  void PostDraw(); //change view properties according to UI
  void Zoom(bool update = false);

  void AddLineToGeometry(QPointF &start, QPointF &end, Color_t color = 1, int width = 1);
  void AddPolygonfToGeometry(QPolygonF &poly, Color_t color, int width);

  void onBusyOn();
  void onBusyOff();

  bool isColorByMaterial() {return ColorByMaterial;}

  bool fRecallWindow = false;
  Double_t CenterX, CenterY, HWidth, HHeight, Phi, Theta;

  void writeWindowPropsToJson(QJsonObject &json);
  void readWindowPropsFromJson(QJsonObject &json);

  int GeoMarkerSize = 2;
  int GeoMarkerStyle = 6;

  bool IsWorldVisible();

  void ShowGeometry(bool ActivateWindow = true, bool SAME = true, bool ColorUpdateAllowed = true);
  void ShowEvent_Particles(size_t iEvent, bool withSecondaries);
  void ShowPMsignals(int iEvent, bool bFullCycle = true);

  void ClearTracks(bool bRefreshWindow = true);

protected:
    bool event(QEvent *event);

public slots:
    void on_pbShowGeometry_clicked();
    void DrawTracks();
    void ShowPMnumbers();
    void ShowMonitorIndexes();
    void ShowText(const QVector<QString> & strData, Color_t color, bool onPMs = true, bool bFullCycle = true); //onPMs=false -> srawing on monitors
    void on_pbTop_clicked();
    void on_pbFront_clicked();
    void onRasterWindowChange(double centerX, double centerY, double hWidth, double hHeight, double phi, double theta);
    void readRasterWindowProperties();

    void on_pbShowPMnumbers_clicked();
    void on_pbShowTracks_clicked();
    void on_pbClearTracks_clicked();
    void on_pbClearDots_clicked();

private slots:
    void onDownloadPngRequested(QWebEngineDownloadItem *item);

private slots:
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

private:
  MainWindow* MW;
  Ui::GeometryWindowClass *ui;
  RasterWindowBaseClass *RasterWindow = 0;

#ifdef __USE_ANTS_JSROOT__
    QWebEngineView * WebView = nullptr;
#endif

  bool TMPignore = false;

  //flags
  bool BarShown = true;
  bool ShowTop = false;
  bool ColorByMaterial = false;

private:
  void doChangeLineWidth(int deltaWidth);
  void showWebView();
  void prepareGeoManager(bool ColorUpdateAllowed = true);
  void adjustGeoAttributes(TGeoVolume *vol, int Mode, int transp, bool adjustVis, int visLevel, int currentLevel);
};

#endif // GEOMETRYWINDOWCLASS_H
