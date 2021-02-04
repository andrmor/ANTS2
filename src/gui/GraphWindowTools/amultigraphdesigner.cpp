#include "amultigraphdesigner.h"
#include "ui_amultigraphdesigner.h"
#include "rasterwindowbaseclass.h"
#include "amessage.h"
#include "abasketlistwidget.h"
#include "abasketmanager.h"
#include "amultigraphconfigurator.h"

#include <QHBoxLayout>
#include <QListWidget>

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

}

void AMultiGraphDesigner::on_action2_x_1_triggered()
{

}

void AMultiGraphDesigner::on_actionAs_pdf_triggered()
{

}

void AMultiGraphDesigner::on_actionSave_triggered()
{

}
