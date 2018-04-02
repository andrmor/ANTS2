#include "ainterfacetoknnscript.h"
#include "nnmoduleclass.h"

#include <QDebug>
#include <limits>

AInterfaceToKnnScript::AInterfaceToKnnScript(NNmoduleClass* knnModule) : knnModule(knnModule)
{
    H["getNeighbours"] = "Returns array of [eventIndex, distance] - there will be numNeighbours elements";
    H["SetSignalNormalizationType"] = "Set pre-processing of event signal before setting as calibration or processing by knn:\n"
            "0 - no normalization, 1 - sum signals is normalized to 1, 2 - sum in quadrature of all signals is normalized to 1";
    H["filterByDistance"] = "Filters currently available events according to the distance to the calibration events. "
            "Average distance is calculated over numNeighbours, cut is performed in respect of the average distance = distanceLimit, "
            "filterOutEventsWithSmallerDistance option sets which events to cut - with smaller or large value than the limit.";
}

QVariant AInterfaceToKnnScript::getNeighbours(int ievent, int numNeighbours)
{    
    QVariant res = knnModule->ScriptInterfacer->getNeighbours(ievent, numNeighbours);
    if (res == QVariantList())
    {
        abort("kNN module reports fail:\n" + knnModule->ScriptInterfacer->ErrorString);
    }
    return res;
}

void AInterfaceToKnnScript::filterByDistance(int numNeighbours, double distanceLimit, bool filterOutEventsWithSmallerDistance)
{
    bool ok = knnModule->ScriptInterfacer->filterByDistance(numNeighbours, distanceLimit, filterOutEventsWithSmallerDistance);
    if (!ok)
    {
        abort("kNN module reports fail:\n" + knnModule->ScriptInterfacer->ErrorString);
        return;
    }
}

void AInterfaceToKnnScript::SetSignalNormalizationType(int type_0None_1sum_2quadraSum)
{
  knnModule->ScriptInterfacer->SetSignalNormalization(type_0None_1sum_2quadraSum);
}

void AInterfaceToKnnScript::clearCalibrationEvents()
{
  knnModule->ScriptInterfacer->clearCalibration();
}

QString AInterfaceToKnnScript::setGoodScanEventsAsCalibration()
{
  knnModule->ScriptInterfacer->setCalibration(true);
  return knnModule->ScriptInterfacer->ErrorString;
}

QString AInterfaceToKnnScript::setGoodReconstructedEventsAsCalibration()
{
  knnModule->ScriptInterfacer->setCalibration(false);
  return knnModule->ScriptInterfacer->ErrorString;
}

QString AInterfaceToKnnScript::setCalibration(QVariant array)
{
    QVariantList VarList = array.toList();
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
    QVector< QVector<float>> data(numDimension);

    for (int iPoint=0; iPoint<VarList.size(); iPoint++)
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
        for (int iDim=0; iDim<numDimension; iDim++)
        {
            dataLine[iDim] = element.at(iDim).toFloat();
        }
    }

  knnModule->ScriptInterfacer->setCalibrationDirect(data);
  return knnModule->ScriptInterfacer->ErrorString;
}

int AInterfaceToKnnScript::countCalibrationEvents()
{
  return knnModule->ScriptInterfacer->countCalibrationEvents();
}

double AInterfaceToKnnScript::getCalibrationEventX(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventX(ievent);
}

double AInterfaceToKnnScript::getCalibrationEventY(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventX(ievent);
}

double AInterfaceToKnnScript::getCalibrationEventZ(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventX(ievent);
}

double AInterfaceToKnnScript::getCalibrationEventEnergy(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventE(ievent);
}

QVariant AInterfaceToKnnScript::getCalibrationEventXYZE(int ievent)
{
  QVariantList l;
  l << knnModule->ScriptInterfacer->getCalibrationEventX(ievent) <<
       knnModule->ScriptInterfacer->getCalibrationEventY(ievent) <<
       knnModule->ScriptInterfacer->getCalibrationEventZ(ievent) <<
       knnModule->ScriptInterfacer->getCalibrationEventE(ievent);
  return l;
}

QVariant AInterfaceToKnnScript::getCalibrationEventSignals(int ievent)
{
    return knnModule->ScriptInterfacer->getCalibrationEventSignals(ievent);
}

QString AInterfaceToKnnScript::getErrorString()
{
    return knnModule->ScriptInterfacer->ErrorString;
}
