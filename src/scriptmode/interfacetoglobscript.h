#ifndef INTERFACETOGLOBSCRIPT
#define INTERFACETOGLOBSCRIPT

#include "ascriptinterface.h"

#include <QVector>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QVariant>

#ifdef GUI
  class MainWindow;
  class QMainWindow;
#endif

#ifdef SIM
  class ASimulationManager;
#endif

class QPlainTextEdit;
class DetectorClass;
class EventsDataClass;
class GeometryWindowClass;
class GraphWindowClass;
class TObject;
class QDialog;
class pms;
class TF2;
class AConfiguration;
class ReconstructionManagerClass;
class SensorLRFs;
class TmpObjHubClass;
class APmGroupsManager;
class TRandom2;
class QJsonValue;

class InterfaceToGlobScript : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToGlobScript() {}
  ~InterfaceToGlobScript() {}
};

// ====== interfaces which do not require GUI ======

// ---- C O N F I G U R A T I O N ----
class InterfaceToConfig : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToConfig(AConfiguration* config);
  ~InterfaceToConfig(){}

//  virtual bool InitOnRun();

  AConfiguration* Config;
  QString LastError;

public slots:
  bool Load(QString FileName);
  bool Save(QString FileName);

  bool Replace(QString Key, QVariant val);
  QVariant GetKeyValue(QString Key);

  QString GetLastError();

  void RebuildDetector();
  void UpdateGui();

signals:
  void requestReadRasterGeometry();

private:
  bool modifyJsonValue(QJsonObject& obj, const QString& path, const QJsonValue& newValue);
  void find(const QJsonObject &obj, QStringList Keys, QStringList& Found, QString Path = "");
  bool keyToNameAndIndex(QString Key, QString &Name, QVector<int> &Indexes);
  bool expandKey(QString &Key);
};

// ---- R E C O N S T R U C T I O N ----
class InterfaceToReconstructor : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToReconstructor(ReconstructionManagerClass* RManager, AConfiguration* Config, EventsDataClass* EventsDataHub, int RecNumThreads);
  ~InterfaceToReconstructor(){}

  virtual void ForceStop();

public slots:
  void ReconstructEvents(int NumThreads = -1, bool fShow = true);
  void UpdateFilters(int NumThreads = -1);

  void DoBlurUniform(double range, bool fUpdateFilters = true);
  void DoBlurGauss(double sigma, bool fUpdateFilters = true);

  //Sensor groups
  int countSensorGroups();
  void clearSensorGroups();
  void addSensorGroup(QString name);
  void setPMsOfGroup(int igroup, QVariant PMlist);
  QVariant getPMsOfGroup(int igroup);

  //passive PMs
  bool isStaticPassive(int ipm);
  void setStaticPassive(int ipm);
  void clearStaticPassive(int ipm);
  void selectSensorGroup(int igroup);
  void clearSensorGroupSelection();

  //reconstruction data save
  void SaveAsTree(QString fileName, bool IncludePMsignals=true, bool IncludeRho=true, bool IncludeTrue=true, int SensorGroup=0);
  void SaveAsText(QString fileName);

  // manifest item handling
  void ClearManifestItems();
  int CountManifestItems();
  void AddRoundManisfetItem(double x, double y, double Diameter);
  void AddRectangularManisfetItem(double x, double y, double dX, double dY, double Angle);  
  void SetManifestItemLineProperties(int i, int color, int width, int style);

private:
  ReconstructionManagerClass* RManager;
  AConfiguration* Config;
  EventsDataClass* EventsDataHub;
  APmGroupsManager* PMgroups;

  int RecNumThreads;

signals:
  void RequestUpdateGuiForManifest();
  void RequestStopReconstruction();
};

// ---- E V E N T S ----
class InterfaceToData : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToData(AConfiguration* Config, ReconstructionManagerClass* RManager, EventsDataClass* EventsDataHub);
  ~InterfaceToData(){}

public slots:
  int GetNumPMs();
  int countPMs();
  int GetNumEvents();
  int countEvents();
  int countTimedEvents();
  int countTimeBins();
  double GetPMsignal(int ievent, int ipm);
  void SetPMsignal(int ievent, int ipm, double value);
  double GetPMsignalTimed(int ievent, int ipm, int iTimeBin);
  QVariant GetPMsignalVsTime(int ievent, int ipm);

  // Reconstructed values
    //assuming there is only one group, and single point reconstruction
  double GetReconstructedX(int ievent);  
  double GetReconstructedY(int ievent); 
  double GetReconstructedZ(int ievent);  
  double GetRho(int ievent, int iPM);  
  double GetRho2(int ievent, int iPM); 
  double GetReconstructedEnergy(int ievent);  
  bool IsReconstructedGoodEvent(int ievent);  
    //general
  double GetReconstructedX(int igroup, int ievent, int ipoint);
  double GetReconstructedY(int igroup, int ievent, int ipoint);
  double GetReconstructedZ(int igroup, int ievent, int ipoint);
  double GetRho(int igroup, int ievent, int ipoint, int iPM);
  double GetRho2(int igroup, int ievent, int ipoint, int iPM);
  double GetReconstructedEnergy(int igroup, int ievent, int ipoint);
  bool IsReconstructedGoodEvent(int igroup, int ievent);
    //counters - return -1 when invalid parameters
  int countReconstructedGroups();
  int countReconstructedEvents(int igroup);
  int countReconstructedPoints(int igroup, int ievent);

  // True values known in sim or from a calibration dataset
  double GetTrueX(int ievent);
  double GetTrueY(int ievent);
  double GetTrueZ(int ievent);
  double GetTrueEnergy(int ievent);
  int GetTruePoints(int ievent);
  bool IsTrueGoodEvent(int ievent);
  bool GetTrueNumberPoints(int ievent);
  void SetScanX(int ievent, double value);
  void SetScanY(int ievent, double value);
  void SetScanZ(int ievent, double value);
  void SetScanEnergy(int ievent, double value);

  //raw signal values
  QVariant GetPMsSortedBySignal(int ievent);
  int GetPMwithMaxSignal(int ievent);

  //for custom reconstrtuctions
    //assuming there is only one group, and single point reconstruction
  void SetReconstructed(int ievent, double x, double y, double z, double e);
  void SetReconstructedX(int ievent, double x);
  void SetReconstructedY(int ievent, double y);
  void SetReconstructedZ(int ievent, double z);
  void SetReconstructedEnergy(int ievent, double e);
  void SetReconstructedGoodEvent(int ievent, bool good);
  void SetReconstructedAllEventsGood(bool flag);
  void SetReconstructionOK(int ievent, bool OK);

    //general
  void SetReconstructed(int igroup, int ievent, int ipoint, double x, double y, double z, double e);
  void SetReconstructedFast(int igroup, int ievent, int ipoint, double x, double y, double z, double e); // no checks!!! unsafe
  void AddReconstructedPoint(int igroup, int ievent, double x, double y, double z, double e);
  void SetReconstructedX(int igroup, int ievent, int ipoint, double x);
  void SetReconstructedY(int igroup, int ievent, int ipoint, double y);
  void SetReconstructedZ(int igroup, int ievent, int ipoint, double z);
  void SetReconstructedEnergy(int igroup, int ievent, int ipoint, double e);
  void SetReconstructedGoodEvent(int igroup, int ievent, int ipoint, bool good);  
  void SetReconstructionOK(int igroup, int ievent, int ipoint, bool OK);
    //set when reconstruction is ready for all events! - otherwise GUI will complain
  void SetReconstructionReady();
    //clear reconstruction and prepare containers
  void ResetReconstructionData(int numGroups);

  //load data
  void LoadEventsTree(QString fileName, bool Append = false, int MaxNumEvents = -1);
  void LoadEventsAscii(QString fileName, bool Append = false);

  //clear data
  void ClearEvents();

  //Purges
  void PurgeBad();
  void Purge(int LeaveOnePer);

  //Statistics
  QVariant GetStatistics(int igroup);

private:
  AConfiguration* Config;
  ReconstructionManagerClass* RManager;
  EventsDataClass* EventsDataHub;

  bool checkEventNumber(int ievent);
  bool checkEventNumber(int igroup, int ievent, int ipoint);
  bool checkPM(int ipm);
  bool checkTrueDataRequest(int ievent);
  bool checkSetReconstructionDataRequest(int ievent);

signals:
  void RequestEventsGuiUpdate();
};

// ---- P M S ----
class AInterfaceToPMs : public AScriptInterface
{
  Q_OBJECT

public:
  AInterfaceToPMs(AConfiguration* Config);
  ~AInterfaceToPMs() {}

public slots:  
  int CountPM();

  double GetPMx(int ipm);
  double GetPMy(int ipm);
  double GetPMz(int ipm);

  QVariant GetPMtypes();

  void RemoveAllPMs();
  bool AddPMToPlane(int UpperLower, int type, double X, double Y, double angle = 0);
  bool AddPM(int UpperLower, int type, double X, double Y, double Z, double phi, double theta, double psi);
  void SetAllArraysFullyCustom();

private:
  AConfiguration* Config;
  pms* PMs;

  bool checkValidPM(int ipm);
  bool checkAddPmCommon(int UpperLower, int type);
};

#ifdef SIM
// ---- S I M U L A T I O N S ----
class InterfaceToSim : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToSim(ASimulationManager* SimulationManager, EventsDataClass* EventsDataHub, AConfiguration* Config, int RecNumThreads, bool fGuiPresent = true);
  ~InterfaceToSim(){}

  virtual void ForceStop();

public slots:
  bool RunPhotonSources(int NumThreads = -1);
  bool RunParticleSources(int NumThreads = -1);

  void SetSeed(long seed);
  long GetSeed();

  void ClearCustomNodes();
  void AddCustomNode(double x, double y, double z);
  QVariant GetCustomNodes();
  bool SetCustomNodes(QVariant ArrayOfArray3);

  bool SaveAsTree(QString fileName);
  bool SaveAsText(QString fileName);

  //monitors
  int countMonitors();
  //int getMonitorHits(int imonitor);
  int getMonitorHits(QString monitor);

  QVariant getMonitorTime(QString monitor);
  QVariant getMonitorAngular(QString monitor);
  QVariant getMonitorWave(QString monitor);
  QVariant getMonitorEnergy(QString monitor);
  QVariant getMonitorXY(QString monitor);

signals:
  void requestStopSimulation();

private:  
  ASimulationManager* SimulationManager;
  EventsDataClass* EventsDataHub;
  AConfiguration* Config;

  int RecNumThreads;
  bool fGuiPresent;

  QVariant getMonitorData1D(QString monitor, QString whichOne);
};
#endif

// ---- L R F ----
class InterfaceToLRF : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToLRF(AConfiguration* Config, EventsDataClass* EventsDataHub);

public slots:
  QString Make();
  double GetLRF(int ipm, double x, double y, double z);
  double GetLRFerror(int ipm, double x, double y, double z);

  //iterations  
  int CountIterations();
  int GetCurrent();
  void SetCurrent(int iterIndex);
  void SetCurrentName(QString name);
  void DeleteCurrent();
  QString Save(QString fileName);
  int Load(QString fileName);

  //void ShowVsXY(int ipm, int PointsX, int PointsY);

private:
  AConfiguration* Config;
  EventsDataClass* EventsDataHub;
  SensorLRFs* SensLRF; //alias
  //TF2 *f2d;

  bool getValidIteration(int &iterIndex);
};

// ---- New LRF Module ----
namespace LRF {
  class ARepository;
}
class ALrfScriptInterface : public AScriptInterface
{
  Q_OBJECT

public:
  ALrfScriptInterface(DetectorClass* Detector, EventsDataClass* EventsDataHub);

public slots:
  QString Make(QString name, QVariantList instructions, bool use_scan_data, bool fit_error, bool scale_by_energy);
  QString Make(int recipe_id, bool use_scan_data, bool fit_error, bool scale_by_energy);
  double GetLRF(int ipm, double x, double y, double z);

  QList<int> GetListOfRecipes();
  int GetCurrentRecipeId();
  int GetCurrentVersionId();
  bool SetCurrentRecipeId(int rid);
  bool SetCurrentVersionId(int vid, int rid = -1);
  void DeleteCurrentRecipe();
  void DeleteCurrentRecipeVersion();

  bool SaveRepository(QString fileName);
  bool SaveCurrentRecipe(QString fileName);
  bool SaveCurrentVersion(QString fileName);
  QList<int> Load(QString fileName); //Returns list of loaded recipes

private:
  DetectorClass* Detector;
  EventsDataClass* EventsDataHub;
  LRF::ARepository *repo; //alias
};

// ---- H I S T O G R A M S ---- (TH1D of ROOT)
class InterfaceToHistD : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToHistD(TmpObjHubClass *TmpHub);
  ~InterfaceToHistD(){}

  virtual bool InitOnRun();

public slots:
  void NewHist(QString HistName, int bins, double start, double stop);
  void NewHist2D(QString HistName, int binsX, double startX, double stopX, int binsY, double startY, double stopY);

  void SetTitles(QString HistName, QString X_Title, QString Y_Title, QString Z_Title = "");
  void SetLineProperties(QString HistName, int LineColor, int LineStyle, int LineWidth);

  void Fill(QString HistName, double val, double weight);
  void Fill2D(QString HistName, double x, double y, double weight);

  void Draw(QString HistName, QString options);

  void Smooth(QString HistName, int times);
  QVariant FitGauss(QString HistName, QString options="");
  QVariant FitGaussWithInit(QString HistName, QVariant InitialParValues, QString options="");

  bool Delete(QString HistName);
  void DeleteAllHist();

signals:
  void RequestDraw(TObject* obj, QString options, bool fFocus);

private:  
  TmpObjHubClass *TmpHub;
};

// ---- G R A P H S ---- (TGraph of ROOT)
class InterfaceToGraphs : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToGraphs(TmpObjHubClass *TmpHub);
  ~InterfaceToGraphs(){}

  virtual bool InitOnRun();

public slots:
  void NewGraph(QString GraphName);
  void SetMarkerProperties(QString GraphName, int MarkerColor, int MarkerStyle, int MarkerSize);
  void SetLineProperties(QString GraphName, int LineColor, int LineStyle, int LineWidth);
  void SetTitles(QString GraphName, QString X_Title, QString Y_Title);

  void AddPoint(QString GraphName, double x, double y);
  void AddPoints(QString GraphName, QVariant xArray, QVariant yArray);
  void AddPoints(QString GraphName, QVariant xyArray);

  void Draw(QString GraphName, QString options);

  bool Delete(QString GraphName);
  void DeleteAllGraph();

signals:
  void RequestDraw(TObject* obj, QString options, bool fFocus);

private:
  TmpObjHubClass *TmpHub;
};

// ---- T R E E ---- (TTree of ROOT)
class AInterfaceToTree : public AScriptInterface
{
  Q_OBJECT

public:
   AInterfaceToTree(TmpObjHubClass *TmpHub);
   ~AInterfaceToTree() {}

public slots:
   void OpenTree(QString TreeName, QString FileName, QString TreeNameInFile);
   QString PrintBranches(QString TreeName);
   QVariant GetBranch(QString TreeName, QString BranchName);
   void CloseTree(QString TreeName);

private:
   TmpObjHubClass *TmpHub;
};

// ---- M A T H ----
class MathInterfaceClass : public AScriptInterface
{
  Q_OBJECT
  Q_PROPERTY(double pi READ pi)
  double pi() const { return 3.141592653589793238462643383279502884; }

public:
  MathInterfaceClass(TRandom2* RandGen);
  void setRandomGen(TRandom2* RandGen);

public slots:
  double abs(double val);
  double acos(double val);
  double asin(double val);
  double atan(double val);
  double atan2(double y, double x);
  double ceil(double val);
  double cos(double val);
  double exp(double val);
  double floor(double val);
  double log(double val);
  double max(double val1, double val2);
  double min(double val1, double val2);
  double pow(double val, double power);
  double sin(double val);
  double sqrt(double val);
  double tan(double val);
  double round(double val);
  double random();
  double gauss(double mean, double sigma);
  double poisson(double mean);
  double maxwell(double a);  // a is sqrt(kT/m)
  double exponential(double tau);

private:
  TRandom2* RandGen;
};

#ifdef GUI
// =============== GUI mode only ===============

// MESSAGE window
class InterfaceToTexter : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToTexter(QMainWindow* parent);
  ~InterfaceToTexter();

  QDialog *D;
  double X, Y;
  double WW, HH;

  QPlainTextEdit* e;
  bool bEnabled;

public slots:
  void Enable(bool flag) {bEnabled = flag;}
  void Append(QString txt);
  void Clear();
  void Show();
  void Hide();
  void Show(QString txt, int ms = -1);
  void SetTransparent(bool flag);

  void Move(double x, double y);
  void Resize(double w, double h);

  void SetFontSize(int size);

public:
  void deleteDialog();
  bool isActive() {return bActivated;}
  void hide();     //does not affect bActivated status
  void restore();  //does not affect bActivated status

private:
  QMainWindow* Parent;
  bool bActivated;

  void init(bool fTransparent);
};

// -- GRAPH WINDOW --
class InterfaceToGraphWin : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToGraphWin(MainWindow* MW);
  ~InterfaceToGraphWin(){}

public slots:
  void Show();
  void Hide();

  void PlotDensityXY();
  void PlotEnergyXY();
  void PlotChi2XY();
  void ConfigureXYplot(int binsX, double X0, double X1, int binsY, double Y0, double Y1);

  void SetLog(bool Xaxis, bool Yaxis);

  void AddLegend(double x1, double y1, double x2, double y2, QString title);
  void AddText(QString text, bool Showframe, int Alignment_0Left1Center2Right);

  //basket operation
  void AddToBasket(QString Title);
  void ClearBasket();

  void SaveImage(QString fileName);  
  void ExportTH2AsText(QString fileName);
  QVariant GetProjection();

  QVariant GetAxis();
private:
  MainWindow* MW;
};

// -- OUTPUT WINDOW --
class AInterfaceToOutputWin : public AScriptInterface
{
  Q_OBJECT

public:
  AInterfaceToOutputWin(MainWindow* MW);
  ~AInterfaceToOutputWin(){}

public slots:
  void ShowOutputWindow(bool flag = true, int tab = -1);
  void Show();
  void Hide();

private:
  MainWindow* MW;
};

// -- GEOMETRY WINDOW --
class InterfaceToGeoWin : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToGeoWin(MainWindow* MW, TmpObjHubClass* TmpHub);
  ~InterfaceToGeoWin();

public slots:  
  void Show();
  void Hide();

  void BlockUpdates(bool on); //forbids updates

  //orientation of TView3D
  double GetPhi();
  double GetTheta();
  void SetPhi(double phi);
  void SetTheta(double theta);
  void Rotate(double Theta, double Phi, int Steps, int msPause = 50);

  //view manipulation
  void SetZoom(int level);
  void SetParallel(bool on);
  void UpdateView();

  //position and size
  int  GetX();
  int  GetY();
  int  GetW();
  int  GetH();

  //show things
  void ShowGeometry();
  void ShowPMnumbers();
  void ShowReconstructedPositions();
  void SetShowOnlyFirstEvents(bool fOn, int number = -1);
  void ShowTruePositions();
  void ShowTracks(int num, int OnlyColor = -1);
  void ShowSPS_position();
  void ShowTracksMovie(int num, int steps, int msDelay, double dTheta, double dPhi, double rotSteps, int color = -1);

  void ShowEnergyVector();

  //clear things
  void ClearTracks();
  void ClearMarkers();

  void SaveImage(QString fileName);

private:
  MainWindow* MW;
  TmpObjHubClass* TmpHub;
  DetectorClass* Detector;
};
#endif // GUI
#endif // INTERFACETOGLOBSCRIPT

