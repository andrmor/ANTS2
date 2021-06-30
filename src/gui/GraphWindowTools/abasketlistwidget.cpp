#include "abasketlistwidget.h"

#include <QDropEvent>
#include <QDebug>

ABasketListWidget::ABasketListWidget(QWidget * parent) :
    QListWidget(parent)
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDropIndicatorShown(true);
}

void ABasketListWidget::dropEvent(QDropEvent * event)
{
    //qDebug() << "Basket: Drop detected";

    QList<QListWidgetItem*> sel = selectedItems();
    QVector<int> indexes;
    for (QListWidgetItem* item : qAsConst(sel))
        indexes << row(item);
    //qDebug() << "   Indexes to be moved:" << indexes;

    QListWidgetItem * itemTo = itemAt(event->pos());
    int rowTo = count();
    if (itemTo)
    {
        rowTo = row(itemTo);
        if (dropIndicatorPosition() == QAbstractItemView::BelowItem) rowTo++;
    }
    //qDebug() << "   Move to position:" << rowTo << itemTo;

    event->ignore();

    emit requestReorder(indexes, rowTo);
}
