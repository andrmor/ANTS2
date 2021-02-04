#ifndef AMULTIGRAPHDESIGNER_H
#define AMULTIGRAPHDESIGNER_H

#include <QMainWindow>

class ABasketManager;
class RasterWindowBaseClass;
class ABasketListWidget;
class AMultiGraphConfigurator;

namespace Ui {
class AMultiGraphDesigner;
}

class AMultiGraphDesigner : public QMainWindow
{
    Q_OBJECT

public:
    explicit AMultiGraphDesigner(ABasketManager & Basket, QWidget * parent = nullptr);
    ~AMultiGraphDesigner();

    void updateBasketGUI();  // triggered from GraphWindow::UpdatebasketGui()

private slots:
    // main MENU
    void on_action1_x_2_triggered();
    void on_action2_x_1_triggered();
    void on_actionAs_pdf_triggered();
    void on_actionSave_triggered();

private:
    ABasketManager & Basket;

    Ui::AMultiGraphDesigner * ui;
    RasterWindowBaseClass   * RasterWindow = nullptr;
    ABasketListWidget       * lwBasket     = nullptr;
    AMultiGraphConfigurator * Configurator = nullptr;

};

#endif // AMULTIGRAPHDESIGNER_H
