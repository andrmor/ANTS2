#ifndef LOCALSCRIPTINTERFACES_H
#define LOCALSCRIPTINTERFACES_H

#include "ascriptinterface.h"

#include <QVector>
#include <QString>
#include <QJsonArray>

class QVector3D;

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
    InterfaceToNodesScript(QVector<QVector3D *> & nodes);

private:
    QVector<QVector3D*> & nodes;

public slots:
    void clear();
    void node(double x, double y, double z);
};

#endif // LOCALSCRIPTINTERFACES_H
