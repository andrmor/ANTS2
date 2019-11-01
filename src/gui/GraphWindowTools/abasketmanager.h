#ifndef ABASKETMANAGER_H
#define ABASKETMANAGER_H

#include "abasketitem.h"

#include <QString>
#include <QVector>

class QStringList;

class ABasketManager
{
public:
    ABasketManager();
    ~ABasketManager();

    void add(const QString & name, const QVector<ADrawObject> & drawObjects); //makes deep copy
    void update(int index, const QVector<ADrawObject> & drawObjects);         //makes shallow copy
    const QVector<ADrawObject> getCopy(int index) const;

    void clear();
    void remove(int index);

    QVector<ADrawObject> & getDrawObjects(int index); // return ref to NotValidItem is index is out of bounds
    const QString getType(int index) const;

    int size() const;

    const QString getName(int index) const;
    void rename(int index, const QString & newName);
    const QStringList getItemNames() const;

    void saveAll(const QString & fileName);
    const QString appendBasket(const QString & fileName);

    const QString appendTxtAsGraph(const QString & fileName);
    const QString appendTxtAsGraphErrors(const QString & fileName);
    void appendRootHistGraphs(const QString & fileName);

    void reorder(const QVector<int> &indexes, int to);

private:
    QVector< ABasketItem > Basket;

    QVector<ADrawObject> NotValidItem; // to return on wrong index
};

#endif // ABASKETMANAGER_H
