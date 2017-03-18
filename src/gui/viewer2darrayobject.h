#ifndef VIEWER2DARRAYOBJECT_H
#define VIEWER2DARRAYOBJECT_H

#include "guiutils.h"

#include <QObject>

class pms;

class PMpropsClass
{
public:
  QColor pen;
  QColor brush;
  QString text;
  QColor textColor;
  bool visible;

  PMpropsClass() {pen = Qt::black; brush = Qt::white; text = ""; textColor = Qt::black; visible = true;}
};

class Viewer2DarrayObject : public QObject
{
  Q_OBJECT
public:
  explicit Viewer2DarrayObject(myQGraphicsView *GV, pms* PM_module);
  ~Viewer2DarrayObject();

  void DrawAll();
  void ResetViewport();

  void ClearColors();
  void SetPenColor(int ipm, QColor color);
  void SetBrushColor(int ipm, QColor color);
  void SetText(int ipm, QString text);
  void SetTextColor(int ipm, QColor color);
  void SetVisible(int ipm, bool fFlag);
  void SetCursorMode(int option){CursorMode = option; gv->setCursorMode(option);} //0-normal (hands), 1-cross only

signals:
  void PMselectionChanged(QVector<int>);
 // void MouseMoved();

public slots:
  void forceResize();

private slots:
  void sceneSelectionChanged();

private:
  pms* PMs;

  QVector<QGraphicsItem*> PMicons;
  QVector<PMpropsClass> PMprops;

  myQGraphicsView *gv;
  QGraphicsScene *scene;
  double GVscale;
  int CursorMode;

};

#endif // VIEWER2DARRAYOBJECT_H
