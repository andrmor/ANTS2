#include "compress.h"
#include "jsonparser.h"
#include <QJsonObject>

Compress1d* Compress1d::Factory(QJsonObject &json)
{
    JsonParser parser(json);
    QString str;

    parser.ParseObject("method", str);

    if (str == "dualslope")
        return new DualSlopeCompress(json);

    return NULL;
}

DualSlopeCompress::DualSlopeCompress(double k, double r0, double lam)
{
    this->r0 = r0;
    this->k = k;
    this->lam = lam;

    a = (k+1)/(k-1);
    lam2 = lam*lam;
    b = sqrt(r0*r0+lam2)+a*r0;
}

DualSlopeCompress::DualSlopeCompress(QJsonObject &json)
{
    JsonParser parser(json);
    parser.ParseObject("r0", r0);
    parser.ParseObject("k", k);
    parser.ParseObject("lam", lam);

    a = (k+1)/(k-1);
    lam2 = lam*lam;
    b = sqrt(r0*r0+lam2)+a*r0;
}

double DualSlopeCompress::Rho(double r) const
{
    double dr = r - r0;
    return std::max(0., b + dr*a - sqrt(dr*dr + lam2));
}

double DualSlopeCompress::RhoDrv(double r) const
{
    double dr = r - r0;
    return dr/sqrt(dr*dr + lam2) + a;
}

void DualSlopeCompress::writeJSON(QJsonObject &json) const
{
    json["method"] = "dualslope";
    json["r0"] = r0;
    json["lam"] = lam;
    json["k"] = k;
}

QJsonObject DualSlopeCompress::reportSettings() const
{
    QJsonObject json;
    json["r0"] = r0;
    json["comp_lam"] = lam;
    json["comp_k"] = k;

    return json;
}

