#ifndef ABASKETITEM_H
#define ABASKETITEM_H

#include "adrawobject.h"
#include <QString>
#include <QVector>

class ABasketItem
{
public:
    ~ABasketItem();

public:
    QString Name;
    QString Type;
    QVector<ADrawObject> DrawObjects;

    void clearObjects();
};

#endif // ABASKETITEM_H
