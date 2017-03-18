#ifndef AVLADIMIRCOMPRESSIONWIDGET_H
#define AVLADIMIRCOMPRESSIONWIDGET_H

#include <QWidget>
#include "avladimircompression.h"

class ACollapsibleGroupBox;
class QLineEdit;

class AVladimirCompressionWidget : public QWidget
{
  Q_OBJECT
  ACollapsibleGroupBox *gb_compression;
  QLineEdit *led_k, *led_r0, *led_lam;
public:
  explicit AVladimirCompressionWidget(QWidget *parent = 0);

  bool isChecked() const;

  AVladimirCompression getCompression() const;
  void setCompression(const AVladimirCompression &compression);
signals:
  void elementChanged();
};

#endif // AVLADIMIRCOMPRESSIONWIDGET_H
