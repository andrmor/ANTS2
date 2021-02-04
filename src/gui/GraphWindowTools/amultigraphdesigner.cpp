#include "amultigraphdesigner.h"
#include "ui_amultigraphdesigner.h"

AMultiGraphDesigner::AMultiGraphDesigner(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AMultiGraphDesigner)
{
    ui->setupUi(this);
}

AMultiGraphDesigner::~AMultiGraphDesigner()
{
    delete ui;
}
