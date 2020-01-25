#ifndef DOTSTGEOSTRUCT
#define DOTSTGEOSTRUCT

struct DotsTGeoStruct
{
  double r[3]; //position

  DotsTGeoStruct(const double* r) {this->r[0]=r[0]; this->r[1]=r[1]; this->r[2]=r[2];}
  DotsTGeoStruct(double x, double y, double z) {this->r[0]=x; this->r[1]=y; this->r[2]=z;}
  DotsTGeoStruct() {}
};

#endif // DOTSTGEOSTRUCT

