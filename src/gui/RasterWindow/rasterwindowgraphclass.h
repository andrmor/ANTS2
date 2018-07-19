#ifndef RASTERWINDOWGRAPHCLASS_H
#define RASTERWINDOWGRAPHCLASS_H

#include "rasterwindowbaseclass.h"

#include <QWindow>

class QMainWindow;
class TLine;
class TEllipse;
class TBox;
class TPolyLine;

class RasterWindowGraphClass : public RasterWindowBaseClass
{
public:  
  explicit RasterWindowGraphClass(QMainWindow *MasterWindow);
  ~RasterWindowGraphClass();

  double extractedX = 0;
  double extracted2DLineA, extracted2DLineB, extracted2DLineC;
  double Line2DstartX, Line2DstopX, Line2DstartY, Line2DstopY;
  double extracted2DEllipseX, extracted2DEllipseY, extracted2DEllipseR1, extracted2DEllipseR2, extracted2DEllipseTheta;
  double extractedX1, extractedY1, extractedX2, extractedY2;
  QList<double> extractedPolygon;

  //extraction of coordinates from graph
  bool IsExtractionComplete() {return ExtractionComplete;}
  void setExtractionComplete(bool flag) {ExtractionComplete = flag;}
  void setShowCursorPosition(bool flag);

  void ExtractX();  //start extraction of X coordinate from a 1D graph/histogram using mouse
  void Extract2DLine();  //start extraction of ABC line coordinate from a 2D graph/histogram using mouse
  void Extract2DEllipse();  //start extraction of (T)ellipse from a 2D graph/histogram using mouse
  void Extract2DBox();  //start extraction of 2D box (2 opposite corners)
  void Extract2DPolygon();  //start extraction of 2D polygon, extraction ends by right click (or doubleclick?)

  double getCanvasMinX();
  double getCanvasMaxX();
  double getCanvasMinY();
  double getCanvasMaxY();

  void PixelToXY(int ix, int iy, double& x, double& y);
  void XYtoPixel(double x, double y, int& ix, int& iy);
  double getXperPixel();
  double getYperPixel();

protected:
  bool event(QEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);

private:
//  QString LastDistributionShown; // "" = nothing was shown

  bool ExtractionComplete = false;
       //for X extraction
  bool ExtractionOfXPending = false;
  bool ExtractionOfXStarted = false;
       //2D line
  bool ExtractionOf2DLinePending = false;
  bool ExtractionOf2DLineStarted = false;
       //Ellipse
  bool ExtractionOf2DEllipsePending = false;
  int ExtractionOf2DEllipsePhase = 0;
       // 2D box
  bool ExtractionOf2DBoxStarted = false;
  bool ExtractionOf2DBoxPending = false;
       //Polygon
  bool ExtractionOf2DPolygonStarted = false;
  bool ExtractionOf2DPolygonPending = false;

  TLine* VertLine1 = 0;
  TLine* Line2D = 0;
  TBox* Box2D = 0;
  TEllipse* Ellipse = 0;
  TPolyLine* Polygon = 0;

  bool ShowCursorPosition = true;

  void DrawVerticalLine();
  void Draw2DLine();
  void DrawEllipse();
  void Draw2DBox();
  void Draw2DPolygon();
};

#endif // RASTERWINDOWGRAPHCLASS_H
