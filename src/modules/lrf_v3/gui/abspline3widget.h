#ifndef ABSPLINE3WIDGET_H
#define ABSPLINE3WIDGET_H

#include <QWidget>

#include "bspline3.h"

class QLineEdit;
class QTableWidget;
class QIntValidator;

class ABspline3Widget : public QWidget
{
  Q_OBJECT
  QIntValidator *iv;
  QLineEdit *nodes;
  QLineEdit *min, *max;
  QTableWidget *coeffs;
public:
  explicit ABspline3Widget(QWidget *parent = 0);

  Bspline3 getSpline() const;
  void setSpline(const Bspline3 &spline);

signals:

private slots:
  void onNodeCountChanged();
};

#endif // ABSPLINE3WIDGET_H
