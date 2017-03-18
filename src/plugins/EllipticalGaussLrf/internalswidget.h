#ifndef INTERNALSWIDGET_H
#define INTERNALSWIDGET_H

#include "ui_internalswidget.h"
#include "idclasses.h"
#include "astateinterface.h"

namespace LRF {
class ALrf;
}

class ATransformWidget;

class InternalsWidget : public QWidget, public AStateInterface, private Ui::InternalsWidget
{
  Q_OBJECT
  ATransformWidget *transform;
public:
  explicit InternalsWidget(QWidget *parent = 0);
signals:
  void elementChanged();
public slots:
  void saveState(QJsonObject &state) const;
  void loadState(const QJsonObject &state);
};

#endif // INTERNALSWIDGET_H
