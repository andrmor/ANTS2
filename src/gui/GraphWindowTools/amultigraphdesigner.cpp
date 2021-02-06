#include "amultigraphdesigner.h"
#include "ui_amultigraphdesigner.h"
#include "rasterwindowbaseclass.h"
#include "amessage.h"
#include "abasketlistwidget.h"
#include "abasketmanager.h"
#include "amultigraphconfigurator.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QDebug>
#include <TObject.h>
#include <TCanvas.h>

AMultiGraphDesigner::AMultiGraphDesigner(ABasketManager & Basket, QWidget *parent) :
    QMainWindow(parent), Basket(Basket),
    ui(new Ui::AMultiGraphDesigner)
{
    ui->setupUi(this);

    RasterWindow = new RasterWindowBaseClass(this);
    RasterWindow->resize(400, 400);
    RasterWindow->ForceResize();
    ui->lMainLayout->insertWidget(0, RasterWindow);

    lwBasket = new ABasketListWidget(this);
    ui->lBasketLayout->addWidget(lwBasket);
    //connect(lwBasket, &ABasketListWidget::customContextMenuRequested, this, &AMultiGraphDesigner::BasketCustomContextMenuRequested);
    //connect(lwBasket, &ABasketListWidget::itemDoubleClicked, this, &AMultiGraphDesigner::onBasketItemDoubleClicked);
    //connect(lwBasket, &ABasketListWidget::requestReorder, this, &AMultiGraphDesigner::BasketReorderRequested);

    Configurator = new AMultiGraphConfigurator();
    ui->lConfigurator->insertWidget(1, Configurator);

    updateBasketGUI();

    resize(width()+1, height());
    resize(width()-1, height());
}

AMultiGraphDesigner::~AMultiGraphDesigner()
{
    delete ui;
}

void AMultiGraphDesigner::updateBasketGUI()
{
    lwBasket->clear();
    lwBasket->addItems(Basket.getItemNames());

    for (int i=0; i < lwBasket->count(); i++)
    {
        QListWidgetItem * item = lwBasket->item(i);
        item->setForeground(QBrush(Qt::black));
        item->setBackground(QBrush(Qt::white));
    }
}

void AMultiGraphDesigner::on_action1_x_2_triggered()
{
    drawBasicLayout(1,2);
}

void AMultiGraphDesigner::on_action2_x_1_triggered()
{
    drawBasicLayout(2,1);
}

void AMultiGraphDesigner::on_action2_x_2_triggered()
{
    drawBasicLayout(2,2);
}

void AMultiGraphDesigner::on_actionAs_pdf_triggered()
{

}

void AMultiGraphDesigner::on_actionSave_triggered()
{

}

void AMultiGraphDesigner::on_drawgraphtriggered()
{
    const QVector<ADrawObject> DrawObjects = Basket.getCopy(0);
    drawGraph(DrawObjects);
}

void AMultiGraphDesigner::drawGraph(const QVector<ADrawObject> DrawObjects)
{
    for (int i=0; i<DrawObjects.length(); i++)
    {
        const ADrawObject & drObj = DrawObjects.at(i);
        TObject * tObj = drObj.Pointer;

        tObj->Draw(drObj.Options.toLatin1().data());

    }
}

void AMultiGraphDesigner::drawBasicLayout(int x, int y)
{
    double margin    = 0.005;
    double padWidth  = 1.0/x;
    double padHeight = 1.0/y;

    qDebug() <<"w" <<padWidth<< "h" << padHeight;

    QVector<TPad*> pads;
    for (int iy = 0; iy < y; iy++)
    {
        double y1 = iy * padHeight;
        double y2 = y1 + padHeight;

        if (iy == 0)     y1 = margin;
        if (iy == y - 1) y2 = 1-margin;

        for (int ix = 0; ix < x; ix++)
        {
            double x1 = ix * padWidth;
            double x2 = x1 + padWidth;

            if (ix == 0) x1 = margin;
            if (ix == x - 1) x2 = 1-margin;

            qDebug() << "x1" << x1;
            qDebug() << "x2" << x2;

            QString padname = "pad" + QString::number(ix) + "_" + QString::number(iy);
            TString padnameT(padname.toLatin1().data());

            TPad* ipad = new TPad(padnameT, padnameT, x1, y1, x2, y2);

            pads << ipad;
            ipad->Draw();

        }

    }

    TCanvas *c1 = RasterWindow->fCanvas;
    const QVector<ADrawObject> DrawObjects1 = Basket.getCopy(0);

    for (int n =0; n< pads.length() ; n++)
    {
        pads.at(n)->cd();
        drawGraph(DrawObjects1);
    }
    c1->Update();
}

void AMultiGraphDesigner::writeToJson(QJsonObject &json)
{

}

void AMultiGraphDesigner::on_pushButton_clicked()
{
//    const QVector<ADrawObject> DrawObjects1 = Basket.getCopy(0);
//    TCanvas *c1 = RasterWindow->fCanvas;
//    int margin = 0.05;

//    TPad*    upperPad = new TPad("upperPad", "upperPad",
//                       margin, .5, (1-margin), (1-margin));
//    TPad*    lowerPad = new TPad("lowerPad", "lowerPad",
//                       margin, margin, (1-margin), .5);
//    upperPad->Draw();
//    lowerPad->Draw();
//    upperPad->cd();
//    drawGraph(DrawObjects1);
//    lowerPad->cd();
//    drawGraph(DrawObjects1);
//    c1->Update();
    drawBasicLayout(2,2);


}
