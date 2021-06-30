#ifndef RASTERWINDOWBASECLASS_H
#define RASTERWINDOWBASECLASS_H

#include <QWidget>
//#include <TMathBase.h>

class TCanvas;
class QMainWindow;
class TView;

struct AGeoViewParameters
{
    double RangeLL[3];
    double RangeUR[3];

    double RotCenter[3];

    double WinX, WinY, WinW, WinH;

    double Long, Lat, Psi;

    void read(TCanvas * Canvas);
    void apply(TCanvas * Canvas) const;
};

class RasterWindowBaseClass : public QWidget
{
    Q_OBJECT
public:
    explicit RasterWindowBaseClass(QMainWindow *MasterWindow);
    virtual ~RasterWindowBaseClass();

    TCanvas * fCanvas = nullptr;
    AGeoViewParameters ViewParameters;

    void setBlockEvents(bool flag) {fBlockEvents = flag;}
    void setInvertedXYforDrag(bool flag) {fInvertedXYforDrag = flag;} //fix ROOT inversion in x-y for parallel view of geometry

    void SetAsActiveRootWindow(); //ROOT will draw on this window!
    void ClearRootCanvas(); //clear root canvas
    void UpdateRootCanvas();

    void ForceResize();

    void SaveAs(const QString & filename);
    //void SetWindowTitle(const QString & title);

    void setWindowProperties();

    void onViewChanged();

signals:
    void LeftMouseButtonReleased();
    void userChangedWindow();

protected:
    void mouseMoveEvent(QMouseEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void wheelEvent(QWheelEvent * event) override;

    void paintEvent(QPaintEvent * event ) override;
    void resizeEvent(QResizeEvent * event ) override;

protected:
    QMainWindow * MasterWindow = nullptr;
    int wid;
    bool PressEventRegistered = false; //to avoid Qt bug - "leaking" of events to another window
    int lastX, lastY;
    double lastCenterX, lastCenterY;
    bool fBlockEvents = false;

    bool fInvertedXYforDrag = false;
    bool bBlockZoom = false;

    void releaseZoomBlock() {bBlockZoom = false;}
};

#endif // RASTERWINDOWBASECLASS_H
