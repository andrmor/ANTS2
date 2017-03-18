#ifndef ATRANSFORM_H
#define ATRANSFORM_H

#include "apoint.h"

class ATransform {
  APoint shift;
  double phi;     //rotation
  double sinphi;  //sin(phi);
  double cosphi;  //cos(phi);
  bool flip;      //mirror: 180º on theta if it were implemented

  //Too dangerous? It means rotate by phi first, and then shift
  ATransform(double phi, const APoint &shift = APoint(), bool flip = false) :
    shift(shift), phi(phi), sinphi(sin(phi)), cosphi(cos(phi)), flip(flip) {}
public:
  ATransform(const APoint &shift = APoint(), double phi = 0, bool flip = false) :
    phi(phi), sinphi(sin(phi)), cosphi(cos(phi)), flip(flip)
  {
    this->shift.x() = cosphi*shift.x() - sinphi*shift.y();
    this->shift.y() = sinphi*shift.x() + cosphi*shift.y();
    this->shift.z() = shift.z();
  }
  ATransform(const QJsonObject &json) { fromJson(json); }

  APoint getShift() const { return shift; }
  void setShift(const APoint &shift) { this->shift = shift; }
  double getPhi() const { return phi; }
  void setPhi(double phi)
  {
    this->phi = phi;
    cosphi = cos(phi);
    sinphi = sin(phi);
  }
  bool getFlip() const { return flip; }
  void setFlip(bool flip) { this->flip = flip; }

  static ATransform fromJson(const QJsonObject &json) {
    ATransform t;
    t.shift.fromJson(json["shift"]);
    t.phi = json["phi"].toDouble(0);
    t.sinphi = sin(t.phi);
    t.cosphi = cos(t.phi);
    t.flip = json["flip"].toBool(false);
    return t;
  }

  QJsonValue toJson() const {
    QJsonObject json;
    json["shift"] = shift.toJson();
    json["phi"] = phi;
    json["flip"] = flip;
    return json;
  }

  APoint transform(const APoint &p) const
  {
    double x = cosphi*p.x() - sinphi*p.y() + shift.x();
    double y = sinphi*p.x() + cosphi*p.y() + shift.y();
    return APoint(x, flip ? -y : y, p.z() + shift.z());
  }

  APoint inverse(const APoint &p) const
  {
    const double p_y = flip ? -p.y() : p.y();
    const double x =  cosphi * (p.x() - shift.x()) + sinphi * (p_y - shift.y());
    const double y = -sinphi * (p.x() - shift.x()) + cosphi * (p_y - shift.y());
    return APoint(x, y, p.z() - shift.z());
  }

  //*this is applied to t, like you would do for a point
  ATransform transform(const ATransform &t) const
  {
    //transf_B * flip * transf_A = flip * transf_B(-phi, -Y) * transf_A
    //so we can move t.flip to the left and merge with this->flip.
    //Then we are left with the trivial transform multiplication
    bool flip = this->flip != t.flip;
    double phi, sinphi, shift_x, shift_y;
    if(t.flip) {
      phi = -this->phi;
      sinphi = -this->sinphi;
      shift_y = -this->shift.y();
    } else {
      phi = this->phi;
      sinphi = this->sinphi;
      shift_y = this->shift.y();
    }
    shift_x = cosphi*t.shift.x() - sinphi*t.shift.y() + shift.x();
    shift_y = sinphi*t.shift.x() + cosphi*t.shift.y() + shift_y;
    double shift_z = shift.z() + t.shift.z();
    return ATransform(phi+t.phi, APoint(shift_x, shift_y, shift_z), flip);
  }

  ATransform &invert()
  {
    double shift_x = -cosphi*shift.x() - sinphi*shift.y();
    if(flip) {
      shift.y() = -sinphi*shift.x() + cosphi*shift.y();
    } else {
      shift.y() = sinphi*shift.x() - cosphi*shift.y();
      phi = -phi;
      sinphi = -sinphi;
    }
    shift.x() = shift_x;
    shift.z() = -shift.z();
    return *this;
  }
};

#endif // ATRANSFORM_H
