#ifndef ATPSPLINE3WIDGET_H
#define ATPSPLINE3WIDGET_H

#include <QWidget>

#ifdef TPS3M
#include "tpspline3m.h"
#else
#include "tpspline3.h"
#endif

class QLineEdit;
class QTableWidget;
class QIntValidator;

class ATPspline3Widget : public QWidget
{
  Q_OBJECT
  QIntValidator *iv;
  QLineEdit *nodesx, *minx, *maxx;
  QLineEdit *nodesy, *miny, *maxy;
  QTableWidget *coeffs;
  bool dimx_is_row;
public:
  explicit ATPspline3Widget(const QString &dimx_name = "X", const QString &dimy_name = "Y",
                            bool dimx_is_row = true, QWidget *parent = 0);

  TPspline3 getSpline() const;
  void setSpline(const TPspline3 &spline);

signals:

private slots:
  void onXNodeCountChanged();
  void onYNodeCountChanged();
};

#endif // ATPSPLINE3WIDGET_H
