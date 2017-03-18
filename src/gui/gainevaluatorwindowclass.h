#ifndef GAINEVALUATORWINDOWCLASS_H
#define GAINEVALUATORWINDOWCLASS_H

#include "flatfield.h"

#include <QMainWindow>
#include <QSet>

class MainWindow;
class ReconstructionWindow;
class MainWindow;
class QLineEdit;
class QGraphicsScene;
class myQGraphicsView;
class QGraphicsItem;
class TH1D;
class EventsDataClass;

class CenterGroupClass
{
    double Distance;
    double Tolerance;

    double MinDist2;
    double MaxDist2;

  public:
    void setDistance(double distance) {Distance = distance; Update();}
    double getDistance() {return Distance;}
    void setTolerance(double tolerance) {Tolerance = tolerance; Update();}
    double getTolerance() {return Tolerance;}
    double getMinDist2() {return MinDist2;}
    double getMaxDist2() {return MaxDist2;}

    CenterGroupClass() {}
    CenterGroupClass(double distance, double tolerance) : Distance(distance), Tolerance(tolerance) {Update();}

  private:
    void Update()
      {
        MinDist2 = Distance - Tolerance; MinDist2 *= MinDist2;
        MaxDist2 = Distance + Tolerance; MaxDist2 *= MaxDist2;
      }
};

struct FourPMs
{
  int PM[4];
  bool Symmetric;  //true - square, false - rhombus

  FourPMs() {}
  FourPMs(int i0, int i1, int i2, int i3, bool sym) { PM[0]=i0; PM[1]=i1; PM[2]=i2; PM[3]=i3; Symmetric = sym;}
  FourPMs(QList<int> pmList, bool sym) { if (pmList.size()<4) return; for (int i=0; i<4; i++) PM[i]=pmList[i]; Symmetric = sym;}
};

namespace Ui {
  class GainEvaluatorWindowClass;
}

class GainEvaluatorWindowClass : public QMainWindow
{
  Q_OBJECT

public:
  explicit GainEvaluatorWindowClass(QWidget* parent, MainWindow *mw, EventsDataClass *eventsDataHub);
  ~GainEvaluatorWindowClass();

  void Reset();

  void UpdateEvaluator(); //prepare all variables, updates visualization

  void ResetViewport();

  bool PMsCoverageCheck();

  double CalculateAreaFraction(int ipm1, int ipm2); //return area(pm1)/area(pm2)
  double getPolygonArea(QPolygonF &qpolygonf); //for non-self intersecting polygons!
  void SetWindowFont(int ptsize);

public slots:
  void UpdateGraphics();
  void onCurrentSensorGroupsChanged();

protected:
  void resizeEvent(QResizeEvent *event);
  bool event(QEvent *event);

private slots:
  void sceneSelectionChanged(); //on scene selection change


  void on_pbEvaluateGains_clicked();

  void on_pbUpdateUI_clicked();

  void on_pbUpdateCutOff_clicked();

  void on_pbCutOffsAddAll_clicked();

  void on_pbCutOffsRemoveAll_clicked();

  void on_pbCutOffsAdd_clicked();

  void on_leCutOffsSet_editingFinished();

  void on_pbCutOffsRemove_clicked();

  void on_pbCutOffsAddCenterPMs_clicked();

  void on_pbCentersAddAll_clicked();

  void on_pbCentersRemoveAll_clicked();

  void on_leCentersSet_editingFinished();

  void on_pbCentersAdd_clicked();

  void on_pbCentersRemove_clicked();

  void on_ledCentersDistance_editingFinished();

  void on_ledCentersTolerance_returnPressed();

  void on_pbUpdateCenters_clicked();

  void on_ledCenterTopFraction_editingFinished();

  void on_sbCentersGroupNumber_valueChanged(int arg1);

  void on_pbCentersAddGroup_clicked();

  void on_pbCentersRemoveGroup_clicked();

  void on_ledCutOffFraction_editingFinished();

  void on_pbUpdate4_clicked();

  void on_pb4add_clicked();

  void on_pb4remove_clicked();

  void on_sb4bins_valueChanged(int arg1);

  void on_led4overlap_editingFinished();

  void on_le4newQuartet_editingFinished();

  void on_cob4segments_currentIndexChanged(int index);

  void on_led4tolerance_editingFinished();

  void on_pbUpdateGraphics_clicked();

  void on_twAlgorithms_currentChanged(int index);

  void on_pbtShowNeighbourGroups_pressed();

  void on_pbtShowNeighbourGroups_released();

  void on_pb4removeAll_clicked();

  void on_pbEvaluateRatio_clicked();

  void on_pbCalcAverage_clicked();

  void on_pbUpdateUniform_clicked();

  void on_pbUpdateLogR_clicked();

  void on_pbShowNonLinkedSet_clicked();

  void on_pbLogAddAll_clicked();

  void on_pbLogAdd_clicked();

  void on_pbLogRemoveAll_clicked();

  void on_pbLogRemove_clicked();

private:
  Ui::GainEvaluatorWindowClass *ui;

  MainWindow* MW;
  EventsDataClass* EventsDataHub;
  bool TMPignore; //temporaly ignore update

  int CurrentGroup;

  int Equations;
  int Variables;
  QList<int> iPMs; //indeces of all PMs to be used in gain reconstruction
  QList<int> iCutOffPMs; //indeces of PMs to be used in CutOffs gain eval procedure
  QList<int> iCentersPMs; //indeces of PMs to be used in CutOffs gain eval procedure
  QList<int> iLogPMs; //indeces of PMs to be used in Log gain eval procedure
  QVector<FourPMs> iSets4PMs;

  QVector<CenterGroupClass> CenterGroups;
  double CenterTopFraction;
  double CutOffFraction;  
  int Bins4PMs;
  double Fraction4PMs;
  QString Overlap4PMs;
  double Tolerance4PMs;

  QGraphicsScene *scene;
  myQGraphicsView *gvGains;
  double GVscale;
  QVector<QGraphicsItem*> PMicons;

  QList<int> SelectedPMsFor4Set;  //tmp set used to draw new 4pm set
  QVector< QSet<int> > SetNeighbour; //all neigbours by group
  QVector< QSet<int> > AllLinks; //link between all PMs - used in check of all PM coverage;

  bool flagShowNeighbourGroups;
  bool flagAllPMsCovered;
  QSet<int> firstUncoveredSet;
  bool flagShowingLinked;
  bool flagShowNonLinkedSet;

  QIcon RedIcon, GreenIcon;

  TH1D *tmpHistForLogR;

  FlatField Triads;

  void cutOffsPMselected(int ipm);
  void centersPMselected(int ipm);
  void set4PMselected(int ipm);
  void AddRemovePMs(QChar option, QString input, QList<int> *data);
  void findNeighbours(int ipm, int igroup, QList<int> *NeighboursiPMindexes);
  bool validate4PMsInput();
  void FindEventsInMiddle(int ipm1, int ipm2, int ipm3, int ipm4, QList<int> *Sorter, QList<int> *Result);
  void FindEventsInCone(int ipmFrom, int ipmTo, QList<int> *Sorter, QList<int> *Result);

  void BuildIndexationData();
  double AdjustGainPair(int ipm1, int ipm2); //Uniform:  adjusts gains: gain(ipm1)=1, returns gain(ipm2)
  double AdjustGainPairLogR(int ipm1, int ipm2); //LogR: adjusts gains: gain(ipm1)=1, returns gain(ipm2)

  bool isPMDistanceCheckFail(int ipm1, int ipm2);
  bool isPMDistanceCheckFailLogR(int ipm1, int ipm2);
  void logPMselected(int ipm);
  void UpdateTriads();
};

#endif // GAINEVALUATORWINDOWCLASS_H
