#ifndef LOCALSCRIPTINTERFACES_H
#define LOCALSCRIPTINTERFACES_H

#include "ascriptinterface.h"

#include <QString>
#include <QJsonArray>

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

#endif // LOCALSCRIPTINTERFACES_H
