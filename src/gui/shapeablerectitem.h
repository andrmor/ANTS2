#ifndef SHAPEABLERECTITEM_H
#define SHAPEABLERECTITEM_H

#include <QAbstractGraphicsShapeItem>

class QPixmap;

class ShapeableRectItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    enum Location { Center = 0, Top = 1, Right = 2, Bottom = 4, Left = 8,
                    TopLeft = Top|Left, TopRight = Top | Right,
                    BottomLeft = Bottom|Left, BottomRight = Bottom|Right };

    explicit ShapeableRectItem(QGraphicsItem *parent = 0);
    virtual ~ShapeableRectItem();

    virtual QRectF       boundingRect() const { return Polygon->boundingRect(); }
    virtual QPainterPath shape() const;
    virtual bool         contains(const QPointF &point) const { return Polygon->contains(point); }
    virtual bool         isObscuredBy(const QGraphicsItem *item) const { return Polygon->isObscuredBy(item); }
    virtual QPainterPath opaqueArea() const { return Polygon->opaqueArea(); }
    virtual int          type() const { return Polygon->type(); }
    virtual void         paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    QPolygonF getPolygon() const { return Polygon->polygon(); }
    QColor    getForegroundColor() const { return foregroundColor; }
    QColor    getBackgroundColor() const { return backgroundColor; }

    void      setScale(double mmPerPixelInX, double mmPerPixelInY);
    double    getTrueAngle() const; //in degrees
    void      setTrueAngle(double angle); //in degrees
    double    getTrueWidth() const {return TrueWidth;}
    double    getTrueHeight() const {return TrueHeight;}

    void      setTrueRectangle(double trueWidth, double trueHeight);
    void      setPolygon_Apparent(const QPolygonF &getPolygon);

    void      setForegroundColor(const QColor &color);
    void      setBackgroundColor(const QColor &color);

protected:
    virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

public slots:
    void      setShowContrast(bool show);

signals:
    void      geometryChanged();
    void      requestResetGeometry(double screenWidth, double screenHeight);

private:
    double    mmPerPixelInX, mmPerPixelInY;
    double    trueAngle;
    double    TrueWidth, TrueHeight;

    double    cornerDetectionMax;
    double    sideDetectionMax;

    Location  mousePress;
    QPointF   pressedPoint;
    QPointF   posOnPress;

    double    trueMouseAngleOnPress;
    double    trueBoxAngleOnPress;

    QGraphicsPolygonItem *Polygon;
    QGraphicsTextItem *xunits, *yunits, *units;

    bool      showContrast;
    QColor    foregroundColor;
    QColor    backgroundColor;
    int       backgroundWidth;

    void      commonConstructor();
    Location  getMouseLocationOnBox(QPointF mpos) const;
    QPointF   makePoint(double trueX, double trueY);
    double    TrueAngleFromApparent(double apparentAngle);
    double    ApparentAngleFromTrue(double trueAngle);

public:
    static double ClipAngleToRange0to360(double angle);
    static double ClipAngleToRangeMinus180to180(double angle);
};

#endif // SHAPEABLERECTITEM_H
