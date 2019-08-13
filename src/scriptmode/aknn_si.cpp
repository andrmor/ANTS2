#include "aknn_si.h"
#include "nnmoduleclass.h"

#include <QDebug>
#include <limits>

AKnn_SI::AKnn_SI(NNmoduleClass* knnModule) : knnModule(knnModule)
{
    H["getNeighbours"] = "Returns array of [eventIndex, distance] - there will be numNeighbours elements";
    H["SetSignalNormalizationType"] = "Set pre-processing of event signal before setting as calibration or processing by knn:\n"
            "0 - no normalization, 1 - sum signals is normalized to 1, 2 - sum in quadrature of all signals is normalized to 1";
    H["filterByDistance"] = "Filters currently available events according to the distance to the calibration events. "
            "Average distance is calculated over numNeighbours, cut is performed in respect of the average distance = distanceLimit, "
            "filterOutEventsWithSmallerDistance option sets which events to cut - with smaller or large value than the limit.";
}

QVariant AKnn_SI::getNeighbours(int ievent, int numNeighbours)
{    
    QVariant res = knnModule->ScriptInterfacer->getNeighbours(ievent, numNeighbours);
    if (res == QVariantList())
    {
        abort("kNN module reports fail:\n" + knnModule->ScriptInterfacer->ErrorString);
    }
    return res;
}

QVariant AKnn_SI::getNeighboursDirect(QVariant onePoint, int numNeighbours)
{
  QVariantList VarList = onePoint.toList();
  if (VarList.isEmpty())
  {
      abort("getNeighboursDirect() requires first argument to be array.");
      return "";
  }
  QVector<float> data;
  for (int i=0; i<VarList.size(); i++)
    {
      data << VarList.at(i).toFloat();
    }

  QVariant res = knnModule->ScriptInterfacer->getNeighboursDirect(data, numNeighbours);
  if (res == QVariantList())
  {
      abort("kNN module reports fail:\n" + knnModule->ScriptInterfacer->ErrorString);
  }
  return res;
}

void AKnn_SI::filterByDistance(int numNeighbours, double distanceLimit, bool filterOutEventsWithSmallerDistance)
{
    bool ok = knnModule->ScriptInterfacer->filterByDistance(numNeighbours, distanceLimit, filterOutEventsWithSmallerDistance);
    if (!ok)
    {
        abort("kNN module reports fail:\n" + knnModule->ScriptInterfacer->ErrorString);
        return;
    }
}

void AKnn_SI::SetSignalNormalizationType(int type_0None_1sum_2quadraSum)
{
  knnModule->ScriptInterfacer->SetSignalNormalization(type_0None_1sum_2quadraSum);
}

void AKnn_SI::clearCalibrationEvents()
{
  knnModule->ScriptInterfacer->clearCalibration();
}

QString AKnn_SI::setGoodScanEventsAsCalibration()
{
  knnModule->ScriptInterfacer->setCalibration(true);
  return knnModule->ScriptInterfacer->ErrorString;
}

QString AKnn_SI::setGoodReconstructedEventsAsCalibration()
{
  knnModule->ScriptInterfacer->setCalibration(false);
  return knnModule->ScriptInterfacer->ErrorString;
}

QString AKnn_SI::setCalibrationDirect(QVariant arrayOfArrays)
{
    QVariantList VarList = arrayOfArrays.toList();
    if (VarList.isEmpty())
    {
        abort("Array of arrays is expected as the second argument in setCalibration()");
        return "";
    }

    QVariantList element = VarList.at(0).toList();
    if (element.isEmpty())
    {
        abort("Array of arrays is expected as the second argument in setCalibration()");
        return "";
    }
    const int numDimension = element.size();
    QVector< QVector<float>> data(VarList.size());

    for (int iPoint = 0; iPoint < VarList.size(); iPoint++)
    {
        QVariantList element = VarList.at(iPoint).toList();
        if (element.isEmpty())
        {
            abort("Array of arrays is expected as the second argument in setCalibration()");
            return "";
        }
        if (element.size() != numDimension)
        {
            abort("Array of arrays of the same size is expected in setCalibration()");
            return "";
        }

        QVector<float>& dataLine = data[iPoint];
        dataLine.resize(numDimension);
        for (int iDim = 0; iDim < numDimension; iDim++)
        {
            dataLine[iDim] = element.at(iDim).toFloat();
        }
    }

  knnModule->ScriptInterfacer->setCalibrationDirect(data);
  if (!knnModule->ScriptInterfacer->ErrorString.isEmpty())
      abort(knnModule->ScriptInterfacer->ErrorString);
  return knnModule->ScriptInterfacer->ErrorString;
}

QVariant AKnn_SI::evaluatePhPerPhE(int numNeighbours, float upperDistanceLimit, float maxSignal)
{
    QVector<float> phe = knnModule->ScriptInterfacer->evaluatePhPerPhE(numNeighbours, upperDistanceLimit, maxSignal);
    const QString& error = knnModule->ScriptInterfacer->ErrorString;
    if (!error.isEmpty())
    {
        abort("Error in knn module: " + error);
        return 0;
    }

    QVariantList vl;
    for (const float& f : phe) vl << f;

    return vl;
}

int AKnn_SI::countCalibrationEvents()
{
  return knnModule->ScriptInterfacer->countCalibrationEvents();
}

double AKnn_SI::getCalibrationEventX(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventX(ievent);
}

double AKnn_SI::getCalibrationEventY(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventX(ievent);
}

double AKnn_SI::getCalibrationEventZ(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventX(ievent);
}

double AKnn_SI::getCalibrationEventEnergy(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventE(ievent);
}

QVariant AKnn_SI::getCalibrationEventXYZE(int ievent)
{
  QVariantList l;
  l << knnModule->ScriptInterfacer->getCalibrationEventX(ievent) <<
       knnModule->ScriptInterfacer->getCalibrationEventY(ievent) <<
       knnModule->ScriptInterfacer->getCalibrationEventZ(ievent) <<
       knnModule->ScriptInterfacer->getCalibrationEventE(ievent);
  return l;
}

QVariant AKnn_SI::getCalibrationEventSignals(int ievent)
{
    return knnModule->ScriptInterfacer->getCalibrationEventSignals(ievent);
}

QString AKnn_SI::getErrorString()
{
    return knnModule->ScriptInterfacer->ErrorString;
}
