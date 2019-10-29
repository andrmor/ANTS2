#ifndef ADRAWEXPLORERWIDGET_H
#define ADRAWEXPLORERWIDGET_H

#include "adrawobject.h"

#include <QTreeWidget>

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
    void remove(int index);
    void setMarker(ADrawObject &obj);
    void setLine(ADrawObject &obj);
    void showPanel(ADrawObject &obj);
    void scale(ADrawObject &obj);

signals:
    void requestRedraw();
};

#endif // ADRAWEXPLORERWIDGET_H
