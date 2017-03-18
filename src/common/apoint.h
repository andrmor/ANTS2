#ifndef APOINT_H
#define APOINT_H

#include <QJsonObject>
#include <QJsonArray>

#include <cmath>

//TODO: Consider using Eigen::Vector3d. Also to/fromJson() should be pure functions
struct APoint {
  double r[3];
  double &x() { return r[0]; }
  double &y() { return r[1]; }
  double &z() { return r[2]; }
  double x() const { return r[0]; }
  double y() const { return r[1]; }
  double z() const { return r[2]; }

  double &operator[](int index) { return r[index]; }
  double operator[](int index) const { return r[index]; }

  explicit APoint(const double *r_) { for(int i = 0; i < 3; i++) r[i] = r_[i]; }
  APoint(double x = 0, double y = 0, double z = 0) { r[0] = x; r[1] = y; r[2] = z; }
  APoint(const QJsonValue &json) { fromJson(json); }
  void fromJson(const QJsonValue &json) {
    QJsonArray pos = json.toArray();
    //Using at() is safe in out-of-bounds, and to*() return default value if invalid.
    for(int i = 0; i < 3; i++) r[i] = pos.at(i).toDouble(0);
  }

  double norm() const { return sqrt(r[0]*r[0] + r[1]*r[1] + r[2]*r[2]); }
  double normxy() const { return hypot(r[0], r[1]); }

  double normSq() const { return r[0]*r[0] + r[1]*r[1] + r[2]*r[2]; }
  double normxySq() const { return r[0]*r[0] + r[1]*r[1]; }

  double angleToOrigin() const {
    double phi = atan2(r[1], r[0]);
    return phi < -1.0e-6 ? 2.*3.14159265358979323846 + phi : phi;
  }

  static double angleToOrigin(double x, double y) {
    double phi = atan2(y, x);
    return phi < -1.0e-6 ? 2.*3.14159265358979323846 + phi : phi;
  }

  APoint &operator+=(const APoint &other)
  { for(int i = 0; i < 3; ++i) r[i] += other.r[i]; return *this; }
  APoint operator+(const APoint &other) const
  { return APoint(r[0] + other.r[0], r[1] + other.r[1], r[2] + other.r[2]); }

  APoint &operator-=(const APoint &other)
  { for(int i = 0; i < 3; ++i) r[i] -= other.r[i]; return *this; }
  APoint operator-(const APoint &other) const
  { return APoint(r[0] - other.r[0], r[1] - other.r[1], r[2] - other.r[2]); }

  APoint operator-() const
  { return APoint(-r[0], -r[1], -r[2]); }

  QJsonValue toJson() const {
    QJsonArray array;
    array.append(r[0]);
    array.append(r[1]);
    array.append(r[2]);
    return array;
  }
};

#endif // APOINT_H
