#include "ainterfacetoannscript.h"

#include <QJsonArray>
//#include <QJsonObject>
#include <QDebug>

/*===========================================================================*/
AInterfaceToANNScript::AInterfaceToANNScript()
{
  resetConfigToDefault();
}

/*===========================================================================*/
AInterfaceToANNScript::~AInterfaceToANNScript()
{
  clearTrainingData();
}

/*===========================================================================*/
void AInterfaceToANNScript::clearTrainingData()
{
  Input.clear(); Input.squeeze();
  Output.clear(); Output.squeeze();

}

/*===========================================================================*/
//! Reset the training data.
void AInterfaceToANNScript::newNetwork()
{
  clearTrainingData();
  qDebug() << "New NN created.";
}

/*===========================================================================*/
QString AInterfaceToANNScript::configure(QVariant configObject)
{
  qDebug() << configObject.typeName();
  if ( QString(configObject.typeName()) != "QVariantMap")  //critical error!
  {
      abort("ANN script: configure function requirs an object-type argument");
      return "";
  }

  QVariantMap map = configObject.toMap();
  Config = QJsonObject::fromVariantMap(map);
  qDebug() << "User config:" << Config;

  if (!Config.contains("type")){
   abort("FANN Type - ('type' key) has to be defined in ANN config");
   return "";
  }

  QString norm = Config["norm"].toString();
  qDebug() << "Norm:"<<norm;
  int val = Config["val"].toInt();
  double val2 = Config["afloat"].toDouble();

  qDebug() << val;
  qDebug() << val2;

  //.....

  return "Anything useful you want to communicate back: e.g. warnings etc";
}

/*===========================================================================*/
void AInterfaceToANNScript::resetConfigToDefault()
{
  Config = QJsonObject();
  Config["type"] = "cascade";

  Config["trainAlgorithm"]="FANN_TRAIN_RPROP";
  Config["outputActivationFunc"]="FANN_SIGMOID";
  Config["errorFunction"]="FANN_ERRORFUNC_LINEAR";

  Config["stopFunction"]="FANN_STOPFUNC_BIT";
  Config["bitFailLimit"]="0.1"; // no default.

  Config["cascade_outputChangeFraction"]="0.01"; // default
  Config["cascade_candidateChangeFraction"]="0.01"; // default

  Config["cascade_outputStagnationEpochs"]="12"; // default
  Config["cascade_minOutEpochs"]="50"; // default
  Config["cascade_maxOutEpochs"]="150"; // default

  Config["cascade_candidateStagnationEpochs"]="12"; // default
  Config["cascade_minCandEpochs"]="50"; // default
  Config["cascade_maxCandEpochs"]="150"; // default

  Config["cascade_candidateLimit"]="1000"; // default
  Config["cascade_candidateGroups"]="2"; // default

  Config["cascade_weightMultiplier"]="0.4"; // default

//  std::vector<fann_type> step = {0.25, 0.5, 0.75, 1.};
  // std::vector<fann_type> step = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.};
//  sClass.cascade_setActivationSteepness(step); // default is 0.25, 0.5, 0.75, 1

//  vector<fann_activationfunc_enum> act={FANN_SIGMOID, FANN_SIGMOID_SYMMETRIC,
//   FANN_GAUSSIAN, FANN_GAUSSIAN_SYMMETRIC, FANN_ELLIOT, FANN_ELLIOT_SYMMETRIC};
  // for (auto const& i: cFANNWrapper::trainActivationFunctions) act.push_back(i.first);
//  sClass.cascade_setActivationFunctions(act);


}

/*===========================================================================*/
QVariant AInterfaceToANNScript::getConfig()
{
  const QVariantMap res = Config.toVariantMap();
  return res;
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
  if (Input.size() != Output.size()) return "Training data size mismatch";

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
