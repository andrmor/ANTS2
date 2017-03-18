#include "atpspline3widget.h"

#include <QIntValidator>
#include <QDoubleValidator>
#include <QLineEdit>
#include <QTableWidget>
#include <QLabel>
#include <QGridLayout>
#include <QGroupBox>
#include <QDebug>

ATPspline3Widget::ATPspline3Widget(const QString &dimx_name, const QString &dimy_name,
                                   bool dimx_is_row, QWidget *parent)
  : QWidget(parent)
{
  this->dimx_is_row = dimx_is_row;

  iv = new QIntValidator(this);
  iv->setBottom(1);
  QDoubleValidator *dv = new QDoubleValidator(this);

  nodesx = new QLineEdit("");
  nodesx->setValidator(iv);
  minx = new QLineEdit("");
  minx->setValidator(dv);
  maxx = new QLineEdit("");
  maxx->setValidator(dv);

  nodesy = new QLineEdit("");
  nodesy->setValidator(iv);
  miny = new QLineEdit("");
  miny->setValidator(dv);
  maxy = new QLineEdit("");
  maxy->setValidator(dv);


  QHBoxLayout *layout_x_props = new QHBoxLayout;
  layout_x_props->addWidget(new QLabel("Nodes:"));
  layout_x_props->addWidget(nodesx);
  layout_x_props->addWidget(new QLabel("Min:"));
  layout_x_props->addWidget(minx);
  layout_x_props->addWidget(new QLabel("Max:"));
  layout_x_props->addWidget(maxx);
  QGroupBox *gb_x_props = new QGroupBox(dimx_name+" properties");
  gb_x_props->setLayout(layout_x_props);

  QHBoxLayout *layout_y_props = new QHBoxLayout;
  layout_y_props->addWidget(new QLabel("Nodes:"));
  layout_y_props->addWidget(nodesy);
  layout_y_props->addWidget(new QLabel("Min:"));
  layout_y_props->addWidget(miny);
  layout_y_props->addWidget(new QLabel("Max:"));
  layout_y_props->addWidget(maxy);
  QGroupBox *gb_y_props = new QGroupBox(dimy_name+" properties");
  gb_y_props->setLayout(layout_y_props);

  coeffs = new QTableWidget;

  QString table_text = "Coefficients table (";
  if(dimx_is_row)
    table_text += "Rows sorted by increasing "+dimy_name+"):";
  else
    table_text += "Rows sorted by increasing "+dimx_name+"):";

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(dimx_is_row ? gb_x_props : gb_y_props);
  layout->addWidget(dimx_is_row ? gb_y_props : gb_x_props);
  layout->addWidget(new QLabel(table_text));
  layout->addWidget(coeffs);
  setLayout(layout);

  if(dimx_is_row) {
    connect(nodesx, &QLineEdit::editingFinished, this, &ATPspline3Widget::onXNodeCountChanged);
    connect(nodesy, &QLineEdit::editingFinished, this, &ATPspline3Widget::onYNodeCountChanged);
  } else {
    connect(nodesx, &QLineEdit::editingFinished, this, &ATPspline3Widget::onYNodeCountChanged);
    connect(nodesy, &QLineEdit::editingFinished, this, &ATPspline3Widget::onXNodeCountChanged);
  }
}

TPspline3 ATPspline3Widget::getSpline() const
{
  int column_count = coeffs->rowCount();
  int row_count = coeffs->columnCount();
  double minx = this->minx->text().toDouble();
  double maxx = this->maxx->text().toDouble();
  int nodesx = this->nodesx->text().toDouble();
  double miny = this->miny->text().toDouble();
  double maxy = this->maxy->text().toDouble();
  int nodesy = this->nodesy->text().toDouble();
  if(dimx_is_row) {
    std::swap(column_count, row_count);
    std::swap(minx, miny);
    std::swap(maxx, maxy);
    std::swap(nodesx, nodesy);
  }

  std::vector<double> c(row_count*column_count);
  TPspline3 spline(minx, maxx, nodesx, miny, maxy, nodesy);
  for(int y = 0; y < row_count; y++) {
    for(int x = 0; x < column_count; x++) {
      if(dimx_is_row)
        c[y*column_count+x] = static_cast<QLineEdit*>(coeffs->cellWidget(y, x))->text().toDouble();
      else
        c[y*column_count+x] = static_cast<QLineEdit*>(coeffs->cellWidget(x, y))->text().toDouble();
    }
  }
  spline.SetCoef(c);
  return spline;
}

void ATPspline3Widget::setSpline(const TPspline3 &spline)
{
  int column_count = spline.GetNintX()+3;
  int row_count = spline.GetNintY()+3;
  double minx = spline.GetXmin();
  double maxx = spline.GetXmax();
  int nodesx = spline.GetNintX();
  double miny = spline.GetYmin();
  double maxy = spline.GetYmax();
  int nodesy = spline.GetNintY();

  if(dimx_is_row) {
    std::swap(column_count, row_count);
    std::swap(minx, miny);
    std::swap(maxx, maxy);
    std::swap(nodesx, nodesy);
  }

  this->nodesx->setText(QString::number(nodesx));
  this->minx->setText(QString::number(minx));
  this->maxx->setText(QString::number(maxx));
  this->nodesy->setText(QString::number(nodesy));
  this->miny->setText(QString::number(miny));
  this->maxy->setText(QString::number(maxy));

  const auto &spline_coeffs = spline.GetCoef();
  coeffs->setRowCount(row_count);
  coeffs->setColumnCount(column_count);
  for(int y = 0; y < row_count; y++) {
    for(int x = 0; x < column_count; x++) {
      QLineEdit *cell = new QLineEdit(QString::number(spline_coeffs[y*column_count+x]));
      cell->setValidator(iv);
      if(dimx_is_row)
        coeffs->setCellWidget(y, x, cell);
      else
        coeffs->setCellWidget(x, y, cell);
    }
  }
}

void ATPspline3Widget::onXNodeCountChanged()
{
  int old_count = coeffs->columnCount();
  int new_count = nodesx->text().toInt()+3; //coeffs.size() == nbas == nint+3
  coeffs->setColumnCount(new_count);
  if(old_count < new_count) {
    for(int i = old_count; i <= new_count; i++) {
      for(int y = 0; y < coeffs->rowCount(); y++) {
        QLineEdit *cell = new QLineEdit("0");
        cell->setValidator(iv);
        coeffs->setCellWidget(y, i, cell);
      }
    }
  }
}

void ATPspline3Widget::onYNodeCountChanged()
{
  int old_count = coeffs->rowCount();
  int new_count = nodesy->text().toInt()+3; //coeffs.size() == nbas == nint+3
  coeffs->setRowCount(new_count);
  if(old_count < new_count) {
    for(int i = old_count; i <= new_count; i++) {
      for(int x = 0; x < coeffs->columnCount(); x++) {
        QLineEdit *cell = new QLineEdit("0");
        cell->setValidator(iv);
        coeffs->setCellWidget(i, x, cell);
      }
    }
  }
}
