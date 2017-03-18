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
    virtual QPainterPath shape() const { return rectangle->shape(); }
    virtual bool contains(const QPointF &point) const { return rectangle->contains(point); }
    virtual bool isObscuredBy(const QGraphicsItem *item) const { return rectangle->isObscuredBy(item); }
    virtual QPainterPath opaqueArea() const { return rectangle->opaqueArea(); }
    virtual int type() const { return rectangle->type(); }
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    QRectF rect() const { return rectangle->rect(); }
    float getBorderPx() const { return borderPx; }
    const QPixmap *getPixmap() const { return pixmap; }
    QColor getForegroundColor() const { return foregroundColor; }
    QColor getBackgroundColor() const { return backgroundColor; }

    void setRect(qreal ax, qreal ay, qreal w, qreal h) { setRect(QRectF(ax, ay, w, h)); }
    void setRect(const QRectF &rect);
    void setBorderPx(float borderPx) { this->borderPx = borderPx; }
    void setPixmap(const QPixmap *value) { pixmap = value; }
    void setForegroundColor(const QColor &color);
    void setBackgroundColor(const QColor &color);

protected:
    virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

public slots:
    void setShowContrast(bool show);

signals:
    void geometryChanged();

private:
    void commonConstructor();
    Location getLocation(QPointF mpos) const;

    Location mousePress;
    QPointF pressedPoint;
    QPointF posOnPress;
    qreal angleOnPress;
    QRectF rectOnPress;

    float borderPx;
    const QPixmap *pixmap;
    QGraphicsRectItem *rectangle;
    QGraphicsTextItem *xunits, *yunits;

    bool showContrast;
    QColor foregroundColor;
    QColor backgroundColor;
    int backgroundWidth;
};

#endif // SHAPEABLERECTITEM_H
