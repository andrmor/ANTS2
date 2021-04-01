#ifndef ADRAWEXPLORERWIDGET_H
#define ADRAWEXPLORERWIDGET_H

#include "adrawobject.h"

#include <QTreeWidget>

class GraphWindowClass;
class QTreeWidgetItem;
class TObject;
class TH2;
class TAxis;
class TGaxis;
class QColor;
class QIcon;
class TAttLine;
class TAttMarker;
class TAttFill;

class ADrawExplorerWidget : public QTreeWidget
{
    Q_OBJECT
public:
    ADrawExplorerWidget(GraphWindowClass & GraphWindow, QVector<ADrawObject> & DrawObjects);

    void updateGui();

    TH2 * getObjectForCustomProjection() {return objForCustomProjection;}

    void activateCustomGuiForItem(int index);    
    void showObjectContextMenu(const QPoint &pos, int index);
    void manipulateTriggered();

    void customProjection(ADrawObject &obj);

private slots:
    void onContextMenuRequested(const QPoint & pos);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    GraphWindowClass & GraphWindow;
    QVector<ADrawObject> & DrawObjects;

private:
    void addToDrawObjectsAndRegister(TObject * pointer, const QString & options);

    void rename(ADrawObject &obj);
    void toggleEnable(ADrawObject &obj);
    void remove(int index);
    void setAttributes(int index);
    void showPanel(ADrawObject &obj);
    void fitPanel(ADrawObject &obj);
    void scale(ADrawObject &obj);
    void scaleIntegralToUnity(ADrawObject &obj);
    void scaleCDR(ADrawObject &obj);
    void scaleAllSameMax();
    void shift(ADrawObject &obj);
    void drawIntegral(ADrawObject &obj);
    void fraction(ADrawObject &obj);
    void fwhm(int index);
    void linDraw(int index);
    void boxDraw(int index);
    void ellipseDraw(int index);

    void linFit(int index);
    void expFit(int index);
    void interpolate(ADrawObject &obj);
    void median(ADrawObject &obj);
    void projection(ADrawObject &obj, bool bX);
    void splineFit(int index);
    void editAxis(ADrawObject &obj, int axisIndex);
    void addAxis(int axisIndex);
    void saveRoot(ADrawObject &obj);
    void saveAsTxt(ADrawObject &obj, bool fUseBinCenters);
    void extract(ADrawObject &obj);
    void editPave(ADrawObject &obj);
    void editTGaxis(ADrawObject &obj);

    bool canScale(ADrawObject &obj);
    void doScale(ADrawObject &obj, double sf);
    void copyAxisProperties(TGaxis & grAxis, TAxis  & axis);
    void copyAxisProperties(TAxis  & axis,   TGaxis & grAxis);
    const QString generateOptionForSecondaryAxis(int axisIndex, double u1, double u2);

    void constructIconForObject(QIcon & icon, const ADrawObject & drObj);
    void construct1DIcon(QIcon & icon, const TAttLine *line, const TAttMarker *marker, const TAttFill *fill);
    void construct2DIcon(QIcon & icon);
    void convertRootColoToQtColor(int rootColor, QColor & qtColor);

private:
    TH2 * objForCustomProjection = nullptr;

    const int IconWidth  = 61;
    const int IconHeight = 31;

signals:
    void requestShowLegendDialog();
};

#endif // ADRAWEXPLORERWIDGET_H
