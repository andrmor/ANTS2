#include "graphicsruler.h"

#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>


#ifndef M_PI
#define M_PI		3.14159265358979323846264338327	/* pi */
#endif

GraphicsRuler::GraphicsRuler(QGraphicsItem *parent) :
    QGraphicsItem(parent), edge(0,0), xyat0(0, 0),
    fgPen(Qt::SolidPattern, 1), bgBrush(Qt::SolidPattern)
{
    bgBrush.setColor(QColor(255, 255, 255, 128));
    pressedlocation = 0;
    edgeradius = 10;
    showcontrast = true;
    showticks = true;
    ticklen = 0;
    scaleX = scaleY = 1;
    setAcceptHoverEvents(true);
}

QRectF GraphicsRuler::boundingRect() const
{
    double padding = edgeradius+1;
    qreal top = edge.y() < 0 ? edge.y() : 0;
    qreal left = edge.x() < 0 ? edge.x() : 0;
    return QRectF(left-padding, top-padding, fabs(edge.x())+2*padding, fabs(edge.y())+2*padding);
}

void GraphicsRuler::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    double len = hypot(edge.x(), edge.y());
    QPointF edgeNormal = QPointF(-edge.y(), edge.x()) * (1.0/len);
    QPointF absTick = edgeNormal*(edgeradius*0.5);
    QPointF absEdgeTick = absTick*1.5;

    painter->setRenderHint(QPainter::Antialiasing, true);
    if(showcontrast && bgBrush.color().alpha())
    {
        painter->setPen(Qt::NoPen);
        painter->setBrush(bgBrush);
        QPointF corners[4] = { +absTick, -absTick, -absTick+edge, +absTick+edge };
        painter->drawPolygon(corners, 4);
    }

    painter->setPen(fgPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(0, 0, edge.x(), edge.y());
    painter->drawLine(edge-absEdgeTick, edge+absEdgeTick);
    painter->drawLine(-absEdgeTick, absEdgeTick);

    if(showticks && ticklen != 0)
    {
        double angle = atan2(-edge.y(), edge.x()); //Can't have scale.. that is handled below
        double tStep = hypot(ticklen/scaleX*cos(angle), -ticklen/scaleY*sin(angle))/len;
        QLineF localLine(0, 0, edge.x(), edge.y());
        for(double t = tStep; t < 1; t += tStep)
        {
            QPointF pt = localLine.pointAt(t);
            painter->drawLine(pt - absTick, pt + absTick);
        }
    }
}

double GraphicsRuler::getAngle() const
{
    double ang = atan2(-edge.y()*scaleY, edge.x()*scaleX);
    //return ang < 0 ? 2*M_PI+ang : ang;
    return ang;
}

void GraphicsRuler::getScale(double &scaleX, double &scaleY) const
{
    scaleX = this->scaleX;
    scaleY = this->scaleY;
}

void GraphicsRuler::setP1Pixel(const QPointF &p1)
{
  prepareGeometryChange();
  edge += pos()-p1;
  QGraphicsItem::setPos(p1);
  emit geometryChanged();
}

void GraphicsRuler::setP2Pixel(const QPointF &p2)
{
  prepareGeometryChange();
  this->edge = p2-pos();
  emit geometryChanged();
}

void GraphicsRuler::setP1(const QPointF &p1)
{
    prepareGeometryChange();
    QPointF newP1 = scaleToPixel(p1-xyat0);
    edge += pos()-newP1;
    QGraphicsItem::setPos(newP1);
    emit geometryChanged();
}

void GraphicsRuler::setP2(const QPointF &p2)
{
    prepareGeometryChange();
    this->edge = scaleToPixel(p2-xyat0)-pos();
    emit geometryChanged();
}

void GraphicsRuler::setLength(double len)
{
    prepareGeometryChange();
    double angle = getAngle();
    edge = QPointF(len/scaleX*cos(angle), -len/scaleY*sin(angle));
    emit geometryChanged();
}

void GraphicsRuler::setDX(double dx)
{
    prepareGeometryChange();
    edge = QPointF(dx/scaleX, edge.y());
    emit geometryChanged();
}

void GraphicsRuler::setDY(double dy)
{
    prepareGeometryChange();
    edge = QPointF(edge.x(), -dy/scaleY);
    emit geometryChanged();
}

void GraphicsRuler::setAngle(double angle)
{
    prepareGeometryChange();
    double len = getLength();
    edge = QPointF(len/scaleX*cos(angle), -len/scaleY*sin(angle));
    emit geometryChanged();
}

void GraphicsRuler::setEdgeRadius(int radius)
{
    prepareGeometryChange();
    this->edgeradius = radius;
    emit geometryChanged();
}

void GraphicsRuler::setScale(double scaleX, double scaleY)
{
    this->scaleX = scaleX;
    this->scaleY = scaleY;
}

void GraphicsRuler::setOrigin(const QPointF &originPixelInXY)
{
    prepareGeometryChange();
    this->xyat0 = originPixelInXY;
}

void GraphicsRuler::setTickLength(double tickLen)
{
    prepareGeometryChange();
    this->ticklen = tickLen;
    emit geometryChanged();
}

void GraphicsRuler::setForegroundColor(const QColor &color)
{
    fgPen.setColor(color);
    update();
}

void GraphicsRuler::setBackgroundColor(const QColor &color)
{
    bgBrush.setColor(color);
    update();
}

void GraphicsRuler::setShowContrast(bool show)
{
    prepareGeometryChange();
    this->showcontrast = show;
}

void GraphicsRuler::setShowTicks(bool show)
{
    prepareGeometryChange();
    this->showticks = show;
}

void GraphicsRuler::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    switch(getLocation(event->pos()))
    {
        case -1: case +1:
            this->setCursor(Qt::PointingHandCursor);
            break;
        case 0:
            this->setCursor(Qt::SizeAllCursor);
            break;
        default:
            this->setCursor(Qt::ArrowCursor);
            break;
    }
}

void GraphicsRuler::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    pressedlocation = getLocation(event->pos());
    if(pressedlocation == 2)
    {
        event->ignore();
        return;
    }
    pressedpoint = event->scenePos();
    lineonpress = QLineF(pos(), edge+pos());
}

void GraphicsRuler::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF delta = event->scenePos()-pressedpoint;
    switch(pressedlocation)
    {
        case -1:
            setLine(QLineF(lineonpress.p1()+delta, lineonpress.p2()));
            break;
        case +1:
            setLine(QLineF(lineonpress.p1(), lineonpress.p2()+delta));
            break;
        case 0:
            setLine(QLineF(lineonpress.p1()+delta, lineonpress.p2()+delta));
            break;
        default:
            event->ignore();
            break;
    }
}

void GraphicsRuler::setLine(const QLineF &line)
{
    prepareGeometryChange();
    edge = (line.p2()-line.p1());
    QGraphicsItem::setPos(line.p1());
    emit geometryChanged();
}

int GraphicsRuler::getLocation(const QPointF &pos) const
{
    QPointF right = pos - edge;
    double leftDist = hypot(pos.x(), pos.y());
    double rightDist = hypot(right.x(), right.y());
    double cornerMax = edgeradius+2;
    if(leftDist < rightDist && fabs(pos.x()) < cornerMax && fabs(pos.y()) < cornerMax)
        return -1;
    else if(rightDist < leftDist && fabs(right.x()) < cornerMax && fabs(right.y()) < cornerMax)
        return +1;
    else
    {
        double y = pos.x()*edge.y()/edge.x();
        if(fabs(y) > fabs(edge.y()) && fabs(pos.x()) < edgeradius)
            return 0;

        double ymin = y + 2*(y < 0 ? +edgeradius : -edgeradius);
        double ymax = y + 2*(y < 0 ? -edgeradius : +edgeradius);
        if((y < 0 && ymin >= pos.y() && pos.y() >= ymax)
        || (y >= 0 && ymin <= pos.y() && pos.y() <= ymax))
            return 0;
    }
    return 2;
}
