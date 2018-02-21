#ifndef LRFCORELRFS_H
#define LRFCORELRFS_H

#include <memory>
#include <QScriptValue>
#include <QScriptString>

#include "alrf.h"
#include "bspline3.h"
#ifdef TPS3M
#include "tpspline3m.h"
#else
#include "tpspline3.h"
#endif
#include "avladimircompression.h"
#include "atransform.h"
#include "ascriptvaluecopier.h"

class QScriptEngine;

namespace LRF { namespace CoreLrfs {

class ANull : public ALrf {
public:
  ANull(ALrfTypeID id) : ALrf(id) {}
  bool inDomain(const APoint &) const override { return false; }
  double eval(const APoint &) const override { return 0; }
  double sigma(const APoint &) const override { return 0; }
  ALrf *clone() const override { return new ANull(type()); }
  void transform(const ATransform &) override { }

  void getAxialRange(APoint &center, double &min, double &max) const override
  {
    center = APoint();
    min = 0;
    max = 0;
  }
  void getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const override
  {
    xmin = 0;
    xmax = 0;
    ymin = 0;
    ymax = 0;
  }
};

class AAxial : public ARelativeLrf {
protected:
  Bspline3 bsr; // spline describing radial dependence
  Bspline3 bse; // spline describing radial error dependence
  double rmax2;	// domain check (actually comes from bsr and is just cached)
  bool flattop; // creation settings for readback

protected:
  double distance(const APoint &pos) const;

public:
  AAxial(ALrfTypeID type, const Bspline3 &bsr, const Bspline3 &bse,
         bool flattop, const ATransform &t = ATransform());

  bool inDomain(const APoint &pos) const override;
  double eval(const APoint &pos) const override;
  double sigma(const APoint &pos) const override;
  ALrf *clone() const override;

  void getAxialRange(APoint &center, double &min, double &max) const override;
  void getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const override;

  int getNint() const { return bsr.GetNint(); }
  double getRmax() const { return sqrt(rmax2); }
  bool isFlattop() const { return flattop; }

  const Bspline3 &getBsr() const { return bsr; }
  const Bspline3 &getBse() const { return bse; }
};


///
/// \brief The ALrfAxialCompressed class.
/// \details Technically this is not needed, since it's the most used case, and
///  it is possible to have a neutral compression, so regular Axial could
///  always be compressed, but because I only realized this later, and for the
///  sake of example, this class was made and kept.
///
class AxialCompressed : public AAxial {
protected:
  AVladimirCompression compress;
public:
  AxialCompressed(const AAxial &uncompressed, const AVladimirCompression &compressor);

  double eval(const APoint &pos) const override;
  double sigma(const APoint &pos) const override;
  ALrf *clone() const override;

  //Used to write to json and show settings
  const AVladimirCompression &getCompressor() const { return compress; }
};


class AAxial3D : public ARelativeLrf {
protected:
  TPspline3 bsr;        // 2D spline describing r+z LRF dependence
  TPspline3 bse;        // 2D spline describing r+z error dependence
  AVladimirCompression compress;
  double rmax2;
  double zmin, zmax; 	  // zrange
  double dz;

public:
  AAxial3D(ALrfTypeID type, const TPspline3 &bsr, const TPspline3 &bse,
           const AVladimirCompression &compressor, const ATransform &t = ATransform());

#ifdef Q_OS_WIN32
  void* operator new(size_t i)
      {
          return _mm_malloc(i,16);
      }
  void operator delete(void* p)
      {
          _mm_free(p);
      }
#endif

  bool inDomain(const APoint &pos) const override;
  double eval(const APoint &pos) const override;
  double sigma(const APoint &pos) const override;
  ALrf *clone() const override;

  void getAxialRange(APoint &center, double &min, double &max) const override;
  void getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const override;

  int getNintXY() const { return bsr.GetNintX(); }
  int getNintZ() const { return bsr.GetNintY(); }

  //Used to write to json
  const TPspline3 &getBsr() const { return bsr; }
  const TPspline3 &getBse() const { return bse; }
  const AVladimirCompression &getCompressor() const { return compress; }
};


class Axy : public ARelativeLrf {
protected:
  TPspline3 bsr;        // 2D spline describing r+z LRF dependence
  TPspline3 bse;        // 2D spline describing r+z error dependence
  double xmin, xmax; 	// xrange
  double ymin, ymax; 	// yrange

public:
  Axy(ALrfTypeID type, const TPspline3 &bsr, const TPspline3 &bse,
      const ATransform &t = ATransform());

#ifdef Q_OS_WIN32
  void* operator new(size_t i)
      {
          return _mm_malloc(i,16);
      }
  void operator delete(void* p)
      {
          _mm_free(p);
      }
#endif

  bool inDomain(const APoint &pos) const override;
  double eval(const APoint &pos) const override;
  double sigma(const APoint &pos) const override;
  ALrf *clone() const override;

  void getAxialRange(APoint &center, double &min, double &max) const override;
  void getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const override;

  int getNintX() const { return bsr.GetNintX(); }
  int getNintY() const { return bsr.GetNintY(); }
  void getMinMax(double &xMin, double &xMax, double &yMin, double &yMax) const { xMin = xmin; xMax = xmax; yMin = ymin; yMax = ymax; }

  //Used to write to json
  const TPspline3 &getBsr() const { return bsr; }
  const TPspline3 &getBse() const { return bse; }
};


//Using a set of Axy could be a bit simpler, but let's keep them independent
class ASlicedXY : public ARelativeLrf {
protected:
  std::vector<TPspline3> bsr;  // 2D splines describing xy+z LRF dependence
  std::vector<TPspline3> bse;  // 2D splines describing xy+z error dependence
  double xmin, xmax; 	         // xrange
  double ymin, ymax; 	         // yrange
  double zmin, zmax; 	         // zrange
  double dz;                   // slice width
  double zbot;                 // middle of the bottom slice
  double ztop;                 // middle of the top slice

  double getLayers(double z, int &lower_layer) const;
public:
  ASlicedXY(ALrfTypeID type, double zmin, double zmax, const std::vector<TPspline3> &bsr,
            const std::vector<TPspline3> &bse, const ATransform &t = ATransform());

  bool inDomain(const APoint &pos) const override;
  double eval(const APoint &pos) const override;
  double sigma(const APoint &pos) const override;
  ALrf *clone() const override;

  void getAxialRange(APoint &center, double &min, double &max) const override;
  void getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const override;
  double getZMin() const { return zmin; }
  double getZMax() const { return zmax; }

  //Used to write to json
  const std::vector<TPspline3> &getBsr() const { return bsr; }
  const std::vector<TPspline3> &getBse() const { return bse; }
};


class AScriptParamInfo {
public:
  QScriptString name;
  double init, min, max;
  AScriptParamInfo() : init(0), min(0), max(0) {}
  AScriptParamInfo(QScriptString name, double init = 0, double min = 0, double max = 0)
    : name(name), init(init), min(min), max(max) { }
};
//TODO: provide better debuggin capabilities by outputting script exceptions
class AScript : public ARelativeLrf
{
protected:
  enum class EvalArgument {
    local_coords,
    global_coords,
    unknown
  };

  QScriptString name;
  QScriptString name_sigma;
  QScriptValue script_var;
  QScriptValue script_var_sigma;

  //These are pointers so that they can be call'ed on const functions
  std::shared_ptr<QScriptValue> lrf_collection;
  std::unique_ptr<QScriptValue> script_eval;
  std::unique_ptr<QScriptValue> script_sigma;

  //Caching of eval parameter names
  std::vector<EvalArgument> arguments;

  //For save/load
  QString script;
  //std::vector<ParamInfo> params;

public:
  static std::vector<AScriptParamInfo> getScriptParams(QScriptValue script_var);
  static QStringList getFunctionArgumentNames(QScriptValue &function);

  AScript(ALrfTypeID type, std::shared_ptr<QScriptValue> lrf_collection, QScriptString name,
          const QString &script, /*std::vector<ParamInfo> params,*/ const ATransform &t = ATransform());

  //No copy problem because we use unique_ptr. If that is removed, put this destructor in a shared_ptr deleter.
  virtual ~AScript();

  QScriptString deepCopyScriptVar(QScriptValue *dest = nullptr) const;
  QScriptString deepCopyScriptVarSigma(QScriptValue *dest = nullptr) const;

  void setSigmaVar(QScriptString name);

  QScriptString getName() const { return name; }
  const QString &getScript() const { return script; }
  //const std::vector<ParamInfo> &getParamsInfo() const { return params; }
  const QScriptValue getScriptVar() const { return script_var; }
  const QScriptValue getScriptVarSigma() const { return script_var_sigma; }
};

class AScriptPolar : public AScript
{
  QScriptValueList makeArguments(const APoint &pos) const;
protected:
  double rmax;
public:
  AScriptPolar(ALrfTypeID type, std::shared_ptr<QScriptValue> lrf_collection, QScriptString name,
            const QString &script, double rmax,
            const ATransform &t = ATransform());

  bool inDomain(const APoint &pos) const override;
  double eval(const APoint &pos) const override;
  double sigma(const APoint &pos) const override;
  ALrf *clone() const override;

  void getAxialRange(APoint &center, double &min, double &max) const override;
  void getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const override;

  double getRmax() const { return rmax; }
};

class AScriptPolarZ : public AScriptPolar
{
  QScriptValueList makeArguments(const APoint &pos) const;
public:
  AScriptPolarZ(ALrfTypeID type, std::shared_ptr<QScriptValue> lrf_collection, QScriptString name,
            const QString &script, double rmax,
            const ATransform &t = ATransform());

  double eval(const APoint &pos) const override;
  double sigma(const APoint &pos) const override;
  ALrf *clone() const override;
};

class AScriptCartesian : public AScript
{
  QScriptValueList makeArguments(const APoint &pos) const;
protected:
  double xmin, xmax; 	// xrange
  double ymin, ymax; 	// yrange
public:
  AScriptCartesian(ALrfTypeID type, std::shared_ptr<QScriptValue> lrf_collection, QScriptString name,
            const QString &script, double xmin,
            double xmax, double ymin, double ymax, const ATransform &t = ATransform());

  bool inDomain(const APoint &pos) const override;
  double eval(const APoint &pos) const override;
  double sigma(const APoint &pos) const override;
  ALrf *clone() const override;

  void getAxialRange(APoint &center, double &min, double &max) const override;
  void getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const override;

  double getXmin() const { return xmin; }
  double getXmax() const { return xmax; }
  double getYmin() const { return ymin; }
  double getYmax() const { return ymax; }
};


class AScriptCartesianZ : public AScriptCartesian
{
  QScriptValueList makeArguments(const APoint &pos) const;
public:
  AScriptCartesianZ(ALrfTypeID type, std::shared_ptr<QScriptValue> lrf_collection, QScriptString name,
            const QString &script, double xmin,
            double xmax, double ymin, double ymax, const ATransform &t = ATransform());

  double eval(const APoint &pos) const override;
  double sigma(const APoint &pos) const override;
  ALrf *clone() const override;
};

} } //namespace LRF::CoreLrfs

#endif // LRFCORELRFS_H
