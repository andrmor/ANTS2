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

    //runtime properties
public:
    bool _flag = false; // used in rearrangment to flag items to remove
};

#endif // ABASKETITEM_H
