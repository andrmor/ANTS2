#ifndef ABASKETLISTWIDGET_H
#define ABASKETLISTWIDGET_H

#include <QListWidget>

class ABasketManager;

class ABasketListWidget : public QListWidget
{
    Q_OBJECT

public:
    ABasketListWidget(QWidget * parent, ABasketManager* & Basket, int & ActiveBasketItem);

protected:
    void dropEvent(QDropEvent * event) override;

private:
    ABasketManager* & Basket;
    int & ActiveBasketItem;

signals:
    void requestReorder(const QVector<int> &indexes, int toRow);
};

#endif // ABASKETLISTWIDGET_H
