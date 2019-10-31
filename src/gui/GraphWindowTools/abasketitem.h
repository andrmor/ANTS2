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
    QVector<ADrawObject> DrawObjects;
    QString Name;
    QString Type;  // ***kill?

    void clearObjects();
};

#endif // ABASKETITEM_H
