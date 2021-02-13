#ifndef QMULTIGRAPHLAYOUTWIDGET_H
#define QMULTIGRAPHLAYOUTWIDGET_H

#include <QFrame>

namespace Ui {
class QMultiGraphLayoutWidget;
}

class QMultiGraphLayoutWidget : public QFrame
{
    Q_OBJECT

public:
    explicit QMultiGraphLayoutWidget(QWidget *parent = 0);
    ~QMultiGraphLayoutWidget();

private:
    Ui::QMultiGraphLayoutWidget *ui;
};

#endif // QMULTIGRAPHLAYOUTWIDGET_H
