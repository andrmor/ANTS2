#include "amultigraphdesigner.h"
#include "ui_amultigraphdesigner.h"
#include "rasterwindowbaseclass.h"
#include "amessage.h"
#include "abasketlistwidget.h"
#include "abasketmanager.h"
#include "amultigraphconfigurator.h"
#include "ajsontools.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QDebug>
#include <TObject.h>
#include <TCanvas.h>
#include <QJsonArray>

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
    QJsonObject js;
    writeAPadsToJson(js);
    SaveJsonToFile(js, "/home/kiram/Documents/jsonTesting/json1.json");
}

void AMultiGraphDesigner::on_actionLoad_triggered()
{
    qDebug() <<"aaaaaaa" <<Pads.length();
    clearGraphs();
    qDebug() <<"bbbbbbbbbbbbbbb" <<Pads.length();
    QJsonObject js;
    LoadJsonFromFile(js, "/home/kiram/Documents/jsonTesting/json1.json");
    QString err = readAPadsFromJson(js);
    qDebug() <<err;
    qDebug() <<"cccccccccccccccccc" <<Pads.length();
    updateCanvas();
}

void AMultiGraphDesigner::on_drawgraphtriggered()
{
    const QVector<ADrawObject> DrawObjects = Basket.getCopy(0);
    drawGraph(DrawObjects);
}

void AMultiGraphDesigner::clearGraphs()
{
    TCanvas *c1 = RasterWindow->fCanvas;
    c1->Clear();
    Pads.clear();
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

void AMultiGraphDesigner::updateCanvas()
{
    TCanvas *c1 = RasterWindow->fCanvas;

    for (const APadProperties& aPad: Pads)
    {
        c1->cd();
        aPad.tPad->Draw();
        const QVector<ADrawObject> DrawObjects = Basket.getCopy(aPad.basketIndex);
        aPad.tPad->cd();
        drawGraph(DrawObjects);
    }
    c1->Update();
}

void AMultiGraphDesigner::drawBasicLayout(int x, int y)
{
    double margin    = 0.005;
    double padWidth  = 1.0/x;
    double padHeight = 1.0/y;

    qDebug() <<"w" <<padWidth<< "h" << padHeight;


    for (int iy = 0; iy < y; iy++)
    {
        double y2 = 1-(iy * padHeight); //y normally is counted from bottom to top so for easier basket use I inverted order it goes (0.95->0.05)
        double y1 = y2 - padHeight;

        if (iy == 0)     y2 = 1-margin;
        if (iy == y - 1) y1 = margin;

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
            APadProperties apad(ipad);

            Pads << apad;
            apad.tPad->Draw();

        }

    }
    Pads[1].basketIndex = 1;
    updateCanvas();
}

//APadGeometry *AMultiGraphDesigner::getPadGeometry(const TPad *pad)
//{
//    APadGeometry *padGeo = new APadGeometry();

//    padGeo->xLow = pad->GetAbsXlowNDC();
//    padGeo->yLow = pad->GetAbsYlowNDC();

//    double w = pad->GetAbsWNDC();
//    double h = pad->GetAbsHNDC();
//    padGeo->xHigh = padGeo->xLow + w;
//    padGeo->yHigh = padGeo->yLow + h;

//    return padGeo;
//}

//void AMultiGraphDesigner::applyPadGeometry(const APadGeometry *padGeo, TPad *pad)
//{
//    pad->SetBBoxX1(padGeo->xLow);
//    pad->SetBBoxX2(padGeo->xHigh);
//    pad->SetBBoxY1(padGeo->yLow);
//    pad->SetBBoxY2(padGeo->yHigh);
//}

//void AMultiGraphDesigner::writePadToJason(TPad *pad, QJsonObject json)
//{
//    double h      = pad->GetAbsHNDC();
//    double w      = pad->GetAbsWNDC();
//    double xLow   = pad->GetAbsXlowNDC();
//    double yLow   = pad->GetAbsYlowNDC();

//    json["h"]     = h;
//    json["w"]     = w;
//    json["xLow"]  = xLow;
//    json["yLow"]  = yLow;
//}

//void AMultiGraphDesigner::readPadFromJason(TPad *pad, QJsonObject json)
//{
//}

void AMultiGraphDesigner::writeAPadsToJson(QJsonObject &json)
{
    QJsonArray ar;
    for (const APadProperties& aPad: Pads)
    {
        QJsonObject js;
        //aPad.updatePadGeometry(); //    EXTREMELY TEMPORARY!!!
        aPad.writeToJson(js);
        ar << js;
    }
    json["Pads"] = ar;
}

QString AMultiGraphDesigner::readAPadsFromJson(const QJsonObject &json)
{
    QJsonArray ar;
    if (!parseJson(json, "Pads", ar))
        return "error in converting object to array";

    int size = ar.size();
    if (size == 0)
        return "error nothing to load";

    for (int i = 0; i < size; i++)
    {
        const QJsonObject &js = ar.at(i).toObject();

        APadProperties newPad;
        newPad.readFromJson(js);
        Pads << newPad;
    }
    return "";
}

//void AMultiGraphDesigner::readAllPadsFromJson(const QJsonArray &jarr)
//{
//    for (int iObj = 0; iObj < jarr.size(); iObj ++)
//    {
//        APadGeometry *padGeo = new APadGeometry;
//        TPad         *pad    = new TPad;

//        QJsonObject js = jarr[iObj].toObject();

//        padGeo->readFromJson(js);
//        applyPadGeometry(padGeo, pad);
//    }
//}

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
    clearGraphs();
    drawBasicLayout(2,2);


}
