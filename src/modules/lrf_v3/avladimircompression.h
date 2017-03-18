#ifndef AVLADIMIRCOMPRESSION_H
#define AVLADIMIRCOMPRESSION_H

#include <cmath>
#include <QJsonObject>

///
/// \brief Named after its creator :)
/// \details Think it's overkill to have a separate class for this? Maybe you
///  can use it in XY.
///
class AVladimirCompression {
  double k_, r0_, lam_;
  double a, b, lam2;

  void update() {
    a = (k_+1)/(k_-1);
    lam2 = lam_*lam_;
    b = sqrt(r0_*r0_+lam2)+r0_*a;
  }
public:
  //Default constructor has no effective compression (only comput. overhead)
  AVladimirCompression() : k_(3), r0_(0), lam_(0) { update(); }
  AVladimirCompression(double k, double r0, double lam)
    : k_(k), r0_(r0), lam_(lam) { update(); }

  explicit AVladimirCompression(const QJsonObject &json) {
    k_ = json["comp_k"].toDouble(3);
    r0_ = json["r0"].toDouble(0);
    lam_ = json["comp_lam"].toDouble(0);
    update();
  }

  void toJson(QJsonObject &json) const {
    json["r0"] = r0_;
    json["comp_k"] = k_;
    json["comp_lam"] = lam_;
  }

  bool isCompressing() const { return k_ != 3 || r0_ != 0 || lam_ != 0; }

  bool operator==(const AVladimirCompression &other) const
  { return k_ == other.k_ && r0_ == other.r0_ && lam_ == other.lam_; }
  bool operator!=(const AVladimirCompression &other) const
  { return k_ != other.k_ || r0_ != other.r0_ || lam_ != other.lam_; }

  //Uncompresses "space"
  double un(double cr) const {
    double a2m1 = a*a-1;
    double dcr = cr - b;
    return r0_ + (a*dcr + sqrt(lam2*a2m1 + dcr*dcr)) / a2m1;
  }

  double operator()(double r) const {
    double dr=r-r0_;
    return std::max(0., b+dr*a-sqrt(dr*dr+lam2));
    //return b+dr*a-sqrt(dr*dr+lam2);
  }

  double k()   const { return k_;   }
  double r0()  const { return r0_;  }
  double lam() const { return lam_; }
};

#endif // AVLADIMIRCOMPRESSION_H
