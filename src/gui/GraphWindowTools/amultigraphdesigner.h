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
class QListWidget;
class QListWidgetItem;

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
    void on_pbRefactor_clicked();
    void on_pbClear_clicked();

    void on_actionAs_pdf_triggered();
    void on_actionSave_triggered();
    void on_actionLoad_triggered();

    void onCoordItemDoubleClicked(QListWidgetItem *);
    void onBasketItemDoubleClicked(QListWidgetItem *);

protected:
    bool event(QEvent * event) override;

signals:
    void basketChanged();

private:
    ABasketManager & Basket;

    Ui::AMultiGraphDesigner * ui;
    RasterWindowBaseClass   * RasterWindow = nullptr;
    ABasketListWidget       * lwBasket     = nullptr;

    QVector<APadProperties> Pads;
    QVector<int> DrawOrder;
    bool bColdStart = true;

    void clearGraphs();

    void updateGUI();
    void updateCanvas();
    void updateNumbers();

    void drawGraph(const QVector<ADrawObject> DrawObjects);
    void fillOutBasicLayout(int numX, int numY);
    void writeToJson(QJsonObject &json);
    QString readFromJson(const QJsonObject &json);
    QString PadsToString();
    void addDraw(QListWidget *lw);
};

#endif // AMULTIGRAPHDESIGNER_H
