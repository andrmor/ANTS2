#include "shapeablerectitem.h"
#include <cmath>
#include <QDebug>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainter>

#ifndef M_PI
#define M_PI		3.14159265358979323846264338327	/* pi */
#endif

ShapeableRectItem::ShapeableRectItem(QGraphicsItem *parent) :
    QGraphicsItem(parent), foregroundColor(Qt::black),
    backgroundColor(QColor(255, 255, 255, 128))
{
  this->showContrast = false;
  this->backgroundWidth = 6;
  this->borderPx = 10;
  mmPerPixelInX = mmPerPixelInY = 1.0;
  trueAngle = 0;
  TrueHeight = TrueWidth = 100;
  this->setRotation(0);
  this->setAcceptHoverEvents(true);
  //    this->pixmap = 0;

  // If we own these objects, then changes made to QPainter *painter in ::paint won't take effect
  this->rectangle = new QGraphicsPolygonItem();
  QBrush brush(Qt::transparent);
  rectangle->setBrush(brush);

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
    delete this->rectangle;
    delete this->xunits;
    delete this->yunits;
    delete this->units;
}

void ShapeableRectItem::setRect(const QPolygonF &rect)
{
    rectangle->setPolygon(rect);

    //xunits->setPos(rect.right()-xunits->boundingRect().width(), rect.bottom());
    yunits->setPos(rect.at(0));

    //yunits->setPos(rect.left()-yunits->boundingRect().width(), rect.top());
    xunits->setPos(rect.at(2));

    units->setPos(rect.at(3));
}

QPointF ShapeableRectItem::makePoint(double trueX, double trueY)
{
    double angle = trueAngle * 3.1415926535/180.0;
    double cosA = cos(angle);
    double sinA = sin(angle);

    return QPointF( (trueX*cosA + trueY*sinA)/mmPerPixelInX, ( -trueX*sinA + trueY*cosA)/mmPerPixelInY );

    this->update();
}

double ShapeableRectItem::TrueAngleFromApparent(double apparentAngle)
{
    //transforming to the system used by the line tool
    double standardAngle = ( apparentAngle > 180.0 ? 360 - apparentAngle : -apparentAngle );

    double trueAngle = standardAngle;
    //taking into account distortions: converting from apparent to true
    if (mmPerPixelInX != mmPerPixelInY)
        trueAngle = atan( tan(standardAngle*M_PI/180.0) * mmPerPixelInY/mmPerPixelInX ) *180.0/M_PI;

    //if (standardAngle < -90) trueAngle = 180.0 + trueAngle;
    //if (standardAngle > +90) trueAngle = 180.0 - trueAngle;

    //qDebug() << "apparent->true: " << apparentAngle << trueAngle;
    qDebug() << "standard->true: " << standardAngle << trueAngle;

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

void ShapeableRectItem::setTrueRect(double trueWidth, double trueHeight)   //qreal ax, qreal ay, qreal w, qreal h)
{
    //HalfWidth1 = 0.5*TrueWidth/mmPerPixelInX;
    //HalfHeight1 = 0.5*TrueHeight/mmPerPixelInY;
    //setRect(QRectF(-HalfWidth1, -HalfHeight1, 2.0*HalfWidth1, 2.0*HalfHeight1));

    TrueWidth = trueWidth;
    TrueHeight = trueHeight;

    QVector<QPointF> vec;
    vec << makePoint(-0.5*trueWidth, -0.5*trueHeight);
    vec << makePoint(+0.5*trueWidth, -0.5*trueHeight);
    vec << makePoint(+0.5*trueWidth, +0.5*trueHeight);
    vec << makePoint(-0.5*trueWidth, +0.5*trueHeight);

    QPolygonF poly(vec);
    setRect(poly);
}

QPainterPath ShapeableRectItem::shape() const
{
    return rectangle->shape();
}

void ShapeableRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  painter->setRenderHint(QPainter::Antialiasing, true);

  if(showContrast)
  {
    QPen pen(backgroundColor);
    pen.setWidth(backgroundWidth);
    rectangle->setPen(pen);
    rectangle->paint(painter, option, widget);
  }

  QPen pen(foregroundColor);
  pen.setWidth(1);
  rectangle->setPen(pen);
  rectangle->paint(painter, option, widget);

  QLine lx( QPoint(rect().at(2).x(), rect().at(2).y()), QPoint(rect().at(3).x(), rect().at(3).y()));
  pen.setWidth(3);
  painter->setPen(pen);
  painter->drawLine(lx);
  QLine ly( QPoint(rect().at(0).x(), rect().at(0).y()), QPoint(rect().at(3).x(), rect().at(3).y()));
  painter->drawLine(ly);

//  if(pixmap)
//  {
//    QRect target(rect().left(), rect().top(), rect().width(), rect().height());
//    painter->drawPixmap(target, *pixmap);
//  }
}

double ShapeableRectItem::getTrueAngle() const
{
    return trueAngle;
}

void ShapeableRectItem::setTrueAngle(double angle)
{
    trueAngle = angle;
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

ShapeableRectItem::Location ShapeableRectItem::getLocation(QPointF mpos) const
{
    QPolygonF p = this->rect();  // ***!!!
    int loc = Center;

//    if(mpos.x()-borderPx < rect.x())
//        loc |= Left;
//    else if(mpos.x()+borderPx > rect.x()+rect.width())
//        loc |= Right;

//    if(mpos.y()-borderPx < rect.y())
//        loc |= Top;
//    else if(mpos.y()+borderPx > rect.y()+rect.height())
//        loc |= Bottom;

    double cornermax = 20;
    double sidemax = 4;
    QVector<bool> corner(4, false); //corners: TL, TR, BR, BL corners
    QVector<bool> side(4, false);   //on side: T, R, B, L

    //  qDebug() << "\n";
    int ii = 0;
    bool bFoundCorner = false;
    for (int i=0; i<4; i++)
    {
        double lm = QLineF(mpos, p.at(i)).length();
        if (lm < cornermax)
        {
            bFoundCorner = true;
            corner[i] = true;
            break;
        }

        ii++;
        if (ii == 4) ii = 0;
        double sum = QLineF(p.at(i), mpos).length() + QLineF(mpos, p.at(ii)).length();
        double base = QLineF(p.at(i), p.at(ii)).length();

        side[i] = ( fabs(sum-base) < sidemax );
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

static qreal clipangle(qreal angle)
{
    qreal rot = fmod(angle, 359.999999999999999999);
    return rot < 0 ? rot + 360 : rot;
}

static ShapeableRectItem::Location rotateLocation(ShapeableRectItem::Location loc, qreal angle)
{
    int x = (clipangle(angle)+45)/90;
    return (ShapeableRectItem::Location)(((loc << x) & 0xf) | (((loc<<x) & ~0xf) >> 4));
}

//void ShapeableRectItem::wheelEvent(QGraphicsSceneWheelEvent *event)
//{
//    QRectF newRect = rect();
//    double scale;
//    if(event->delta() > 0)
//        scale = 1+0.1/120*event->delta();
//    else
//        scale = 1/(1-0.1/120*event->delta());
//    newRect.setWidth(newRect.width()*scale);
//    newRect.setHeight(newRect.height()*scale);
//    newRect.setX(-0.5*newRect.width());
//    newRect.setY(-0.5*newRect.height());
//    this->setRect(newRect);
//    emit geometryChanged();
//}

void ShapeableRectItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    switch(rotateLocation(getLocation(event->pos()), rotation()))
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
    this->mousePress = getLocation(event->pos());
    this->pressedPoint = event->scenePos();
    this->posOnPress = pos();
    this->rectOnPress = rect();

    apparentMouseAngleOnPress = 0;
    if (event->pos().x() != event->pos().y() || event->pos().x() != 0)
    apparentMouseAngleOnPress = atan2(event->pos().y(), event->pos().x()) *180.0/M_PI;
    apparentBoxAngleOnPress = ApparentAngleFromTrue( getTrueAngle() );

    trueMouseAngleOnPress = TrueAngleFromApparent(apparentMouseAngleOnPress);
    trueBoxAngleOnPress = getTrueAngle();

    //qDebug() << "On press angles -> mouse:"<<apparentMouseAngleOnPress<<" Box true:"<<getTrueAngle()<<" Box apparent:"<<apparentBoxAngleOnPress;
}

void ShapeableRectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    //QPolygonF newRect = rectOnPress;
    QPointF delta = event->scenePos()-pressedPoint;
    //QPointF projectedDelta = delta*QTransform().rotate(-rotation());
    //double ang = rotation()*M_PI/180.0;
    switch(this->mousePress)
    {
        case Center:    //Move box
            setPos(delta+posOnPress);
            break;

        case TopLeft: case TopRight: case BottomLeft: case BottomRight: //Rotate box
        {
            //double mouseAngle = atan2(event->scenePos().y()-posOnPress.y(), event->scenePos().x()-posOnPress.x());
            double mouseAngle = 0;
            if ( event->scenePos().y() != event->scenePos().x() || event->scenePos().y() != 0)
                mouseAngle = atan2(event->pos().y(), event->pos().x()) * 180.0/M_PI;
            clipangle(mouseAngle);

            //double newApparentAngle = clipangle( apparentBoxAngleOnPress + (mouseAngle - apparentMouseAngleOnPress) );
            //double trueAngle = apparentToTrueAngle(newApparentAngle);
            //qDebug() << "Box rotation, angles-> mouse:"<< mouseAngle << "Box, new apparent:"<<newApparentAngle<<"Box true:"<<trueAngle;

            double newTrueAngle = trueBoxAngleOnPress + TrueAngleFromApparent(mouseAngle) - trueMouseAngleOnPress;
            qDebug() << "moOnP"<<trueMouseAngleOnPress<<"truMouse"<<TrueAngleFromApparent(mouseAngle)<<"appMouse"<<mouseAngle;
            trueAngle = clipangle(newTrueAngle);

            setTrueAngle(trueAngle);
            setTrueRect(TrueWidth, TrueHeight);

        } break;

//        case Top: // ***!!!
//            //newRect.setHeight(newRect.height()-projectedDelta.y()*2);
//        case Bottom:
//            setPos(QPointF(-projectedDelta.y()*sin(ang)*0.5, projectedDelta.y()*cos(ang)*0.5)+posOnPress);
//            newRect.setHeight(newRect.height()+projectedDelta.y());
//            newRect.moveCenter(QPointF(0, 0));
//            this->setRect(newRect);
//            break;

//        case Left:
//            newRect.setWidth(newRect.width()-projectedDelta.x()*2);
//        case Right:
//            setPos(QPointF(projectedDelta.x()*cos(ang)*0.5, projectedDelta.x()*sin(ang)*0.5)+posOnPress);
//            newRect.setWidth(newRect.width()+projectedDelta.x());
//            newRect.moveCenter(QPointF(0, 0));
//            this->setRect(newRect);
//            break;
    }

    update(boundingRect()); //forces update of the scene inside affected area

    emit geometryChanged();
}

void ShapeableRectItem::setShowContrast(bool show)
{
  this->showContrast = show;
  update();
}
