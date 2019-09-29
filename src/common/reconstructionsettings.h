#ifndef RECONSTRUCTIONSETTINGS_H
#define RECONSTRUCTIONSETTINGS_H

#include <QJsonObject>
#include <QJsonArray>
#include <QString>

class ReconstructionSettings
{
public:
  //general
  bool   fReconstructZ;
  double SuggestedZ;
  int    Zstrategy;
  bool   fReconstructEnergy;
  double SuggestedEnergy;
  bool   fIncludePassive;
  bool   fWeightedChi2calculation;

  //Algorithm
  int    ReconstructionAlgorithm;  //0-CoG, 1-Multigrid, 2-RootMinimizer, 4-ANN, 5-GPU_CG

  //Dynamic passives
  bool   fUseDynamicPassivesDistance;
  bool   fUseDynamicPassivesSignal;
  bool   fUseDynamicPassives; // just OR for two above
  double SignalThresholdLow, SignalThresholdHigh;
  double MaxDistance, MaxDistanceSquare;

  //CoG
  bool   fCoGStretch;
  double CoGStretchX, CoGStretchY, CoGStretchZ;
  bool   fCoGIgnoreBySignal;
  double CoGIgnoreThresholdLow, CoGIgnoreThresholdHigh;
  bool   fCoGIgnoreFar;
  double CoGIgnoreDistance;
  double CoGIgnoreDistance2;
  bool   fCogForceFixedZ;

  //Multigrid
  int    CGstartOption; //0 - cog, 1 - PM with max signal, 2 - given X and Y, 3 - Scan position
  double CGstartX, CGstartY;
  int    CGoptimizeWhat;  //0 - Chi2, 1 -ML
  int    CGnodesXY, CGiterations;
  double CGinitialStep, CGreduction;
    //now here, later exapnd to other stat methods?
  bool   fLimitNodes;   // suppress some nodes based on theyr position
  int    LimitNodesShape; // 0 - round, 1 - rectangular
  double LimitNodesSize1, LimitNodesSize2;

  //Root minimiser
  int     RMtype;  //0 - LS, 1 - ML, 2 - TFormula
  int     RMminuitOption;
  double  RMstepX, RMstepY, RMstepZ, RMstepEnergy;
  int     RMmaxCalls;
  int     RMstartOption; //0 - cog, 1 - PM with max signal
  bool    fRMsuppressConsole;
  QString RMformula;

  //multiples
  int    MultipleEventOption;

  //neural
  QJsonObject ANNsettings;

  //CUDA
  QJsonObject CGonCUDAsettings;
  int BufferSize = 500000;

  //kNNReconstruction
  QJsonObject kNNrecSet;

  QString ErrorString;

  bool readFromJson(QJsonObject &Json);

  ReconstructionSettings() : fReconstructZ(false), SuggestedZ(0), Zstrategy(0), fReconstructEnergy(false), SuggestedEnergy(1),
    fIncludePassive(false), fWeightedChi2calculation(true),
    ReconstructionAlgorithm(0),
    fUseDynamicPassivesDistance(false), fUseDynamicPassivesSignal(false), fUseDynamicPassives(false),
    SignalThresholdLow(0), SignalThresholdHigh(1e10), MaxDistance(100), MaxDistanceSquare(10000),
    fCoGStretch (false), fCoGIgnoreBySignal(false), fCoGIgnoreFar(false), fCogForceFixedZ(true), fLimitNodes(false) {}
};


#endif // RECONSTRUCTIONSETTINGS_H
