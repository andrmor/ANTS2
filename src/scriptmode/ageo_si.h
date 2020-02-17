#ifndef AINTERFACETOADDOBJSCRIPT_H
#define AINTERFACETOADDOBJSCRIPT_H

#include "ascriptinterface.h"

#include <QVariant>
#include <QVector>
#include <QList>
#include <QString>

class AGeoObject;
class DetectorClass;

class AGeo_SI : public AScriptInterface
{
  Q_OBJECT
public:
  AGeo_SI(DetectorClass* Detector);
  ~AGeo_SI();

  virtual bool InitOnRun();

  QList<AGeoObject*> GeoObjects;

public slots:
  void Box(QString name, double Lx, double Ly, double Lz, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Cylinder(QString name, double D, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Polygone(QString name, int edges, double Dtop, double Dbot, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Cone(QString name, double Dtop, double Dbot, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Sphere(QString name, double D, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Arb8(QString name, QVariant NodesX, QVariant NodesY, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);

  void Monitor(QString name, int shape, double size1, double size2,
               QString container, double x, double y, double z, double phi, double theta, double psi,
               bool SensitiveTop, bool SensitiveBottom, bool StopsTraking);
  void Monitor_ConfigureForPhotons(QString MonitorName, QVariant Position, QVariant Time, QVariant Angle, QVariant Wave);
  void Monitor_ConfigureForParticles(QString MonitorName, int ParticleIndex, int Both_Primary_Secondary, int Both_Direct_Indirect,
                                     QVariant Position, QVariant Time, QVariant Angle, QVariant Energy);

  void TGeo(QString name, QString generationString, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);

  void RecalculateStack(QString name);

  void MakeStack(QString name, QString container);
  void InitializeStack(QString StackName, QString Origin_MemberName);

  void MakeGroup(QString name, QString container);

  void Array(QString name, int numX, int numY, int numZ, double stepX, double stepY, double stepZ, QString container, double x, double y, double z, double psi);
  void ReconfigureArray(QString name, int numX, int numY, int numZ, double stepX, double stepY, double stepZ);

  void SetLine(QString name, int color, int width, int style);
  void ClearAll();
  void Remove(QString Object);
  void RemoveRecursive(QString Object);
  void RemoveAllExceptWorld();

  void EnableObject(QString Object);
  void DisableObject(QString Object);

  void setEnable(QString ObjectOrWildcard, bool flag);

  void UpdateGeometry(bool CheckOverlaps = true);

  QString printOverrides();

signals:
  void clearRequested();
  void requestShowCheckUpWindow();

private:
  DetectorClass* Detector;
  void clearGeoObjects();
};

#endif // AINTERFACETOADDOBJSCRIPT_H
