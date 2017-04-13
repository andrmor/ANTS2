#include "corelrfstypes.h"

#include <map>
#include <thread>

#include <QDebug>
#include <QJsonObject>
#include <QScriptEngine>
#include <QScriptValueIterator>
#include <QApplication>
#include <QDateTime>

#include <Rtypes.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TProfile3D.h>
#include <Fit/Fitter.h>
#include <Fit/BinData.h>
#include <Math/WrappedMultiTF1.h>
#include <TFitResult.h>
#include <TH1D.h>
#include <TF1.h>
#include <TF2.h>
#include <TF3.h>
#include <TPad.h>
#include <TCanvas.h>

#include "corelrfs.h"
#ifdef GUI
#include "corelrfswidgets.h"
#endif
#include "ascriptvaluecopier.h"
#include "ascriptvalueconverter.h"
#include "spline.h"
#include "bspline3.h"

namespace LRF { namespace CoreLrfs {

/***************************************************************************\
*                   Implementation of Axial and related                     *
\***************************************************************************/
void AxialType::getCudaParameters(std::shared_ptr<const ALrf> lrf)
{
  const AAxial *axial = dynamic_cast<const AAxial *>(lrf.get());
  if(!axial) return;

  QJsonObject json;
  json["transform"] = axial->getTransform().toJson();
  json["nodes"] = axial->getNint();
  auto response_poly = axial->getBsr().GetPoly();
  auto error_poly = axial->getBse().GetPoly();

  auto *compr = dynamic_cast<const AxialCompressed *>(lrf.get());
  if(compr) {
    QJsonObject compression;
    compr->getCompressor().toJson(compression);
    json["compression"] = compression;
  }
}

QJsonObject AxialType::lrfToJson(const ALrf *lrf) const
{
  const AAxial *axial = dynamic_cast<const AAxial *>(lrf);
  if(!axial) return QJsonObject();

  QJsonObject json;
  json["transform"] = axial->getTransform().toJson();
  json["flattop"] = axial->isFlattop();
  QJsonObject response, spline;
  write_bspline3_json(&axial->getBsr(), spline);
  response["bspline3"] = spline;
  json["response"] = response;
  if(axial->getBse().GetXmax() != 0.) {
    QJsonObject error, spline;
    write_bspline3_json(&axial->getBse(), spline);
    error["bspline3"] = spline;
    json["error"] = error;
  }

  auto *compr = dynamic_cast<const AxialCompressed *>(lrf);
  if(compr) {
      compr->getCompressor().toJson(json);
  }
  json["type"] = compr ? "ComprAxial" : "Axial";

  return json;
}

ALrf *AxialType::lrfFromJson(const QJsonObject &json) const
{
  ATransform t = ATransform::fromJson(json["transform"].toObject());

  //Try to read response, fail if not available
  QJsonObject json_bsr = json["response"].toObject()["bspline3"].toObject();
  Bspline3 bsr = FromJson::mkBspline3(json_bsr);
  if(bsr.isInvalid())
    return nullptr;

  //Try to read error. Don't fail!
  QJsonObject json_bse = json["error"].toObject()["bspline3"].toObject();
  Bspline3 bse = FromJson::mkBspline3(json_bse);

  //These used to not be stored, and were default true. Only to show settings.
  bool flattop = json["flattop"].toBool(true);

  //If there is non-default compression use compressed Lrf, else use regular
  ALrf *lrf;
  AVladimirCompression compr(json);
  if(compr != AVladimirCompression()) {
    AAxial axial = AAxial(id(), bsr, bse, flattop, t);
    lrf = new AxialCompressed(axial, compr);
  }
  else lrf = new AAxial(id(), bsr, bse, flattop, t);

  return lrf;
}

ALrf *AxialType::lrfFromData(const QJsonObject &settings, bool fit_error,
                             const std::vector<APoint> &event_pos,
                             const std::vector<double> &event_signal) const
{
  std::vector <double> vr, va;
  try {
    vr.resize(event_pos.size());
    if(fit_error)
      va.resize(event_pos.size());
  } catch(...) {
    qDebug()<< "AxialFit: not enough memory!";
    return nullptr;
  }

  AVladimirCompression compress(settings["compression"].toObject());
  const bool hasCompression = compress.isCompressing();

  double rmax = 0;
  for (size_t i = 0; i < event_pos.size(); i++) {
    double r = hypot(event_pos[i].x(), event_pos[i].y());
    if(hasCompression)
      r = compress(r);
    vr[i] = r;
    if(r > rmax) rmax = r;
  }

  Bspline3 bsr(0., rmax, settings["nint"].toInt());
  Bspline3 bse(0., rmax, settings["nint"].toInt());

  bool flattop = settings["flat top"].toBool();

  fit_bspline3_grid(&bsr, event_signal.size(), vr.data(), event_signal.data(), flattop);

  if(fit_error) {
    for (size_t i = 0; i < vr.size(); i++)
      va[i] = event_signal[i] - bsr.Eval(vr[i]);

    fit_bspline3_grid(&bse, va.size(), vr.data(), va.data(), flattop);
  }

  if(hasCompression)
    return new AxialCompressed(AAxial(id(), bsr, bse, flattop), compress);
  else
    return new AAxial(id(), bsr, bse, flattop);
}

#ifdef GUI
QWidget *AxialType::newSettingsWidget(QWidget *parent) const
{ return new AxialSettingsWidget(parent); }

QWidget *AxialType::newInternalsWidget(QWidget *parent) const
{
  return new AxialInternalsWidget(parent);
}
#endif


/***************************************************************************\
*                 Implementation of Axial3D and related                     *
\***************************************************************************/
QJsonObject AAxial3DType::lrfToJson(const ALrf *lrf) const
{
  const AAxial3D *a3d = dynamic_cast<const AAxial3D*>(lrf);
  if(!a3d) return QJsonObject();

  QJsonObject json;
  json["transform"] = a3d->getTransform().toJson();

  QJsonObject compression;
  a3d->getCompressor().toJson(compression);
  json["compression"] = compression;

  QJsonObject response, spline;
  write_tpspline3_json(&a3d->getBsr(), spline);
  response["tpspline3"] = spline;
  json["response"] = response;

  if(!a3d->getBse().isInvalid())
  {
    QJsonObject error, spline;
    write_tpspline3_json(&a3d->getBse(), spline);
    error["tpspline3"] = spline;
    json["error"] = error;
  }

  return json;
}

ALrf *AAxial3DType::lrfFromJson(const QJsonObject &json) const
{
  //Try to read response, fail if not available
  QJsonObject json_bsr = json["response"].toObject()["tpspline3"].toObject();
  TPspline3 bsr = FromJson::mkTPspline3(json_bsr);
  if(bsr.isInvalid())
    return nullptr;

  //Try to read error. Don't fail!
  QJsonObject json_bse = json["error"].toObject()["tpspline3"].toObject();
  TPspline3 bse = FromJson::mkTPspline3(json_bse);

  ATransform t = ATransform::fromJson(json["transform"].toObject());
  AVladimirCompression compr(json["compression"].toObject());

  return new AAxial3D(id(), bsr, bse, compr, t);
}

ALrf *AAxial3DType::lrfFromData(const QJsonObject &settings, bool fit_error,
                                    const std::vector<APoint> &event_pos,
                                    const std::vector<double> &event_signal) const
{
  const size_t size = event_pos.size();
  if (size != event_signal.size()) {
    qWarning() << "Data mismatch in AAxial3DType fit!";
    return nullptr;
  }

  std::vector <double> vr, vz, va;
  try {
    vr.resize(event_pos.size());
    vz.resize(event_pos.size());
    if(fit_error)
      va.resize(event_pos.size());
  } catch(...) {
    qDebug()<< "AAxial3DType: not enough memory!";
    return nullptr;
  }

  AVladimirCompression compress(settings["compression"].toObject());
  const bool hasCompression = compress.isCompressing();

  double rmax = 0;
  double zmin = 1e300, zmax = -1e300;
  for (size_t i = 0; i < size; i++)
  {
    double r = event_pos[i].normxy();
    if(hasCompression)
      r = compress(r);
    vr[i] = r;
    vz[i] = event_pos[i].z();
    if(r > rmax) rmax = r;
    if (vz[i] < zmin) zmin = vz[i];
    if (vz[i] > zmax) zmax = vz[i];
  }

  int nintr = settings["nintr"].toInt();
  int nintz = settings["nintz"].toInt();
  TPspline3 bsr(0, rmax, nintr, zmin, zmax, nintz);
  TPspline3 bse(0, rmax, nintr, zmin, zmax, nintz);

  fit_tpspline3_grid(&bsr, event_signal.size(), vr.data(), vz.data(), event_signal.data(), true);

  if (fit_error) {
    for (size_t i = 0; i < event_pos.size(); i++)
      va[i] = event_signal[i] - bsr.Eval(vr[i], vz[i]);
    fit_tpspline3_grid(&bse, event_signal.size(), vr.data(), vz.data(), va.data(), true);
  }

  return new AAxial3D(id(), bsr, bse, compress);
}

QWidget *AAxial3DType::newSettingsWidget(QWidget *parent) const
{
  return new Axial3DSettingsWidget(parent);
}

QWidget *AAxial3DType::newInternalsWidget(QWidget *parent) const
{
  return new Axial3DInternalsWidget(parent);
}


/***************************************************************************\
*                   Implementation of Axy and related                       *
\***************************************************************************/
QJsonObject AxyType::lrfToJson(const ALrf *lrf) const
{
    const Axy *axy = dynamic_cast<const Axy *>(lrf);
    if(!axy) return QJsonObject();

    QJsonObject json;
    json["type"] = "XY";
    json["transform"] = axy->getTransform().toJson();

    QJsonObject response, spline;
    write_tpspline3_json(&axy->getBsr(), spline);
    response["tpspline3"] = spline;
    json["response"] = response;

    if(!axy->getBse().isInvalid())
    {
      QJsonObject error, spline;
      write_tpspline3_json(&axy->getBse(), spline);
      error["tpspline3"] = spline;
      json["error"] = error;
    }

    return json;
}

ALrf *AxyType::lrfFromJson(const QJsonObject &json) const
{
    //Try to read response, fail if not available
    QJsonObject json_bsr = json["response"].toObject()["tpspline3"].toObject();
    TPspline3 bsr = FromJson::mkTPspline3(json_bsr);
    if(bsr.isInvalid())
      return nullptr;

    //Try to read error. Don't fail!
    QJsonObject json_bse = json["error"].toObject()["tpspline3"].toObject();
    TPspline3 bse = FromJson::mkTPspline3(json_bse);

    ATransform t = ATransform::fromJson(json["transform"].toObject());

    return new Axy(id(), bsr, bse, t);
}

ALrf *AxyType::lrfFromData(const QJsonObject &settings, bool fit_error,
                          const std::vector<APoint> &event_pos,
                          const std::vector<double> &event_signal) const
{
    const size_t size = event_pos.size();
    if (size != event_signal.size()) {
        qWarning() << "Data mismatch in Axy fit!";
        return nullptr;
    }

    std::vector <double> va;
    if(fit_error) {
      try {
        va.resize(size);
      } catch(...) {
        qWarning()<< "Axy fit: not enough memory!";
        return nullptr;
      }
    }

    double xmin = 1e10, xmax = -1e10;
    double ymin = 1e10, ymax = -1e10;
    for (size_t i = 0; i < size; i++)
    {
      if (event_pos[i].x() < xmin) xmin = event_pos[i].x();
      if (event_pos[i].x() > xmax) xmax = event_pos[i].x();
      if (event_pos[i].y() < ymin) ymin = event_pos[i].y();
      if (event_pos[i].y() > ymax) ymax = event_pos[i].y();
    }

    int nintx = settings["nintx"].toInt();
    int ninty = settings["ninty"].toInt();
    TPspline3 bsr(xmin, xmax, nintx, ymin, ymax, ninty);
    TPspline3 bse(xmin, xmax, nintx, ymin, ymax, ninty);

    fit_tpspline3_grid(&bsr, event_signal.size(), event_pos.data(), event_signal.data());

    if (fit_error)
    {
      for (size_t i = 0; i < event_pos.size(); i++)
        va[i] = event_signal[i] - bsr.Eval(event_pos[i].x(), event_pos[i].y());
      fit_tpspline3_grid(&bse, event_signal.size(), event_pos.data(), va.data());
    }

    return new Axy(id(), bsr, bse);
}

#ifdef GUI
QWidget *AxyType::newSettingsWidget(QWidget *parent) const
{
  return new AxySettingsWidget(parent);
}

QWidget *AxyType::newInternalsWidget(QWidget *parent) const
{
  return new AxyInternalsWidget(parent);
}
#endif


/***************************************************************************\
*                Implementation of ASlicedXY and related                    *
\***************************************************************************/
QJsonObject ASlicedXYType::lrfToJson(const ALrf *lrf) const
{
  const ASlicedXY *sliced = dynamic_cast<const ASlicedXY*>(lrf);
  if(!sliced) return QJsonObject();

  QJsonObject json;
  json["transform"] = sliced->getTransform().toJson();
  json["zmin"] = sliced->getZMin();
  json["zmax"] = sliced->getZMax();

  QJsonArray splines;
  for(const TPspline3 &spline : sliced->getBsr()) {
    QJsonObject json_spline;
    write_tpspline3_json(&spline, json_spline);
    splines.append(json_spline);
  }
  QJsonObject response;
  response["tpspline3"] = splines;
  json["response"] = response;

  QJsonArray error_splines;
  for(const TPspline3 &spline : sliced->getBse()) {
    if(spline.isInvalid()) continue;
    QJsonObject json_spline;
    write_tpspline3_json(&spline, json_spline);
    error_splines.append(json_spline);
  }
  QJsonObject error;
  error["tpspline3"] = error_splines;
  json["error"] = error;

  return json;
}

ALrf *ASlicedXYType::lrfFromJson(const QJsonObject &json) const
{
  std::vector<TPspline3> bsr;
  //Try to read response, fail if not available
  for(const QJsonValue &json_bsr : json["response"].toObject()["tpspline3"].toArray()) {
    QJsonObject j = json_bsr.toObject();
    TPspline3 spline = FromJson::mkTPspline3(j);
    if(spline.isInvalid())
      return nullptr;
    bsr.push_back(spline);
  }

  std::vector<TPspline3> bse;
  //Try to read error. Don't fail!
  for(const QJsonValue &json_bse : json["error"].toObject()["tpspline3"].toArray()) {
    QJsonObject j = json_bse.toObject();
    bse.push_back(FromJson::mkTPspline3(j));
  }

  ATransform t = ATransform::fromJson(json["transform"].toObject());
  double zmin = json["zmin"].toDouble();
  double zmax = json["zmax"].toDouble();

  return new ASlicedXY(id(), zmin, zmax, bsr, bse, t);
}

ALrf *ASlicedXYType::lrfFromData(const QJsonObject &settings, bool fit_error,
                                 const std::vector<APoint> &event_pos,
                                 const std::vector<double> &event_signal) const
{
  const size_t size = event_pos.size();
  if (size != event_signal.size()) {
      qWarning() << "Data mismatch in ASlicedXYType fit!";
      return nullptr;
  }

  int nintz = settings["nintz"].toInt();
  std::vector<std::vector<double>> vvx(nintz);
  std::vector<std::vector<double>> vvy(nintz);
  std::vector<std::vector<double>> vva(nintz);

  //Check if we have enough memory
  try {
    std::vector <double> vx(size); (void)vx;
    std::vector <double> vy(size); (void)vy;
    std::vector <double> va(size); (void)va;
  } catch(...) {
    qWarning()<< "ASlicedXYType fit: not enough memory!";
    return nullptr;
  }

  double xmin = 1e300, xmax = -1e300;
  double ymin = 1e300, ymax = -1e300;
  double zmin = 1e300, zmax = -1e300;
  for (size_t i = 0; i < size; i++)
  {
    if (event_pos[i].x() < xmin) xmin = event_pos[i].x();
    if (event_pos[i].x() > xmax) xmax = event_pos[i].x();
    if (event_pos[i].y() < ymin) ymin = event_pos[i].y();
    if (event_pos[i].y() > ymax) ymax = event_pos[i].y();
    if (event_pos[i].z() < zmin) zmin = event_pos[i].z();
    if (event_pos[i].z() > zmax) zmax = event_pos[i].z();
  }

  double dz = (zmax - zmin) / nintz;
  for(size_t i = 0; i < size; i++) {
      int iz = (int)((event_pos[i].z() - zmin) / dz);
      if(iz >= nintz) iz = nintz-1; //For z=zmax cases
      vvx[iz].push_back(event_pos[i].x());
      vvy[iz].push_back(event_pos[i].y());
      vva[iz].push_back(event_signal[i]);
  }

  int nintx = settings["nintx"].toInt();
  int ninty = settings["ninty"].toInt();
  std::vector<TPspline3> bsrs, bses;
  for(int iz = 0; iz < nintz; iz++) {
    TPspline3 bsr(xmin, xmax, nintx, ymin, ymax, ninty);
    TPspline3 bse(xmin, xmax, nintx, ymin, ymax, ninty);

    fit_tpspline3_grid(&bsr, vva[iz].size(), vvx[iz].data(), vvy[iz].data(), vva[iz].data());

    if(fit_error) {
      for (size_t i = 0; i < vva[iz].size(); i++)
        vva[iz][i] -= bsr.Eval(vvx[iz][i], vvy[iz][i]);
      fit_tpspline3_grid(&bse, vva[iz].size(), vvx[iz].data(), vvy[iz].data(), vva[iz].data());
    }

    bsrs.push_back(bsr);
    bses.push_back(bse);
  }

  return new ASlicedXY(id(), zmin, zmax, bsrs, bses);
}

QWidget *ASlicedXYType::newSettingsWidget(QWidget *parent) const
{
  return new ASlicedXYSettingsWidget(parent);
}

QWidget *ASlicedXYType::newInternalsWidget(QWidget *parent) const
{
  return new ASlicedXYInternalsWidget(parent);
}


/***************************************************************************\
*                   Implementation of Script and related                    *
\***************************************************************************/

struct ScriptType::Private {
  QScriptEngine engine;
  std::shared_ptr<QScriptValue> lrf_collection;
  std::map<std::thread::id, std::weak_ptr<QScriptValue>> thread_lrf_collections;
};

ScriptType::ScriptType(const std::string &name) : ALrfType(name), p(new Private)
{
  QScriptValue lrfs = p->engine.newObject();
  p->engine.globalObject().setProperty("lrfs", lrfs);
  p->lrf_collection = std::shared_ptr<QScriptValue>(new QScriptValue(lrfs));
}

QScriptString ScriptType::generateLrfName() const
{
  //Name of var is: 0xrandom_0xtimestamp (doesn't really matter as long as it's unique)
  QString timestamp = QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch(), 16);
  return p->engine.toStringHandle(QString::number(qrand(), 16)+'_'+timestamp);
}

QScriptString ScriptType::evaluateScript(QString script_code) const
{
  QScriptEngine *engine = &p->engine;
  //Push new context so script doesn't interfere with anything outside
  QScriptContext *this_context = engine->pushContext();
  //Run script. Script var is activation object of new context
  engine->evaluate(script_code);
  //Check for errors
  if(engine->hasUncaughtException()) {
    qDebug()<<"Script threw an exception in line"<<engine->uncaughtExceptionLineNumber();
    return QScriptString();
  }

  QScriptValue script_var = this_context->activationObject();
  //Check if there is an eval function. Fail if not
  if(script_var.property("eval").isFunction()) {
    QScriptString name = generateLrfName();
    //Do lrfs[name] = script_var and pop new context
    p->lrf_collection->setProperty(name, script_var);
    engine->popContext();
    return name;
  } else {
    engine->popContext();
    return QScriptString();
  }
}

QScriptString ScriptType::copyScriptToCurrentThread(std::shared_ptr<const ALrf> lrf, std::shared_ptr<QScriptValue> &lrf_collection) const
{
  const AScript *slrf = static_cast<const AScript*>(lrf.get());
  auto it_thr_lrfs = p->thread_lrf_collections.find(std::this_thread::get_id());

  //Thread already has lrf collection (consequently, a script engine)?
  if(it_thr_lrfs == p->thread_lrf_collections.end()) {
    //It doesn't. Create script engine and lrf collection
    QScriptEngine *thr_engine = new QScriptEngine;
    QScriptValue new_obj = thr_engine->newObject();
    thr_engine->globalObject().setProperty("lrfs", new_obj);
    //Reserve space in map for this thread (to get a valid iterator to use in deleter)
    auto thr_lrf_val = std::make_pair(std::this_thread::get_id(), std::weak_ptr<QScriptValue>());
    it_thr_lrfs = p->thread_lrf_collections.insert(thr_lrf_val).first;
    //Make a shared_ptr to lrf_collection with a custom deleter. It HAS to be
    //returned somehow (here by function argument), it will be immediately deleted
    lrf_collection = std::shared_ptr<QScriptValue>(new QScriptValue(new_obj), [=](QScriptValue *lrfs) {
      QScriptEngine *engine = lrfs->engine();
      delete lrfs;
      delete engine;
      p->thread_lrf_collections.erase(it_thr_lrfs);
    });
    //Store weak_ptr so that we can get it back for other lrfs while not counting for deletion
    it_thr_lrfs->second = std::weak_ptr<QScriptValue>(lrf_collection);
  } else {
    lrf_collection = it_thr_lrfs->second.lock();
    //If object already exists in this thread, no point in copying it!
    if(lrf_collection->property(slrf->getName()).isObject())
      return QScriptString();
  }

  //With lrf collection creation handled, we just copy the script vars to it.
  return slrf->deepCopyScriptVar(lrf_collection.get());
}

bool ScriptType::loadScriptVarFromJson(const QJsonObject &json, QScriptValue &var, QScriptValue &var_sigma) const
{
  QScriptContext *ctx = p->engine.pushContext();
  var = AScriptValueConverter().fromJson(json["script_var"], &p->engine);
  ctx->setActivationObject(var);
  p->engine.popContext();
  if(!var.isObject() || !var.property("eval").isFunction())
    return false;

  ctx = p->engine.pushContext();
  var_sigma = AScriptValueConverter().fromJson(json["script_var_sigma"], &p->engine);
  ctx->setActivationObject(var_sigma);
  p->engine.popContext();
  if(!var_sigma.isObject() || !var_sigma.property("eval").isFunction())
    return false;

  return true;
}

bool ScriptType::rootFitToScriptVar(const std::vector<AScriptParamInfo> &param_info,
                                    const TFitResultPtr &fit, QScriptValue &script_var) const
{
  return rootFitToScriptVar(param_info, fit->Parameters(), script_var);
}

bool ScriptType::rootFitToScriptVar(const std::vector<AScriptParamInfo> &param_info,
                                    const std::vector<double> &fitted_params, QScriptValue &script_var) const
{
  //Check for failed fit
  if(fitted_params.size() != param_info.size())
    return false;

  //Retrieve results into script var
  //TMatrixDSym cov = fit->GetCovarianceMatrix();
  //Double_t chi2 = fit->Chi2();
  for(size_t i = 0; i < param_info.size(); i++) {
    Double_t par = fitted_params[i];
    script_var.setProperty(param_info[i].name, par);
    qDebug()<<"Fit parameter"<<param_info[i].name<<"to"<<par;
  }
  return true;
}

QJsonObject ScriptType::lrfToJson(const ALrf *lrf) const
{
  const AScript *script_lrf = dynamic_cast<const AScript*>(lrf);
  if(!script_lrf) return QJsonObject();
  QJsonObject json;
  json["script_code"] = script_lrf->getScript();
  json["transform"] = script_lrf->getTransform().toJson();
  json["script_var"] = AScriptValueConverter().toJson(script_lrf->getScriptVar());
  json["script_var_sigma"] = AScriptValueConverter().toJson(script_lrf->getScriptVarSigma());
  return json;
}

#ifdef GUI
QWidget *ScriptType::newInternalsWidget(QWidget *parent) const
{
  return new ScriptInternalsWidget(parent);
}
#endif


QJsonObject ScriptPolarType::lrfToJson(const ALrf *lrf) const
{
  QJsonObject json = ScriptType::lrfToJson(lrf);
  APoint dummy;
  double double_dummy, rmax;
  lrf->getAxialRange(dummy, double_dummy, rmax);
  json["rmax"] = rmax;
  return json;
}

ALrf *ScriptPolarType::lrfFromData(const QJsonObject &settings, bool fit_error,
                                const std::vector<APoint> &event_pos,
                                const std::vector<double> &event_signal) const
{
  QString script_code = settings["script"].toString();
  QScriptString name = evaluateScript(script_code);
  if(!name.isValid())
    return nullptr;
  QScriptValue script_var = p->lrf_collection->property(name);
  QScriptValue script_eval = script_var.property("eval");
  QScriptEngine *engine = script_var.engine();
  int arg_count = script_eval.property("length").toInt32();

  //Get rmin, rmax, and nbins
  double rmin = script_var.property("rmin").toNumber();
  if(rmin <= 0) rmin = 0; //Radial.. can't be < 0
  double rmax = script_var.property("rmax").toNumber();
  if(rmax <= rmin) {
    //Auto-detect rmax from fitting data
    double rmax2 = 0;
    for(size_t i = 0; i < event_pos.size(); i++) {
      double r2 = event_pos[i].normxySq();
      if(r2 > rmax2) rmax2 = r2;
    }
    rmax = sqrt(rmax2);
  }

  if(rmax <= rmin)
    //Event data is not inDomain of script function
    return nullptr;

  //Collect parameter info of script function.
  //(TODO) init/min/max may be functions of:
  //  r (local event positions = global event positions)
  //  sensors_r (positions of the sensors)
  //  sensor (index of the sensor and some extra info?)
  //  signals (event signals)
  //eval may be a function of:
  //  R (global event positions)
  //  r (local event positions)
  //  TODO sensor (index of the sensor and some extra info?)
  //  TODO signals (event signals)
  std::vector<AScriptParamInfo> param_info = AScript::getScriptParams(script_var);

  int nbinsr = 300;
  int nbinsz = 300;
  /*if(event_pos.size() > 100000) nbins = 1000;
else if(event_pos.size() >  10000) nbins = event_pos.size() / 100 + 1;
else if(event_pos.size() >    100) nbins = event_pos.size() /  10 + 1;*/

  if(with_z) { //polar with Z  //////////////////////////////////

    //Get zmin, zmax
    double zmin = script_var.property("zmin").toNumber();
    double zmax = script_var.property("zmax").toNumber();
    if(zmax <= zmin) {
      //Auto-detect xmax from fitting data
      zmin = 1e10;
      zmax = -1e10;
      for(size_t i = 0; i < event_pos.size(); i++) {
        if (event_pos[i].z() < zmin) zmin = event_pos[i].z();
        if (event_pos[i].z() > zmax) zmax = event_pos[i].z();
      }
    }

    //Setup fit data. Would be more general if it could be done in script
    auto signal_profile = std::unique_ptr<TProfile2D>(new TProfile2D("ScriptLrfSignalProf", "", nbinsr, rmin, rmax, nbinsz, zmin, zmax));
    //"s" means error is sigma. The default would be sigma/sqrt(N), but for radial
    //this would result in very accurate fit of tail at the expense of bad fit
    //close to origin, which is our area of most interest! Best fit would be
    //without errors (by using W option in Fit call), but that seems like cheating
    signal_profile->SetErrorOption("s");
    signal_profile->Approximate(kTRUE); //Attempt to use low stat bins during fit
    for(size_t i = 0; i < event_pos.size(); i++) {
      double r = hypot(event_pos[i].x(), event_pos[i].y());
      signal_profile->Fill(r, event_pos[i].z(), event_signal[i]);
    }

    //Setup fit function (which calls script eval())
    //using & here is important for the fit error part!
    TF2 func("ScriptLrfFunc", [=,&script_var,&script_eval](double *v, double *p)
    {
      for(auto &param : param_info)
        script_var.setProperty(param.name, *p++);

      QScriptValueList args;
      //During fit r = R (local = global)
      for(int i = 0; i < arg_count; i++) {
        QScriptValue arr = engine->newArray(2);
        arr.setProperty(0, v[0]);
        arr.setProperty(1, v[1]);
        args<<arr;
      }
      QScriptValue res = script_eval.call(script_var, args);
      return (double)res.toNumber();
    },
    rmin, rmax, zmin, zmax, param_info.size(), "");

    //Setup fit parameters
    for(size_t i = 0; i < param_info.size(); i++) {
      func.SetParameter(i, param_info[i].init);
      //qDebug()<<"Set parameter"<<param_info[i].name<<"to"<<param_info[i].init;
      func.SetParLimits(i, param_info[i].min, param_info[i].max);
      //qDebug()<<"Set limits to"<<param_info[i].min<<"\t"<<param_info[i].max;
    }

    //Do fit and set parameters in script
    TFitResultPtr fit = signal_profile->Fit(&func, "SQ");
    if(!rootFitToScriptVar(param_info, fit, script_var))
      return nullptr;

    auto lrf = std::unique_ptr<AScriptPolarZ>(new AScriptPolarZ(id(), p->lrf_collection, name, script_code, rmax));
    if(fit_error) {
      name = evaluateScript(script_code);
      if(!name.isValid())
        return nullptr;
      script_var = p->lrf_collection->property(name);
      script_eval = script_var.property("eval");

      auto signal_profile = std::unique_ptr<TProfile2D>(new TProfile2D("ScriptLrfSignalProfSigma", "", nbinsr, rmin, rmax, nbinsz, zmin, zmax));
      signal_profile->SetErrorOption("s");
      signal_profile->Approximate(kTRUE); //Attempt to use low stat bins during fit
      for(size_t i = 0; i < event_pos.size(); i++) {
        double r = hypot(event_pos[i].x(), event_pos[i].y());
        signal_profile->Fill(r, event_pos[i].z(), event_signal[i] - lrf->eval(event_pos[i]));
      }

      //Do fit and set parameters in script
      TFitResultPtr fit = signal_profile->Fit(&func, "SQ");
      if(!rootFitToScriptVar(param_info, fit, script_var))
        return nullptr;
      lrf->setSigmaVar(name);
    }
    return lrf.release();

  } else { //polar without Z ///////////////////////////

    //Setup fit data. Would be more general if it could be done in script
    auto signal_profile = std::unique_ptr<TProfile>(new TProfile("ScriptLrfSignalProf", "", nbinsr, rmin, rmax));
    signal_profile->SetErrorOption("s");
    signal_profile->Approximate(kTRUE); //Attempt to use low stat bins during fit
    for(size_t i = 0; i < event_pos.size(); i++) {
      double r = hypot(event_pos[i].x(), event_pos[i].y());
      signal_profile->Fill(r, event_signal[i]);
    }

    //Setup fit function (which calls script eval())
    //using & here is important for the fit error part!
    TF1 func("ScriptLrfFunc", [=,&script_var,&script_eval](double *v, double *p)
    {
      for(auto &param : param_info)
        script_var.setProperty(param.name, *p++);

      QScriptValueList args;
      //During fit r = R (local = global)
      for(int i = 0; i < arg_count; i++) {
        QScriptValue arr = engine->newArray(2);
        arr.setProperty(0, v[0]);
        args<<arr;
      }
      QScriptValue res = script_eval.call(script_var, args);
      return (double)res.toNumber();
    },
    rmin, rmax, param_info.size(), "");

    //Setup fit parameters
    for(size_t i = 0; i < param_info.size(); i++) {
      func.SetParameter(i, param_info[i].init);
      //qDebug()<<"Set parameter"<<param_info[i].name<<"to"<<param_info[i].init;
      func.SetParLimits(i, param_info[i].min, param_info[i].max);
      //qDebug()<<"Set limits to"<<param_info[i].min<<"\t"<<param_info[i].max;
    }

    //Do fit and set parameters in script
    TFitResultPtr fit = signal_profile->Fit(&func, "SQ");
    if(!rootFitToScriptVar(param_info, fit, script_var))
      return nullptr;

    auto lrf = std::unique_ptr<AScriptPolar>(new AScriptPolar(id(), p->lrf_collection, name, script_code, rmax));
    if(fit_error) {
      //Get new name for error script_var
      name = evaluateScript(script_code);
      if(!name.isValid())
        return nullptr;
      //Setting these is important for TF* func!
      script_var = p->lrf_collection->property(name);
      script_eval = script_var.property("eval");

      auto signal_profile = std::unique_ptr<TProfile>(new TProfile("ScriptLrfSignalProfSigma", "", nbinsr, rmin, rmax));
      signal_profile->SetErrorOption("s");
      signal_profile->Approximate(kTRUE); //Attempt to use low stat bins during fit
      for(size_t i = 0; i < event_pos.size(); i++) {
        double r = hypot(event_pos[i].x(), event_pos[i].y());
        signal_profile->Fill(r, event_signal[i] - lrf->eval(event_pos[i]));
      }

      //Do fit and set parameters in script
      TFitResultPtr fit = signal_profile->Fit(&func, "SQ");
      if(!rootFitToScriptVar(param_info, fit, script_var))
        return nullptr;
      lrf->setSigmaVar(name);
    }
    return lrf.release();
  }
}

ALrf *ScriptPolarType::lrfFromJson(const QJsonObject &json) const
{
  QJsonValue script_code_json = json["script_code"];
  if(!script_code_json.isString())
    return nullptr;
  QString script_code = script_code_json.toString();

#if 0 //We shouldn't evaluateScript, might get undesired values. But here's code if needed :)
  QScriptString name = evaluateScript(script_code);
  if(!name.isValid())
    return nullptr;
  QScriptValue script_var = p->lrf_collection->property(name);
  std::vector<AScript::ParamInfo> param_info = getScriptParams(script_var);
  if(param_info.size() < 1)
    return nullptr;
#else
  QScriptString name = generateLrfName();
  QScriptString name_sigma = generateLrfName();
#endif

  QScriptValue script_var, script_var_sigma;
  if(!loadScriptVarFromJson(json, script_var, script_var_sigma))
    return nullptr;
  p->lrf_collection->setProperty(name, script_var);
  p->lrf_collection->setProperty(name_sigma, script_var_sigma);

  double rmax = json["rmax"].toDouble();
  ATransform t = ATransform::fromJson(json["transform"].toObject());

  AScriptPolar *lrf = new AScriptPolar(id(), p->lrf_collection, name, script_code, rmax, t);
  lrf->setSigmaVar(name_sigma);
  return lrf;
}

#ifdef GUI
QWidget *ScriptPolarType::newSettingsWidget(QWidget *parent) const
{
  QString code =
      "//Variables to be fitted and their initial values:\n"
      "var A = 150; var mu = 0\n"
      "var sigma2 = 2; var tail = 1\n"
      "\n"
      "function eval(r)\n"
      "{\n"
      "    var dr = r[0]-mu\n"
      "    return A*Math.exp(-0.5*dr*dr/sigma2)+tail\n"
      "}\n";
  return new ScriptSettingsWidget(code, parent);
}
#endif

std::shared_ptr<const ALrf> ScriptPolarType::copyToCurrentThread(std::shared_ptr<const ALrf> lrf) const
{
  std::shared_ptr<QScriptValue> thr_lrfs;
  QScriptString new_name = copyScriptToCurrentThread(lrf, thr_lrfs);
  if(new_name.isValid()) {
    const AScriptPolar *slrf = static_cast<const AScriptPolar*>(lrf.get());
    return std::shared_ptr<AScriptPolar>(new AScriptPolar(id(), thr_lrfs, new_name,
      slrf->getScript(), slrf->getRmax(), slrf->getTransform()));
  } else {
    return lrf;
  }
}


QJsonObject ScriptCartesianType::lrfToJson(const ALrf *lrf) const
{
  QJsonObject json = ScriptType::lrfToJson(lrf);
  double xmin, xmax, ymin, ymax;
  lrf->getXYRange(xmin, xmax, ymin, ymax);
  json["xmin"] = xmin;
  json["xmax"] = xmax;
  json["ymin"] = ymin;
  json["ymax"] = ymax;
  return json;
}

ALrf *ScriptCartesianType::lrfFromData(const QJsonObject &settings, bool fit_error, const std::vector<APoint> &event_pos, const std::vector<double> &event_signal) const
{
  QString script_code = settings["script"].toString();
  QScriptString name = evaluateScript(script_code);
  if(!name.isValid())
    return nullptr;
  QScriptValue script_var = p->lrf_collection->property(name);
  QScriptValue script_eval = script_var.property("eval");
  QScriptEngine *engine = script_var.engine();
  int arg_count = script_eval.property("length").toInt32();

  //Get xmin, xmax
  double xmin = script_var.property("xmin").toNumber();
  double xmax = script_var.property("xmax").toNumber();
  if(xmax <= xmin) {
    //Auto-detect xmax from fitting data
    xmin = 1e10;
    xmax = -1e10;
    for(size_t i = 0; i < event_pos.size(); i++) {
      if (event_pos[i].x() < xmin) xmin = event_pos[i].x();
      if (event_pos[i].x() > xmax) xmax = event_pos[i].x();
    }
  }

  //Get ymin, ymax
  double ymin = script_var.property("ymin").toNumber();
  double ymax = script_var.property("ymax").toNumber();
  if(ymax <= ymin) {
    //Auto-detect xmax from fitting data
    ymin = 1e10;
    ymax = -1e10;
    for(size_t i = 0; i < event_pos.size(); i++) {
      if (event_pos[i].y() < ymin) ymin = event_pos[i].y();
      if (event_pos[i].y() > ymax) ymax = event_pos[i].y();
    }
  }

  //Collect parameter info of script function
  std::vector<AScriptParamInfo> param_info = AScript::getScriptParams(script_var);

  int nbinsx = 300;
  int nbinsy = 300;

  if(with_z) { //cartesian with Z  //////////////////////////////////

    int nbinsz = 100;
    {
      QScriptValue script_nbinsz = script_var.property("_ants2_nbinsz");
      if(script_nbinsz.isNumber())
        nbinsz = script_nbinsz.toNumber();
    }

    bool use_fitter = false;
    {
      QScriptValue script_nbinsz = script_var.property("_ants2_use_fitter");
      if(script_nbinsz.isBool())
        use_fitter = script_nbinsz.toBool();
    }

    //Get zmin, zmax
    double zmin = script_var.property("zmin").toNumber();
    double zmax = script_var.property("zmax").toNumber();
    if(zmax <= zmin) {
      //Auto-detect xmax from fitting data
      zmin = 1e10;
      zmax = -1e10;
      for(size_t i = 0; i < event_pos.size(); i++) {
        if (event_pos[i].z() < zmin) zmin = event_pos[i].z();
        if (event_pos[i].z() > zmax) zmax = event_pos[i].z();
      }
    }

    //Setup fit function (which calls script eval())
    //using & here is important for the fit error part!
    TF3 func("ScriptLrfFunc", [=,&script_var,&script_eval](double *v, double *p)
    {
      for(auto &param : param_info)
        script_var.setProperty(param.name, *p++);

      QScriptValueList args;
      //During fit r = R (local = global)
      for(int i = 0; i < arg_count; i++) {
        QScriptValue arr = engine->newArray(3);
        arr.setProperty(0, v[0]);
        arr.setProperty(1, v[1]);
        arr.setProperty(2, v[2]);
        args<<arr;
      }
      QScriptValue res = script_eval.call(script_var, args);
      //qDebug()<<v[0]<<v[1]<<res.toNumber();
      return (double)res.toNumber();
    },
    xmin, xmax, ymin, ymax, zmin, zmax, param_info.size(), "");

    //Setup fit parameters
    for(size_t i = 0; i < param_info.size(); i++) {
      func.SetParameter(i, param_info[i].init);
      //qDebug()<<"Set parameter"<<param_info[i].name<<"to"<<param_info[i].init;
      func.SetParLimits(i, param_info[i].min, param_info[i].max);
      //qDebug()<<"Set limits to"<<param_info[i].min<<"\t"<<param_info[i].max;
    }

    if(use_fitter) {

      qDebug()<<"Using fitter";
      //Setup fit data
      ROOT::Fit::BinData events_bin(event_pos.size(), 3, ROOT::Fit::BinData::kNoError);
      for(size_t i = 0; i < event_pos.size(); i++)
        events_bin.Add(event_pos[i].r, event_signal[i]);

      //Do fit and set parameters in script
      ROOT::Fit::Fitter fitter;
      ROOT::Math::WrappedMultiTF1 wtf3(func, 3);
      fitter.SetFunction(wtf3);
      if(!fitter.Fit(events_bin))
        return nullptr;

      //Can't copy vector, it's illformed! Must copy parameter-by-parameter
      //auto fit_params = fitter.Result().Parameters();
      std::vector<double> fit_params(param_info.size());
      for(unsigned int i = 0; i < fit_params.size(); i++)
        fit_params[i] = fitter.Result().Parameter(i);
      if(!rootFitToScriptVar(param_info, fit_params, script_var))
        return nullptr;

    } else { //Don't use fitter

      qDebug()<<"Not using fitter";
      //Setup fit data
      auto signal_profile = std::unique_ptr<TProfile3D>(new TProfile3D("ScriptLrfSignalProf", "", nbinsx, xmin, xmax, nbinsy, ymin, ymax, nbinsz, zmin, zmax));
      signal_profile->SetErrorOption("s");
      signal_profile->Approximate(kTRUE); //Attempt to use low stat bins during fit
      for(size_t i = 0; i < event_pos.size(); i++)
        signal_profile->Fill(event_pos[i].x(), event_pos[i].y(), event_pos[i].z(), event_signal[i]);

      //Do fit and set parameters in script
      TFitResultPtr fit = signal_profile->Fit(&func, "SQ");
      if(!rootFitToScriptVar(param_info, fit, script_var))
        return nullptr;
    }

    auto lrf = std::unique_ptr<AScriptCartesianZ>(new AScriptCartesianZ(id(), p->lrf_collection, name, script_code, xmin, xmax, ymin, ymax));
    if(fit_error) {
      name = evaluateScript(script_code);
      if(!name.isValid())
        return nullptr;
      script_var = p->lrf_collection->property(name);
      script_eval = script_var.property("eval");

      if(use_fitter) {

        //Setup fit data
        ROOT::Fit::BinData events_bin(event_pos.size(), 3, ROOT::Fit::BinData::kNoError);
        for(size_t i = 0; i < event_pos.size(); i++)
          events_bin.Add(event_pos[i].r, event_signal[i] - lrf->eval(event_pos[i]));

        //Do fit and set parameters in script
        ROOT::Fit::Fitter fitter;
        ROOT::Math::WrappedMultiTF1 wtf3(func, 3);
        fitter.SetFunction(wtf3);
        if(!fitter.Fit(events_bin))
          return nullptr;

        //Can't copy vector, it's illformed! Must copy parameter-by-parameter
        //auto fit_params = fitter.Result().Parameters();
        std::vector<double> fit_params(param_info.size());
        for(unsigned int i = 0; i < fit_params.size(); i++)
          fit_params[i] = fitter.Result().Parameter(i);
        if(!rootFitToScriptVar(param_info, fit_params, script_var))
          return nullptr;

      } else { //Don't use fitter

      auto signal_profile = std::unique_ptr<TProfile3D>(new TProfile3D("ScriptLrfSignalProfSigma", "", nbinsx, xmin, xmax, nbinsy, ymin, ymax, nbinsz, zmin, zmax));
      signal_profile->SetErrorOption("s");
      signal_profile->Approximate(kTRUE); //Attempt to use low stat bins during fit
      for(size_t i = 0; i < event_pos.size(); i++)
        signal_profile->Fill(event_pos[i].x(), event_pos[i].y(), event_pos[i].z(), event_signal[i] - lrf->eval(event_pos[i]));

      //Do fit and set parameters in script
      TFitResultPtr fit = signal_profile->Fit(&func, "SQ");
      if(!rootFitToScriptVar(param_info, fit, script_var))
        return nullptr;
      }

      lrf->setSigmaVar(name);
    }
    return lrf.release();

  } else { //cartesian without Z ///////////////////////////

    //Setup fit data
    auto signal_profile = std::unique_ptr<TProfile2D>(new TProfile2D("ScriptLrfSignalProf", "", nbinsx, xmin, xmax, nbinsy, ymin, ymax));
    signal_profile->SetErrorOption("s");
    signal_profile->Approximate(kTRUE); //Attempt to use low stat bins during fit
    for(size_t i = 0; i < event_pos.size(); i++)
      signal_profile->Fill(event_pos[i].x(), event_pos[i].y(), event_signal[i]);

    //Setup fit function (which calls script eval())
    //using & here is important for the fit error part!
    TF2 func("ScriptLrfFunc", [=,&script_var,&script_eval](double *v, double *p)
    {
      for(auto &param : param_info)
        script_var.setProperty(param.name, *p++);

      QScriptValueList args;
      //During fit r = R (local = global)
      for(int i = 0; i < arg_count; i++) {
        QScriptValue arr = engine->newArray(2);
        arr.setProperty(0, v[0]);
        arr.setProperty(1, v[1]);
        args<<arr;
      }
      QScriptValue res = script_eval.call(script_var, args);
      //qDebug()<<v[0]<<v[1]<<res.toNumber();
      return (double)res.toNumber();
    },
    xmin, xmax, ymin, ymax, param_info.size(), "");

    //Setup fit parameters
    for(size_t i = 0; i < param_info.size(); i++) {
      func.SetParameter(i, param_info[i].init);
      //qDebug()<<"Set parameter"<<param_info[i].name<<"to"<<param_info[i].init;
      func.SetParLimits(i, param_info[i].min, param_info[i].max);
      //qDebug()<<"Set limits to"<<param_info[i].min<<"\t"<<param_info[i].max;
    }

    //Do fit and set parameters in script
    TFitResultPtr fit = signal_profile->Fit(&func, "SQ");
    if(!rootFitToScriptVar(param_info, fit, script_var))
      return nullptr;

    auto lrf = std::unique_ptr<AScriptCartesian>(new AScriptCartesian(id(), p->lrf_collection, name, script_code, xmin, xmax, ymin, ymax));
    if(fit_error) {
      name = evaluateScript(script_code);
      if(!name.isValid())
        return nullptr;
      script_var = p->lrf_collection->property(name);
      script_eval = script_var.property("eval");

      signal_profile = std::unique_ptr<TProfile2D>(new TProfile2D("ScriptLrfSignalProfSigma", "", nbinsx, xmin, xmax, nbinsy, ymin, ymax));
      signal_profile->SetErrorOption("s");
      signal_profile->Approximate(kTRUE); //Attempt to use low stat bins during fit
      for(size_t i = 0; i < event_pos.size(); i++)
        signal_profile->Fill(event_pos[i].x(), event_pos[i].y(), event_signal[i] - lrf->eval(event_pos[i]));

      //Do fit and set parameters in script
      TFitResultPtr fit = signal_profile->Fit(&func, "SQ");
      if(!rootFitToScriptVar(param_info, fit, script_var))
        return nullptr;

      lrf->setSigmaVar(name);
    }
    return lrf.release();

  }
}

ALrf *ScriptCartesianType::lrfFromJson(const QJsonObject &json) const
{
  QJsonValue script_code_json = json["script_code"];
  if(!script_code_json.isString())
    return nullptr;
  QString script_code = script_code_json.toString();

  //We shouldn't evaluateScript(), might get undesired values. But if you need
  //code for it, check ScriptPolarType::lrfFromJson.
  QScriptString name = generateLrfName();
  QScriptString name_sigma = generateLrfName();

  QScriptValue script_var, script_var_sigma;
  if(!loadScriptVarFromJson(json, script_var, script_var_sigma))
    return nullptr;
  p->lrf_collection->setProperty(name, script_var);
  p->lrf_collection->setProperty(name_sigma, script_var_sigma);

  double xmin = json["xmin"].toDouble();
  double xmax = json["xmax"].toDouble();
  double ymin = json["ymin"].toDouble();
  double ymax = json["ymax"].toDouble();
  ATransform t = ATransform::fromJson(json["transform"].toObject());

  AScriptCartesian *lrf = new AScriptCartesian(id(), p->lrf_collection, name, script_code, xmin, xmax, ymin, ymax, t);
  lrf->setSigmaVar(name_sigma);
  return lrf;
}

#ifdef GUI
QWidget *ScriptCartesianType::newSettingsWidget(QWidget *parent) const
{
  QString code; code.sprintf(
      "//Variables to be fitted and their initial values:\n"
      "var A = 150\n"
      "var sigma2x = 2\n"
      "var sigma2y = 2\n"
      "%s"
      "var tail = 1\n"
      "function eval(r)\n"
      "{\n"
      "    var expo = r[0]*r[0]/sigma2x + r[1]*r[1]/sigma2y%s\n"
      "    return A * Math.exp(-0.5*expo) + tail\n"
      "}\n",
        with_z ? "var sigma2z = 2\n":"",
        with_z ? " + r[2]*r[2]/sigma2z":"");
  return new ScriptSettingsWidget(code, parent);
}
#endif

std::shared_ptr<const ALrf> ScriptCartesianType::copyToCurrentThread(std::shared_ptr<const ALrf> lrf) const
{
  std::shared_ptr<QScriptValue> thr_lrfs;
  QScriptString new_name = copyScriptToCurrentThread(lrf, thr_lrfs);
  if(new_name.isValid()) {
    const AScriptCartesian *slrf = static_cast<const AScriptCartesian*>(lrf.get());
    return std::shared_ptr<AScriptCartesian>(new AScriptCartesian(id(), thr_lrfs, new_name,
      slrf->getScript(), slrf->getXmin(), slrf->getXmin(),
      slrf->getYmin(), slrf->getYmax(), slrf->getTransform()));
  } else {
    return lrf;
  }
}

} } //namespace LRF::CorePlugin

/***************************************************************************\
*                   Method that registers Core LrfTypes                     *
\***************************************************************************/
#include "alrftypemanager.h"
void LRF::CoreLrfs::Setup(ALrfTypeManager &manager)
{
  ALrfTypeID axial_id = manager.registerLrfType(std::unique_ptr<AxialType>(new AxialType));
  manager.registerAlias(axial_id, "ComprAxial");
  manager.registerToGroup(axial_id, "Bspline");

  ALrfTypeID xy_id = manager.registerLrfType(std::unique_ptr<AxyType>(new AxyType));
  manager.registerToGroup(xy_id, "Bspline");

  ALrfTypeID axial3d_id = manager.registerLrfType(std::unique_ptr<AAxial3DType>(new AAxial3DType));
  manager.registerToGroup(axial3d_id, "Bspline");

  ALrfTypeID xy3d_id = manager.registerLrfType(std::unique_ptr<ASlicedXYType>(new ASlicedXYType));
  manager.registerToGroup(xy3d_id, "Bspline");

  //Register Script::Polar(+Z)
  ALrfTypeID script1d_id = manager.registerLrfType(std::unique_ptr<ScriptType>(new ScriptPolarType(false)));
  manager.registerToGroup(script1d_id, "Script");
  ALrfTypeID script1dz_id = manager.registerLrfType(std::unique_ptr<ScriptType>(new ScriptPolarType(true)));
  manager.registerToGroup(script1dz_id, "Script");

  //Register Script::Cartesian(+Z)
  ALrfTypeID script2d_id = manager.registerLrfType(std::unique_ptr<ScriptType>(new ScriptCartesianType(false)));
  manager.registerToGroup(script2d_id, "Script");
  ALrfTypeID script2dz_id = manager.registerLrfType(std::unique_ptr<ScriptType>(new ScriptCartesianType(true)));
  manager.registerToGroup(script2dz_id, "Script");
}
