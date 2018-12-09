#ifndef LRFFACTORY_H
#define LRFFACTORY_H

#include "lrfaxial.h"
#include "lrfaxial3d.h"
#include "lrfcaxial3d.h"
#include "lrfxy.h"
#include "lrfxyz.h"
#include "lrfcomposite.h"
#include "lrfsliced3d.h"

#include <QJsonObject>

class LRFfactory
{
public:
  static LRF2* createLRF(const char *type, QJsonObject &lrf_json)
  {
    if      (!strcmp(type, "Axial"))      return new LRFaxial(lrf_json);
    else if (!strcmp(type, "Radial"))     return new LRFaxial(lrf_json);   //compatibility
    else if (!strcmp(type, "ComprAxial")) return new LRFcAxial(lrf_json);
    else if (!strcmp(type, "ComprRad"))   return new LRFcAxial(lrf_json);  //compatibility
    else if (!strcmp(type, "Axial3D"))    return new LRFaxial3d(lrf_json);
    else if (!strcmp(type, "ComprAxial3D"))    return new LRFcAxial3d(lrf_json);
    else if (!strcmp(type, "Radial3D"))   return new LRFaxial3d(lrf_json); //compatibility
    else if (!strcmp(type, "XY"))         return new LRFxy(lrf_json);
    else if (!strcmp(type, "Freeform"))   return new LRFxy(lrf_json);      //compatibility
    else if (!strcmp(type, "XYZ"))        return new LRFxyz(lrf_json);
    else if (!strcmp(type, "Composite"))  return new LRFcomposite(lrf_json);
    else if (!strcmp(type, "Sliced3D"))   return new LRFsliced3D(lrf_json);
    else return 0;
  }

  static LRF2* copyLRF(const LRF2* lrf)
  {
    if (!lrf) return 0;

    const char *type = lrf->type();

    QJsonObject json;
    lrf->writeJSON(json);

    if      (!strcmp(type, "Axial"))      return new LRFaxial(json);
    else if (!strcmp(type, "Radial"))     return new LRFaxial(json);   //compatibility
    else if (!strcmp(type, "ComprAxial")) return new LRFcAxial(json);
    else if (!strcmp(type, "ComprRad"))   return new LRFcAxial(json);  //compatibility
    else if (!strcmp(type, "Axial3D"))    return new LRFaxial3d(json);
    else if (!strcmp(type, "ComprAxial3D")) return new LRFcAxial3d(json);
    else if (!strcmp(type, "Radial3D"))   return new LRFaxial3d(json); //compatibility
    else if (!strcmp(type, "XY"))         return new LRFxy(json);
    else if (!strcmp(type, "Freeform"))   return new LRFxy(json);      //compatibility
    else if (!strcmp(type, "XYZ"))        return new LRFxyz(json);
    else if (!strcmp(type, "Composite"))  return new LRFcomposite(json);
    else if (!strcmp(type, "Sliced3D"))   return new LRFsliced3D(json);
    else return 0;
  }
};



#endif // LRFFACTORY_H
