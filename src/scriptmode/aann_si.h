#ifndef AINTERFACETOANNSCRIPT_H
#define AINTERFACETOANNSCRIPT_H

/*

#include "ascriptinterface.h"

#include <QObject>
#include <QVariant>
#include <QVector>
#include <QJsonObject>

class AInterfaceToANNScript : public AScriptInterface
{
  Q_OBJECT
public:
  AInterfaceToANNScript();
  ~AInterfaceToANNScript();

public slots:
  void clearTrainingData();
  void newNetwork();

  QString configure(QVariant configObject);
  void resetConfigToDefault();
  QVariant getConfig();

  void addTrainingInput(QVariant arrayOfArrays);
  void addTrainingOutput(QVariant arrayOfArrays);

  QString train();  //on success, returns empty string, otherwise returns error message

  QVariant process(QVariant arrayOfArrays);

private:
  QVector< QVector<float> > Input;
  QVector< QVector<float> > Output;

  QJsonObject Config;

  bool convertQVariantToVectorOfVectors(QVariant* var, QVector< QVector<float> >* vec , int fixedSize = -1);
};

*/

#endif // AINTERFACETOANNSCRIPT_H
