#ifndef LRF_H
#define LRF_H

#include "alrf.h"

class Lrf : public LRF::ARelativeLrf
{
public:
  double rmax; //For drawing
  double a, s2x, s2y, tail;
  double e_a, e_s2x, e_s2y, e_tail;
  Lrf(LRF::ALrfTypeID id, double rmax, double a, double s2x, double s2y, double tail,
      const ATransform &t = ATransform(), double e_a = 0, double e_s2x = 1, double e_s2y = 1, double e_tail = 0);

  bool inDomain(const APoint &pos) const override;
  double eval(const APoint &pos) const override;

  void getAxialRange(APoint &center, double &min, double &max) const override;
  void getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const override;

  double sigma(const APoint &pos) const override;

  LRF::ALrf *clone() const override;

  void setSigmaCoeffs(double a, double s2x, double s2y, double tail);
};

#endif // LRF_H
