#ifndef ABASKETLISTWIDGET_H
#define ABASKETLISTWIDGET_H

#include <QListWidget>

class ABasketListWidget : public QListWidget
{
    Q_OBJECT

public:
    ABasketListWidget(QWidget * parent);

protected:
    void dropEvent(QDropEvent * event) override;

signals:
    void requestReorder(const QVector<int> &indexes, int toRow);
};

#endif // ABASKETLISTWIDGET_H
