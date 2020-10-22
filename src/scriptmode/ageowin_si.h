#ifndef AGEOWIN_SI_H
#define AGEOWIN_SI_H

#include "ascriptinterface.h"

#include <QVariant>
#include <QString>

class MainWindow;
class ASimulationManager;
class DetectorClass;

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
  /*
  double GetPhi();
  double GetTheta();
  void SetPhi(double phi);
  void SetTheta(double theta);
  void Rotate(double Theta, double Phi, int Steps, int msPause = 50);
  */

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
  void ShowCustomNodes(int firstN = -1);
  void ShowSPS_position();
  void ShowTracksMovie(int num, int steps, int msDelay, double dTheta, double dPhi, double rotSteps, int color = -1);

  void ShowEnergyVector();

  //clear things
  void ClearTracks();
  void ClearMarkers();

  void SaveImage(QString fileName);

  // VS
  int  AddTrack();
  void AddNodeToTrack(int trk, float x, float y, float z);
  void DeleteAllTracks();

  void AddMarkers(QVariantList XYZs, int color);

private:
  MainWindow* MW;
  ASimulationManager* SimManager;
  DetectorClass* Detector;
};

#endif // AGEOWIN_SI_H
