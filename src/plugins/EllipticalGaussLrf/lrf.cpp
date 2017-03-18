#include "lrf.h"

#include "apoint.h"

Lrf::Lrf(LRF::ALrfTypeID id, double rmax, double a, double s2x, double s2y, double tail,
         const ATransform &t, double e_a, double e_s2x, double e_s2y, double e_tail)
  : LRF::ARelativeLrf(id, t), rmax(rmax),
    a(a), s2x(s2x), s2y(s2y), tail(tail),
    e_a(e_a), e_s2x(e_s2x), e_s2y(e_s2y), e_tail(e_tail)
{ }

bool Lrf::inDomain(const APoint &) const
{
  return true;
}

double Lrf::eval(const APoint &pos) const
{
  APoint p = transf.inverse(pos);
  double expo = p[0]*p[0]/s2x + p[1]*p[1]/s2y;
  return a * std::exp(-0.5*expo) + tail;
}

void Lrf::getAxialRange(APoint &center, double &min, double &max) const
{
  center = transf.transform(APoint());
  min = 0;
  max = rmax;
}

void Lrf::getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const
{
  APoint center = transf.transform(APoint());
  xmin = center.x() - rmax;
  xmax = center.x() + rmax;
  ymin = center.y() - rmax;
  ymax = center.y() + rmax;
}

double Lrf::sigma(const APoint &pos) const
{
  APoint p = transf.inverse(pos);
  double expo = p[0]*p[0]/e_s2x + p[1]*p[1]/e_s2y;
  return e_a * std::exp(-0.5*expo) + e_tail;
}

LRF::ALrf *Lrf::clone() const
{
  return new Lrf(type(), rmax, a, s2x, s2y, tail, transf, e_a, e_s2x, e_s2y, e_tail);
}

void Lrf::setSigmaCoeffs(double a, double s2x, double s2y, double tail)
{
  e_a = a;
  e_s2x = s2x;
  e_s2y = s2y;
  e_tail = tail;
}
