#include "settingswidget.h"

#include <cmath>

#include "lrf.h"

SettingsWidget::SettingsWidget(QWidget *parent) :
  QWidget(parent)
{
  setupUi(this);
  QDoubleValidator* dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);
  for(QLineEdit *le : this->findChildren<QLineEdit *>()) {
    le->setValidator(dv);
    connect(le, &QLineEdit::editingFinished, this, &SettingsWidget::elementChanged);
  }
}

void SettingsWidget::saveState(QJsonObject &settings) const
{
  settings["amp"] = hint_amp->text().toDouble();
  settings["sigma2x"] = hint_s2x->text().toDouble()*hint_s2x->text().toDouble();
  settings["sigma2y"] = hint_s2y->text().toDouble()*hint_s2y->text().toDouble();
  settings["tail"] = hint_tail->text().toDouble();
  settings["e_amp"] = hint_e_amp->text().toDouble();
  settings["e_sigma2x"] = hint_e_s2x->text().toDouble()*hint_e_s2x->text().toDouble();
  settings["e_sigma2y"] = hint_e_s2y->text().toDouble()*hint_e_s2y->text().toDouble();
  settings["e_tail"] = hint_e_tail->text().toDouble();
}

void SettingsWidget::loadState(const QJsonObject &settings)
{
  hint_amp->setText(QString::number(settings["amp"].toDouble()));
  hint_s2x->setText(QString::number(std::sqrt(settings["sigma2x"].toDouble())));
  hint_s2y->setText(QString::number(std::sqrt(settings["sigma2y"].toDouble())));
  hint_tail->setText(QString::number(settings["tail"].toDouble()));
  hint_e_amp->setText(QString::number(settings["e_amp"].toDouble()));
  hint_e_s2x->setText(QString::number(std::sqrt(settings["e_sigma2x"].toDouble())));
  hint_e_s2y->setText(QString::number(std::sqrt(settings["e_sigma2y"].toDouble())));
  hint_e_tail->setText(QString::number(settings["e_tail"].toDouble()));
}
