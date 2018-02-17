#ifndef SHAPEABLERECTITEM_H
#define SHAPEABLERECTITEM_H

#include <QAbstractGraphicsShapeItem>

class QPixmap;

class ShapeableRectItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    enum Location { Center = 0, Top = 1, Right = 2, Bottom = 4, Left = 8,
                    TopLeft = Top|Left, TopRight = Top | Right,
                    BottomLeft = Bottom|Left, BottomRight = Bottom|Right };

    explicit ShapeableRectItem(QGraphicsItem *parent = 0);
    virtual ~ShapeableRectItem();

    virtual QRectF boundingRect() const { return rectangle->boundingRect(); }
    virtual QPainterPath shape() const;
    virtual bool contains(const QPointF &point) const { return rectangle->contains(point); }
    virtual bool isObscuredBy(const QGraphicsItem *item) const { return rectangle->isObscuredBy(item); }
    virtual QPainterPath opaqueArea() const { return rectangle->opaqueArea(); }
    virtual int type() const { return rectangle->type(); }
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    QPolygonF rect() const { return rectangle->polygon(); }
    float  getBorderPx() const { return borderPx; }
    //  const QPixmap *getPixmap() const { return pixmap; }
    QColor getForegroundColor() const { return foregroundColor; }
    QColor getBackgroundColor() const { return backgroundColor; }

    void   setScale(double mmPerPixelInX, double mmPerPixelInY);
    double getTrueAngle() const; //in degrees
    void   setTrueAngle(double angle); //in degrees
    double getTrueWidth() const {return TrueWidth;}
    double getTrueHeight() const {return TrueHeight;}

    void   setTrueRect(double trueWidth, double trueHeight);

    //apparent shape
    void   setRect(const QPolygonF &rect);

    void   setBorderPx(float borderPx) { this->borderPx = borderPx; }
    //  void setPixmap(const QPixmap *value) { pixmap = value; }
    void   setForegroundColor(const QColor &color);
    void   setBackgroundColor(const QColor &color);

    static double ClipAngleToRange0to360(double angle);
    static double ClipAngleToRangeMinus180to180(double angle);
protected:
    //virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

public slots:
    void    setShowContrast(bool show);

signals:
    void    geometryChanged();
    void    requestResetGeometry(double screenWidth, double screenHeight);

private:
    double  mmPerPixelInX, mmPerPixelInY;
    double  trueAngle;

    double  TrueWidth, TrueHeight;  //in true space

    void    commonConstructor();
    Location getLocation(QPointF mpos) const;

    Location mousePress;
    QPointF pressedPoint;
    QPointF posOnPress;
    qreal apparentMouseAngleOnPress;             // angle of mouse cursor as vector from center (on LMB press)
    double apparentBoxAngleOnPress;    // projection box apparent orientation angle (on LMB press)
    QPolygonF rectOnPress;

    double  trueMouseAngleOnPress;
    double  trueBoxAngleOnPress;

    float borderPx;
        //const QPixmap *pixmap;
    //QGraphicsRectItem *rectangle;
    QGraphicsPolygonItem *rectangle;
    QGraphicsTextItem *xunits, *yunits, *units;

    bool showContrast;
    QColor foregroundColor;
    QColor backgroundColor;
    int backgroundWidth;
    QPointF makePoint(double trueX, double trueY);
    double  TrueAngleFromApparent(double apparentAngle);
    double  ApparentAngleFromTrue(double trueAngle);

};

#endif // SHAPEABLERECTITEM_H
