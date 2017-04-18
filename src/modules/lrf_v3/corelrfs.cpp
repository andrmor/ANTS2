#include "corelrfs.h"

#include <QDateTime>
#include <QScriptEngine>
#include <QScriptValueList>
#include <QScriptValueIterator>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>

#include "alrf.h"
#include "alrftypemanager.h"
#include "atransform.h"
#include "avladimircompression.h"
#include "spline.h"
#include "bspline3.h"

namespace LRF { namespace CoreLrfs {

/***************************************************************************\
*                   Implementation of Axial and related                     *
\***************************************************************************/
double AAxial::distance(const APoint &pos) const
{
  return transf.inverse(pos).normxy();
}

AAxial::AAxial(ALrfTypeID type, const Bspline3 &bsr, const Bspline3 &bse,
               bool flattop, const ATransform &t)
  : ARelativeLrf(type, t), bsr(bsr), bse(bse),
    rmax2(bsr.GetXmax()*bsr.GetXmax()), flattop(flattop) { }

bool AAxial::inDomain(const APoint &pos) const
{
  return transf.inverse(pos).normxySq() < rmax2;
}

double AAxial::eval(const APoint &pos) const
{
  return bsr.Eval(distance(pos));
}

double AAxial::sigma(const APoint &pos) const
{
  return bse.GetXmax() != 0. ? bse.Eval(distance(pos)) : 0.;
}

ALrf *AAxial::clone() const
{
  return new AAxial(type(), bsr, bse, flattop, transf);
}

void AAxial::getAxialRange(APoint &center, double &min, double &max) const
{
  //TODO: Handle min properly, perhaps based on input data of fit
  center = transf.transform(APoint());
  min = 0;
  max = sqrt(rmax2);
}

void AAxial::getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const
{
  APoint center = transf.transform(APoint());
  double rmax = sqrt(rmax2);
  xmin = center.x() - rmax;
  xmax = center.x() + rmax;
  ymin = center.y() - rmax;
  ymax = center.y() + rmax;
}

AxialCompressed::AxialCompressed(const AAxial &uncompressed,
                                 const AVladimirCompression &compressor)
  : AAxial(uncompressed), compress(compressor)
{
  double rmax = compressor.un(sqrt(rmax2));
  rmax2 = rmax*rmax;
}

double AxialCompressed::eval(const APoint &pos) const
{
  return bsr.Eval(compress(distance(pos)));
}

double AxialCompressed::sigma(const APoint &pos) const
{
  return bse.GetXmax() != 0. ? bse.Eval(compress(distance(pos))) : 0.;
}

ALrf *AxialCompressed::clone() const
{
  AAxial *axial = static_cast<AAxial*>(AAxial::clone());
  AxialCompressed *compr = new AxialCompressed(*axial, compress);
  delete axial;
  return compr;
}


/***************************************************************************\
*                     Implementation of ASlicedAxial                        *
\***************************************************************************/
AAxial3D::AAxial3D(ALrfTypeID type, const TPspline3 &bsr, const TPspline3 &bse,
                   const AVladimirCompression &compressor, const ATransform &t) :
    ARelativeLrf(type, t), bsr(bsr), bse(bse), compress(compressor),
    zmin(bsr.GetYmin()), zmax(bsr.GetYmax())
{
  double rmax = compressor.un(bsr.GetXmax());
  rmax2 = rmax*rmax;
}

bool AAxial3D::inDomain(const APoint &pos) const
{
  APoint p = transf.inverse(pos);
  return p.normxySq() < rmax2 && p.z() > zmin && p.z() < zmax;
}

double AAxial3D::eval(const APoint &pos) const
{
  APoint p = transf.inverse(pos);
  return bsr.Eval(compress(p.normxy()), p.z());
}

double AAxial3D::sigma(const APoint &pos) const
{
  APoint p = transf.inverse(pos);
  return bse.isInvalid() ? 0. : bse.Eval(compress(p.normxy()), p.z());
}

ALrf *AAxial3D::clone() const
{
  return new AAxial3D(type(), bsr, bse, compress, transf);
}

void AAxial3D::getAxialRange(APoint &center, double &min, double &max) const
{
  //TODO: Handle min properly, perhaps based on input data of fit
  center = transf.transform(APoint());
  min = 0;
  max = sqrt(rmax2);
}

void AAxial3D::getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const
{
  APoint center = transf.transform(APoint());
  xmin = center.x() - sqrt(this->rmax2);
  xmax = center.x() + sqrt(this->rmax2);
  ymin = center.y() - sqrt(this->rmax2);
  ymax = center.y() + sqrt(this->rmax2);
}


/***************************************************************************\
*                   Implementation of Axy and related                       *
\***************************************************************************/
Axy::Axy(ALrfTypeID type, const TPspline3 &bsr, const TPspline3 &bse,
         const ATransform &t) :
    ARelativeLrf(type, t), bsr(bsr), bse(bse), xmin(bsr.GetXmin()),
    xmax(bsr.GetXmax()), ymin(bsr.GetYmin()), ymax(bsr.GetYmax()) { }

bool Axy::inDomain(const APoint &pos) const
{
  APoint p = transf.inverse(pos);
  return (p.x()>xmin && p.x()<xmax && p.y()>ymin && p.y()<ymax);
}

double Axy::eval(const APoint &pos) const
{
  APoint p = transf.inverse(pos);
  return bsr.Eval(p.x(), p.y());
}

double Axy::sigma(const APoint &pos) const
{
  APoint p = transf.inverse(pos);
  return bse.Eval(p.x(), p.y());
}

ALrf *Axy::clone() const
{
  return new Axy(type(), bsr, bse, transf);
}

void Axy::getAxialRange(APoint &center, double &min, double &max) const
{
  center = transf.transform(APoint());
  if((xmin < 0) != (xmax < 0) || (ymin < 0) != (ymax < 0)) min = 0;
  else min = hypot(std::min(std::abs(xmin), std::abs(xmax)), std::min(std::abs(ymin), std::abs(ymax)));
  max = hypot(std::max(std::abs(xmin), std::abs(xmax)), std::max(std::abs(ymin), std::abs(ymax)));
}

void Axy::getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const
{
  APoint center = transf.transform(APoint());
  xmin = center.x() + this->xmin;
  xmax = center.x() + this->xmax;
  ymin = center.y() + this->ymin;
  ymax = center.y() + this->ymax;
}


/***************************************************************************\
*                 Implementation of ASlicedXY and related                   *
\***************************************************************************/
double ASlicedXY::getLayers(double z, int &lower_layer) const
{
  if (z <= zbot)
      lower_layer = 0;
  else if (z >= ztop)
      lower_layer = bsr.size()-1;
  else {
      double zrel = (z-zbot)/dz;
      lower_layer = (int)zrel;
      return z - lower_layer;
  }
  return -1;
}

ASlicedXY::ASlicedXY(ALrfTypeID type, double zmin, double zmax, const std::vector<TPspline3> &bsr,
                     const std::vector<TPspline3> &bse, const ATransform &t) :
    ARelativeLrf(type, t), bsr(bsr), bse(bse), zmin(zmin), zmax(zmax)
{
  xmin = +1e300; xmax = -1e300;
  ymin = +1e300; ymax = -1e300;
  for(size_t i = 0; i < bsr.size(); i++) {
    const TPspline3 &spline = bsr[i];
    if(spline.GetXmin() < xmin) xmin = spline.GetXmin();
    if(spline.GetXmax() > xmax) xmax = spline.GetXmax();
    if(spline.GetYmin() < ymin) ymin = spline.GetYmin();
    if(spline.GetYmax() > ymax) ymax = spline.GetYmax();
  }

  dz = (zmax - zmin)/bsr.size();
  zbot = zmin + dz/2.;
  ztop = zmax - dz/2.;
}

bool ASlicedXY::inDomain(const APoint &pos) const
{
  APoint p = transf.inverse(pos);
  return p.x()>xmin && p.x()<xmax
      && p.y()>ymin && p.y()<ymax
      && p.z()>zmin && p.z()<zmax;
}

double ASlicedXY::eval(const APoint &pos) const
{
  APoint p = transf.inverse(pos);

  int layer;
  double frac = getLayers(p.z(), layer);
  if(frac < 0)
    return bsr[layer].Eval(p.x(), p.y());
  else
    return (1.-frac)*bsr[layer].Eval(p.x(), p.y()) + frac*bsr[layer+1].Eval(p.x(), p.y());
}

double ASlicedXY::sigma(const APoint &pos) const
{
  APoint p = transf.inverse(pos);
  int layer;
  double frac = getLayers(p.z(), layer);
  if(frac < 0)
      return bse[layer].Eval(p.x(), p.y());
  else {
      double a = bse[layer].Eval(p.x(), p.y());
      double b = bse[layer+1].Eval(p.x(), p.y());
      return sqrt((1.-frac)*a*a + frac*b*b);
  }
}

ALrf *ASlicedXY::clone() const
{
  return new ASlicedXY(type(), zmin, zmax, bsr, bse, transf);
}

void ASlicedXY::getAxialRange(APoint &center, double &min, double &max) const
{
  center = transf.transform(APoint());
  if((xmin < 0) != (xmax < 0) || (ymin < 0) != (ymax < 0)) min = 0;
  else min = hypot(std::min(std::abs(xmin), std::abs(xmax)), std::min(std::abs(ymin), std::abs(ymax)));
  max = hypot(std::max(std::abs(xmin), std::abs(xmax)), std::max(std::abs(ymin), std::abs(ymax)));
}

void ASlicedXY::getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const
{
  APoint center = transf.transform(APoint());
  xmin = center.x() + this->xmin;
  xmax = center.x() + this->xmax;
  ymin = center.y() + this->ymin;
  ymax = center.y() + this->ymax;
}


/***************************************************************************\
*                   Implementation of Script and related                    *
\***************************************************************************/
std::vector<AScriptParamInfo> AScript::getScriptParams(QScriptValue script_var)
{
  //Parameters are variables not starting with _ which are: numbers, or objects
  // which contain init and/or min+max
  std::vector<AScriptParamInfo> param_info;
  QScriptValueIterator val(script_var);
  while (val.hasNext()) {
    val.next();
    if(val.flags() & QScriptValue::SkipInEnumeration)
      continue;
    if(val.name().startsWith('_'))
      continue;

    QScriptValue param_obj = val.value();
    if(param_obj.isFunction())
      continue;

    AScriptParamInfo pinfo(script_var.engine()->toStringHandle(val.name()));
    if(param_obj.isNumber()) {
      pinfo.init = param_obj.toNumber();
    } else if(param_obj.isObject()) {
      //TODO: allow for functions as "init", "min", and "max"
      pinfo.init = param_obj.property("init").toNumber();
      pinfo.min = param_obj.property("min").toNumber();
      pinfo.max = param_obj.property("max").toNumber();
    }
    param_info.push_back(pinfo);
  }
  return param_info;
}

QStringList AScript::getFunctionArgumentNames(QScriptValue &function)
{
  //expression to match /* */ comments
  QRegularExpression re_block_comments("/\\*.*?\\*/");
  //expression to match //comments
  QRegularExpression re_line_comments("//.*");
  //expression to match the declaration
  QRegularExpression re_declaration("function \\w*\\((.*?)\\)");

  //remove extras, and return the split parameters
  QString function_definition = function.toString();
  function_definition.remove(re_block_comments);
  function_definition.remove(re_line_comments);
  QString parameters = re_declaration.match(function_definition).captured(1);
  return parameters.split(", ");
}

AScript::AScript(ALrfTypeID type, std::shared_ptr<QScriptValue> lrf_collection, QScriptString name,
                 const QString &script, const ATransform &t)
  : ARelativeLrf(type, t), name(name), lrf_collection(lrf_collection), script(script)/*, params(params)*/
{
  script_var = lrf_collection->property(name);
  //Set default values
  script_eval = std::unique_ptr<QScriptValue>(new QScriptValue);
  script_sigma = std::unique_ptr<QScriptValue>(new QScriptValue);

  //Set script functions if valid
  if(script_var.isObject()) {
    QScriptValue tmp = script_var.property("eval");
    if(!script_var.engine()->hasUncaughtException() && tmp.isFunction()) {
      script_eval = std::unique_ptr<QScriptValue>(new QScriptValue(tmp));
      for(const QString &arg_name : AScript::getFunctionArgumentNames(*script_eval)) {
        if(arg_name == "r")
          arguments.push_back(EvalArgument::local_coords);
        else if(arg_name == "R")
          arguments.push_back(EvalArgument::global_coords);
        else
          arguments.push_back(EvalArgument::unknown);
      }
    }
  }
}

AScript::~AScript()
{
  lrf_collection->setProperty(name, QScriptValue());
}

QScriptString AScript::deepCopyScriptVar(QScriptValue *dest) const
{
  if(dest == nullptr)
    dest = this->lrf_collection.get();

  QScriptEngine *engine = dest->engine();
  QString timestamp = QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch(), 16);
  QScriptString new_name = engine->toStringHandle(QString::number(qrand(), 16)+'_'+timestamp);

  QScriptContext *ctx = engine->pushContext();
  QScriptValue new_val = AScriptValueCopier(*engine).copy(script_var);
  ctx->setActivationObject(new_val);
  dest->setProperty(new_name, new_val);
  engine->popContext();

  return new_name;
}

QScriptString AScript::deepCopyScriptVarSigma(QScriptValue *dest) const
{
  if(dest == nullptr)
    dest = this->lrf_collection.get();

  QScriptEngine *engine = dest->engine();
  QString timestamp = QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch(), 16);
  QScriptString new_name = engine->toStringHandle(QString::number(qrand(), 16)+'_'+timestamp);

  QScriptContext *ctx = engine->pushContext();
  QScriptValue new_val = AScriptValueCopier(*engine).copy(script_var_sigma);
  ctx->setActivationObject(new_val);
  dest->setProperty(new_name, new_val);
  engine->popContext();

  return new_name;
}

void AScript::setSigmaVar(QScriptString name)
{
  name_sigma = name;
  script_var_sigma = lrf_collection->property(name);
  //Set default values
  script_sigma = std::unique_ptr<QScriptValue>(new QScriptValue);

  //Set script functions if valid
  if(script_var_sigma.isObject()) {
    QScriptValue tmp = script_var_sigma.property("eval");
    if(!script_var_sigma.engine()->hasUncaughtException() && tmp.isFunction())
      script_sigma = std::unique_ptr<QScriptValue>(new QScriptValue(tmp));
  }
}


QScriptValueList AScriptPolar::makeArguments(const APoint &pos) const
{
  QScriptValueList args;
  for(EvalArgument arg : arguments) {
    switch(arg) {
      case EvalArgument::local_coords: {
        APoint p = transf.inverse(pos);
        QScriptValue arr = script_var.engine()->newArray(1);
        arr.setProperty(0, p.normxy());
        args<<arr;
      } break;
      case EvalArgument::global_coords: {
        QScriptValue arr = script_var.engine()->newArray(1);
        arr.setProperty(0, pos.normxy());
        args<<arr;
      } break;
      default: args<<0;
    }
  }
  return args;
}

AScriptPolar::AScriptPolar(ALrfTypeID type, std::shared_ptr<QScriptValue> lrf_collection,
                     QScriptString name, const QString &script,
                     double rmax, const ATransform &t)
  : AScript(type, lrf_collection, name, script, t), rmax(rmax)
{ }

bool AScriptPolar::inDomain(const APoint &pos) const
{
  return transf.inverse(pos).normxySq() < rmax*rmax;
}

double AScriptPolar::eval(const APoint &pos) const
{
  QScriptValue res = script_eval->call(script_var, makeArguments(pos));
  //toNumber may throw exception in engine. The result is undefined.
  return res.toNumber();
}

double AScriptPolar::sigma(const APoint &pos) const
{
  QScriptValue res = script_sigma->call(script_var_sigma, makeArguments(pos));
  //toNumber may throw exception in engine. The result is undefined.
  return res.toNumber();
}

ALrf *AScriptPolar::clone() const
{
  QScriptString new_name = deepCopyScriptVar();

  AScriptPolar *lrf = new AScriptPolar(type(), lrf_collection, new_name, script, rmax, transf);
  lrf->setSigmaVar(deepCopyScriptVarSigma());
  return lrf;
}

void AScriptPolar::getAxialRange(APoint &center, double &min, double &max) const
{
  //TODO: Handle min properly, perhaps based on input data of fit
  center = transf.transform(APoint());
  min = 0;
  max = rmax;
}

void AScriptPolar::getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const
{
  APoint center = transf.transform(APoint());
  xmin = center.x() - rmax;
  xmax = center.x() + rmax;
  ymin = center.y() - rmax;
  ymax = center.y() + rmax;
}


QScriptValueList AScriptPolarZ::makeArguments(const APoint &pos) const
{
  QScriptValueList args;
  for(EvalArgument arg : arguments) {
    switch(arg) {
      case EvalArgument::local_coords: {
        APoint p = transf.inverse(pos);
        QScriptValue arr = script_var.engine()->newArray(2);
        arr.setProperty(0, p.normxy());
        arr.setProperty(1, p.z());
        args<<arr;
      } break;
      case EvalArgument::global_coords: {
        QScriptValue arr = script_var.engine()->newArray(2);
        arr.setProperty(0, pos.normxy());
        arr.setProperty(1, pos.z());
        args<<arr;
      } break;
      default: args<<0;
    }
  }
  return args;
}

AScriptPolarZ::AScriptPolarZ(ALrfTypeID type, std::shared_ptr<QScriptValue> lrf_collection,
                             QScriptString name, const QString &script, double rmax, const ATransform &t)
  : AScriptPolar(type, lrf_collection, name, script, rmax, t)
{

}

double AScriptPolarZ::eval(const APoint &pos) const
{
  QScriptValue res = script_eval->call(script_var, makeArguments(pos));
  //toNumber may throw exception in engine. The result is undefined.
  return res.toNumber();
}

double AScriptPolarZ::sigma(const APoint &pos) const
{
  QScriptValue res = script_sigma->call(script_var_sigma, makeArguments(pos));
  //toNumber may throw exception in engine. The result is undefined.
  return res.toNumber();
}

ALrf *AScriptPolarZ::clone() const
{
  QScriptString new_name = deepCopyScriptVar();

  AScriptPolarZ *lrf = new AScriptPolarZ(type(), lrf_collection, new_name, script, rmax, transf);
  lrf->setSigmaVar(deepCopyScriptVarSigma());
  return lrf;
}


QScriptValueList AScriptCartesian::makeArguments(const APoint &pos) const
{
  QScriptValueList args;
  for(EvalArgument arg : arguments) {
    switch(arg) {
      case EvalArgument::local_coords: {
        APoint inv = transf.inverse(pos);
        QScriptValue arr = script_var.engine()->newArray(2);
        arr.setProperty(0, inv[0]);
        arr.setProperty(1, inv[1]);
        args<<arr;
      } break;
      case EvalArgument::global_coords: {
        QScriptValue arr = script_var.engine()->newArray(2);
        arr.setProperty(0, pos[0]);
        arr.setProperty(1, pos[1]);
        args<<arr;
      } break;
      default: args<<0;
    }
  }
  return args;
}

AScriptCartesian::AScriptCartesian(ALrfTypeID type, std::shared_ptr<QScriptValue> lrf_collection,
                     QScriptString name, const QString &script, double xmin,
                     double xmax, double ymin, double ymax, const ATransform &t)
  : AScript(type, lrf_collection, name, script, t),
    xmin(xmin), xmax(xmax), ymin(ymin), ymax(ymax)
{ }

bool AScriptCartesian::inDomain(const APoint &pos) const
{
  APoint p = transf.inverse(pos);
  return (p.x()>xmin && p.x()<xmax && p.y()>ymin && p.y()<ymax);
}

double AScriptCartesian::eval(const APoint &pos) const
{
  QScriptValue res = script_eval->call(script_var, makeArguments(pos));
  //toNumber may throw exception in engine. The result is undefined.
  return res.toNumber();
}

double AScriptCartesian::sigma(const APoint &pos) const
{
  QScriptValue res = script_sigma->call(script_var_sigma, makeArguments(pos));
  //toNumber may throw exception in engine. The result is undefined.
  return res.toNumber();
}

ALrf *AScriptCartesian::clone() const
{
  QScriptString new_name = deepCopyScriptVar();

  AScriptCartesian *lrf = new AScriptCartesian(type(), lrf_collection, new_name, script,
                                               xmin, xmax, ymin, ymax, transf);
  lrf->setSigmaVar(deepCopyScriptVarSigma());
  return lrf;
}

void AScriptCartesian::getAxialRange(APoint &center, double &min, double &max) const
{
  center = transf.transform(APoint());
  if((xmin < 0) != (xmax < 0) || (ymin < 0) != (ymax < 0)) min = 0;
  else min = hypot(std::min(std::abs(xmin), std::abs(xmax)), std::min(std::abs(ymin), std::abs(ymax)));
  max = hypot(std::max(std::abs(xmin), std::abs(xmax)), std::max(std::abs(ymin), std::abs(ymax)));
}

void AScriptCartesian::getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const
{
  APoint center = transf.transform(APoint());
  xmin = center.x() + this->xmin;
  xmax = center.x() + this->xmax;
  ymin = center.y() + this->ymin;
  ymax = center.y() + this->ymax;
}


QScriptValueList AScriptCartesianZ::makeArguments(const APoint &pos) const
{
  QScriptValueList args;
  for(EvalArgument arg : arguments) {
    switch(arg) {
      case EvalArgument::local_coords: {
        APoint inv = transf.inverse(pos);
        QScriptValue arr = script_var.engine()->newArray(3);
        arr.setProperty(0, inv[0]);
        arr.setProperty(1, inv[1]);
        arr.setProperty(2, inv[2]);
        args<<arr;
      } break;
      case EvalArgument::global_coords: {
        QScriptValue arr = script_var.engine()->newArray(3);
        arr.setProperty(0, pos[0]);
        arr.setProperty(1, pos[1]);
        arr.setProperty(2, pos[2]);
        args<<arr;
      } break;
      default: args<<0;
    }
  }
  return args;
}

AScriptCartesianZ::AScriptCartesianZ(ALrfTypeID type, std::shared_ptr<QScriptValue> lrf_collection,
                                     QScriptString name, const QString &script, double xmin,
                                     double xmax, double ymin, double ymax, const ATransform &t)
  : AScriptCartesian(type, lrf_collection, name, script, xmin, xmax, ymin, ymax, t)
{ }

double AScriptCartesianZ::eval(const APoint &pos) const
{
  QScriptValue res = script_eval->call(script_var, makeArguments(pos));
  //toNumber may throw exception in engine. The result is undefined.
  return res.toNumber();
}

double AScriptCartesianZ::sigma(const APoint &pos) const
{
  QScriptValue res = script_sigma->call(script_var_sigma, makeArguments(pos));
  //toNumber may throw exception in engine. The result is undefined.
  return res.toNumber();
}

ALrf *AScriptCartesianZ::clone() const
{
  QScriptString new_name = deepCopyScriptVar();

  AScriptCartesianZ *lrf = new AScriptCartesianZ(type(), lrf_collection, new_name, script,
                                               xmin, xmax, ymin, ymax, transf);
  lrf->setSigmaVar(deepCopyScriptVarSigma());
  return lrf;
}

} } //namespace LRF::CoreLrfs
