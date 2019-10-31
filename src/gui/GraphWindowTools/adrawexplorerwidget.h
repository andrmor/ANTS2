#ifndef ADRAWEXPLORERWIDGET_H
#define ADRAWEXPLORERWIDGET_H

#include "adrawobject.h"

#include <QTreeWidget>

class TObject;

class ADrawExplorerWidget : public QTreeWidget
{
    Q_OBJECT
public:
    ADrawExplorerWidget(QVector<ADrawObject> & DrawObjects);

    void updateGui();

private slots:
    void onContextMenuRequested(const QPoint & pos);

private:
    QVector<ADrawObject> & DrawObjects;

private:
    void rename(ADrawObject &obj);
    void toggleEnable(ADrawObject &obj);
    void remove(int index);
    void setMarker(ADrawObject &obj);
    void setLine(ADrawObject &obj);
    void showPanel(ADrawObject &obj);
    void scale(ADrawObject &obj);
    void shift(ADrawObject &obj);
    void drawIntegral(ADrawObject &obj);
    void fraction(ADrawObject &obj);
    void interpolate(ADrawObject &obj);
    void median(ADrawObject &obj);
    void projection(ADrawObject &obj, bool bX);
    void splineFit(int index);
    void editTitle(ADrawObject &obj, int X0Y1);

signals:
    void requestRedraw();
    void requestMakeCopy();
    void requestInvalidateCopy();
    void requestRegister(TObject * tobj);
};

#endif // ADRAWEXPLORERWIDGET_H
