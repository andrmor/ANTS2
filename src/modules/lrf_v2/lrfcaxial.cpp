#include "lrfcaxial.h"
#include "jsonparser.h"

#include <math.h>

LRFcAxial::LRFcAxial(double r, int nint,  double k, double r0, double lam) :
    LRFaxial(r, nint)
{
    this->r0 = r0;
    a = (k+1)/(k-1);
    lam2 = lam*lam;
    b = sqrt(r0*r0+lam2)+a*r0;

    comp_lam = lam;
    comp_k = k;
}

LRFcAxial::LRFcAxial(QJsonObject &json) : LRFaxial(json)
{
    JsonParser parser(json);
    parser.ParseObject("a", a);
    parser.ParseObject("b", b);
    parser.ParseObject("r0", r0);
    parser.ParseObject("lam2", lam2);
    if(!parser.ParseObject("comp_lam", comp_lam))
        comp_lam = sqrt(lam2);
    if(!parser.ParseObject("comp_k", comp_k))
        comp_k = (a+1)/(a-1); //funny how this turned out :)
}

QJsonObject LRFcAxial::reportSettings() const
{
   QJsonObject json(LRFaxial::reportSettings());
   json["r0"] = r0;
   json["comp_lam"] = comp_lam;
   json["comp_k"] = comp_k;

   return json;
}

double LRFcAxial::compress(double r) const
{
    double dr=r-r0;
    return std::max(0., b+dr*a-sqrt(dr*dr+lam2));
}

double LRFcAxial::comprDev(double r) const
{
    double dr=r-r0;
    return dr/sqrt(dr*dr+lam2)+a;
}

void LRFcAxial::writeJSON(QJsonObject &json) const
{
    LRFaxial::writeJSON(json);
    json["type"] = QString(type());
    json["a"] = a;
    json["b"] = b;
    json["r0"] = r0;
    json["lam2"] = lam2;
    json["comp_lam"] = comp_lam;
    json["comp_k"] = comp_k;
}
