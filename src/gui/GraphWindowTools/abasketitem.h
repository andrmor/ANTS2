#ifndef ABASKETITEM_H
#define ABASKETITEM_H

#include "adrawobject.h"
#include <QString>
#include <QVector>

class ABasketItem
{
public:
    ABasketItem(QString name, QVector<ADrawObject> * drawObjects);
    ABasketItem(){}
    ~ABasketItem();

public:
    QString Name;
    QString Type;
    QVector<ADrawObject> DrawObjects;

    void clearObjects();
};

#endif // ABASKETITEM_H
