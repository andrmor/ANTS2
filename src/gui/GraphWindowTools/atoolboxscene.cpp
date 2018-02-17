#include "atoolboxscene.h"

#include <QGraphicsSceneMouseEvent>
#include <QDebug>

AToolboxScene::AToolboxScene(QObject *parent) : QGraphicsScene(parent)
{
    activeTool = ToolNone;
    toolDrag = ToolDragDisabled;

    for(Tool t = (Tool)0; t < ToolActive; t = (Tool)(t+1))
        getTool(t)->setPos(1000000, 1000000);
}

AToolboxScene::~AToolboxScene()
{
    QGraphicsItem *toolptr = getTool();
    if(toolptr) removeItem(toolptr);
}

QGraphicsItem *AToolboxScene::getTool(AToolboxScene::Tool tool)
{
    switch(tool)
    {
    default: qWarning()<<"ToolboxScene::getTool(): unknown Tool!";
    case ToolNone: return 0;
    case ToolRuler: return &ruler;
    case ToolSelBox: return &selBox;
    case ToolActive: return getTool(this->activeTool);
    }
}

void AToolboxScene::setActiveTool(AToolboxScene::Tool tool)
{
    if(tool >= ToolActive || tool < ToolNone)
    {
        qWarning() << "ToolboxScene::setActiveTool: Unknown Tool, using None";
        tool = ToolNone;
    }
    QGraphicsItem *toolptr = getTool();
    if(toolptr) removeItem(toolptr);
    this->activeTool = tool;
    toolptr = getTool(tool);
    if(toolptr) addItem(toolptr);
}

void AToolboxScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(event->button() != Qt::LeftButton)
        return QGraphicsScene::mousePressEvent(event);

    if(event->modifiers() == Qt::ControlModifier)
    {
        if(toolDrag != ToolDragDisabled)
            return QGraphicsScene::mousePressEvent(event);
    }
    else if(toolDrag != ToolDragActivated)
        return QGraphicsScene::mousePressEvent(event);

    switch(activeTool)
    {
    case ToolNone: case ToolActive: break;
    case ToolRuler:
        ruler.setP1Pixel(event->scenePos());
        ruler.setP2Pixel(event->scenePos());
        break;
    case ToolSelBox:
        selBox.setTrueRect(0, 0);
        selBox.setPos(event->scenePos());
        break;
    }
    toolDrag = ToolDragStarted;
}

void AToolboxScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(toolDrag != ToolDragStarted)
        return QGraphicsScene::mouseMoveEvent(event);

    switch(activeTool)
    {
      case ToolNone: case ToolActive: break;
      case ToolRuler:
        ruler.setP2Pixel(event->scenePos());
        break;
      case ToolSelBox:
      {
        QPointF p1 = event->buttonDownScenePos(Qt::LeftButton);
        QPointF diff = event->scenePos()-p1;
        selBox.setRect(QRectF(-abs(diff.x())*0.5,-abs(diff.y())*0.5, abs(diff.x()), abs(diff.y())));
        selBox.setPos(diff*0.5 + p1);
        break;
      }
    }
}

void AToolboxScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(toolDrag != ToolDragStarted)
        QGraphicsScene::mouseReleaseEvent(event);
    toolDrag = ToolDragDisabled;
}

void AToolboxScene::activateItemDrag()
{
    toolDrag = ToolDragActivated;
}

void AToolboxScene::resetTool(AToolboxScene::Tool tool)
{
    switch(tool)
    {
    default: case ToolNone: break;
    case ToolRuler:
    {
        //Occupy half the screen
        ruler.setP1Pixel(QPointF(width()*0.25, height()*0.5));
        ruler.setP2Pixel(QPointF(width()*0.75, height()*0.5));
        ruler.setTickLength(width()*0.5/20);
        //emit ruler.geometryChanged();
        break;
    }
    case ToolSelBox:
    {
        //Square of side 1/4 the screen
        //qreal w = width()*0.25, h = height()*0.25;
        //selBox.setRect(QRectF(-w*0.5, -h*0.5, w, h));
        //selBox.setPos(width()*0.5, height()*0.5);
        //selBox.setRotation(0);

        double w = 0.5 * width(), h = 0.5 * height();
        emit selBox.requestResetGeometry(w, h);
        break;
    }
    case ToolActive:
        resetTool(this->activeTool);
        break;
    }
    toolGeometryChanged(tool);
}

void AToolboxScene::moveToolToVisible(AToolboxScene::Tool tool)
{
    QRectF s = sceneRect();
    QGraphicsItem *item = getTool(tool);
    if(!item) return;
    QRectF itemRect = item->boundingRect();
    itemRect.moveTo(item->pos());
    if(!s.intersects(itemRect))
    {
        //  qDebug() << itemRect << "no intersection";
        resetTool(tool);
    }
    toolGeometryChanged(tool);
}

void AToolboxScene::toolGeometryChanged(AToolboxScene::Tool tool)
{
    switch(tool)
    {
    default: case ToolNone: break;
    case ToolRuler: emit ruler.geometryChanged(); break;
    case ToolSelBox: emit selBox.geometryChanged(); break;
    case ToolActive: toolGeometryChanged(this->activeTool); break;
    }
}
