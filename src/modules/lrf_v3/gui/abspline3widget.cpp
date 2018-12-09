#include "abspline3widget.h"

#include <QGridLayout>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QSpinBox>
#include <QLabel>
#include <QTableWidget>

ABspline3Widget::ABspline3Widget(QWidget *parent) : QWidget(parent)
{
  iv = new QIntValidator(this);
  iv->setBottom(1);
  QDoubleValidator *dv = new QDoubleValidator(this);

  nodes = new QLineEdit("");
  nodes->setValidator(iv);

  min = new QLineEdit("");
  min->setValidator(dv);
  max = new QLineEdit("");
  max->setValidator(dv);

  coeffs = new QTableWidget;
  coeffs->setColumnCount(1);
  coeffs->setVerticalHeaderLabels(QStringList()<<""<<"Coefficient");

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(new QLabel("Nodes:"), 0, 0);
  layout->addWidget(nodes, 0, 1);
  layout->addWidget(new QLabel("Coefficients:"), 1, 0, 1, 2);
  layout->addWidget(coeffs, 2, 0, 1, 2);
  setLayout(layout);

  connect(nodes, &QLineEdit::editingFinished, this, &ABspline3Widget::onNodeCountChanged);
}

Bspline1d ABspline3Widget::getSpline() const
{
  Bspline1d spline(min->text().toDouble(), max->text().toDouble(), nodes->text().toInt());
  std::vector<double> c(coeffs->rowCount());
  for(int i = 0; i < coeffs->rowCount(); i++)
    c[i] = static_cast<QLineEdit*>(coeffs->cellWidget(i, 0))->text().toDouble();
  spline.SetCoef(c);
  return spline;
}

void ABspline3Widget::setSpline(const Bspline1d &spline)
{
  nodes->setText(QString::number(spline.GetNint()));
  min->setText(QString::number(spline.GetXmin()));
  max->setText(QString::number(spline.GetXmax()));

  const auto &spline_coeffs = spline.GetCoef();
  coeffs->setRowCount(spline_coeffs.size());
  QStringList hor_headers;
  for(size_t i = 0; i < spline_coeffs.size(); i++) {
    hor_headers << QString::number(i);
    QLineEdit *cell = new QLineEdit(QString::number(spline_coeffs[i]));
    cell->setValidator(iv);
    coeffs->setCellWidget(i, 0, cell);
  }
  coeffs->setHorizontalHeaderLabels(hor_headers);
}

void ABspline3Widget::onNodeCountChanged()
{
  int old_count = coeffs->rowCount();
  int new_count = nodes->text().toInt()+3; //coeffs.size() == nbas == nint+3
  coeffs->setRowCount(new_count);
  if(old_count < new_count) {
    QStringList hor_headers;
    for(int i = 0; i <= old_count; i++)
      hor_headers << QString::number(i);
    for(int i = old_count; i <= new_count; i++) {
      hor_headers << QString::number(i);
      QLineEdit *cell = new QLineEdit("0");
      cell->setValidator(iv);
      coeffs->setCellWidget(i, 0, cell);
    }
    coeffs->setHorizontalHeaderLabels(hor_headers);
  }
}
