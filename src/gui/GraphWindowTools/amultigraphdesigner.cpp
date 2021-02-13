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
    fillOutBasicLayout(1,2);
}

void AMultiGraphDesigner::on_action2_x_1_triggered()
{
    fillOutBasicLayout(2,1);
}

void AMultiGraphDesigner::on_action2_x_2_triggered()
{
    fillOutBasicLayout(2,2);
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
//    TCanvas *c1 = RasterWindow->fCanvas;

//    for (const APadProperties& aPad: Pads)
//    {
//        c1->cd();
//        aPad.tPad->Draw();
//        const QVector<ADrawObject> DrawObjects = Basket.getCopy(aPad.basketIndex);
//        aPad.tPad->cd();
//        drawGraph(DrawObjects);
//    }
//    c1->Update();

    TCanvas *c1 = RasterWindow->fCanvas;

    for (const QVector<APadProperties> aPadGroup : APads)
    {
        for (const APadProperties& aPad: aPadGroup)
        {
            c1->cd();
            aPad.tPad->Draw();
            const QVector<ADrawObject> DrawObjects = Basket.getCopy(aPad.basketIndex);
            aPad.tPad->cd();
            drawGraph(DrawObjects);
        }
    }
    c1->Update();
}

void AMultiGraphDesigner::fillOutBasicLayout(int M, int m, bool horizontal)
{
    double margin    = 0;
    double padM  = 1.0/M;
    double padm = 1.0/m;

//    if (vertical)
//    double *x = nullptr;
//    double *y = nullptr;

    qDebug() <<"M" <<padM<< "m" << padm;


    for (int iM = 0; iM < M; iM++)
    {
        QVector <APadProperties> aPadGroup;
        double M2;
        double M1;

        if (horizontal)   //y is counted from bottom to top so for easier basket use I inverted order it goes (1->0)
        {
            M2 = 1-(iM * padM);
            M1 = M2 - padM;

            if (iM == 0)     M2 = 1-margin;
            if (iM == M - 1) M1 = margin;
        }
        else
        {
            M1 = iM * padM;
            M2 = M1 + padM;

            if (iM == 0) M1 = margin;
            if (iM == M - 1) M2 = 1-margin;
        }

        qDebug() << "M1" << M1;
        qDebug() << "M2" << M2;

        for (int im = 0; im < m; im++)
        {
            double m1 = im * padm;
            double m2 = m1 + padm;

            if (im == 0) m1 = margin;
            if (im == m - 1) m2 = 1-margin;

            qDebug() << "m1" << m1;
            qDebug() << "m2" << m2;

            QString padname = "pad" + QString::number(iM) + "_" + QString::number(im);
            TString padnameT(padname.toLatin1().data());

            double y1;
            double y2;
            double x1;
            double x2;

            if (horizontal)
            {
                y1 = M1;
                y2 = M2;
                x1 = m1;
                x2 = m2;
            }
            else
            {
                y1 = m1;
                y2 = m2;
                x1 = M1;
                x2 = M2;
            }

            TPad* ipad = new TPad(padnameT, padnameT, x1, y1, x2, y2);
            APadProperties apad(ipad);

            aPadGroup << apad;
            //apad.tPad->Draw();

        }
        APads << aPadGroup;

    }

    qDebug() << "pads" << APadsToString();
    APads[2][1].basketIndex = 1;
    updateCanvas();
}

//void AMultiGraphDesigner::FillOutBasicLayout(int numA, int numB)
//{
//    bSizes.clear();
//    double bIncrement = 1.0/numB;

//    for (int ia = 0; ia < numA; ia++)
//    {
//        QVector<double> bSize;

//        for (int ib = 0; ib < numB; ib++)
//        {
//            bSize << bIncrement;
//        }

//        bSizes << bSize;
//    }

//    qDebug() << "bSizes"<<bSizes;
//}

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
    for (APadProperties& aPad: Pads)
    {
        QJsonObject js;
        aPad.updatePadGeometry(); //    EXTREMELY TEMPORARY!!!
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

QString AMultiGraphDesigner::APadsToString()
{
    QString str = "";
    for (QVector<APadProperties> prop : APads)
    {
        str += "[";
        for (APadProperties pad : prop)
        {
            str += "{";
            str += pad.toString();
            str += "}, ";
        }
        str.chop(2);
        str += "] ; ";

    }
    str.chop(3);
    return str;
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
    fillOutBasicLayout(4,3, false);
//   FillOutBasicLayout(4,5);


}
