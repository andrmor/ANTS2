#ifndef COMPRESS_H
#define COMPRESS_H

#include <cmath>

class QJsonObject;

class Compress1d
{
public:
    Compress1d() {}
    virtual ~Compress1d() {}
    virtual double Rho(double r) const = 0;
    virtual double RhoDrv(double r) const = 0;

    virtual void writeJSON(QJsonObject &json) const = 0;
    virtual QJsonObject reportSettings() const = 0;

    static Compress1d* Factory(QJsonObject &json);
};

// Dual slope

class DualSlopeCompress : public Compress1d
{
public:
    DualSlopeCompress(double k, double r0, double lam);
    DualSlopeCompress(QJsonObject &json);
    virtual double Rho(double r) const;
    virtual double RhoDrv(double r) const;

    virtual void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;

private:
    double k;
    double r0;
    double lam;

    double a;
    double b;
    double lam2;
};

#endif // COMPRESS_H
