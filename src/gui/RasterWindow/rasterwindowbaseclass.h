#ifndef RASTERWINDOWBASECLASS_H
#define RASTERWINDOWBASECLASS_H

#include <QWidget>
//#include <TMathBase.h>

class TCanvas;
class QMainWindow;

class RasterWindowBaseClass : public QWidget
{
    Q_OBJECT
public:
    explicit RasterWindowBaseClass(QMainWindow *MasterWindow);
    virtual ~RasterWindowBaseClass();

    TCanvas* fCanvas = 0;

    void setBlockEvents(bool flag) {fBlockEvents = flag;}
    void setInvertedXYforDrag(bool flag) {fInvertedXYforDrag = flag;} //fix ROOT inversion in x-y for parallel view of geometry

    void SetAsActiveRootWindow(); //ROOT will draw on this window!
    void ClearRootCanvas(); //clear root canvas
    void UpdateRootCanvas();

    void ForceResize();

    void SaveAs(const QString filename);
    void SetWindowTitle(const QString &title);

    void getWindowProperties(double &centerX, double &centerY, double &hWidth, double &hHeight, double &phi, double &theta);
    void setWindowProperties(double  centerX, double  centerY, double  hWidth, double  hHeight, double  phi, double  theta);

signals:
    void LeftMouseButtonReleased();
    void UserChangedWindow(double centerX, double centerY, double hWidth, double hHeight, double phi, double theta);

protected:
    //void exposeEvent(QExposeEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

    virtual void    paintEvent( QPaintEvent *event ) override;
    virtual void    resizeEvent(QResizeEvent *event ) override;

protected:
    QMainWindow *MasterWindow = 0;
    int wid;
    bool PressEventRegistered = false; //to avoid Qt bug - "leaking" of events to another window
    int lastX, lastY;
    double lastCenterX, lastCenterY;
    bool fBlockEvents = false;

    bool fInvertedXYforDrag = false;

};

#endif // RASTERWINDOWBASECLASS_H
