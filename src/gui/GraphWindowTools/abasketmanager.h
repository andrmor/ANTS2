#ifndef ABASKETMANAGER_H
#define ABASKETMANAGER_H

#include "abasketitem.h"

#include <QVector>

class QStringList;

class ABasketManager
{
public:
    ABasketManager();
    ~ABasketManager();

    void addItem(const QString & name, const QVector<ADrawObject> & drawObjects); //makes deep copy!
    const QVector<ADrawObject> getItemCopy(int index) const;

    void clear();

    QVector<ADrawObject> * getDrawObjects(int index); // const?

    int getSize() const;
    const QStringList & getItemNames() const;

private:
    QVector< ABasketItem > Basket;
};

#endif // ABASKETMANAGER_H
