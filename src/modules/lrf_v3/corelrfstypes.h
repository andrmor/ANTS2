#ifndef CORELRFSTYPES_H
#define CORELRFSTYPES_H

#include "alrftype.h"

class QScriptEngine;
class QScriptValue;
class QScriptString;
class TFitResultPtr;

namespace LRF {

class ALrfTypeManager;

namespace CoreLrfs {

class AScriptParamInfo;

void Setup(ALrfTypeManager &manager);

class AxialType : public ALrfType {
public:
  AxialType() : ALrfType("Axial") {}
  bool isCudaCapable() const override { return true; }
  void getCudaParameters(std::shared_ptr<const ALrf> lrf) override;

  QJsonObject lrfToJson(const ALrf *lrf) const override;

  ALrf *lrfFromJson(const QJsonObject &json) const override;

  ALrf *lrfFromData(const QJsonObject &settings, bool fit_error,
                    const std::vector<APoint> &event_pos,
                    const std::vector<double> &event_signal) const override;

#ifdef GUI
  QWidget *newSettingsWidget(QWidget *parent) const override;
  QWidget *newInternalsWidget(QWidget *parent) const override;
#endif
};


class AAxial3DType : public ALrfType {
public:
  AAxial3DType() : ALrfType("Axial3D") {}
  bool isCudaCapable() const override { return true; }

  QJsonObject lrfToJson(const ALrf *lrf) const override;

  ALrf *lrfFromJson(const QJsonObject &json) const override;

  ALrf *lrfFromData(const QJsonObject &settings, bool fit_error,
                    const std::vector<APoint> &event_pos,
                    const std::vector<double> &event_signal) const override;

#ifdef GUI
  QWidget *newSettingsWidget(QWidget *parent) const override;
  QWidget *newInternalsWidget(QWidget *parent) const override;
#endif
};


class AxyType : public ALrfType {
public:
  AxyType() : ALrfType("XY") {}
  bool isCudaCapable() const override { return true; }

  QJsonObject lrfToJson(const ALrf *lrf) const override;

  ALrf *lrfFromJson(const QJsonObject &json) const override;

  ALrf *lrfFromData(const QJsonObject &settings, bool fit_error,
                    const std::vector<APoint> &event_pos,
                    const std::vector<double> &event_signal) const override;

#ifdef GUI
  QWidget *newSettingsWidget(QWidget *parent) const override;
  QWidget *newInternalsWidget(QWidget *parent) const override;
#endif
};


class ASlicedXYType : public ALrfType {
public:
  ASlicedXYType() : ALrfType("SlicedXY") {}
  bool isCudaCapable() const override { return true; }

  QJsonObject lrfToJson(const ALrf *lrf) const override;

  ALrf *lrfFromJson(const QJsonObject &json) const override;

  ALrf *lrfFromData(const QJsonObject &settings, bool fit_error,
                    const std::vector<APoint> &event_pos,
                    const std::vector<double> &event_signal) const override;

#ifdef GUI
  QWidget *newSettingsWidget(QWidget *parent) const override;
  QWidget *newInternalsWidget(QWidget *parent) const override;
#endif
};


class ScriptType : public ALrfType {
protected:
  struct Private;
  std::shared_ptr<Private> p;

  ScriptType(const std::string &name);

  QScriptString generateLrfName() const;
  QScriptString evaluateScript(QString script_code) const;
  QScriptString copyScriptToCurrentThread(std::shared_ptr<const ALrf> lrf, std::shared_ptr<QScriptValue> &lrf_collection) const;
  bool loadScriptVarFromJson(const QJsonObject &json, QScriptValue &var, QScriptValue &var_sigma) const;
  bool rootFitToScriptVar(const std::vector<AScriptParamInfo> &param_info,
                          const TFitResultPtr &fit, QScriptValue &script_var) const;
  bool rootFitToScriptVar(const std::vector<AScriptParamInfo> &param_info,
                          const std::vector<double> &fitted_params, QScriptValue &script_var) const;

public:
  QJsonObject lrfToJson(const ALrf *lrf) const override;

#ifdef GUI
  QWidget *newInternalsWidget(QWidget *parent) const override;
#endif
};

class ScriptPolarType : public ScriptType {
  bool with_z;
public:
  ScriptPolarType(bool with_z) : ScriptType(std::string("Script Polar")+(with_z?"+Z":"")), with_z(with_z) { }
  std::string nameUi() const override { return with_z ? "Polar+Z" : "Polar"; }
  QJsonObject lrfToJson(const ALrf *lrf) const override;

  ALrf *lrfFromData(const QJsonObject &settings, bool fit_error,
                    const std::vector<APoint> &event_pos,
                    const std::vector<double> &event_signal) const override;

  ALrf *lrfFromJson(const QJsonObject &json) const override;

#ifdef GUI
  QWidget *newSettingsWidget(QWidget *parent) const override;
#endif

  std::shared_ptr<const ALrf> copyToCurrentThread(std::shared_ptr<const ALrf> lrf) const override;
};

class ScriptCartesianType : public ScriptType {
  bool with_z;
public:
  ScriptCartesianType(bool with_z) : ScriptType(std::string("Script Cartesian")+(with_z?"+Z":"")), with_z(with_z) { }
  std::string nameUi() const override { return with_z ? "Cartesian+Z" : "Cartesian"; }
  QJsonObject lrfToJson(const ALrf *lrf) const override;

  ALrf *lrfFromData(const QJsonObject &settings, bool fit_error,
                    const std::vector<APoint> &event_pos,
                    const std::vector<double> &event_signal) const override;

  ALrf *lrfFromJson(const QJsonObject &json) const override;

#ifdef GUI
  QWidget *newSettingsWidget(QWidget *parent) const override;
#endif

  std::shared_ptr<const ALrf> copyToCurrentThread(std::shared_ptr<const ALrf> lrf) const override;
};

} } //namespace LRF::CorePlugin


#endif // CORELRFSTYPES_H
