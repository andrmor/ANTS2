#ifndef AINTERFACETOKNNSCRIPT_H
#define AINTERFACETOKNNSCRIPT_H

#include "localscriptinterfaces.h"

#include <QObject>
#include <QVariant>

class NNmoduleClass;

class AKnn_SI : public AScriptInterface
{
  Q_OBJECT
public:
  AKnn_SI(NNmoduleClass *knnModule);

public slots:
  // Main functionality
  QVariant getNeighbours(int ievent, int numNeighbours);  //return: array of [eventIndex, distance] - there will be numNeighbours elements
  QVariant getNeighboursDirect(QVariant onePoint, int numNeighbours);  //onePoint - array of signals for one point;   return: array of [eventIndex, distance] - there will be numNeighbours elements
  void filterByDistance(int numNeighbours, double distanceLimit, bool filterOutEventsWithSmallerDistance);

  // options - set BEFORE calibration dataset is given
  void SetSignalNormalizationType(int type_0None_1sum_2quadraSum);

  // Clear all calibration data
  void clearCalibrationEvents();
  // Define calibration data (overrides old)
  QString setGoodScanEventsAsCalibration();
  QString setGoodReconstructedEventsAsCalibration();
  QString setCalibrationDirect(QVariant arrayOfArrays);

  //stat calibration
  QVariant evaluatePhPerPhE(int numNeighbours, float upperDistanceLimit, float maxSignal);

  // Request calibration data
  int countCalibrationEvents();
  double getCalibrationEventX(int ievent);
  double getCalibrationEventY(int ievent);
  double getCalibrationEventZ(int ievent);
  double getCalibrationEventEnergy(int ievent);
  QVariant getCalibrationEventXYZE(int ievent);
  QVariant getCalibrationEventSignals(int ievent);

  QString getErrorString();

private:
  NNmoduleClass *knnModule;

};

#endif // AINTERFACETOKNNSCRIPT_H
