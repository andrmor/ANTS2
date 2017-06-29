#ifndef SCRIPTINTERFACES_H
#define SCRIPTINTERFACES_H

#include "ascriptinterface.h"

#include <QVector>
#include <QVariant>
#include <QJsonArray>
#include <QString>
#include <QStringList>

class AGeoObject;
class AScriptManager;
class DetectorClass;

class CoreInterfaceClass : public AScriptInterface
{
  Q_OBJECT

public:
  CoreInterfaceClass(AScriptManager *ScriptManager);

public slots:
  //abort execution of the script
  void abort(QString message = "Aborted!");

  //sleep
  void sleep(int ms);

  //output part of the script window
  void print(QString text);
  void clearText();
  QString str(double value, int precision);

  //time stamps
  QString GetTimeStamp();
  QString GetDateTimeStamp();

  //save to file
  bool createFile(QString fileName, bool AbortIfExists = true);
  bool isFileExists(QString fileName);
  bool deleteFile(QString fileName);
  bool createDir(QString path);
  QString getCurrentDir();
  bool setCirrentDir(QString path);
  bool save(QString fileName, QString str);
  bool saveArray(QString fileName, QVariant array);
  bool saveObject(QString FileName, QVariant Object, bool CanOverride);

  //load column of doubles from file and return it as an array
  QVariant loadColumn(QString fileName, int column = 0);
  QVariant loadArray(QString fileName, int columns);

  //dirs
  QString GetWorkDir();
  QString GetScriptDir();
  QString GetExamplesDir();

private:
  AScriptManager* ScriptManager;
};

class InterfaceToAddObjScript : public AScriptInterface
{
  Q_OBJECT
public:
  InterfaceToAddObjScript(DetectorClass* Detector);
  ~InterfaceToAddObjScript();

  virtual bool InitOnRun();

  QList<AGeoObject*> GeoObjects;
  QList< QStringList > Stacks;

public slots:
  void Box(QString name, double Lx, double Ly, double Lz, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Cylinder(QString name, double D, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Polygone(QString name, int edges, double Dtop, double Dbot, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Cone(QString name, double Dtop, double Dbot, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Sphere(QString name, double D, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);
  void Arb8(QString name, QVariant NodesX, QVariant NodesY, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);

  void TGeo(QString name, QString generationString, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi);

  void Stack(QString StackName, QVariant Members);
  void Stack(QString StackName);
  void AddToStack(QString name, QString StackName);
  void AddToStack(QVariant names, QString StackName);
  void RecalculateStack(QString StackName);

  void SetLine(QString name, int color, int width, int style);
  void ClearAll();
  void Remove(QString Object);
  void RemoveRecursive(QString Object);
  void RemoveAllExceptWorld();

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

#endif // SCRIPTINTERFACES_H
