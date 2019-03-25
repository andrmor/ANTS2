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

class DetectorClass;
class EventsDataClass;
class GeometryWindowClass;
class GraphWindowClass;
class APmHub;
class TF2;
class AConfiguration;
class AReconstructionManager;
class SensorLRFs;
class TmpObjHubClass;
class APmGroupsManager;
class QJsonValue;

// ====== interfaces which do not require GUI ======

// ---- P M S ----
class AInterfaceToPMs : public AScriptInterface
{
  Q_OBJECT

public:
  AInterfaceToPMs(AConfiguration* Config);
  ~AInterfaceToPMs() {}

  bool IsMultithreadCapable() const override {return true;}

public slots:  
  int      CountPM() const;

  double   GetPMx(int ipm);
  double   GetPMy(int ipm);
  double   GetPMz(int ipm);

  bool     IsPmCenterWithin(int ipm, double x, double y, double distance_in_square);
  bool     IsPmCenterWithinFast(int ipm, double x, double y, double distance_in_square) const;

  QVariant GetPMtypes();
  QVariant GetPMpositions() const;

  void     RemoveAllPMs();
  bool     AddPMToPlane(int UpperLower, int type, double X, double Y, double angle = 0);
  bool     AddPM(int UpperLower, int type, double X, double Y, double Z, double phi, double theta, double psi);
  void     SetAllArraysFullyCustom();

private:
  AConfiguration* Config;
  APmHub* PMs;

  bool checkValidPM(int ipm);
  bool checkAddPmCommon(int UpperLower, int type);
};

// ---- L R F ----
class AInterfaceToLRF : public AScriptInterface
{
  Q_OBJECT

public:
  AInterfaceToLRF(AConfiguration* Config, EventsDataClass* EventsDataHub);

  bool IsMultithreadCapable() const override {return true;}

public slots:
  QString Make();
  double GetLRF(int ipm, double x, double y, double z);
  double GetLRFerror(int ipm, double x, double y, double z);

  QVariant GetAllLRFs(double x, double y, double z);

  //iterations  
  int CountIterations();
  int GetCurrent();
  void SetCurrent(int iterIndex);
  void SetCurrentName(QString name);
  void DeleteCurrent();
  QString Save(QString fileName);
  int Load(QString fileName);

private:
  AConfiguration* Config;
  EventsDataClass* EventsDataHub;
  SensorLRFs* SensLRF; //alias

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

#ifdef GUI // =============== GUI mode only ===============

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
  void ConfigureXYplotExtra(bool suppress0, bool plotVsTrue, bool showPMs, bool showManifest, bool invertX, bool invertY);

  void SetLog(bool Xaxis, bool Yaxis);

  void SetStatPanelVisible(bool flag);

  void AddLegend(double x1, double y1, double x2, double y2, QString title);
  void SetLegendBorder(int color, int style, int size);

  void AddText(QString text, bool Showframe, int Alignment_0Left1Center2Right);

  void AddLine(double x1, double y1, double x2, double y2, int color, int width, int style);

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

  void SetWindowGeometry(QVariant xywh);

private:
  MainWindow* MW;
};

// -- GEOMETRY WINDOW --
class AGeoWin_SI : public AScriptInterface
{
  Q_OBJECT

public:
  AGeoWin_SI(MainWindow* MW, ASimulationManager* SimManager);

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
  QVariant GetWindowGeometry();
  void SetWindowGeometry(QVariant xywh);

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
  ASimulationManager* SimManager;
  DetectorClass* Detector;
};
#endif // GUI

#endif // INTERFACETOGLOBSCRIPT

