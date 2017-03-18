#ifndef LRFCOREPLUGINWIDGETS_H
#define LRFCOREPLUGINWIDGETS_H

#include <memory>
#include <QJsonObject>

#include "completingtexteditclass.h"
#include "ahighlighters.h"
#include "idclasses.h"
#include "astateinterface.h"

class QSpinBox;
class QCheckBox;
class QGroupBox;
class ACollapsibleGroupBox;
class QLineEdit;
class QTextEdit;
class QTableWidget;
class QDoubleValidator;
class QStackedWidget;
class QRadioButton;

class AVladimirCompressionWidget;
class ATransformWidget;
class ABspline3Widget;
class ATPspline3Widget;

namespace LRF { namespace CoreLrfs {

class AxialSettingsWidget : public QWidget, public AStateInterface {
  Q_OBJECT
  QSpinBox *sb_nint;
  QCheckBox *cb_flattop;
  ACollapsibleGroupBox *gb_compression;
  QLineEdit *led_k, *led_r0, *led_lam;
public:
  AxialSettingsWidget(QWidget *parent = nullptr);
  void saveState(QJsonObject &json) const override;
  void loadState(const QJsonObject &settings) override;
signals:
  void elementChanged();
};

class AxialInternalsWidget : public QWidget, public AStateInterface {
  Q_OBJECT
  ATransformWidget *transform;
  ABspline3Widget *response, *sigma;
  QCheckBox *flat_top;
  AVladimirCompressionWidget *compression;
public:
  AxialInternalsWidget(QWidget *parent = nullptr);
  void saveState(QJsonObject &state) const override;
  void loadState(const QJsonObject &state) override;
signals:
  void elementChanged();
};


class Axial3DSettingsWidget : public QWidget, public AStateInterface {
  Q_OBJECT
  QSpinBox *sb_nintr, *sb_nintz;
  AVladimirCompressionWidget *compression;
public:
  Axial3DSettingsWidget(QWidget *parent = nullptr);
  void saveState(QJsonObject &json) const override;
  void loadState(const QJsonObject &settings) override;
signals:
  void elementChanged();
};

class Axial3DInternalsWidget : public QWidget, public AStateInterface {
  Q_OBJECT
  ATransformWidget *transform;
  ATPspline3Widget *response, *sigma;
  AVladimirCompressionWidget *compression;
public:
  Axial3DInternalsWidget(QWidget *parent = nullptr);
  void saveState(QJsonObject &state) const override;
  void loadState(const QJsonObject &state) override;
signals:
  void elementChanged();
};


class AxySettingsWidget : public QWidget, public AStateInterface {
  Q_OBJECT
  QSpinBox *sb_nintx;
  QSpinBox *sb_ninty;
public:
  AxySettingsWidget(QWidget *parent = nullptr);
  void saveState(QJsonObject &json) const override;
  void loadState(const QJsonObject &settings) override;
signals:
  void elementChanged();
};

class AxyInternalsWidget : public QWidget, public AStateInterface {
  Q_OBJECT
  ATransformWidget *transform;
  ATPspline3Widget *response, *sigma;
public:
  AxyInternalsWidget(QWidget *parent = nullptr);
  void saveState(QJsonObject &state) const override;
  void loadState(const QJsonObject &state) override;
signals:
  void elementChanged();
};


class ASlicedXYSettingsWidget : public QWidget, public AStateInterface {
  Q_OBJECT
  QSpinBox *sb_nintx;
  QSpinBox *sb_ninty;
  QSpinBox *sb_nintz;
public:
  ASlicedXYSettingsWidget(QWidget *parent = nullptr);
  void saveState(QJsonObject &json) const override;
  void loadState(const QJsonObject &settings) override;
signals:
  void elementChanged();
};

class ASlicedXYInternalsWidget : public QWidget, public AStateInterface {
  Q_OBJECT
  ATransformWidget *transform;
  QLineEdit *minz, *maxz, *nodesz;
  QRadioButton *rb_sigma;
  QSpinBox *sb_slice;
  QStackedWidget *sw_spline_choice;
  QVector<ATPspline3Widget*> response, sigma;
public:
  ASlicedXYInternalsWidget(QWidget *parent = nullptr);
  void saveState(QJsonObject &state) const override;
  void loadState(const QJsonObject &state) override;
signals:
  void elementChanged();
private slots:
  void onZnodesChanged();
  void onSetZSlice();
};


//TODO: add tree view to check script results to avoid user (and dev!) headaches
class ScriptSettingsWidget : public QWidget, public AStateInterface {
  Q_OBJECT
  CompletingTextEditClass *code;
  AHighlighterScriptWindow* highlighter;
  QTableWidget *parameters;
public:
  ScriptSettingsWidget(const QString &example_code, QWidget *parent = nullptr);
  void saveState(QJsonObject &json) const override;
  void loadState(const QJsonObject &settings) override;
signals:
  void elementChanged();
};

class ScriptInternalsWidget : public QWidget, public AStateInterface {
  Q_OBJECT
  ATransformWidget *transform;
  CompletingTextEditClass *code;
  AHighlighterScriptWindow* highlighter;
  QJsonObject config;
  QTableWidget *parameters, *parameters_sigma;
  QDoubleValidator *dv;
public:
  ScriptInternalsWidget(QWidget *parent = nullptr);
  void saveState(QJsonObject &state) const override;
  void loadState(const QJsonObject &state) override;
signals:
  void elementChanged();
};

} } //namespace LRF::CoreLrfs

#endif // LRFCOREPLUGINWIDGETS_H
