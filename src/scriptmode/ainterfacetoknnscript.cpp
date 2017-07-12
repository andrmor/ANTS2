#include "ainterfacetoknnscript.h"
#include "nnmoduleclass.h"

#include <QDebug>
#include <limits>

AInterfaceToKnnScript::AInterfaceToKnnScript(NNmoduleClass* knnModule) : knnModule(knnModule) {}

QVariant AInterfaceToKnnScript::getNeighbours(int ievent, int numNeighbours)
{
   return knnModule->ScriptInterfacer->getNeighbours(ievent, numNeighbours);
}

void AInterfaceToKnnScript::clearCalibrationEvents()
{
  knnModule->ScriptInterfacer->clearCalibration();
}

bool AInterfaceToKnnScript::setGoodScanEventsAsCalibration()
{
  return knnModule->ScriptInterfacer->setCalibration(true);
}

bool AInterfaceToKnnScript::setGoodReconstructedEventsAsCalibration()
{
  return knnModule->ScriptInterfacer->setCalibration(false);
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
