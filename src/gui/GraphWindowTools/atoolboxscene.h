#ifndef ATOOLBOXSCENE_H
#define ATOOLBOXSCENE_H

#include "shapeablerectitem.h"
#include "graphicsruler.h"

#include <QGraphicsScene>

class GraphicsRuler;
class ShapeableRectItem;

class AToolboxScene : public QGraphicsScene
{
public:
  enum ToolDrag { ToolDragDisabled, ToolDragActivated, ToolDragStarted };
  enum Tool { ToolNone = -1, ToolRuler = 0, ToolSelBox = 1, ToolActive };

  AToolboxScene(QObject *parent = 0);
  ~AToolboxScene();

  GraphicsRuler *getRuler() { return &ruler; }
  ShapeableRectItem *getSelBox() { return &selBox; }
  QGraphicsItem *getTool(Tool tool = ToolActive);

  void setActiveTool(Tool tool);

  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

public slots:
  void activateItemDrag();
  void resetTool(Tool tool = ToolActive);
  void moveToolToVisible(Tool tool = ToolActive);

private:
  void toolGeometryChanged(Tool tool = ToolActive);

  Tool activeTool;
  ShapeableRectItem selBox;
  GraphicsRuler ruler;
  ToolDrag toolDrag;
};

#endif // ATOOLBOXSCENE_H
