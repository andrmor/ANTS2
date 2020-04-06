#include "lrfcomposite.h"
#include "lrfaxial.h"
#include "lrfxy.h"
#include "jsonparser.h"

#include <math.h>

#include <QDebug>
#include <QJsonObject>

void LRFcomposite::init_limits(LRF2 *lrf)
{
    xmin = lrf->getXmin();
    xmax = lrf->getXmax();
    ymin = lrf->getYmin();
    ymax = lrf->getYmax();
}

void LRFcomposite::update_limits(LRF2 *lrf)
{
    xmin = std::max(xmin, lrf->getXmin());
    xmax = std::min(xmax, lrf->getXmax());
    ymin = std::max(ymin, lrf->getYmin());
    ymax = std::min(ymax, lrf->getYmax());
}

LRFcomposite::LRFcomposite(LRF2 *lrf) : LRF2()
{
    lrfdeck.push_back(lrf);
    init_limits(lrf);
    fit_all = true;
}

LRFcomposite::LRFcomposite(QJsonObject &json) : LRF2()
{
    JsonParser parser(json);
    QJsonArray jsarr;
    QVector <QJsonObject> deck;
    QString type;
    LRF2 *lrf;

    if (parser.ParseObject("deck", jsarr)) {
        parser.ParseArray(jsarr, deck);
        for (int i=0; i<deck.size(); i++) {
            parser.SetObject(deck[i]);
            parser.ParseObject("type", type);
            if      (type == "Axial")      lrf = new LRFaxial(deck[i]);
            else if (type == "ComprAxial") lrf = new LRFcAxial(deck[i]);
         /* else if (type == "Axial3D")    lrf = new LRFaxial3D(deck[i]);*/
            else if (type == "XY")         lrf = new LRFxy(deck[i]);
            else {
                qWarning()<<"LRF in composite is not Axial or cAxial or XY. Skipping...";
                continue;
            }
            lrfdeck.push_back(lrf);
            if (i == 0)
                init_limits(lrf);
            else
                update_limits(lrf);
        }
    }

    valid = true; ///*** to do
}

LRFcomposite::~LRFcomposite()
{
    for (int i=0; i<(int)lrfdeck.size(); i++)
        delete lrfdeck[i];
    lrfdeck.clear();
}

bool LRFcomposite::inDomain(double x, double y, double z) const
{
    for (int i=0; i<(int)lrfdeck.size(); i++)
        if (!lrfdeck[i]->inDomain(x, y, z))
            return false;
    return true;
}

// Logic behind errors: it makes sense to calculate errors
// only for the last component

bool LRFcomposite::errorDefined() const
{
    // assume that error is stored in the last added component
    return lrfdeck.back()->errorDefined();
}

double LRFcomposite::getRmax() const
{
    return std::max(std::max(fabs(xmin), fabs(ymin)), std::max(fabs(xmax), fabs(ymax)));
}

double LRFcomposite::eval(double x, double y, double z) const
{
    if (!inDomain(x, y, z))
        return 0.;
    double sum = 0.;
    for (int i=0; i<(int)lrfdeck.size(); i++)
        sum += lrfdeck[i]->eval(x, y, z);
    return sum;
}

double LRFcomposite::evalErr(double x, double y, double z) const
{
    // assume that error is stored in the last added component
    return lrfdeck.back()->evalErr(x, y, z);
}

double LRFcomposite::eval(double x, double y, double z, double *err) const
{
    if (lrfdeck.size() == 0 || !inDomain(x, y, z))
        return 0.;
    // assume that error is stored in the last added component
    double sum  = lrfdeck.back()->eval(x, y, z, err);
    for (int i=0; i<(int)lrfdeck.size()-1; i++)
        sum += lrfdeck[i]->eval(x, y, z);

    return sum;
}

double LRFcomposite::evalDrvX(double x, double y, double z) const
{
    double sum = 0.;
    for (int i=0; i<(int)lrfdeck.size(); i++)
        sum += lrfdeck[i]->evalDrvX(x, y, z);
    return sum;
}

double LRFcomposite::evalDrvY(double x, double y, double z) const
{
    double sum = 0.;
    for (int i=0; i<(int)lrfdeck.size(); i++)
        sum += lrfdeck[i]->evalDrvY(x, y, z);
    return sum;
}

void LRFcomposite::add(LRF2* lrf)
{
    lrfdeck.push_back(lrf);
    update_limits(lrf);
}

double LRFcomposite::fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid)
{
    std::vector <double> vx;
    std::vector <double> vy;
    std::vector <double> vz;
    std::vector <double> va;
    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i], z[i]))
            continue;
        vx.push_back(x[i]);
        vy.push_back(y[i]);
        vz.push_back(z[i]);
        va.push_back(data[i]);
    }
// Fitting can work in two modes (selected by fit_all private variable):
//    I - fit all, i.e. fitting each LRF in turn
//    II - fit only last
    int residual;
    for (int i=0; i<(int)lrfdeck.size(); i++) {
        if (fit_all || i == lrfdeck.size()-1)
            residual = lrfdeck[i]->fit(va.size(), &vx[0], &vy[0], &vz[0], &va[0], grid);
        for (int j=0; j<(int)va.size(); j++)
            va[j] -= lrfdeck[i]->eval(vx[j], vy[j], vz[j]);
    }
    valid = true;

    return residual;
}

void LRFcomposite::writeJSON(QJsonObject &json) const
{
    json["type"] = QString(type());
    QJsonArray jsarr;
    QVector <QJsonObject> deck;
    deck.resize(lrfdeck.size());
    for (int i=0; i<(int)lrfdeck.size(); i++) {
        lrfdeck[i]->writeJSON(deck[i]);
        jsarr.append(deck[i]);
    }
    json["deck"] = jsarr;
}

QJsonObject LRFcomposite::reportSettings() const
{
   QJsonObject json;
   json["type"] = QString(type());

   QJsonArray jsarr;
   QVector <QJsonObject> deck;
   deck.resize(lrfdeck.size());
   for (int i=0; i<(int)lrfdeck.size(); i++)
       jsarr.append( lrfdeck[i]->reportSettings() );
   json["deck"] = jsarr;

   return json;
}
