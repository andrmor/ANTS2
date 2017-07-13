#ifndef AINTERFACETOKNNSCRIPT_H
#define AINTERFACETOKNNSCRIPT_H

#include "scriptinterfaces.h"

class NNmoduleClass;

class AInterfaceToKnnScript : public AScriptInterface
{
  Q_OBJECT
public:
  AInterfaceToKnnScript(NNmoduleClass *knnModule);

public slots:
  // Main functionality
  QVariant getNeighbours(int ievent, int numNeighbours);  //array of [eventIndex, distance] - there will be numNeighbours elements
  void filterByDistance(int numNeighbours, double maxDistance);

  // options - set BEFORE calibration dataset is given
  void SetSignalNormalizationType(int type_0None_1sum_2quadraSum);

  // Clear all calibration data
  void clearCalibrationEvents();
  // Define calibration data (overrides old)
  bool setGoodScanEventsAsCalibration();
  bool setGoodReconstructedEventsAsCalibration();
  // Request calibration data
  int countCalibrationEvents();
  double getCalibrationEventX(int ievent);
  double getCalibrationEventY(int ievent);
  double getCalibrationEventZ(int ievent);
  double getCalibrationEventEnergy(int ievent);
  QVariant getCalibrationEventXYZE(int ievent);
  QVariant getCalibrationEventSignals(int ievent);

private:
  NNmoduleClass *knnModule;

};

#endif // AINTERFACETOKNNSCRIPT_H
