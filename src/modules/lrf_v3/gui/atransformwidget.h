#ifndef ATRANSFORMWIDGET_H
#define ATRANSFORMWIDGET_H

#include <QWidget>

#include "atransform.h"

class QLineEdit;
class QCheckBox;

class ATransformWidget : public QWidget
{
  Q_OBJECT
  QLineEdit *x, *y, *z;
  QLineEdit *phi;
  QCheckBox *flip;
public:
  explicit ATransformWidget(QWidget *parent = 0);
  ATransform getTransform() const;
  void setTransform(const ATransform &transform);
};

#endif // ATRANSFORMWIDGET_H
