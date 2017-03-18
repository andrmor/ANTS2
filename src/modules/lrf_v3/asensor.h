#ifndef LRFASENSOR_H
#define LRFASENSOR_H

#include <vector>
#include <memory>

#include "alrf.h"

namespace LRF {

class ASensor
{
public:
  struct Parcel {
    double coeff;
    std::shared_ptr<const ALrf> lrf;
  };

  static ASensor fromJson(const QJsonObject &state, bool *ok = nullptr);

  int ipm;
  std::vector<Parcel> deck;

  explicit ASensor(int ipm = 0) : ipm(ipm) { }
  explicit ASensor(int ipm, double coeff, std::shared_ptr<const ALrf> lrf) : ipm(ipm) {
    Parcel p;
    p.coeff = coeff;
    p.lrf = lrf;
    deck.push_back(p);
  }

  QJsonObject toJson() const;
  ASensor copyToCurrentThread() const;
  bool isCudaCapable() const;

  double eval(const APoint &pos) const {
    double sum = 0;
    for(auto &parcel : deck)
      sum += parcel.coeff * parcel.lrf->eval(pos);
    return sum;
  }

  bool inDomain(const APoint &pos) const {
    for(auto &parcel : deck) {
      if(!parcel.lrf->inDomain(pos))
        return false;
    }
    return true;
  }

  //TODO: Not sure if this is mathematically correct, but skip for now
  double sigma(const APoint &pos) const {
    double sum = 0;
    for(auto &parcel : deck)
      sum += parcel.coeff * parcel.lrf->sigma(pos);
    return sum;
  }

  ///
  /// \brief
  /// \details Since it's not possible to generally convert two donuts with
  ///  different centers into one donut, the center parameter is discarded.
  ///  Assume the center is the position of the respective sensor.
  ///
  void getAxialRange(double &min, double &max) const
  {
    double minimum, maximum;
    min = 0; max = std::numeric_limits<double>::infinity();
    for(auto &parcel : deck) {
      APoint center;
      parcel.lrf->getAxialRange(center, minimum, maximum);
      min = std::max(minimum, min);
      max = std::min(maximum, max);
    }
  }

  void getXYRange(double &xmin, double &xmax, double &ymin, double &ymax) const
  {
    double xminimum, xmaximum, yminimum, ymaximum;
    xmin = ymin = -std::numeric_limits<double>::infinity();
    ymax = xmax = +std::numeric_limits<double>::infinity();
    for(auto &parcel : deck) {
      parcel.lrf->getXYRange(xminimum, xmaximum, yminimum, ymaximum);
      xmin = std::max(xminimum, xmin);
      xmax = std::min(xmaximum, xmax);
      ymin = std::max(yminimum, ymin);
      ymax = std::min(ymaximum, ymax);
    }
  }

  bool operator<(const ASensor &other) const { return ipm < other.ipm; }
};

class ASensorGroup
{
  std::vector<ASensor> sensors;
public:
  ASensorGroup() {}
  explicit ASensorGroup(const QJsonValue &state);
  QJsonValue toJson() const;
  bool isCudaCapable() const;

  size_t size() const { return sensors.size(); }
  ASensor &operator[](int i) { return sensors[i]; }
  const ASensor &operator[](int i) const { return sensors[i]; }

  ASensor *getSensor(int ipm);
  const ASensor *getSensor(int ipm) const;

  const ASensorGroup *copyToCurrentThread() const;

  void stack(const ASensor &sensor);
  void stack(const ASensorGroup &group);

  void overwrite(const ASensor &sensor);
  void overwrite(const ASensorGroup &group);
};

}

#endif // LRFASENSOR_H
