#ifndef APROXYSTYLE_H
#define APROXYSTYLE_H

#include <QProxyStyle>

class AProxyStyle : public QProxyStyle
{
public:
    AProxyStyle(QStyle* style = 0) : QProxyStyle(style) {}

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
};

#endif // APROXYSTYLE_H
