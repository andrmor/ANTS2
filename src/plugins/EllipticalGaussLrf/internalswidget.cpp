#include "internalswidget.h"

#include <cmath>

#include "atransformwidget.h"

InternalsWidget::InternalsWidget(QWidget *parent) :
  QWidget(parent)
{
  setupUi(this);
  transform = new ATransformWidget;
  static_cast<QGridLayout*>(layout())->addWidget(transform, 9, 0, 1, 3);
  QDoubleValidator* dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);
  for(QLineEdit *le : this->findChildren<QLineEdit *>())
    le->setValidator(dv);
}

void InternalsWidget::saveState(QJsonObject &state) const
{
  state["transform"] = transform->getTransform().toJson();
  state["rmax"] = draw_distance->text().toDouble();
  state["amp"] = hint_amp->text().toDouble();
  state["sigma2x"] = hint_s2x->text().toDouble()*hint_s2x->text().toDouble();
  state["sigma2y"] = hint_s2y->text().toDouble()*hint_s2y->text().toDouble();
  state["tail"] = hint_tail->text().toDouble();;
  state["e_amp"] = hint_e_amp->text().toDouble();;
  state["e_sigma2x"] = hint_e_s2x->text().toDouble()*hint_e_s2x->text().toDouble();
  state["e_sigma2y"] = hint_e_s2y->text().toDouble()*hint_e_s2y->text().toDouble();
  state["e_tail"] = hint_e_tail->text().toDouble();
}

void InternalsWidget::loadState(const QJsonObject &state)
{
  transform->setTransform(ATransform::fromJson(state["transform"].toObject()));
  draw_distance->setText(QString::number(state["rmax"].toDouble()));
  hint_amp->setText(QString::number(state["amp"].toDouble()));
  hint_s2x->setText(QString::number(std::sqrt(state["sigma2x"].toDouble())));
  hint_s2y->setText(QString::number(std::sqrt(state["sigma2y"].toDouble())));
  hint_tail->setText(QString::number(state["tail"].toDouble()));
  hint_e_amp->setText(QString::number(state["e_amp"].toDouble()));
  hint_e_s2x->setText(QString::number(std::sqrt(state["e_sigma2x"].toDouble())));
  hint_e_s2y->setText(QString::number(std::sqrt(state["e_sigma2y"].toDouble())));
  hint_e_tail->setText(QString::number(state["e_tail"].toDouble()));
}
