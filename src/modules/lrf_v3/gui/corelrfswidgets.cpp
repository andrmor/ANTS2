#include "corelrfswidgets.h"
#include "acollapsiblegroupbox.h"

#include <QDebug>
#include <QJsonObject>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QGroupBox>

#include "corelrfs.h"
#include "acollapsiblegroupbox.h"
#include "avladimircompressionwidget.h"
#include "atransformwidget.h"
#include "abspline3widget.h"
#include "atpspline3widget.h"
#include "spline.h"
#include "ascriptvalueconverter.h"
#include "amessage.h"

namespace LRF { namespace CoreLrfs {

/***************************************************************************\
*                   Implementation of Axial and related                     *
\***************************************************************************/
AxialSettingsWidget::AxialSettingsWidget(QWidget *parent)
  : QWidget(parent)
{
  QFormLayout *lay_nodes = new QFormLayout;
  {
    sb_nint = new QSpinBox;
    sb_nint->setValue(10);
    sb_nint->setMinimum(2);
    sb_nint->setMaximum(99999999);

    cb_flattop = new QCheckBox;
    cb_flattop->setChecked(true);

    lay_nodes->setContentsMargins(0, 0, 0, 0);
    lay_nodes->setHorizontalSpacing(5);
    lay_nodes->addRow("Nodes:", sb_nint);
    lay_nodes->addRow("Set 0 deriv. at origin:", cb_flattop);
  }

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
  layout->addLayout(lay_nodes);
  layout->addWidget(gb_compression);
  setLayout(layout);

  connect(sb_nint, &QSpinBox::editingFinished, this, &AxialSettingsWidget::elementChanged);
  connect(cb_flattop, &QCheckBox::toggled, this, &AxialSettingsWidget::elementChanged);
  connect(led_k, &QLineEdit::editingFinished, this, &AxialSettingsWidget::elementChanged);
  connect(led_r0, &QLineEdit::editingFinished, this, &AxialSettingsWidget::elementChanged);
  connect(led_lam, &QLineEdit::editingFinished, this, &AxialSettingsWidget::elementChanged);
  connect(gb_compression, &ACollapsibleGroupBox::clicked, this, &AxialSettingsWidget::elementChanged);
}

void AxialSettingsWidget::saveState(QJsonObject &json) const
{
  json["nint"] = sb_nint->value();
  json["flat top"] = cb_flattop->isChecked();
  if(gb_compression->isChecked()) {
    QJsonObject compr;
    AVladimirCompression(led_k->text().toDouble(), led_r0->text().toDouble(),
                         led_lam->text().toDouble()).toJson(compr);
    json["compression"] = compr;
  }
}

void AxialSettingsWidget::loadState(const QJsonObject &settings)
{
  sb_nint->setValue(settings["nint"].toInt());
  cb_flattop->setChecked(settings["flat top"].toBool());

  QJsonObject compr_settings = settings["compression"].toObject();
  bool has_compression = !compr_settings.isEmpty();
  if(has_compression) {
    AVladimirCompression compression(compr_settings);
    gb_compression->setChecked(compression.isCompressing());
    led_k->setText(QString::number(compression.k()));
    led_r0->setText(QString::number(compression.r0()));
    led_lam->setText(QString::number(compression.lam()));
  }
}


AxialInternalsWidget::AxialInternalsWidget(QWidget *parent)
  : QWidget(parent)
{
  transform = new ATransformWidget;
  flat_top = new QCheckBox("Flatten Lrf top");
  flat_top->setEnabled(false);

  compression = new AVladimirCompressionWidget;
  response = new ABspline3Widget;
  sigma = new ABspline3Widget;

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(transform, 0, 0, 1, 2);
  layout->addWidget(flat_top, 1, 0);
  layout->addWidget(compression, 2, 0);
  layout->addWidget(new QLabel("Response spline:"), 3, 0);
  layout->addWidget(response, 4, 0);
  layout->addWidget(new QLabel("Sigma spline:"), 3, 1);
  layout->addWidget(sigma, 4, 1);
  setLayout(layout);
}

void AxialInternalsWidget::saveState(QJsonObject &state) const
{
  state["transform"] = transform->getTransform().toJson();
  state["flattop"] = flat_top->isChecked();
  if(compression->isChecked())
    compression->getCompression().toJson(state);

  QJsonObject state_response_spline;
  Bspline3 spline_response = response->getSpline();
  write_bspline3_json(&spline_response, state_response_spline);
  QJsonObject state_response;
  state_response["bspline3"] = state_response_spline;
  state["response"] = state_response;

  QJsonObject state_sigma_spline;
  Bspline3 spline_sigma = sigma->getSpline();
  write_bspline3_json(&spline_sigma, state_sigma_spline);
  QJsonObject state_sigma;
  state_sigma["bspline3"] = state_sigma_spline;
  state["error"] = state_sigma;
}

void AxialInternalsWidget::loadState(const QJsonObject &state)
{
  ATransform t = ATransform::fromJson(state["transform"].toObject());
  transform->setTransform(t);
  flat_top->setChecked(state["flattop"].toBool());

  compression->setCompression(AVladimirCompression(state));

  QJsonObject state_reponse_spline = state["response"].toObject()["bspline3"].toObject();
  Bspline3 *spline_response = read_bspline3_json(state_reponse_spline);
  if(spline_response == nullptr) return;
  response->setSpline(*spline_response);
  delete spline_response;

  QJsonObject state_sigma_spline = state["error"].toObject()["bspline3"].toObject();
  Bspline3 *spline_sigma = read_bspline3_json(state_sigma_spline);
  if(spline_sigma == nullptr) return;
  sigma->setSpline(*spline_sigma);
  delete spline_sigma;
}


/***************************************************************************\
*                  Implementation of Axial3D and related                    *
\***************************************************************************/
Axial3DSettingsWidget::Axial3DSettingsWidget(QWidget *parent)
  : QWidget(parent)
{
  QFormLayout *lay_nodes = new QFormLayout;
  {
    sb_nintr = new QSpinBox;
    sb_nintr->setValue(10);
    sb_nintr->setMinimum(2);
    sb_nintr->setMaximum(99999999);

    sb_nintz = new QSpinBox;
    sb_nintz->setValue(10);
    sb_nintz->setMinimum(2);
    sb_nintz->setMaximum(99999999);

    lay_nodes->setContentsMargins(0, 0, 0, 0);
    lay_nodes->setHorizontalSpacing(5);
    lay_nodes->addRow("R Nodes:", sb_nintr);
    lay_nodes->addRow("Z Nodes:", sb_nintz);
  }

  compression = new AVladimirCompressionWidget;

  QVBoxLayout *layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(lay_nodes);
  layout->addWidget(compression);
  setLayout(layout);

  connect(sb_nintr, &QSpinBox::editingFinished, this, &Axial3DSettingsWidget::elementChanged);
  connect(sb_nintz, &QSpinBox::editingFinished, this, &Axial3DSettingsWidget::elementChanged);
  connect(compression, &AVladimirCompressionWidget::elementChanged, this, &Axial3DSettingsWidget::elementChanged);
}

void Axial3DSettingsWidget::saveState(QJsonObject &json) const
{
  json["nintr"] = sb_nintr->value();
  json["nintz"] = sb_nintz->value();
  if(compression->isChecked()) {
    QJsonObject compr;
    compression->getCompression().toJson(compr);
    json["compression"] = compr;
  }
}

void Axial3DSettingsWidget::loadState(const QJsonObject &settings)
{
  sb_nintr->setValue(settings["nintr"].toInt());
  sb_nintz->setValue(settings["nintz"].toInt());

  QJsonObject compr_settings = settings["compression"].toObject();
  bool has_compression = !compr_settings.isEmpty();
  if(has_compression)
    compression->setCompression(AVladimirCompression(compr_settings));
}


Axial3DInternalsWidget::Axial3DInternalsWidget(QWidget *parent)
 : QWidget(parent)
{
  transform = new ATransformWidget;
  compression = new AVladimirCompressionWidget;

  QRadioButton *rb_response = new QRadioButton("Response spline");
  rb_response->setChecked(true);
  QRadioButton *rb_sigma = new QRadioButton("Sigma spline");

  response = new ATPspline3Widget("R", "Z", false);
  sigma = new ATPspline3Widget("R", "Z", false);
  QStackedWidget *sw_spline_choice = new QStackedWidget;
  sw_spline_choice->addWidget(response);
  sw_spline_choice->addWidget(sigma);

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(transform, 0, 0, 1, 2);
  layout->addWidget(compression, 1, 0, 1, 2);
  layout->addWidget(rb_response, 2, 0);
  layout->addWidget(rb_sigma, 2, 1);
  layout->addWidget(sw_spline_choice, 3, 0, 1, 2);
  setLayout(layout);

  connect(rb_response, &QRadioButton::clicked, [=]() { sw_spline_choice->setCurrentIndex(0); });
  connect(rb_sigma, &QRadioButton::clicked, [=]() { sw_spline_choice->setCurrentIndex(1); });
}

void Axial3DInternalsWidget::saveState(QJsonObject &state) const
{
  state["transform"] = transform->getTransform().toJson();
  if(compression->isChecked()) {
    QJsonObject compr;
    compression->getCompression().toJson(compr);
    state["compression"] = compr;
  }

  QJsonObject state_response_spline;
  TPspline3 spline_response = response->getSpline();
  write_tpspline3_json(&spline_response, state_response_spline);
  QJsonObject state_response;
  state_response["tpspline3"] = state_response_spline;
  state["response"] = state_response;

  TPspline3 spline_sigma = sigma->getSpline();
  if(!spline_sigma.isInvalid()) {
    QJsonObject state_sigma_spline;
    write_tpspline3_json(&spline_sigma, state_sigma_spline);
    QJsonObject state_sigma;
    state_sigma["tpspline3"] = state_sigma_spline;
    state["error"] = state_sigma;
  }
}

void Axial3DInternalsWidget::loadState(const QJsonObject &state)
{
  ATransform t = ATransform::fromJson(state["transform"].toObject());
  transform->setTransform(t);

  compression->setCompression(AVladimirCompression(state["compression"].toObject()));

  QJsonObject state_reponse_spline = state["response"].toObject()["tpspline3"].toObject();
  TPspline3 *spline_response = read_tpspline3_json(state_reponse_spline);
  if(spline_response == nullptr) return;
  response->setSpline(*spline_response);
  delete spline_response;

  QJsonObject state_sigma_spline = state["error"].toObject()["tpspline3"].toObject();
  TPspline3 *spline_sigma = read_tpspline3_json(state_sigma_spline);
  if(spline_sigma == nullptr) return;
  sigma->setSpline(*spline_sigma);
  delete spline_sigma;
}


/***************************************************************************\
*                    Implementation of Axy and related                      *
\***************************************************************************/
AxySettingsWidget::AxySettingsWidget(QWidget *parent)
    : QWidget(parent)
{
  sb_nintx = new QSpinBox(this);
  sb_nintx->setValue(10);
  sb_nintx->setMinimum(2);
  sb_nintx->setMaximum(99999999);
  sb_ninty = new QSpinBox(this);
  sb_ninty->setValue(10);
  sb_ninty->setMinimum(2);
  sb_ninty->setMaximum(99999999);

  QFormLayout *layout = new QFormLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(3);
  layout->addRow("Number of x nodes:", sb_nintx);
  layout->addRow("Number of y nodes:", sb_ninty);
  setLayout(layout);

  connect(sb_nintx, &QSpinBox::editingFinished, this, &AxySettingsWidget::elementChanged);
  connect(sb_ninty, &QSpinBox::editingFinished, this, &AxySettingsWidget::elementChanged);
}

void AxySettingsWidget::saveState(QJsonObject &json) const
{
  json["nintx"] = sb_nintx->value();
  json["ninty"] = sb_ninty->value();
}

void AxySettingsWidget::loadState(const QJsonObject &settings)
{
  sb_nintx->setValue(settings["nintx"].toInt());
  sb_ninty->setValue(settings["ninty"].toInt());
}


AxyInternalsWidget::AxyInternalsWidget(QWidget *parent)
 : QWidget(parent)
{
  transform = new ATransformWidget;

  QRadioButton *rb_response = new QRadioButton("Response spline");
  rb_response->setChecked(true);
  QRadioButton *rb_sigma = new QRadioButton("Sigma spline");

  response = new ATPspline3Widget;
  sigma = new ATPspline3Widget;
  QStackedWidget *sw_spline_choice = new QStackedWidget;
  sw_spline_choice->addWidget(response);
  sw_spline_choice->addWidget(sigma);

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(transform, 0, 0, 1, 2);
  layout->addWidget(rb_response, 1, 0);
  layout->addWidget(rb_sigma, 1, 1);
  layout->addWidget(sw_spline_choice, 2, 0, 1, 2);
  setLayout(layout);

  connect(rb_response, &QRadioButton::clicked, [=]() { sw_spline_choice->setCurrentIndex(0); });
  connect(rb_sigma, &QRadioButton::clicked, [=]() { sw_spline_choice->setCurrentIndex(1); });
}

void AxyInternalsWidget::saveState(QJsonObject &state) const
{
  state["transform"] = transform->getTransform().toJson();

  QJsonObject state_response_spline;
  TPspline3 spline_response = response->getSpline();
  write_tpspline3_json(&spline_response, state_response_spline);
  QJsonObject state_response;
  state_response["tpspline3"] = state_response_spline;
  state["response"] = state_response;

  QJsonObject state_sigma_spline;
  TPspline3 spline_sigma = sigma->getSpline();
  write_tpspline3_json(&spline_sigma, state_sigma_spline);
  QJsonObject state_sigma;
  state_sigma["tpspline3"] = state_sigma_spline;
  state["error"] = state_sigma;
}

void AxyInternalsWidget::loadState(const QJsonObject &state)
{
  ATransform t = ATransform::fromJson(state["transform"].toObject());
  transform->setTransform(t);

  QJsonObject state_reponse_spline = state["response"].toObject()["tpspline3"].toObject();
  TPspline3 *spline_response = read_tpspline3_json(state_reponse_spline);
  if(spline_response == nullptr) return;
  response->setSpline(*spline_response);
  delete spline_response;

  QJsonObject state_sigma_spline = state["error"].toObject()["tpspline3"].toObject();
  TPspline3 *spline_sigma = read_tpspline3_json(state_sigma_spline);
  if(spline_sigma == nullptr) return;
  sigma->setSpline(*spline_sigma);
  delete spline_sigma;
}


/***************************************************************************\
*                  Implementation of ASlicedXY and related                  *
\***************************************************************************/
ASlicedXYSettingsWidget::ASlicedXYSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
  sb_nintx = new QSpinBox(this);
  sb_nintx->setValue(10);
  sb_nintx->setMinimum(2);
  sb_nintx->setMaximum(99999999);
  sb_ninty = new QSpinBox(this);
  sb_ninty->setValue(10);
  sb_ninty->setMinimum(2);
  sb_ninty->setMaximum(99999999);
  sb_nintz = new QSpinBox(this);
  sb_nintz->setValue(10);
  sb_nintz->setMinimum(2);
  sb_nintz->setMaximum(99999999);

  QFormLayout *layout = new QFormLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(3);
  layout->addRow("Number of x nodes:", sb_nintx);
  layout->addRow("Number of y nodes:", sb_ninty);
  layout->addRow("Number of z nodes:", sb_nintz);
  setLayout(layout);

  connect(sb_nintx, &QSpinBox::editingFinished, this, &ASlicedXYSettingsWidget::elementChanged);
  connect(sb_ninty, &QSpinBox::editingFinished, this, &ASlicedXYSettingsWidget::elementChanged);
  connect(sb_nintz, &QSpinBox::editingFinished, this, &ASlicedXYSettingsWidget::elementChanged);
}

void ASlicedXYSettingsWidget::saveState(QJsonObject &json) const
{
  json["nintx"] = sb_nintx->value();
  json["ninty"] = sb_ninty->value();
  json["nintz"] = sb_nintz->value();
}

void ASlicedXYSettingsWidget::loadState(const QJsonObject &settings)
{
  sb_nintx->setValue(settings["nintx"].toInt());
  sb_ninty->setValue(settings["ninty"].toInt());
  sb_nintz->setValue(settings["nintz"].toInt());
}


ASlicedXYInternalsWidget::ASlicedXYInternalsWidget(QWidget *parent)
 : QWidget(parent)
{
  transform = new ATransformWidget;

  QRadioButton *rb_response = new QRadioButton("Response spline");
  rb_response->setChecked(true);
  rb_sigma = new QRadioButton("Sigma spline");

  QDoubleValidator *dv = new QDoubleValidator(this);
  QIntValidator *iv = new QIntValidator(this);
  iv->setBottom(1);

  QGroupBox *gb_zrange = new QGroupBox("Z properties");
  {
    nodesz = new QLineEdit("1");
    nodesz->setValidator(iv);
    minz = new QLineEdit("0");
    minz->setValidator(dv);
    maxz = new QLineEdit("0");
    maxz->setValidator(dv);

    QHBoxLayout *layout_zrange = new QHBoxLayout;
    layout_zrange->addWidget(new QLabel("Nodes:"));
    layout_zrange->addWidget(nodesz);
    layout_zrange->addWidget(new QLabel("Min:"));
    layout_zrange->addWidget(minz);
    layout_zrange->addWidget(new QLabel("Max:"));
    layout_zrange->addWidget(maxz);
    gb_zrange->setLayout(layout_zrange);
  }

  sb_slice = new QSpinBox;
  sb_slice->setMinimum(0);
  sb_slice->setMaximum(0);
  sw_spline_choice = new QStackedWidget;

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(transform, 0, 0, 1, 4);
  layout->addWidget(gb_zrange, 1, 0, 1, 4);
  layout->addWidget(rb_response, 2, 0);
  layout->addWidget(rb_sigma, 2, 1);
  layout->addWidget(new QLabel("Slice:"), 2, 2);
  layout->addWidget(sb_slice, 2, 3);
  layout->addWidget(sw_spline_choice, 3, 0, 1, 4);
  setLayout(layout);

  connect(nodesz, &QLineEdit::editingFinished, this, &ASlicedXYInternalsWidget::onZnodesChanged);
  connect(sb_slice, &QSpinBox::editingFinished, this, &ASlicedXYInternalsWidget::onSetZSlice);
  connect(rb_response, &QRadioButton::clicked, this, &ASlicedXYInternalsWidget::onSetZSlice);
  connect(rb_sigma, &QRadioButton::clicked, this, &ASlicedXYInternalsWidget::onSetZSlice);
}

void ASlicedXYInternalsWidget::saveState(QJsonObject &state) const
{
  state["transform"] = transform->getTransform().toJson();

  state["zmin"] = minz->text().toDouble();
  state["zmax"] = maxz->text().toDouble();

  QJsonArray state_response_spline;
  for(int i = 0; i < response.size(); i++) {
    TPspline3 spline_response = response[i]->getSpline();
    QJsonObject json_spline;
    write_tpspline3_json(&spline_response, json_spline);
    state_response_spline.append(json_spline);
  }
  QJsonObject state_response;
  state_response["tpspline3"] = state_response_spline;
  state["response"] = state_response;

  QJsonArray state_sigma_spline;
  for(int i = 0; i < response.size(); i++) {
    TPspline3 spline_sigma = response[i]->getSpline();
    QJsonObject json_spline;
    write_tpspline3_json(&spline_sigma, json_spline);
    state_sigma_spline.append(json_spline);
  }
  QJsonObject state_sigma;
  state_sigma["tpspline3"] = state_sigma_spline;
  state["error"] = state_sigma;
}

void ASlicedXYInternalsWidget::loadState(const QJsonObject &state)
{
  for(int i = 0; i < response.size(); i++) delete response[i];
  response.clear();
  for(int i = 0; i < sigma.size(); i++) delete sigma[i];
  sigma.clear();

  ATransform t = ATransform::fromJson(state["transform"].toObject());
  transform->setTransform(t);

  minz->setText(QString::number(state["zmin"].toDouble()));
  maxz->setText(QString::number(state["zmax"].toDouble()));

  QJsonArray state_reponse_spline = state["response"].toObject()["tpspline3"].toArray();
  nodesz->setText(QString::number(state_reponse_spline.size()));
  sb_slice->setMaximum(state_reponse_spline.size());

  response.resize(state_reponse_spline.size());
  for(int i = 0; i < state_reponse_spline.size(); i++) {
    QJsonObject json_spline = state_reponse_spline[i].toObject();
    TPspline3 *spline_response = read_tpspline3_json(json_spline);
    if(spline_response == nullptr) return;
    response[i] = new ATPspline3Widget;
    response[i]->setSpline(*spline_response);
    sw_spline_choice->addWidget(response[i]);
    delete spline_response;
  }

  QJsonArray state_sigma_spline = state["error"].toObject()["tpspline3"].toArray();
  sigma.resize(state_sigma_spline.size());
  for(int i = 0; i < state_sigma_spline.size(); i++) {
    QJsonObject json_spline = state_sigma_spline[i].toObject();
    TPspline3 *spline_sigma = read_tpspline3_json(json_spline);
    if(spline_sigma == nullptr) return;
    sigma[i] = new ATPspline3Widget;
    sigma[i]->setSpline(*spline_sigma);
    sw_spline_choice->addWidget(sigma[i]);
    delete spline_sigma;
  }

  rb_sigma->setEnabled(sigma.size() == response.size());
}

void ASlicedXYInternalsWidget::onZnodesChanged()
{
  int old_nodes = response.size()-1;
  int new_nodes = sb_slice->value();
  sb_slice->setMaximum(new_nodes);
  for(int i = old_nodes; i >= new_nodes; i--) {
    delete response[i];
    delete sigma[i];
  }
  response.resize(new_nodes);
  sigma.resize(new_nodes);
  for(int i = old_nodes+1; i < new_nodes; i++) {
    response[i] = new ATPspline3Widget;
    response[i]->setSpline(response[old_nodes]->getSpline());
    sw_spline_choice->addWidget(response[i]);
    sigma[i] = new ATPspline3Widget;
    sigma[i]->setSpline(sigma[old_nodes]->getSpline());
    sw_spline_choice->addWidget(sigma[i]);
  }
}

void ASlicedXYInternalsWidget::onSetZSlice()
{
  int i = sb_slice->value();
  if(sigma.size() == response.size()) {
    i *= 2;
    if(rb_sigma->isChecked())
      i++;
  }
  sw_spline_choice->setCurrentIndex(i);
}


/***************************************************************************\
*                   Implementation of Script and related                    *
\***************************************************************************/
ScriptSettingsWidget::ScriptSettingsWidget(const QString &example_code, QWidget *parent)
  : QWidget(parent)
{
  code = new ATextEdit(this);
  highlighter = new AHighlighterLrfScript(code->document());
  code->setLineWrapMode(QPlainTextEdit::NoWrap);
  code->setFixedHeight(130);
  code->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  code->appendPlainText(example_code);
  //code->clearFocus();

  QPushButton *help_button = new QPushButton("Help");
  help_button->setMaximumWidth(60);
  connect(help_button, &QPushButton::clicked, [this] () {
    message("\n"
            "The script code inserted will be read as a javascript-like language,\n"
            "specifically the one described in https://doc.qt.io/qt-5/qtscript-index.html.\n"
            "There are however some additional rules:\n\n"
            "1. The script requires an eval function, which is effectively the LRF,\n"
            "    to be defined. It can take an 'r' parameter (containing the event\n"
            "    coordinates relative to the lrf), an 'R' parameter (containing the\n"
            "    global event coordinates), or both, and returns the light response\n"
            "    of the sensor. For Polar these parameters are a 2-element array,\n"
            "    and for Cartesian they are a 3-element array.\n\n"
            "2. The function will be fit to some set of data provided by the user. In\n"
            "    order to do that, the script defines free parameters which will be fit.\n"
            "    Every variable that does not start with a '_', and is a number, or an\n"
            "    object which contains an 'init' and/or the 'min' and 'max' members,\n"
            "    is considered to be a free parameter.\n\n"
            "3. The fitting procedure is done using ROOT's TProfile(2D) and TF1/TF2\n"
            "    classes, by filling a TProfile with the events' positions and signals and\n"
            "    then using a proxy TF1 (which just calls the script's eval function) to\n"
            "    fit the TProfile.\n\n"
            "4. All variables must preceded with the 'var' keyword (be them free\n"
            "    parameters or not). Not doing so is undefined behaviour!\n", this);
  });

  QPushButton *expand_button = new QPushButton("+");
  expand_button->setMaximumWidth(60);
  QPushButton *contract_button = new QPushButton("-");
  contract_button->setMaximumWidth(60);

  auto *layout_toolbar = new QHBoxLayout;
  layout_toolbar->addWidget(help_button);
  layout_toolbar->addWidget(expand_button);
  layout_toolbar->addWidget(contract_button);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(layout_toolbar);
  layout->addWidget(code);
  this->setLayout(layout);

  connect(code, &ATextEdit::editingFinished, this, &ScriptSettingsWidget::elementChanged);
  connect(expand_button, &QPushButton::clicked, [=] () {
    code->setFixedHeight(code->height()+20);
    emit elementChanged();
  });
  connect(contract_button, &QPushButton::clicked, [=] () {
    int height = code->height() - 20;
    if(height < 0) height = 0;
    code->setFixedHeight(height);
    emit elementChanged();
  });
}

void ScriptSettingsWidget::saveState(QJsonObject &json) const
{
  json["script"] = code->toPlainText();
  json["text height"] = code->height();
}

void ScriptSettingsWidget::loadState(const QJsonObject &settings)
{
  code->clear();
  code->appendPlainText(settings["script"].toString());
  code->setFixedHeight(settings["text height"].toInt());
}


ScriptInternalsWidget::ScriptInternalsWidget(QWidget *parent)
  : QWidget(parent)
{
  dv = new QDoubleValidator(this);
  code = new ATextEdit;
  highlighter = new AHighlighterLrfScript(code->document());
  code->setLineWrapMode(QPlainTextEdit::NoWrap);
  code->setMinimumHeight(80);
  code->setMaximumHeight(150);
  code->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  code->appendPlainText("");
  //code->clearFocus();

  QPushButton *expand_button = new QPushButton("+");
  expand_button->setMaximumWidth(60);
  QPushButton *contract_button = new QPushButton("-");
  contract_button->setMaximumWidth(60);

  transform = new ATransformWidget;

  parameters = new QTableWidget;
  parameters->setColumnCount(2);
  parameters->setHorizontalHeaderLabels(QStringList()<<"Name"<<"Value");
  parameters_sigma = new QTableWidget;
  parameters_sigma->setColumnCount(2);
  parameters_sigma->setHorizontalHeaderLabels(QStringList()<<"Name"<<"Value");

  auto *layout_toolbar = new QHBoxLayout;
  layout_toolbar->addWidget(expand_button);
  layout_toolbar->addWidget(contract_button);

  auto *layout = new QGridLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(layout_toolbar, 0, 0, 1, 2);
  layout->addWidget(code, 1, 0, 1, 2);
  layout->addWidget(transform, 2, 0, 1, 2);
  layout->addWidget(new QLabel("Parameters:"), 3, 0);
  layout->addWidget(parameters, 4, 0, 1, 1);
  layout->addWidget(new QLabel("Sigma parameters:"), 3, 1);
  layout->addWidget(parameters_sigma, 4, 1, 1, 1);
  this->setLayout(layout);

  connect(expand_button, &QPushButton::clicked, [=] () {
    code->setFixedHeight(code->height()+20);
    emit elementChanged();
  });
  connect(contract_button, &QPushButton::clicked, [=] () {
    int height = code->height() - 20;
    if(height < 0) height = 0;
    code->setFixedHeight(height);
    emit elementChanged();
  });
}

void ScriptInternalsWidget::saveState(QJsonObject &state) const
{
  state = config;

  state["transform"] = transform->getTransform().toJson();
  state["script_code"] = code->document()->toPlainText();

  QJsonObject state_script_var = state["script_var"].toObject();
  QJsonObject script_var = state_script_var["value"].toObject();
  for(int i = 0; i < parameters->rowCount(); i++) {
    QString name = static_cast<QLineEdit*>(parameters->cellWidget(i, 0))->text();
    double value = static_cast<QLineEdit*>(parameters->cellWidget(i, 1))->text().toDouble();
    script_var[name] = value;
  }
  state_script_var["value"] = script_var;
  state["script_var"] = state_script_var;

  QJsonObject state_script_var_sigma = state["script_var_sigma"].toObject();
  QJsonObject script_var_sigma = state_script_var_sigma["value"].toObject();
  for(int i = 0; i < parameters_sigma->rowCount(); i++) {
    QString name = static_cast<QLineEdit*>(parameters_sigma->cellWidget(i, 0))->text();
    double value = static_cast<QLineEdit*>(parameters_sigma->cellWidget(i, 1))->text().toDouble();
    script_var_sigma[name] = value;
  }
  state_script_var_sigma["value"] = script_var;
  state["script_var_sigma"] = state_script_var_sigma;
}

void ScriptInternalsWidget::loadState(const QJsonObject &state)
{
  config = state;

  ATransform t = ATransform::fromJson(state["transform"].toObject());
  transform->setTransform(t);

  QJsonObject script_var = state["script_var"].toObject()["value"].toObject();
  for(const auto &name : script_var.keys()) {
    if(name.startsWith("_") || !script_var[name].isDouble())
       continue;
    int row = parameters->rowCount();
    parameters->setRowCount(row+1);

    QLineEdit *le_name = new QLineEdit(name);
    le_name->setReadOnly(true);
    parameters->setCellWidget(row, 0, le_name);

    QLineEdit *le_number = new QLineEdit(QString::number(script_var[name].toDouble()));
    le_number->setValidator(dv);
    parameters->setCellWidget(row, 1, le_number);
  }

  QJsonObject script_var_sigma = state["script_var_sigma"].toObject()["value"].toObject();
  for(const auto &name : script_var_sigma.keys()) {
    if(name.startsWith("_") || !script_var_sigma[name].isDouble())
       continue;
    int row = parameters_sigma->rowCount();
    parameters_sigma->setRowCount(row+1);

    QLineEdit *le_name = new QLineEdit(name);
    le_name->setReadOnly(true);
    parameters_sigma->setCellWidget(row, 0, le_name);

    QLineEdit *le_number = new QLineEdit(QString::number(script_var_sigma[name].toDouble()));
    le_number->setValidator(dv);
    parameters_sigma->setCellWidget(row, 1, le_number);
  }

  code->appendPlainText(state["script_code"].toString());
}

} } //namespace LRF::CoreLrfs
