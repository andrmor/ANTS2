#include "avladimircompressionwidget.h"

#include <QLineEdit>
#include <QDoubleValidator>
#include <QFormLayout>

#include "acollapsiblegroupbox.h"

AVladimirCompressionWidget::AVladimirCompressionWidget(QWidget *parent) : QWidget(parent)
{
  gb_compression = new ACollapsibleGroupBox("Compression");
  {
    QDoubleValidator *dv = new QDoubleValidator(gb_compression);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    dv->setBottom(0);

    led_k = new QLineEdit("6");
    led_k->setValidator(dv);
    led_k->setMinimumHeight(20);

    led_r0 = new QLineEdit("8");
    led_r0->setValidator(dv);
    led_r0->setMinimumHeight(20);

    led_lam = new QLineEdit("5");
    led_lam->setValidator(dv);
    led_lam->setMinimumHeight(20);

    QFormLayout *lay_compr = new QFormLayout;
    lay_compr->setContentsMargins(2, 2, 2, 6);
    lay_compr->setSpacing(4);
    lay_compr->addRow("Factor:", led_k);
    lay_compr->addRow("Switchover:", led_r0);
    lay_compr->addRow("Smoothness:", led_lam);

    gb_compression->getClientAreaWidget()->setLayout(lay_compr);
    gb_compression->setChecked(false);
  }

  QVBoxLayout *layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(gb_compression);
  setLayout(layout);

  connect(led_k, &QLineEdit::editingFinished, this, &AVladimirCompressionWidget::elementChanged);
  connect(led_r0, &QLineEdit::editingFinished, this, &AVladimirCompressionWidget::elementChanged);
  connect(led_lam, &QLineEdit::editingFinished, this, &AVladimirCompressionWidget::elementChanged);
  connect(gb_compression, &ACollapsibleGroupBox::clicked, this, &AVladimirCompressionWidget::elementChanged);
}

bool AVladimirCompressionWidget::isChecked() const
{
  return gb_compression->isChecked();
}

AVladimirCompression AVladimirCompressionWidget::getCompression() const
{
  return AVladimirCompression(led_k->text().toDouble(),
                              led_r0->text().toDouble(),
                              led_lam->text().toDouble());
}

void AVladimirCompressionWidget::setCompression(const AVladimirCompression &compression)
{
  gb_compression->setChecked(compression.isCompressing());
  led_k->setText(QString::number(compression.k()));
  led_r0->setText(QString::number(compression.r0()));
  led_lam->setText(QString::number(compression.lam()));
}
