#ifndef AMULTIGRAPHDESIGNER_H
#define AMULTIGRAPHDESIGNER_H

#include <QMainWindow>

class ABasketManager;
class RasterWindowBaseClass;
class ABasketListWidget;

namespace Ui {
class AMultiGraphDesigner;
}

class AMultiGraphDesigner : public QMainWindow
{
    Q_OBJECT

public:
    explicit AMultiGraphDesigner(ABasketManager & Basket, QWidget * parent = nullptr);
    ~AMultiGraphDesigner();

private:
    ABasketManager & Basket;

    Ui::AMultiGraphDesigner * ui;
    RasterWindowBaseClass   * RasterWindow = nullptr;
    ABasketListWidget       * lwBasket = nullptr;

    void updateBasketGUI();
};

#endif // AMULTIGRAPHDESIGNER_H
