#ifndef AMULTIGRAPHDESIGNER_H
#define AMULTIGRAPHDESIGNER_H

#include <QMainWindow>

namespace Ui {
class AMultiGraphDesigner;
}

class AMultiGraphDesigner : public QMainWindow
{
    Q_OBJECT

public:
    explicit AMultiGraphDesigner(QWidget *parent = 0);
    ~AMultiGraphDesigner();

private:
    Ui::AMultiGraphDesigner *ui;
};

#endif // AMULTIGRAPHDESIGNER_H
