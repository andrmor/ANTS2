#ifndef CUDAMANAGERCLASS_H
#define CUDAMANAGERCLASS_H

#ifdef __USE_ANTS_CUDA__

#include <QVector>
#include <QString>

class EventsDataClass;
//class DetectorClass;
class pms;
class APmGroupsManager;
class SensorLRFs;
class ReconstructionSettings;

struct RecDataStruct
{
  float X;
  float Y;
  float Chi2;
  float Energy;
  float Probability;
};

class CudaManagerClass
{
public:
  CudaManagerClass(pms* PMs, APmGroupsManager* PMgroups, SensorLRFs* SensLRF, EventsDataClass* eventsDataHub, ReconstructionSettings* RecSet, int currentGroup);
  ~CudaManagerClass();

  //run in GUI
  bool           RunCuda(int bufferSize = 500000);

  double         GetUsPerEvent() const;
  const QString& GetLastError() const;

private:
  bool           Perform2D(int from, int to);
  bool           PerformSliced(int from, int to);

  void           ConfigurePMcenters();
  void           ConfigureLRFs();
  void           ConfigureLRFs_Axial();
  void           ConfigureLRFs_XY();
  void           ConfigureLRFs_Sliced3D();
  void           ConfigureLRFs_Composite();

  bool           ConfigureEvents(int from, int to);  //also configures the starting XY
  bool           ConfigureOutputs(int bufferSize);

  void           DataToAntsContainers(int from, int to);

  void           Clear();

private:
  pms* PMs;
  APmGroupsManager* PMgroups;
  SensorLRFs* SensLRF;
  EventsDataClass* EventsDataHub;
  int CurrentGroup;

  //outside configuration data
  int Method;
  int BlockSizeXY;
  int Iterations;
  float Scale;
  float ScaleReductionFactor;
  int OffsetOption;
  float OffsetX;
  float OffsetY;
  int MLorChi2;
  bool IgnoreLowSigPMs;
  float IgnoreThresholdLow, IgnoreThresholdHigh;
  bool IgnoreFarPMs;
  float IgnoreDistance;
  float StarterZ;
 // float comp_k;
  float comp_r0;
 // float comp_lam;
  float comp_a;
  float comp_b;
  float comp_lam2;

  //PM data
  QVector<float> PMsX;
  QVector<float> PMsY; 
  int numPMs;
  int numPMsStaticActive;

  // input event buffer
  float* eventsData; //note: also contains the starting XY
  int numEvents;

  // lrf data buffer
  float* lrfData;  // to do: !!! can reuse lrfSplineData
  int lrfFloatsPerPM;
  int lrfFloatsAxialPerPM; //used only for Composite lrfs
  int p1; //parameters - nintx for XY
  int p2; //parameters - ninty for XY
  QVector< QVector<float> > lrfSplineData; //for 3D data

  //Slices
  int zSlices;
  double SliceThickness;
  double Slice0Z;

  //output buffers
  float* recX;
  float* recY;
  float* recEnergy;
  float* chi2;
  float *probability;

  //temporary storage
  QVector< QVector<RecDataStruct> > RecData;

  //last kernel execution time
  float ElapsedTime;
  double usPerEvent;

  QString LastError;
};

#endif  //__USE_ANTS_CUDA__

#endif // CUDAMANAGERCLASS_H
