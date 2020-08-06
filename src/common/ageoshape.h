#ifndef AGEOSHAPE_H
#define AGEOSHAPE_H

#include <QString>
#include <QVector>

class TGeoShape;
class QJsonObject;
class QStringList;

class AGeoShape
{
public:
  AGeoShape() {}
  virtual ~AGeoShape() {}

  virtual bool readFromString(QString /*GenerationString*/) {return false;}

  //general: the same for all objects of the given shape
  virtual const QString getShapeType() const = 0;
  virtual const QString getShapeTemplate() {return "";}  //string used in auto generated help: Name(paramType param1, paramType param2, etc)
  virtual const QString getHelp() {return "";}

  //specific for this particular object
  virtual TGeoShape* createGeoShape(const QString /*shapeName*/ = "") {return 0;} //create ROOT TGeoShape
  virtual const QString getGenerationString() const {return "";} //return string which can be used to generate this object: e.g. Name(param1, param2, etc)
  virtual double getHeight() {return 0;}  //if 0, cannot be used for stack  ***!!!
  virtual void setHeight(double /*dz*/) {}
  virtual double maxSize() {return 0;} //for world size evaluation
  virtual double minSize() {return 0;} //for monitors only!
  virtual QString updateShape() {return "";}

  //json
  virtual void writeToJson(QJsonObject &/*json*/) const = 0;
  virtual void readFromJson(QJsonObject &/*json*/){}

  //from Tshape if geometry was loaded from GDML
  virtual bool readFromTShape(TGeoShape* /*Tshape*/) {return false;}

  virtual AGeoShape * clone() const; // without override it uses Factory and save/load to/from json  !!! do it for COMPOSITE !!!

protected:
  bool extractParametersFromString(QString GenerationString, QStringList& parameters, int numParameters);

public:
  static AGeoShape * GeoShapeFactory(const QString ShapeType);  // SHAPE FACTORY !!!

  static QList<AGeoShape*> GetAvailableShapes();               // list of available shapes for generation of help and highlighter: do not forget to add new here!

  static bool CheckPointsForArb8(QList<QPair<double, double> > V );
  QString updateParameter(QString str, double &returnValue);

};

// -------------- Particular shapes ---------------

class AGeoBox : public AGeoShape
{
public:
  AGeoBox(double dx, double dy, double dz) : dx(dx), dy(dy), dz(dz) {}
  AGeoBox() : dx(10), dy(10), dz(10) {}

  const QString getShapeType() const override {return "TGeoBBox";}
  virtual const QString getShapeTemplate() {return "TGeoBBox( dx, dy, dz )";}
  virtual bool readFromString(QString GenerationString);
  virtual const QString getHelp();
  QString updateShape() override;

  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();
  virtual double minSize() override;

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json) override;

  virtual bool readFromTShape(TGeoShape* Tshape);

  //AGeoShape * clone() const override;

  double      dx,     dy,     dz;
  QString str2dx, str2dy, str2dz;
};

class AGeoTube : public AGeoShape
{
public:
  AGeoTube(double rmin, double rmax, double dz) : rmin(rmin), rmax(rmax), dz(dz) {}
  AGeoTube(double r, double dz) : rmin(0), rmax(r), dz(dz) {}
  AGeoTube() : rmin(0), rmax(10), dz(5) {}

  const QString getShapeType() const override {return "TGeoTube";}
  virtual const QString getShapeTemplate() {return "TGeoTube( rmin, rmax, dz )";}
  virtual const QString getHelp();
  QString updateShape() override;

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();
  virtual double minSize() override;

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double rmin, rmax, dz;
  QString str2rmin, str2rmax, str2dz;
};

class AGeoScaledShape  : public AGeoShape
{
public:
  AGeoScaledShape(QString BaseShapeGenerationString, double scaleX, double scaleY, double scaleZ);
  AGeoScaledShape() {}
  virtual ~AGeoScaledShape() {delete BaseShape;}

  const QString getShapeType() const override {return "TGeoScaledShape";}
  virtual const QString getShapeTemplate() {return "TGeoScaledShape( TGeoShape(parameters), scaleX, scaleY, scaleZ )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  //virtual double getHeight() {return 0;}
  //virtual void setHeight(double /*dz*/) {}
  virtual const QString getGenerationString() const;
  virtual double maxSize() {return 0;}  //***!!!

  const QString getBaseShapeType() const;
  TGeoShape * generateBaseTGeoShape(const QString & BaseShapeGenerationString) const;

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  QString BaseShapeGenerationString;  //compatibility

  double scaleX, scaleY, scaleZ;
  AGeoShape * BaseShape = nullptr;
};

class AGeoParaboloid : public AGeoShape
{
public:
  AGeoParaboloid(double rlo, double rhi, double dz) :
    rlo(rlo), rhi(rhi), dz(dz) {}
  AGeoParaboloid() :
    rlo(0), rhi(40), dz(10)  {}
  virtual ~AGeoParaboloid() {}

  const QString getShapeType() const override {return "TGeoParaboloid";}
  virtual const QString getShapeTemplate() {return "TGeoParaboloid( rlo, rhi, dz )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double /*dz*/) {}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject &json) const override;
  virtual void readFromJson( QJsonObject &json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  //paraboloid specific
  double rlo, rhi, dz;
};

class AGeoConeSeg : public AGeoShape
{
public:
  AGeoConeSeg(double dz, double rminL, double rmaxL, double rminU, double rmaxU, double phi1, double phi2) :
    dz(dz), rminL(rminL), rmaxL(rmaxL), rminU(rminU), rmaxU(rmaxU), phi1(phi1), phi2(phi2) {}
  AGeoConeSeg() :
    dz(10), rminL(0), rmaxL(20), rminU(0), rmaxU(5), phi1(0), phi2(180) {}
  virtual ~AGeoConeSeg() {}

  const QString getShapeType() const override {return "TGeoConeSeg";}
  virtual const QString getShapeTemplate() {return "TGeoConeSeg( dz, rminL, rmaxL, rminU, rmaxU, phi1, phi2 )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape *Tshape);

  double dz;
  double rminL, rmaxL, rminU, rmaxU;
  double phi1, phi2;
};

class AGeoCone : public AGeoShape
{
public:
  AGeoCone(double dz, double rminL, double rmaxL, double rminU, double rmaxU) :
    dz(dz), rminL(rminL), rmaxL(rmaxL), rminU(rminU), rmaxU(rmaxU) {}
  AGeoCone(double dz, double rmaxL, double rmaxU) :
    dz(dz), rminL(0), rmaxL(rmaxL), rminU(0), rmaxU(rmaxU) {}
  AGeoCone() :
    dz(10), rminL(0), rmaxL(20), rminU(0), rmaxU(0) {}
  virtual ~AGeoCone() {}

  const QString getShapeType() const override {return "TGeoCone";}
  virtual const QString getShapeTemplate() {return "TGeoCone( dz, rminL, rmaxL, rminU, rmaxU )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject &json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double dz;
  double rminL, rmaxL, rminU, rmaxU;
};

class AGeoPolygon : public AGeoShape
{
public:
  AGeoPolygon(int nedges, double dphi, double dz, double rminL, double rmaxL, double rminU, double rmaxU) :
    nedges(nedges), dphi(dphi), dz(dz), rminL(rminL), rmaxL(rmaxL), rminU(rminU), rmaxU(rmaxU) {}
  AGeoPolygon(int nedges, double dz, double rmaxL, double rmaxU) :
    nedges(nedges), dphi(360), dz(dz), rminL(0), rmaxL(rmaxL), rminU(0), rmaxU(rmaxU) {}
  AGeoPolygon() :
    nedges(6), dphi(360), dz(10), rminL(0), rmaxL(20), rminU(0), rmaxU(20) {}
  virtual ~AGeoPolygon() {}

  const QString getShapeType() const override {return "TGeoPolygon";}
  virtual const QString getShapeTemplate() {return "TGeoPolygon( nedges, dphi, dz, rminL, rmaxL, rminU, rmaxU )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* /*Tshape*/) { return false; } //it is not a base root class, so not valid for import from GDML

  int nedges;
  double dphi, dz;
  double rminL, rmaxL, rminU, rmaxU;
};

struct APolyCGsection
{
  double z;
  double rmin, rmax;

  APolyCGsection() : z(0), rmin(0), rmax(100) {}
  APolyCGsection(double z, double rmin, double rmax) : z(z), rmin(rmin), rmax(rmax) {}

  bool fromString(QString string);
  const QString toString() const;
  void writeToJson(QJsonObject& json) const;
  void readFromJson(QJsonObject& json);
};

class AGeoPcon : public AGeoShape
{
public:
  AGeoPcon();
  virtual ~AGeoPcon() {}

  const QString getShapeType() const override {return "TGeoPcon";}
  virtual const QString getShapeTemplate() {return "TGeoPcon( phi, dphi, { z0 : rmin0 : rmaz0 }, { z1 : rmin1 : rmax1 } )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double phi, dphi;
  QVector<APolyCGsection> Sections;

};

class AGeoPgon : public AGeoPcon
{
public:
  AGeoPgon() :
    AGeoPcon(), nedges(6) {}
  virtual ~AGeoPgon() {}

  const QString getShapeType() const override {return "TGeoPgon";}
  virtual const QString getShapeTemplate() {return "TGeoPgon( phi, dphi, nedges, { z0 : rmin0 : rmaz0 }, { zN : rminN : rmaxN } )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  int nedges;
};

class AGeoTrd1 : public AGeoShape
{
public:
  AGeoTrd1(double dx1, double dx2, double dy, double dz) :
    dx1(dx1), dx2(dx2), dy(dy), dz(dz) {}
  AGeoTrd1() :
    dx1(15), dx2(5), dy(10), dz(10) {}
  virtual ~AGeoTrd1() {}

  const QString getShapeType() const override {return "TGeoTrd1";}
  virtual const QString getShapeTemplate() {return "TGeoTrd1( dx1, dx2, dy, dz )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double dx1, dx2, dy, dz;
};

class AGeoTrd2 : public AGeoShape
{
public:
  AGeoTrd2(double dx1, double dx2, double dy1, double dy2, double dz) :
    dx1(dx1), dx2(dx2), dy1(dy1), dy2(dy2), dz(dz) {}
  AGeoTrd2() :
    dx1(15), dx2(5), dy1(10), dy2(20), dz(10) {}
  virtual ~AGeoTrd2() {}

  const QString getShapeType() const override {return "TGeoTrd2";}
  virtual const QString getShapeTemplate() {return "TGeoTrd2( dx1, dx2, dy1, dy2, dz )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double dx1, dx2, dy1, dy2, dz;
};

class AGeoTubeSeg : public AGeoShape
{
public:
  AGeoTubeSeg(double rmin, double rmax, double dz, double phi1, double phi2) :
    rmin(rmin), rmax(rmax), dz(dz), phi1(phi1), phi2(phi2) {}
  AGeoTubeSeg() :
    rmin(0), rmax(10), dz(5), phi1(0), phi2(180) {}
  virtual ~AGeoTubeSeg() {}

  const QString getShapeType() const override {return "TGeoTubeSeg";}
  virtual const QString getShapeTemplate() {return "TGeoTubeSeg( rmin, rmax, dz, phi1, phi2 )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double rmin, rmax, dz, phi1, phi2;
};

class AGeoCtub : public AGeoShape
{
public:
  AGeoCtub(double rmin, double rmax, double dz, double phi1, double phi2,
           double nxlow, double nylow, double nzlow,
           double nxhi, double nyhi, double nzhi) :
    rmin(rmin), rmax(rmax), dz(dz), phi1(phi1), phi2(phi2),
    nxlow(nxlow), nylow(nylow), nzlow(nzlow),
    nxhi(nxhi), nyhi(nyhi), nzhi(nzhi) {}
  AGeoCtub() :
    rmin(0), rmax(10), dz(5), phi1(0), phi2(180),
    nxlow(0), nylow(0.64), nzlow(-0.77),
    nxhi(0), nyhi(0.09), nzhi(0.87) {}
  virtual ~AGeoCtub() {}

  const QString getShapeType() const override {return "TGeoCtub";}
  virtual const QString getShapeTemplate() {return "TGeoCtub( rmin, rmax, dz, phi1, phi2, nxlow, nylow, nzlow, nxhi, nyhi, nzhi )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double rmin, rmax, dz, phi1, phi2;
  double nxlow, nylow, nzlow;
  double nxhi, nyhi, nzhi;
};

class AGeoEltu : public AGeoShape
{
public:
  AGeoEltu(double a, double b, double dz) :
    a(a), b(b), dz(dz) {}
  AGeoEltu() :
    a(10), b(20), dz(5) {}
  virtual ~AGeoEltu() {}

  const QString getShapeType() const override {return "TGeoEltu";}
  virtual const QString getShapeTemplate() {return "TGeoEltu( a, b, dz )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() { return dz; }
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double a, b, dz;
};

class AGeoSphere : public AGeoShape
{
public:
  AGeoSphere(double rmin, double rmax, double theta1, double theta2, double phi1, double phi2) :
    rmin(rmin), rmax(rmax), theta1(theta1), theta2(theta2), phi1(phi1), phi2(phi2) {}
  AGeoSphere(double r) :
    rmin(0), rmax(r), theta1(0), theta2(180), phi1(0), phi2(360) {}
  AGeoSphere() :
    rmin(0), rmax(10), theta1(0), theta2(180), phi1(0), phi2(360) {}
  virtual ~AGeoSphere() {}

  const QString getShapeType() const override {return "TGeoSphere";}
  virtual const QString getShapeTemplate() {return "TGeoSphere( rmin,  rmax, theta1, theta2, phi1, phi2 )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return rmax;}
  virtual void setHeight(double dz) {rmax = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize() { return rmax;}

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double rmin, rmax, theta1, theta2, phi1, phi2;
};

class AGeoPara : public AGeoShape
{
public:
  AGeoPara(double dx, double dy, double dz, double alpha, double theta, double phi) :
    dx(dx), dy(dy), dz(dz), alpha(alpha), theta(theta), phi(phi) {}
  AGeoPara() :
    dx(10), dy(10), dz(10), alpha(10), theta(25), phi(45) {}
  virtual ~AGeoPara() {}

  const QString getShapeType() const override {return "TGeoPara";}
  virtual const QString getShapeTemplate() {return "TGeoPara( dX, dY, dZ, alpha, theta, phi )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double dx, dy, dz;
  double alpha, theta, phi;
};

class AGeoArb8 : public AGeoShape
{
public:
  AGeoArb8(double dz, QList<QPair<double, double> > VertList);
  AGeoArb8();
  virtual ~AGeoArb8() {}

  const QString getShapeType() const override {return "TGeoArb8";}
  virtual const QString getShapeTemplate() {return "TGeoArb8( dz,  xL1,yL1, xL2,yL2, xL3,yL3, xL4,yL4, xU1,yU1, xU2,yU2, xU3,yU3, xU4,yU4  )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return dz;}
  virtual void setHeight(double dz) {this->dz = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double dz;
  QList<QPair<double, double> > Vertices;

private:
  void init();

};

class AGeoComposite : public AGeoShape
{
public:
  AGeoComposite(const QStringList members, const QString GenerationString);
  AGeoComposite() {}
  virtual ~AGeoComposite() {}

  const QString getShapeType() const override {return "TGeoCompositeShape";}
  virtual const QString getShapeTemplate() {return "TGeoCompositeShape( (A + B) * (C - D) )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  //virtual double getHeight() {return 0;}
  //virtual void setHeight(double /*dz*/) {}
  virtual const QString getGenerationString() const {return GenerationString;}
  virtual double maxSize() {return 0;}  //***!!!

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* /*Tshape*/) {return false;} //cannot be retrieved this way! need cooperation with AGeoObject itself

  QStringList members;
  QString GenerationString;
};

class AGeoTorus  : public AGeoShape
{
public:
  AGeoTorus(double R, double Rmin, double Rmax, double Phi1, double Dphi) :
    R(R), Rmin(Rmin), Rmax(Rmax), Phi1(Phi1), Dphi(Dphi) {}
  AGeoTorus() {}
  virtual ~AGeoTorus () {}

  const QString getShapeType() const override {return "TGeoTorus";}
  virtual const QString getShapeTemplate() {return "TGeoTorus( R, Rmin, Rmax, Phi1, Dphi )";}
  virtual const QString getHelp();

  virtual bool readFromString(QString GenerationString);
  virtual TGeoShape* createGeoShape(const QString shapeName = "");

  virtual double getHeight() {return Rmax;}
  virtual void setHeight(double dz) {this->Rmax = dz;}
  virtual const QString getGenerationString() const;
  virtual double maxSize();

  void writeToJson(QJsonObject& json) const override;
  virtual void readFromJson(QJsonObject& json);

  virtual bool readFromTShape(TGeoShape* Tshape);

  double R = 100.0;
  double Rmin = 0, Rmax = 20.0;
  double Phi1 = 0, Dphi = 360.0;
};


#endif // AGEOSHAPE_H
