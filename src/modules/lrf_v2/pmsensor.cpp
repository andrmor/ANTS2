#include "pmsensor.h"
#include "jsonparser.h"

#include <QDebug>
#include <QJsonObject>

#include <math.h>

PMsensor::PMsensor(int index) : index(index), gain(1.), lrf(0)
{
    NullTransform();
}

PMsensor::PMsensor() : gain(1.), lrf(0)
{
    NullTransform();
}

//PMsensor::PMsensor(QJsonObject &json)
//{
//    readJSON(json);
//}

void PMsensor::SetPhi(double phi)
{
    this->phi = phi;
    sinphi = sin(phi);
    cosphi = cos(phi);
}

void PMsensor::writeJSON(QJsonObject &json) const
{
    json["iPM"] = index;
    json["gain"] = gain;

    QJsonObject transform;
      transform["dx"] = dx;
      transform["dy"] = dy;
      transform["phi"] = phi;
      transform["flip"] = flip;
      json["transform"] = transform;
}

void PMsensor::readOldJSON(QJsonObject &json, int *group, const double *lrfx, const double *lrfy)
{
    index = json["id"].toInt();
    *group = json["group"].toInt();
    gain = json["gain"].toDouble();

    QJsonObject trobj = json["transform"].toObject();
    SetTransform(trobj["dx"].toDouble()-lrfx[*group], trobj["dy"].toDouble()-lrfy[*group], trobj["phi"].toDouble(), trobj["flip"].toBool());
}

void PMsensor::readJSON(QJsonObject &json)
{
    index = json["iPM"].toInt();
    gain = json["gain"].toDouble();

    QJsonObject trobj = json["transform"].toObject();
    SetTransform(trobj["dx"].toDouble(), trobj["dy"].toDouble(), trobj["phi"].toDouble(), trobj["flip"].toBool());
}

void PMsensor::SetTransform(double dx_, double dy_, double phi_, bool flip_)
{
    dx = dx_;
    dy = dy_;
    phi = phi_;
    sinphi = sin(phi);
    cosphi = cos(phi);
    flip = flip_;
}

void PMsensor::NullTransform()
{
    dx = dy = phi = 0;
    sinphi = 0; cosphi = 1;
    flip = false;
}

// Three-stage transform: (in this order)
// 1) shift
// 2) rotation around the origin
// 3) flip (mirror reflection) around X axis
//
// not sure if it's an optimal order
// but for now I can't see any reason to mix shifts and rotations
// so probably no difference

// convention:
// pos_world - position in the detector (input)
// pos_local - position relative to PMT (output)

void PMsensor::transform(const double *pos_world, double *pos_local) const
{
    double x = pos_world[0] + dx;
    double y = pos_world[1] + dy;
    pos_local[0] = x*cosphi - y*sinphi;
    if (!flip)
        pos_local[1] = x*sinphi + y*cosphi;
    else
        pos_local[1] = -x*sinphi - y*cosphi;
    pos_local[2] = pos_world[2];
}

void PMsensor::transformBack(const double *pos_local, double *pos_world) const
{
   double x = pos_local[0]*cosphi + pos_local[1]*sinphi;
   double y;
   if (!flip)
     y = -pos_local[0]*sinphi + pos_local[1]*cosphi;
   else
     y = +pos_local[0]*sinphi - pos_local[1]*cosphi;

   pos_world[0] = x - dx;
   pos_world[1] = y - dy;
   pos_world[2] = pos_local[2];
}

/*
double *PMsensor::transform(const double *pos_world) const
{
    static double pos_local[3];
    transform(pos_world, pos_local);
    return pos_local;
}
*/

void PMsensor::getGlobalMinMax(double *minmax) const
{
    double xmin = lrf->getXmin();
    double xmax = lrf->getXmax();
    double ymin = lrf->getYmin();
    double ymax = lrf->getYmax();

    //corners in local
    double tl[3] = {xmin, ymax, 0};  //local top-left
    double tr[3] = {xmax, ymax, 0};
    double bl[3] = {xmin, ymin, 0};
    double br[3] = {xmax, ymin, 0};

    //corners in global
    double gtl[3];  //global top-left
    double gtr[3];
    double gbl[3];
    double gbr[3];
    transformBack(tl, gtl);
    transformBack(tr, gtr);
    transformBack(bl, gbl);
    transformBack(br, gbr);

    minmax[0] = std::min( std::min(gtl[0], gtr[0]), std::min(gbl[0], gbr[0])); //min x global
    minmax[1] = std::max( std::max(gtl[0], gtr[0]), std::max(gbl[0], gbr[0])); //max x global
    minmax[2] = std::min( std::min(gtl[1], gtr[1]), std::min(gbl[1], gbr[1])); //min y global
    minmax[3] = std::max( std::max(gtl[1], gtr[1]), std::max(gbl[1], gbr[1])); //max y global

    /*
    double local[3], globalmin[3], globalmax[3];
    local[0] = lrf->getXmin();
    local[1] = lrf->getYmin();
    local[2] = 0;
    transformBack(local, globalmin);
    local[0] = lrf->getXmax();
    local[1] = lrf->getYmax();
    transformBack(local, globalmax);
    for(int i = 0; i < 2; i++)
    {
        if(globalmin[i] < globalmax[i])
        {
            minmax[0] = globalmin[i];
            minmax[1] = globalmax[i];
        }
        else
        {
            minmax[0] = globalmax[i];
            minmax[1] = globalmin[i];
        }
        minmax += 2;
    }
    */
}

bool PMsensor::inDomain(double *pos_world) const
{
  ///return lrf->inDomain(transform(pos_world));
  double pos_local[3];
  transform(pos_world, pos_local);
  return lrf->inDomain(pos_local);
}

double PMsensor::eval(const double *pos_world) const
{
  ///return lrf->eval(transform(pos_world))*gain;
  double pos_local[3];
  transform(pos_world, pos_local);
// VS(8/12/2018) -- a hack to fix the problem with negative lrfs
// problem stems from the fact that we return 0 to signal
// out of domain situation and then test for <= 0
// so I convert negatives to small positive values here
  double val = lrf->eval(pos_local)*gain;
  return val < 0 ? 1e-6 : val;
}

double PMsensor::evalErr(const double *pos_world) const
{
  ///return lrf->evalErr(transform(pos_world))*gain;
  double pos_local[3];
  transform(pos_world, pos_local);
  return lrf->evalErr(pos_local)*gain;
}

double PMsensor::evalDrvX(double *pos_world) const
{
  ///return lrf->evalDrvX(transform(pos_world))*gain;
  double pos_local[3];
  transform(pos_world, pos_local);
  return lrf->evalDrvX(pos_local)*gain;
}

double PMsensor::evalDrvY(double *pos_world) const
{
  ///return lrf->evalDrvY(transform(pos_world))*gain;
  double pos_local[3];
  transform(pos_world, pos_local);
  return lrf->evalDrvY(pos_local)*gain;

}

void PMsensor::GetTransform(double *dx_, double *dy_, double *phi_, bool *flip_) const
{
  *dx_ = dx;
  *dy_ = dy;
    *phi_ = phi;
    *flip_ = flip;
}
