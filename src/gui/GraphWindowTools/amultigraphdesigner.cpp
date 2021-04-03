#include "amultigraphdesigner.h"
#include "ui_amultigraphdesigner.h"
#include "rasterwindowbaseclass.h"
#include "amessage.h"
#include "abasketlistwidget.h"
#include "abasketmanager.h"
#include "ajsontools.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QDebug>
#include <TObject.h>
#include <TCanvas.h>
#include <QJsonArray>
#include <QFileDialog>

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

    updateBasketGUI();
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

void AMultiGraphDesigner::requestAutoconfigureAndDraw(const QVector<int> & basketItems)
{
    clearGraphs();

    DrawOrder = basketItems;

    const int size = basketItems.size();
    if      (size == 2) fillOutBasicLayout(2, 1);
    else if (size == 3) fillOutBasicLayout(3, 1);
    else if (size == 4) fillOutBasicLayout(2, 2);
    else if (size == 5) fillOutBasicLayout(2, 3);
    else if (size == 6) fillOutBasicLayout(2, 3);
    else if (size >= 7) fillOutBasicLayout(3, 3);
}

void AMultiGraphDesigner::on_actionAs_pdf_triggered()
{
    QString fileName = QFileDialog::getSaveFileName();
    RasterWindow->SaveAs(fileName);
}

void AMultiGraphDesigner::on_actionSave_triggered()
{
    QJsonObject js;
    writeAPadsToJson(js);
    SaveJsonToFile(js, "/home/kiram/Documents/jsonTesting/json1.json");
}

void AMultiGraphDesigner::on_actionLoad_triggered()
{
    /*
    qDebug() <<"aaaaaaa" <<Pads.length();
    clearGraphs();
    qDebug() <<"bbbbbbbbbbbbbbb" <<Pads.length();
    QJsonObject js;
    LoadJsonFromFile(js, "/home/kiram/Documents/jsonTesting/json1.json");
    QString err = readAPadsFromJson(js);
    qDebug() <<err;
    qDebug() <<"cccccccccccccccccc" <<Pads.length();
    updateCanvas();
    */
}

/*
void AMultiGraphDesigner::on_drawgraphtriggered()
{
    const QVector<ADrawObject> DrawObjects = Basket.getCopy(0);
    drawGraph(DrawObjects);
}
*/

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
    TCanvas * canvas = RasterWindow->fCanvas;

    for (int iPad = 0; iPad < Pads.size(); iPad++)
    {
        const APadProperties & pad = Pads.at(iPad);
        canvas->cd();
        pad.tPad->Draw();

        if (iPad < DrawOrder.size())
        {
            int iBasketIndex = DrawOrder.at(iPad);
            if (iBasketIndex < Basket.size() && iBasketIndex >= 0)
            {
                const QVector<ADrawObject> DrawObjects = Basket.getCopy(iBasketIndex);
                pad.tPad->cd();
                drawGraph(DrawObjects);
            }
        }
    }
    canvas->Update();
}

void AMultiGraphDesigner::fillOutBasicLayout(int numX, int numY)
{
    if (numX < 1) numX = 1;
    if (numY < 1) numY = 1;

    ui->sbNumX->setValue(numX);
    ui->sbNumY->setValue(numY);

    double margin = 0;
    double padY   = 1.0 / numY;
    double padX   = 1.0 / numX;

    for (int iY = 0; iY < numY; iY++)
    {
        double y2 = 1.0 - (iY * padY);
        double y1 = y2 - padY;

        if (iY == 0)     y2 = 1 - margin;
        if (iY == numY - 1) y1 = margin;

        for (int iX = 0; iX < numX; iX++)
        {
            double x1 = iX * padX;
            double x2 = x1 + padX;

            if (iX == 0)        x1 = margin;
            if (iX == numX - 1) x2 = 1.0 - margin;

            QString padName = "pad" + QString::number(iX) + "_" + QString::number(iY);

            TPad * ipad = new TPad(padName.toLatin1().data(), "", x1, y1, x2, y2);
            APadProperties apad(ipad);

            Pads << apad;
        }
    }

    qDebug() << "pads" << PadsToString();
    updateCanvas();
}

void AMultiGraphDesigner::writeAPadsToJson(QJsonObject &json)
{
    /*
    QJsonArray ar;
    for (APadProperties& aPad: Pads)
    {
        QJsonObject js;
        aPad.updatePadGeometry(); //    EXTREMELY TEMPORARY!!!
        aPad.writeToJson(js);
        ar << js;
    }
    json["Pads"] = ar;
    */
}

QString AMultiGraphDesigner::readAPadsFromJson(const QJsonObject &json)
{
    /*
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
    */
}

QString AMultiGraphDesigner::PadsToString()
{
    QString str;
    for (const APadProperties & pad : Pads)
    {
        str += "{";
        str += pad.toString();
        str += "}, ";
    }
    str.chop(2);
    return str;
}

void AMultiGraphDesigner::on_pbRefactor_clicked()
{
    clearGraphs();
    fillOutBasicLayout(ui->sbNumX->value(), ui->sbNumY->value());
}

#include <QTimer>
bool AMultiGraphDesigner::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate)
    {
        RasterWindow->UpdateRootCanvas();
    }

    if (event->type() == QEvent::Show)
    {
        if (bColdStart)
        {
            //first time this window is shown
            bColdStart = false;
            this->resize(width()+1, height());
            this->resize(width()-1, height());
        }
        else
        {
            //qDebug() << "Graph win show event";
            //RasterWindow->UpdateRootCanvas();
            //QTimer::singleShot(100, [this](){RasterWindow->UpdateRootCanvas();}); // without delay canvas is not shown in Qt 5.9.5
        }
    }

    return QMainWindow::event(event);
}

void AMultiGraphDesigner::on_pbClear_clicked()
{
    DrawOrder.clear();
    clearGraphs();
    updateCanvas();
}
