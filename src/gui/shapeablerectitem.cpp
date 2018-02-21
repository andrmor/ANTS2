#include "shapeablerectitem.h"
#include <cmath>
#include <QDebug>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainter>
//#include <QGraphicsScene>

#ifndef M_PI
#define M_PI		3.14159265358979323846264338327	/* pi */
#endif

ShapeableRectItem::ShapeableRectItem(QGraphicsItem *parent) :
    QGraphicsItem(parent), foregroundColor(Qt::black),
    backgroundColor(QColor(255, 255, 255, 128))
{
  this->showContrast = false;
  this->backgroundWidth = 6;
  mmPerPixelInX = mmPerPixelInY = 1.0;
  trueAngle = 0;
  TrueHeight = TrueWidth = 100;

  cornerDetectionMax = 20;
  sideDetectionMax = 1.5;

  this->setRotation(0);
  this->setAcceptHoverEvents(true);

  // If we own these objects, then changes made to QPainter *painter in ::paint won't take effect
  this->Polygon = new QGraphicsPolygonItem();
  QBrush brush(Qt::transparent);
  Polygon->setBrush(brush);

  // We don't care how these are drawn, but if we don't own them they don't get drawn in correct pos
  // Tried moving and transforming *this object before painting units, but nothing works.
  // They're ALWAYS where *this was in the beggining of ::paint()
  this->xunits = new QGraphicsTextItem("+X", this);
  this->xunits->setAcceptHoverEvents(false);
  this->xunits->setAcceptTouchEvents(false);

  this->yunits = new QGraphicsTextItem("+Y", this);
  this->yunits->setAcceptHoverEvents(false);
  this->yunits->setAcceptTouchEvents(false);

  this->units = new QGraphicsTextItem("-X", this);
  this->units->setAcceptHoverEvents(false);
  this->units->setAcceptTouchEvents(false);
}

ShapeableRectItem::~ShapeableRectItem()
{
    delete this->Polygon;
    delete this->xunits;
    delete this->yunits;
    delete this->units;
}

void ShapeableRectItem::setPolygon_Apparent(const QPolygonF &rect)
{
    prepareGeometryChange();

    Polygon->setPolygon(rect);

    //xunits->setPos(rect.right()-xunits->boundingRect().width(), rect.bottom());
    yunits->setPos(rect.at(0));

    //yunits->setPos(rect.left()-yunits->boundingRect().width(), rect.top());
    xunits->setPos(rect.at(2));

    units->setPos(rect.at(3));
}

QPointF ShapeableRectItem::makePoint(double trueX, double trueY)
{
    double angle = trueAngle * M_PI/180.0;
    double cosA = cos(angle);
    double sinA = sin(angle);

    return QPointF( (trueX*cosA + trueY*sinA)/mmPerPixelInX, ( -trueX*sinA + trueY*cosA)/mmPerPixelInY );

    this->update();
}

double ShapeableRectItem::TrueAngleFromApparent(double apparentAngle)
{
    //transforming to the system used by the line tool
    double standardAngle = ( apparentAngle > 180.0 ? 360 - apparentAngle : -apparentAngle );

    double trueAngle = ClipAngleToRangeMinus180to180( standardAngle );

    bool bAbove90 =      (trueAngle >  90.0);
    bool bBelowMinus90 = (trueAngle < -90.0);

    //taking into account distortions: converting from apparent to true
    if (mmPerPixelInX != mmPerPixelInY)
        trueAngle = atan( tan(standardAngle*M_PI/180.0) * mmPerPixelInY/mmPerPixelInX ) *180.0/M_PI;

    if (bAbove90)      trueAngle += 180.0;
    if (bBelowMinus90) trueAngle -= 180.0;

    //  qDebug() << "standard->true: " << standardAngle << trueAngle;
    return trueAngle;
}

double ShapeableRectItem::ApparentAngleFromTrue(double trueAngle)
{
    //transforming from the system used by the line tool
    double standardAngle = -trueAngle;
    if (standardAngle < 0) standardAngle += 360.0;

    double apparentAngle = standardAngle;
    //taking into account distortions: converting from true to apparent
    if (mmPerPixelInX != mmPerPixelInY)
        apparentAngle = atan( tan(standardAngle*3.1415926535/180.0)*mmPerPixelInX/mmPerPixelInY)*180.0/3.1415926535;

    //qDebug() << "true->apparent: " << trueAngle << apparentAngle;
    return apparentAngle;
}

void ShapeableRectItem::setTrueRectangle(double trueWidth, double trueHeight)
{
    TrueWidth = trueWidth;
    TrueHeight = trueHeight;

    QVector<QPointF> vec;
    vec << makePoint(-0.5*trueWidth, -0.5*trueHeight);
    vec << makePoint(+0.5*trueWidth, -0.5*trueHeight);
    vec << makePoint(+0.5*trueWidth, +0.5*trueHeight);
    vec << makePoint(-0.5*trueWidth, +0.5*trueHeight);

    QPolygonF poly(vec);
    setPolygon_Apparent(poly);
}

QPainterPath ShapeableRectItem::shape() const
{
    return Polygon->shape();
}

void ShapeableRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  painter->setRenderHint(QPainter::Antialiasing, true);

  if(showContrast)
  {
    QPen pen(backgroundColor);
    pen.setWidth(backgroundWidth);
    Polygon->setPen(pen);
    Polygon->paint(painter, option, widget);
  }

  QPen pen(foregroundColor);
  pen.setWidth(1);
  Polygon->setPen(pen);
  Polygon->paint(painter, option, widget);

  QLine lx( QPoint(getPolygon().at(2).x(), getPolygon().at(2).y()), QPoint(getPolygon().at(3).x(), getPolygon().at(3).y()));
  pen.setWidth(3);
  painter->setPen(pen);
  painter->drawLine(lx);
  QLine ly( QPoint(getPolygon().at(0).x(), getPolygon().at(0).y()), QPoint(getPolygon().at(3).x(), getPolygon().at(3).y()));
  painter->drawLine(ly);
}

void ShapeableRectItem::setForegroundColor(const QColor &color)
{
  this->foregroundColor = color;
  update();
}

void ShapeableRectItem::setBackgroundColor(const QColor &color)
{
  this->backgroundColor = color;
  update();
}

ShapeableRectItem::Location ShapeableRectItem::getMouseLocationOnBox(QPointF mpos) const
{
    QPolygonF p = this->getPolygon();
    int loc = Center;

    QVector<bool> corner(4, false); //corners: TL, TR, BR, BL corners
    QVector<bool> side(4, false);   //on side: T, R, B, L

    //  qDebug() << "\n";
    int ii = 0;
    bool bFoundCorner = false;
    for (int i=0; i<4; i++)
    {
        double lm = QLineF(mpos, p.at(i)).length();
        if (lm < cornerDetectionMax)
        {
            bFoundCorner = true;
            corner[i] = true;
            break;
        }

        ii++;
        if (ii == 4) ii = 0;
        double sum = QLineF(p.at(i), mpos).length() + QLineF(mpos, p.at(ii)).length();
        double base = QLineF(p.at(i), p.at(ii)).length();

        side[i] = ( fabs(sum-base) < sideDetectionMax );
        //  qDebug() << i << sum << base << side[i];
    }

    if (bFoundCorner)
    {
        if (corner.at(0)) loc = Top | Left;
        else if (corner.at(1)) loc = Top | Right;
        else if (corner.at(2)) loc = Bottom | Right;
        else if (corner.at(3)) loc = Bottom | Left;
    }
    else
    {
        //keeping Raimundo's way of doing - it was used for the corner detection as well
        if (side.at(3))     loc |= Left;
        else if(side.at(1)) loc |= Right;

        if (side.at(0))     loc |= Top;
        else if(side.at(2)) loc |= Bottom;
    }

    return (Location)loc;
}

double ShapeableRectItem::ClipAngleToRange0to360(double angle)
{
    double a = fmod(angle, 360.0);
    if (a < 0) a += 360.0;
    if (a > 359.9999999999999999999) a = 0;
    return a;
}

double ShapeableRectItem::ClipAngleToRangeMinus180to180(double angle)
{
    double a = fmod(angle, 360.0);
    if (a < -180.0) a += 360.0;
    if (a > +180.0) a -= 360.0;
    return a;
}

void ShapeableRectItem::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if (event->delta()>0) setTrueRectangle( TrueWidth * 1.1, TrueHeight * 1.1);
    else                  setTrueRectangle( TrueWidth / 1.1, TrueHeight / 1.1);

    emit geometryChanged();
}

void ShapeableRectItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    switch ( getMouseLocationOnBox(event->pos()) )
    {
        case TopRight: case BottomLeft:
        case TopLeft: case BottomRight:
            setCursor(Qt::PointingHandCursor);
            break;
        case Top: case Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case Left: case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case Center:
            setCursor(Qt::ClosedHandCursor);
            break;
    }
}

void ShapeableRectItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    this->mousePress = getMouseLocationOnBox(event->pos());
    this->pressedPoint = event->scenePos();
    this->posOnPress = pos();

    double apparentMouseAngleOnPress = 0;
    if (event->pos().x() != event->pos().y() || event->pos().x() != 0)
      apparentMouseAngleOnPress = atan2(event->pos().y(), event->pos().x()) *180.0/M_PI;

    trueMouseAngleOnPress = TrueAngleFromApparent(apparentMouseAngleOnPress);
    trueBoxAngleOnPress = getTrueAngle();

    //qDebug() << "On press angles -> mouse:"<<apparentMouseAngleOnPress<<" Box true:"<<getTrueAngle()<<" Box apparent:"<<apparentBoxAngleOnPress;
}

void ShapeableRectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF delta = event->scenePos() - pressedPoint;

    switch(this->mousePress)
    {
        case Center:    //Move box
            setPos(delta + posOnPress);
            break;

        case TopLeft:   //Rotate box
        case TopRight:
        case BottomLeft:
        case BottomRight:
        {
            double mouseAngle = 0;
            if ( event->scenePos().y() != event->scenePos().x() || event->scenePos().y() != 0)
                mouseAngle = atan2(event->pos().y(), event->pos().x()) * 180.0/M_PI;
            ClipAngleToRange0to360(mouseAngle);

            double newTrueAngle = trueBoxAngleOnPress + TrueAngleFromApparent(mouseAngle) - trueMouseAngleOnPress;
            trueAngle = ClipAngleToRangeMinus180to180(newTrueAngle);

            setTrueAngle(trueAngle);
            setTrueRectangle(TrueWidth, TrueHeight);
            break;
        }

        case Top:      //Resize box
        case Bottom:
        {
            double trueY = event->pos().y() * mmPerPixelInY;
            setTrueRectangle(TrueWidth, 2.0*fabs(trueY));
            break;
        }
        case Left:
        case Right:
        {
            double trueX = event->pos().x() * mmPerPixelInX;
            setTrueRectangle(2.0*fabs(trueX), TrueHeight);
            break;
        }
    }

    //update(boundingRect());
    //scene()->update(scene()->sceneRect());
    emit geometryChanged();
}

void ShapeableRectItem::setShowContrast(bool show)
{
  this->showContrast = show;
  update();
}
