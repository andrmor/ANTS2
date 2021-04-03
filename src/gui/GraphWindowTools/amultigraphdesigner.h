#ifndef AMULTIGRAPHDESIGNER_H
#define AMULTIGRAPHDESIGNER_H

#include "apadgeometry.h"
#include "apadproperties.h"
#include <QMainWindow>
#include <TPad.h>

class ABasketManager;
class RasterWindowBaseClass;
class ABasketListWidget;
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
    void requestAutoconfigureAndDraw(const QVector<int> & basketItems);

private slots:
    void on_actionAs_pdf_triggered();
    void on_actionSave_triggered();
    void on_actionLoad_triggered();

    void on_pbRefactor_clicked();

    void on_pbClear_clicked();

//public slots:
//    void on_drawgraphtriggered();

protected:
    bool event(QEvent * event) override;

private:
    ABasketManager & Basket;

    Ui::AMultiGraphDesigner * ui;
    RasterWindowBaseClass   * RasterWindow = nullptr;
    ABasketListWidget       * lwBasket     = nullptr;

    void clearGraphs();
    QVector<QVector<double>> bSizes;

    void drawGraph(const QVector<ADrawObject> DrawObjects);
    void updateCanvas();
    void fillOutBasicLayout(int numX, int numY);
    void writeAPadsToJson(QJsonObject &json);
    QString readAPadsFromJson(const QJsonObject &json);
    QString PadsToString();

    QVector<APadProperties> Pads;
    QVector<int> DrawOrder;

    bool bColdStart = true;
};

#endif // AMULTIGRAPHDESIGNER_H
