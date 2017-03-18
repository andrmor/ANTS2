#ifndef LRF_ALRF_H
#define LRF_ALRF_H

#include "alrftype.h"
#include "atransform.h"

class ATransform;

namespace LRF {

///
/// \brief ALrf is used to obtain the average signal and its sigma value of a
///  sensor as a function of the event position.
/// \details The ALrf class only contains methods for direct usage of Lrfs in
///  reconstruction. Storage, loading, and settings exposition of ALrf
///
class ALrf {
  ALrfTypeID id;
public:
  ALrf(ALrfTypeID id) : id(id) { }
  ALrfTypeID type() const { return id; }

  virtual ~ALrf() { }
  virtual bool inDomain(const APoint &pos) const = 0;
  virtual double eval(const APoint &pos) const = 0;

  ///
  /// \brief Gets the minimum and maximum distance, relative to center, that an
  ///  event can be, in order for it to be successfully evaluated. Only used for
  ///  radial drawing.
  ///
  virtual void getAxialRange(APoint &center, double &min, double &max) const = 0;

  ///
  /// \brief Gets the maximum valid boundaries of the Lrf. Only used for XY drawing.
  ///
  virtual void getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const = 0;

  //Sigma of 0 means it's not defined. Is this acceptable?
  virtual double sigma(const APoint &pos) const = 0;

  virtual void eval(size_t count, const APoint *pos, double *result) const
  { for(size_t i = 0; i < count; ++i) result[i] = eval(pos[i]); }

  ///
  /// \brief Creates a copy of the lrf. Changes made to the original do not
  ///  propagate to the clone and vice-versa. The clone is only required to be
  ///  valid in the lrf's thread (see ALrfType::copyToCurrentThread).
  /// \return The clone of the lrf.
  ///
  virtual ALrf *clone() const = 0;

  ///
  /// \brief Allocates a new ALrf which is a spatially transformed clone of the current LRF
  /// \details The returned ALrf will be owned by the caller function.
  /// \return The new transformed LRF.
  ///
  virtual void transform(const ATransform &t) = 0;
};


//For the cases where implementing optimized transform() is too troublesome/boring
class ARelativeLrf : public ALrf {
protected:
  ATransform transf;
public:
  ARelativeLrf(ALrfTypeID id, const ATransform &t = ATransform())
    : ALrf(id), transf(t) { }

  const ATransform &getTransform() const { return transf; }
  void transform(const ATransform &t) override { transf = t.transform(transf); }
};

}

#endif // LRF_ALRF_H
