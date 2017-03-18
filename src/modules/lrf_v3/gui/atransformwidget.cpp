#include "atransformwidget.h"

#include <QGridLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDoubleValidator>

ATransformWidget::ATransformWidget(QWidget *parent) : QWidget(parent)
{
  QDoubleValidator *dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);

  x = new QLineEdit;
  x->setValidator(dv);
  y = new QLineEdit;
  y->setValidator(dv);
  z = new QLineEdit;
  z->setValidator(dv);
  phi = new QLineEdit;
  phi->setValidator(dv);
  flip = new QCheckBox("Flip y axis");

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(new QLabel("x"), 0, 0, 1, 1, Qt::AlignRight);
  layout->addWidget(x, 0, 1);
  layout->addWidget(new QLabel("y"), 0, 2);
  layout->addWidget(y, 0, 3);
  layout->addWidget(new QLabel("z"), 0, 4);
  layout->addWidget(z, 0, 5);
  layout->addWidget(new QLabel("phi"), 1, 0);
  layout->addWidget(phi, 1, 1);
  layout->addWidget(flip, 1, 3, 1, 2);
  setLayout(layout);
}

ATransform ATransformWidget::getTransform() const {
  APoint shift(x->text().toDouble(), y->text().toDouble(), z->text().toDouble());
  return ATransform(shift, phi->text().toDouble(), flip->isChecked());
}

void ATransformWidget::setTransform(const ATransform &transform) {
  APoint shift = transform.getShift();
  x->setText(QString::number(shift.x()));
  y->setText(QString::number(shift.y()));
  z->setText(QString::number(shift.z()));
  phi->setText(QString::number(transform.getPhi()));
  flip->setChecked(transform.getFlip());
}
