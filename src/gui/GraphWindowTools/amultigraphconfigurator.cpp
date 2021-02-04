#include "amultigraphconfigurator.h"

#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

AMultiGraphConfigurator::AMultiGraphConfigurator(QWidget *parent) : QWidget(parent)
{
    QGridLayout * gl = new QGridLayout(this);

    gl->addWidget(new QLabel("1"), 0,0);
    gl->addWidget(new QSpinBox(),  1,0);
    gl->addWidget(new QSpinBox(),  0,1);
}
