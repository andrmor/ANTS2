#ifndef LRFCOMPRAXIAL_H
#define LRFCOMPRAXIAL_H

#include "lrfaxial.h"
//#include <algorithm>

class Bspline3;

class LRFcAxial : public LRFaxial
{
public:
//    LRFcAxial(double r, int nint_, double scale_);
    LRFcAxial(double r, int nint,  double k, double r0, double lam);
    virtual void writeJSON(QJsonObject &json) const;
    LRFcAxial(QJsonObject &json);
    virtual const char *type() const { return "ComprAxial"; }
    virtual QJsonObject reportSettings() const;

  //  double getCompr_k() {return comp_k;}
    double getCompr_r0() const { return r0; }
 //   double getCompr_lam() {return comp_lam;}
    double getCompr_a() const { return a; }
    double getCompr_b() const { return b; }
    double getCompr_lam2() const { return lam2; }

//    virtual double Compress(double r) {return log(r/scale+1.);}
//    virtual double ComprDev(double r) {return 1/(r+scale);}
//    virtual double Uncompress(double x) {return (exp(x)-1.)*scale;}
    virtual double compress(double r) const;
    virtual double comprDev(double r) const;

protected:
    double a;
    double b;
    double r0;
    double lam2;

    double comp_k;
    double comp_lam;
};

#endif // LRFCOMPRAXIAL_H
