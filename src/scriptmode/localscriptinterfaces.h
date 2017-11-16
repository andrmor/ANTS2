#ifndef LOCALSCRIPTINTERFACES_H
#define LOCALSCRIPTINTERFACES_H

#include "ascriptinterface.h"

#include <QVector>
#include <QList>
#include <QJsonArray>
#include <QString>

class AGeoObject;
class DetectorClass;
class QVector3D;

class InterfaceToAddObjScript : public AScriptInterface
{
  Q_OBJECT
public:
  InterfaceToAddObjScript(DetectorClass* Detector);
  ~InterfaceToAddObjScript();

  virtual bool InitOnRun();

  QList<AGeoObject*> GeoObjects;

public slots:
  void Box(QString name, double Lx, double Ly, double Lz, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Cylinder(QString name, double D, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Polygone(QString name, int edges, double Dtop, double Dbot, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Cone(QString name, double Dtop, double Dbot, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Sphere(QString name, double D, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Arb8(QString name, QVariant NodesX, QVariant NodesY, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);

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

  void UpdateGeometry(bool CheckOverlaps = true);

signals:
  void clearRequested();
  void requestShowCheckUpWindow();

private:
  DetectorClass* Detector;
  void clearGeoObjects();
};

class InterfaceToPMscript : public AScriptInterface
{
  Q_OBJECT
public:
  QJsonArray arr; //output is placed here

  InterfaceToPMscript();

public slots:
  void pm(double x, double y);
  void PM(double x, double y, double z, QString type);
  void PM(double x, double y, double z, QString type, double phi, double theta, double psi);
};

class InterfaceToNodesScript : public AScriptInterface
{
  Q_OBJECT
public:
  QVector<QVector3D*> nodes; //output is placed here

  InterfaceToNodesScript();
  ~InterfaceToNodesScript();

public slots:
  void node(double x, double y, double z);
};

#endif // LOCALSCRIPTINTERFACES_H
