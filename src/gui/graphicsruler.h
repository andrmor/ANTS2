#ifndef GRAPHICSRULER_H
#define GRAPHICSRULER_H

#include <QGraphicsLineItem>
#include <QPen>
#include <cmath>

class GraphicsRuler : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    explicit GraphicsRuler(QGraphicsItem *parent = 0);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QPointF getP1() const{ return scaleToXY(pos())+xyat0; }
    inline QPointF getP2() const { return scaleToXY(pos()+edge)+xyat0; }
    inline double getLength() const { return hypot(edge.x()*scaleX, edge.y()*scaleY); }
    double getAngle() const;
    inline int getEdgeRadius() const { return edgeradius; }
    inline void getScale(double &scaleX, double &scaleY) const;
    inline const QPointF &getOrigin() const { return xyat0; }
    inline double getTickLength() const { return ticklen; }
    QColor getForegroundColor() const { return fgPen.color(); }
    QColor getBackgroundColor() const { return bgBrush.color(); }

    void setP1Pixel(const QPointF &p1);
    void setP2Pixel(const QPointF &p2);
    void setP1(const QPointF &p1);
    void setP2(const QPointF &p2);
    void setLength(double len);
    void setDX(double dx);
    void setDY(double dy);
    void setAngle(double angle);
    void setEdgeRadius(int radius);
    void setScale(double scaleX, double scaleY);
    void setOrigin(const QPointF &originPixelInXY);
    void setTickLength(double ticklen);
    void setForegroundColor(const QColor &color);
    void setBackgroundColor(const QColor &color);

    QPointF scaleToPixel(const QPointF &xy) const { return QPointF(xy.x()/scaleX, -xy.y()/scaleY); }
    QPointF scaleToXY(const QPointF &pix) const { return QPointF(pix.x()*scaleX, -pix.y()*scaleY); }

signals:
    void geometryChanged();

public slots:
    void setShowContrast(bool show);
    void setShowTicks(bool show);

protected:
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
    void setLine(const QLineF &line);
    int getLocation(const QPointF &pos) const;

    QPointF edge;
    QPointF xyat0; //Because ROOT thinks there are no sub-pixels...
    QPen fgPen;
    QBrush bgBrush;
    double scaleX, scaleY;
    double ticklen;
    int edgeradius;
    bool showcontrast;
    bool showticks;

    int pressedlocation;
    QPointF pressedpoint;
    QLineF lineonpress;
};

#endif // GRAPHICSRULER_H
