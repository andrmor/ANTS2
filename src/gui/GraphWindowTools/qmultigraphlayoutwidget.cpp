#include "qmultigraphlayoutwidget.h"
#include "ui_qmultigraphlayoutwidget.h"

QMultiGraphLayoutWidget::QMultiGraphLayoutWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::QMultiGraphLayoutWidget)
{
    ui->setupUi(this);
}

QMultiGraphLayoutWidget::~QMultiGraphLayoutWidget()
{
    delete ui;
}
