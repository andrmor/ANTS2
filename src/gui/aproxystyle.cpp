#include "aproxystyle.h"

#include <QPainter>
#include <QStyleOption>
#include <QPen>

void AProxyStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (element == QStyle::PE_IndicatorItemViewItemDrop)
    {
        QColor c(Qt::black);
        QPen pen(c);
        pen.setWidth(3);
        pen.setStyle(Qt::DashLine);
        painter->setPen(pen);

        if (!option->rect.isNull())
            painter->drawLine(option->rect.topLeft(), option->rect.topRight());
    }
    else
        QProxyStyle::drawPrimitive(element, option, painter, widget);
}
