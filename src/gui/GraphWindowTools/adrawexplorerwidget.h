#ifndef ADRAWEXPLORERWIDGET_H
#define ADRAWEXPLORERWIDGET_H

#include "adrawobject.h"

#include <QTreeWidget>

class GraphWindowClass;
class TObject;
class TH2;

class ADrawExplorerWidget : public QTreeWidget
{
    Q_OBJECT
public:
    ADrawExplorerWidget(GraphWindowClass & GraphWindow, QVector<ADrawObject> & DrawObjects);

    void updateGui();

    TH2 * getObjectForCustomProjection() {return objForCustomProjection;}

private slots:
    void onContextMenuRequested(const QPoint & pos);

private:
    GraphWindowClass & GraphWindow;
    QVector<ADrawObject> & DrawObjects;

private:
    void addToDrawObjectsAndRegister(TObject * pointer, const QString & options);

    void rename(ADrawObject &obj);
    void toggleEnable(ADrawObject &obj);
    void remove(int index);
    void setMarker(ADrawObject &obj);
    void setLine(ADrawObject &obj);
    void showPanel(ADrawObject &obj);
    void fitPanel(ADrawObject &obj);
    void scale(ADrawObject &obj);
    void shift(ADrawObject &obj);
    void drawIntegral(ADrawObject &obj);
    void fraction(ADrawObject &obj);
    void fwhm(int index);
    void linFit(int index);
    void interpolate(ADrawObject &obj);
    void median(ADrawObject &obj);
    void projection(ADrawObject &obj, bool bX);
    void customProjection(ADrawObject &obj);
    void splineFit(int index);
    void editAxis(ADrawObject &obj, int axisIndex);
    void saveRoot(ADrawObject &obj);
    void saveAsTxt(ADrawObject &obj, bool fUseBinCenters);

private:
    TH2 * objForCustomProjection = nullptr;
};

#endif // ADRAWEXPLORERWIDGET_H
