#ifndef AGEOMARKERCLASS
#define AGEOMARKERCLASS

#include <QString>
#include "TPolyMarker3D.h"

class GeoMarkerClass : public TPolyMarker3D
{
public:
  QString Type; //Source, Nodes, Recon, Scan

  GeoMarkerClass(QString type, int style, int size, Color_t color) {configure(type, style, size, color);}
  GeoMarkerClass() {}
  void configure(QString type, int style, int size, Color_t color) {Type = type; SetMarkerStyle(style); SetMarkerSize(size); SetMarkerColor(color);}
};

#endif // AGEOMARKERCLASS

