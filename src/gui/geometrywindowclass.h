#ifndef GEOMETRYWINDOWCLASS_H
#define GEOMETRYWINDOWCLASS_H

#include <QMainWindow>
#include "TMathBase.h"

class MainWindow;
class RasterWindowBaseClass;

namespace Ui {
  class GeometryWindowClass;
}

class GeometryWindowClass : public QMainWindow
{
  Q_OBJECT

public:
  explicit GeometryWindowClass(QWidget *parent, MainWindow *mw);
  ~GeometryWindowClass();

  bool ModePerspective;
  int ZoomLevel; //0 is fastest, +2 is typically the most "natural"
  bool fNeedZoom; //set to true by "ShowGeometry", which force-resets the view

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

  bool fRecallWindow;
  Double_t CenterX, CenterY, HWidth, HHeight, Phi, Theta;

  void writeWindowPropsToJson(QJsonObject &json);
  void readWindowPropsFromJson(QJsonObject &json);

  int GeoMarkerSize;
  int GeoMarkerStyle;

  bool IsWorldVisible();

  void ShowGeometry(bool ActivateWindow = true, bool SAME = true, bool ColorUpdateAllowed = true);

protected:
    void resizeEvent(QResizeEvent *event);
    bool event(QEvent *event);

public slots:
    void on_pbShowGeometry_clicked();
    void ShowPMnumbers();
    void ShowTextOnPMs(QVector<QString> strData, Color_t color);
    void on_pbTop_clicked();
    void on_pbFront_clicked();
    void onRasterWindowChange(Double_t centerX, Double_t centerY, Double_t hWidth, Double_t hHeight, Double_t phi, Double_t theta);
    void readRasterWindowProperties();

    void on_pbShowPMnumbers_clicked();
    void on_pbShowTracks_clicked();
    void on_pbClearTracks_clicked();
    void on_pbClearDots_clicked();

private slots:
    void on_pbHideBar_clicked();

    void on_pbShowBar_clicked();

    void on_cbShowTop_toggled(bool checked);

    void on_cbColor_toggled(bool checked);

    void on_pbSaveAs_clicked();

    void on_pbShowGLview_clicked();

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

    void on_pbWebViewer_clicked();

private:
  Ui::GeometryWindowClass *ui;
  MainWindow* MW;
  RasterWindowBaseClass *RasterWindow;
  QWidget *QWinContainer;

  bool TMPignore;

  //flags
  bool BarShown;
  bool ColdStart;
};

#endif // GEOMETRYWINDOWCLASS_H
