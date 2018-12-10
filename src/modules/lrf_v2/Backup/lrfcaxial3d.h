#ifndef LRFCAXIAL3D_H
#define LRFCAXIAL3D_H

#include "lrfaxial3d.h"

class LRFcAxial3d : public LRFaxial3d
{
public:
    LRFcAxial3d(double r, int nint_, double z_min,
              double z_max, int n_intz,  double k, double r0, double lam);
    virtual void writeJSON(QJsonObject &json) const;
    LRFcAxial3d(QJsonObject &json);
    virtual const char *type() const { return "ComprAxial3D"; }
    virtual QJsonObject reportSettings() const;

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

#endif // LRFCAXIAL3D_H
