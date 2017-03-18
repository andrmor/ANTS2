#ifndef RASTERWINDOWBASECLASS_H
#define RASTERWINDOWBASECLASS_H

#include <QWindow>
#include <TMathBase.h>

class TCanvas;
class QMainWindow;

class RasterWindowBaseClass : public QWindow
{
    Q_OBJECT
public:
    explicit RasterWindowBaseClass(QMainWindow *parent);
    virtual ~RasterWindowBaseClass();

    TCanvas* fCanvas;

    void setBlockEvents(bool flag) {fBlockEvents = flag;}
    void setInvertedXYforDrag(bool flag) {fInvertedXYforDrag = flag;} //fix ROOT inversion in x-y for parallel view of geometry

    void SetAsActiveRootWindow(); //ROOT will draw on this window!
    void ClearRootCanvas(); //clear root canvas
    void UpdateRootCanvas();

    void ForceResize();

    void SaveAs(const QString filename);
    void SetWindowTitle(const QString &title);

    void getWindowProperties(Double_t &centerX, Double_t &centerY, Double_t &hWidth, Double_t &hHeight, Double_t &phi, Double_t &theta);
    void setWindowProperties(Double_t  centerX, Double_t  centerY, Double_t  hWidth, Double_t  hHeight, Double_t  phi, Double_t  theta);

signals:
    void LeftMouseButtonReleased();
    void UserChangedWindow(Double_t centerX, Double_t centerY, Double_t hWidth, Double_t hHeight, Double_t phi, Double_t theta);

protected:
    void exposeEvent(QExposeEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

protected:
    QMainWindow *MasterWindow;
    int wid;
    bool PressEventRegistered; //to avoid Qt bug - "leaking" of events to another window
    int lastX, lastY;
    double lastCenterX, lastCenterY;
    bool fBlockEvents;

    bool fInvertedXYforDrag;

};

#endif // RASTERWINDOWBASECLASS_H
