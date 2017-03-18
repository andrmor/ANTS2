#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include "ui_settingswidget.h"
#include "astateinterface.h"
#include "idclasses.h"

namespace LRF {
class ALrf;
}

class SettingsWidget : public QWidget, public AStateInterface, private Ui::SettingsWidget
{
  Q_OBJECT

public:
  explicit SettingsWidget(QWidget *parent = 0);

signals:
  void elementChanged();
public slots:
  void saveState(QJsonObject &settings) const;
  void loadState(const QJsonObject &settings);
};

#endif // SETTINGSWIDGET_H
