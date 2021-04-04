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

    connect(lwBasket,     &ABasketListWidget::itemDoubleClicked, this, &AMultiGraphDesigner::onBasketItemDoubleClicked);
    connect(ui->lwCoords, &QListWidget::itemDoubleClicked,       this, &AMultiGraphDesigner::onCoordItemDoubleClicked);

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
    QString fileName = QFileDialog::getSaveFileName(this, "Export multigraph\nFile suffix defines the file type", "");
    if (fileName.isEmpty()) return;

    RasterWindow->SaveAs(fileName);
}

void AMultiGraphDesigner::on_actionSave_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save this multigraph");
    if (fileName.isEmpty()) return;

    QJsonObject js;
    writeToJson(js);
    SaveJsonToFile(js, fileName);

    Basket.saveAll(fileName+".basket");
}

#include <QFileInfo>
#include <QInputDialog>
void AMultiGraphDesigner::on_actionLoad_triggered()
{
    if (Basket.size() != 0)
    {
        bool ok = confirm("The Basket is not empty. If you proceed, the content will be replaced!\nContinue?", this);
        if (!ok) return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "Load multigraph");
    if (fileName.isEmpty()) return;

    QString basketFileName = fileName + ".basket";
    if (!QFileInfo(basketFileName).exists())
    {
        message(QString("Not found corresponding basket file:\n%1").arg(basketFileName));
        return;
    }
    Basket.clear();
    QString err = Basket.appendBasket(basketFileName);
    if (!err.isEmpty())
    {
        message(err, this);
        return;
    }
    updateBasketGUI();

    clearGraphs();
    DrawOrder.clear();

    QJsonObject js;
    LoadJsonFromFile(js, fileName);
    err = readFromJson(js);
    if (!err.isEmpty()) message(err, this);

    updateGUI();
}

void AMultiGraphDesigner::addDraw(QListWidget * lw)
{
    const int currentRow = lw->currentRow();

    if (DrawOrder.contains(currentRow))
        message("Already drawn!", lwBasket);
    else
    {
        DrawOrder << currentRow;
        on_pbRefactor_clicked();
    }
}

void AMultiGraphDesigner::onCoordItemDoubleClicked(QListWidgetItem *)
{
    addDraw(ui->lwCoords);
}

void AMultiGraphDesigner::onBasketItemDoubleClicked(QListWidgetItem *)
{
    addDraw(lwBasket);
}

void AMultiGraphDesigner::clearGraphs()
{
    TCanvas *c1 = RasterWindow->fCanvas;
    c1->Clear();
    Pads.clear();
}

void AMultiGraphDesigner::updateGUI()
{
    updateCanvas();
    updateNumbers();
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
                const QVector<ADrawObject> DrawObjects = Basket.getCopy(iBasketIndex);  // ***!!! is it safe? The copy is deleted on exiting this {}
                pad.tPad->cd();
                drawGraph(DrawObjects);
            }
        }
    }
    canvas->Update();
}

void AMultiGraphDesigner::updateNumbers()
{
    ui->lwCoords->clear();

    const int numX = ui->sbNumX->value();
    const int numY = ui->sbNumY->value();

    int max = std::min(DrawOrder.size(), numX * numY);

    for (int iItem = 0; iItem < Basket.size(); iItem++)
    {
        QListWidgetItem * it = new QListWidgetItem("-");
        it->setTextAlignment(Qt::AlignCenter);
        ui->lwCoords->addItem(it);
    }

    int counter = 0;
    for (int iy = 0; iy < numY; iy++)
        for (int ix = 0; ix < numX; ix++)
        {
            if (counter >= max) break;
            int iBasket = DrawOrder.at(counter);
            ui->lwCoords->item(iBasket)->setText(QString("%1-%2").arg(ix).arg(iy));
            counter++;
        }
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

        if (iY == 0)        y2 = 1 - margin;
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

    updateGUI();
}

void AMultiGraphDesigner::writeToJson(QJsonObject & json)
{
    json["NumX"] = ui->sbNumX->value();
    json["NumY"] = ui->sbNumY->value();

    QJsonArray ar;
    for (APadProperties & p : Pads)
    {
        QJsonObject js;
        p.updatePadGeometry();
        p.writeToJson(js);
        ar << js;
    }
    json["Pads"] = ar;

    QJsonArray arI;
    for (int i : DrawOrder) arI << i;
    json["DrawOrder"] = arI;

    json["WinWidth"]  = width();
    json["WinHeight"] = height();
}

QString AMultiGraphDesigner::readFromJson(const QJsonObject & json)
{
    QJsonArray ar;
    if (!parseJson(json, "Pads", ar)) return "Wrong file format!";

    const int size = ar.size();
    if (size == 0) return "No pads in the file!";

    for (int i = 0; i < size; i++)
    {
        QJsonObject js = ar.at(i).toObject();

        APadProperties newPad;
        newPad.readFromJson(js);
        Pads << newPad;
    }

    int numX = 2, numY = 1;
    parseJson(json, "NumX", numX);
    parseJson(json, "NumY", numY);
    ui->sbNumX->setValue(numX);
    ui->sbNumY->setValue(numY);

    QJsonArray arI;
    parseJson(json, "DrawOrder", arI);
    for (int i = 0; i < arI.size(); i++) DrawOrder << arI.at(i).toInt();

    int w = 600, h = 400;
    parseJson(json, "WinWidth", w);
    parseJson(json, "WinHeight", h);
    resize(w, h);

    return "";
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
    updateGUI();
}
