#ifndef AMULTIGRAPHDESIGNER_H
#define AMULTIGRAPHDESIGNER_H

#include "apadgeometry.h"
#include <QMainWindow>
#include <TPad.h>

class ABasketManager;
class RasterWindowBaseClass;
class ABasketListWidget;
class AMultiGraphConfigurator;
class ADrawObject;
class QJsonObject;

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
    void on_action2_x_2_triggered();

    void on_actionAs_pdf_triggered();
    void on_actionSave_triggered();

    void on_pushButton_clicked();

public slots:
    void on_drawgraphtriggered();

private:
    ABasketManager & Basket;

    Ui::AMultiGraphDesigner * ui;
    RasterWindowBaseClass   * RasterWindow = nullptr;
    ABasketListWidget       * lwBasket     = nullptr;
    AMultiGraphConfigurator * Configurator = nullptr;

    void drawGraph(const QVector<ADrawObject> DrawObjects);
    void drawBasicLayout(int x, int y);
    //void writePadToJason(TPad* pad, QJsonObject jj);
    //void readPadFromJason(TPad* pad, QJsonObject json);
    APadGeometry* getPadGeometry(const TPad *pad);
    void applyPadGeometry(const APadGeometry* padGeo, TPad *pad);
    void writeAllPadGeometryToJson(QJsonObject &json);
    //void readAllPadsFromJson(const QJsonArray &jarr);

    QVector<TPad*> pads;

};

#endif // AMULTIGRAPHDESIGNER_H
