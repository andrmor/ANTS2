#include "ainterfacetoannscript.h"

#include <QJsonArray>
#include <QDebug>

AInterfaceToANNScript::AInterfaceToANNScript()
{
  //.....
}

AInterfaceToANNScript::~AInterfaceToANNScript()
{
  clear();

  //.....
}

void AInterfaceToANNScript::clear()
{
  Input.clear(); Input.squeeze();
  Output.clear(); Output.squeeze();

  //.....
}

void AInterfaceToANNScript::newNetwork()
{
  clear();

  //.....
}

void AInterfaceToANNScript::addTrainingInput(QVariant arrayOfArrays)
{
  //unpacking data
  int fixedSize = Input.isEmpty() ? -1 : Input.first().size();
  if ( !convertQVariantToVectorOfVectors(&arrayOfArrays, &Input, fixedSize) ) return;
  qDebug() << "New training input is set to:" << Input;

  //.....
}

void AInterfaceToANNScript::addTrainingOutput(QVariant arrayOfArrays)
{
  //unpacking data
  int fixedSize = Output.isEmpty() ? -1 : Output.first().size();
  if ( !convertQVariantToVectorOfVectors(&arrayOfArrays, &Output, fixedSize) ) return;
  qDebug() << "New training output is set to:" << Output;

  //.....
}

QString AInterfaceToANNScript::train()
{
  if (Input.isEmpty() || Output.isEmpty()) return "Training data empty";
  if (Input.size() != Output.size()) return "Training data mismatch";

  //.....

  return ""; //success
}

QVariant AInterfaceToANNScript::process(QVariant arrayOfArrays)
{
  //unpacking data
  QVector< QVector<float> > Data;
  int fixedSize = Input.isEmpty() ? -1 : Input.first().size();
  if ( !convertQVariantToVectorOfVectors(&arrayOfArrays, &Data, fixedSize) ) return QVariant();
  qDebug() << "received to process:" << Data;

  //.....

  //packing the results to send back to the script
  QVector< QVector<float> > dummy; dummy << QVector<float>(5,0) << QVector<float>(5,1) << QVector<float>(5, 2);
  QVariantList l;
  for (int i=0; i<dummy.size(); i++)
    {
      const QVector<float>& thisOne = dummy.at(i);
      QVariantList ll;
      for (int ii=0; ii<thisOne.size(); ii++) ll << thisOne.at(ii);
      l << QVariant(ll);
    }
  return l;
}

bool AInterfaceToANNScript::convertQVariantToVectorOfVectors(QVariant *var, QVector<QVector<float> > *vec, int fixedSize)
{
  if ( QString(var->typeName()) != "QVariantList")
  {
      abort("ANN script: array (or array of arrays) is required as an argument");
      return false;
  }
  QVariantList tmp = var->toList();
  QJsonArray arr = QJsonArray::fromVariantList(tmp);
  for (int i=0; i<arr.size(); i++)
    {
      QJsonArray el = arr[i].toArray();
      if (  el.isEmpty() || (fixedSize!=-1 && el.size()!=fixedSize)  )
        {
          abort("ANN script: Invalid argument or size mismatch");
          return false;
        }
      QVector<float> tmp;
      for (int ii=0; ii<el.size(); ii++) tmp << el[ii].toDouble();
      vec->append(tmp);
    }
  return true;
}
