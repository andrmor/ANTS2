#ifndef RECONSTRUCTIONWINDOW_H
#define RECONSTRUCTIONWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>

#include "TMathBase.h"

namespace Ui {
  class ReconstructionWindow;
}

class MainWindow;
class TTree;
class TH1D;
class TH2D;
class TLine;
class TEllipse;
class TPolyLine;
class Viewer2DarrayObject;
class QComboBox;
class EventsDataClass;
struct CorrelationFilterStructure;
class QSpinBox;
class QLineEdit;
class ReconstructionManagerClass;
class TObject;
class DetectorClass;
class APmGroupsManager;
class pms;

class linkCustomClass
{
public:
  QSpinBox* Qbins;
  int bins;
  QLineEdit* Qfrom;
  double from;
  QLineEdit* Qto;
  double to;
  linkCustomClass() {} //not used
  linkCustomClass(QSpinBox* Qbins, QLineEdit* Qfrom, QLineEdit* Qto) : Qbins(Qbins), Qfrom(Qfrom), Qto(Qto) {bins=100; from = 0; to = 0;}
  void updateBins();
  void updateRanges();
};

struct TreeViewStruct
{
    QString what;
    QString cuts;
    QString options;

    int Bins[3];
    double From[3];
    double To[3];
    bool fCustomBins, fCustomRanges;

    TreeViewStruct() {}
    TreeViewStruct(QString What, QString Cuts, QString Options) {what = What; cuts = Cuts; options = Options;}
    void fillCustom(bool fCustomBins, bool fCustomRanges, QVector<linkCustomClass> &cus);
};

class ReconstructionWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit ReconstructionWindow(QWidget *parent, MainWindow *mw, EventsDataClass *eventsDataHub);
  ~ReconstructionWindow();

  MainWindow *MW; //public to be visible by gain evaluator
  QVector<CorrelationFilterStructure*> CorrelationFilters;
  CorrelationFilterStructure* tmpCorrFilter;

  QIcon RedIcon;
  QIcon YellowIcon;

  //GUI updates:
public slots:
  void onRequestEventsGuiUpdate();
  void OnEventsDataAdded();
  void onUpdatePMgroupIndication();
  void onRequestReconstructionGuiUpdate();  // Update reconstruction GUI from settings in Config object
  void onManifestItemsGuiUpdate();
  void onReconstructionFinished(bool fSuccess, bool fShow);
  void onUpdateGainsIndication();
  void onUpdatePassiveIndication();  //only static passives!

public:
  void UpdateStatusAllEvents();

  //config reconstruction module
  void InitWindow(); //configures properties defined in the interface

  void PMnumChanged();

  void ShowEnergySpectrum();  
  void ShowChi2Spectrum();

  void ShowFiltering();

  void ResetStop(); //reset stop button
  void SetProgress(int val);

  void onBusyOn();
  void onBusyOff();

  void UpdateSimVizData(int iev);

  void showCorrelation(const char *title, const char *xTitle, const char *yTitle, QVector<double> *x, QVector<double> *y);

  void HideCutShapes(); //hide correlation cut shapes

  void DotActualPositions();

  void SaveIndividualCutOffs(QString fileName);
  void LoadIndividualCutOffs(QString fileName);

  void ShowGainWindow();
  void VisualizeScan(int iev);

  bool GetCenterShift(int ipm, bool fDraw = false);
  void EvaluateShiftBrute(int ipm, double *dx, double *dy);
  double getSuggestedZ() const;
  double isReconstructEnergy() const;

  void writeToJson(QJsonObject &json); //all settings
  void updateReconSettings();
  void updateFilterSettings();
  bool readFromJson(QJsonObject &json); //all settings except LRF

  bool readReconSettingsFromJson(QJsonObject &jsonMaster);
  bool readFilterSettingsFromJson(QJsonObject &jsonMaster);
  void writeLrfModuleSelectionToJson(QJsonObject &json);
  void readLrfModuleSelectionFromJson(QJsonObject &json);
  void writeMiscGUIsettingsToJson(QJsonObject &json);
  void readMiscGUIsettingsFromJson(QJsonObject &json);
  void updateFiltersGui();

public slots:
  void ShowStatistics(bool bCopyToTextLog = false);
  void LRF_ModuleReadySlot(bool ready);
  void onSelectionChange(QVector<int> selectionArray);

  //can be triggered by script
  void on_pbReconstructAll_clicked();
  void ConfigurePlotXY(int binsX, double X0, double X1, int binsY, double Y0, double Y1);
  void DoPlotXY(int i);
  void DoBlur(double sigma, int type = 0);  //type 0 - uniform, type 1 - gauss
  void DoCenterShift(int ipm);
  void updateGUIsettingsInConfig();

  int getReconstructionAlgorithm();
  void on_pbUpdateFilters_clicked();

private slots:
  void PaintPMs();

  void on_pbShowLRFwindow_clicked();

  void on_pbReconstructTrack_clicked();

  void on_cobReconstructionAlgorithm_currentIndexChanged(int index);

  void on_sbEventNumberInspect_valueChanged(int arg1);

  void on_sbPMsignalIndividual_valueChanged(int arg1);

  void on_pbCutOffsLoad_clicked();

  void on_pbCutOffsSave_clicked();

  void on_ledFilterIndividualMin_editingFinished();

  void on_ledFilterIndividualMax_editingFinished();

  void on_pbShowSumSignal_clicked();

  void on_pbShowIndividualSpectrum_clicked();

  void on_pbGoToNextEvent_clicked();

  void on_pbShowReconstructionPositions_clicked();

  void on_pbShowMeasuredVsExpected_clicked();

  void on_pbAnalyzeResolution_clicked();

  void on_pbPointShowDistribution_clicked();

  void on_sbAnalysisEventNumber_valueChanged(int arg1);

  void on_pbShowSigmaVsArea_clicked();

  void on_pbSaveScanTree_clicked();

  void on_pbConfigureNN_clicked();

  void on_pbLoadLRFs_clicked();

  void on_pbSaveLRFs_clicked();

  void on_pbEnergySpectrum1_clicked();

  void on_pbEnergySpectrum2_clicked();

  void on_pbChi2Spectrum1_clicked();

  void on_pbChi2Spectrum2_clicked();

  void on_sbZshift_valueChanged(int arg1);

  void on_cobHowToAverageZ_currentIndexChanged(int index);

  void on_pbShowSpatialFilter_clicked();

  void on_pbSpF_UpdateTable_clicked();

  void on_pbSpF_AddLine_clicked();

  void on_pbSpF_RemoveLine_clicked();

  void on_tabwidSpF_cellChanged(int row, int column);

  void on_pbSpF_AddLineAfter_clicked();

  void on_pbGoToNextNoise_clicked();

  void on_pbGoToNextNoiseFoundGood_clicked();

  void on_pbStopReconstruction_toggled(bool checked);

  void on_pbCutOffApplyToAll_clicked();

  void on_cbRecType_currentIndexChanged(int index);

  void on_pbAddToActive_clicked();

  void on_pbAddToPassive_clicked();

  void on_pbShowPassivePMs_clicked();

  void on_pbAllPMsActive_clicked();

  void on_pbSaveReconstructionAsText_clicked();

  void on_pbSaveReconstructionAsRootTree_clicked();

  void on_pbMainWindow_clicked();

  void on_pbShowLog_clicked();

  void on_pbDoAutogroup_clicked();

  void on_pbUpdateGainsIndication_clicked();

  void on_leiPMnumberForGains_editingFinished();

  void on_ledGain_editingFinished();

  void on_pbShowAllGainsForGroup_clicked();

  void on_pbGainsToUnity_clicked();

  void on_pbFindNextPMinGains_clicked();

  void on_pbSaveGains_clicked();

  void on_pbLoadGains_clicked();

  void on_twOptions_currentChanged(int index);

  void on_pbClearGroups_clicked();

  void on_pbAssignPMtoGroup_clicked();

  void on_pbDefineNewGroup_clicked();

  void on_pbShowVsXY_clicked();

  void on_cbPlotVsActualPosition_toggled(bool checked);

  void on_cb3Dreconstruction_toggled(bool checked);

  void on_pbShowCorr_clicked();

  void on_pbDefSumCutOffs_clicked();

  void on_pbDefSumCutOffs2_clicked();

  void on_pbDefCutOffMin_clicked();

  void on_pbDefCutOffMax_clicked();

  void on_pbCutOffsNextPM_clicked();

  void on_pbDefEnMin_clicked();

  void on_pbDefEnMax_clicked();

  void on_pbDefChi2Min_clicked();

  void on_pbDefChi2Max_clicked();

  void on_cbCorrAutoSize_toggled(bool checked);

  void on_leCorrList1_editingFinished();

  void on_leCorrList2_editingFinished();

  void on_cobCorr1_currentIndexChanged(int index);

  void on_cobCorr2_currentIndexChanged(int index);

  void on_pbCorrUpdateTMP_clicked();

  void on_pbCorrExtractLine_clicked();

  void on_pbCorrRemove_clicked();

  void on_pbCorrNew_clicked();

  void on_pbCorrAccept_clicked();

  void on_cbCorrActive_toggled(bool checked);

  void on_sbCorrFNumber_valueChanged(int arg1);

  void on_pbCorrExtractEllipse_clicked();

  void on_cbActivateCorrelationFilters_toggled(bool checked);

  void on_pbDefLoadEnMin_clicked();

  void on_pbDefLoadEnMax_clicked();

  void on_pbLoadEnergySpectrum_clicked();

  void on_pbShowCorrShowRangeOnGraph_clicked();

  void on_pbCorr_AddLine_clicked();

  void on_ppCorr_AddLineAfter_clicked();

  void on_pbCorr_RemoveLine_clicked();

  void on_tabwidCorrPolygon_cellChanged(int row, int column);

  void on_pbCorrExtractPolygon_clicked();

  void on_pbEvaluateGains_clicked();

  void on_pbSetPresprocessingForGains_clicked();

  void on_pbPurgeEvents_clicked();

  void on_cbCoGapplyStretch_toggled(bool checked);

  void on_pbTreeView_clicked();

  void on_pbTreeViewHelpWhat_clicked();

  void on_pbTreeViewHelpCuts_clicked();

  void on_pbTreeViewHelpOptions_clicked();

  void on_sbTreeViewHistory_valueChanged(int arg1);

  void on_cobCUDAoffsetOption_currentIndexChanged(int index);

  void on_pbEvaluateZfromDistribution_clicked();

  void on_pbBlurReconstructedXY_clicked();

  void on_pbNNshowSpectrum_clicked();

  void on_pbDefNNMin_clicked();

  void on_pbDefNNMax_clicked();

  void on_pbShowMap_clicked();

  void on_cobMapZ_currentIndexChanged(int index);

  void on_pbTogglePassives_clicked();

  void on_pbGainsMultiply_clicked();

  void on_cobShowWhat_currentIndexChanged(int index);

  void on_pbExportData_clicked();

  void on_pbCenterControlEvaluate_clicked();

  void on_pbMakePictureVsXY_clicked();

  void on_pbEvaluateAndShift_clicked();

  void on_pbShiftAll_clicked();

  void on_pbShiftAllWithDis_clicked();

  void on_pbEvaluateGrid_clicked();

  void on_pbEvAllBruteThenShift_clicked();

  void on_pbSensorGainsToPMgains_clicked();

  void on_cbCustomBinning_toggled(bool checked);

  void on_cbCustomRanges_toggled(bool checked);

  void on_cbForceCoGgiveZof_toggled(bool checked);

  void on_cobInRecShape_currentIndexChanged(int index);

  void on_pbCopyFromRec_clicked();

  void on_pbAnalyzeChanPerPhEl_clicked();

  void on_pbChanPerPhElShow_clicked();

  void on_sbChanPerPhElPM_valueChanged(int arg1);

  void on_pbExtractChansPerPhEl_clicked();

  void on_pbGotoPreprocessAdjust_clicked();

  void on_pbResetKNNfilter_clicked();

  void on_pbCalibrateX_clicked();

  void on_pbCalibrateY_clicked();

  void on_pbCalibrateXY_clicked();

  void on_cobKNNcalibration_activated(int index);

  void on_pbKNNupdate_clicked();

  void on_sbKNNuseNeighboursInRec_editingFinished();

  void on_cobSpFXY_currentIndexChanged(int index);

  void on_cbReconstructEnergy_toggled(bool checked);

  void on_cobZ_currentIndexChanged(int index);

  void on_cbLimitNumberEvents_toggled(bool checked);

  void on_sbXbins_editingFinished();

  void on_cbXYsymmetric_toggled(bool checked);

  void on_ledXto_editingFinished();

  void on_pbTrueToRec_clicked();

  void on_pbTrueScanToManifest_clicked();

  void on_pbManifestLineAttributes_clicked();

  void on_pbShiftManifested_clicked();

  void on_pbDeclareNumRuns_clicked();

  void on_pbPurgeFraction_clicked();

  void on_pbRotateManifested_clicked();

  void on_pbAddValueToAllSignals_clicked();

  void on_pbPMTcol_clicked();

  void on_pbUpdateReconConfig_clicked();

  void on_cobCurrentGroup_activated(int index);

  void on_pbConfigureGroups_toggled(bool checked);

  void on_pbBackToRecon_clicked();

  void on_lwPMgroups_customContextMenuRequested(const QPoint &pos);

  void on_pbRemovePMfromGroup_clicked();

  void on_pbShowUnassigned_clicked();

  void on_lwPMgroups_activated(const QModelIndex &index);

  void on_lwPMgroups_clicked(const QModelIndex &index);

  void on_cobLRFmodule_currentIndexChanged(int index);

  void on_sbShowEventsEvery_valueChanged(int arg1);

  void on_lwPMgroups_doubleClicked(const QModelIndex &index);

  void on_pbPMsAsGroup_clicked();

  void on_cobAssignToPMgroup_currentIndexChanged(int index);

  void on_pbClearThisGroup_clicked();

  void on_pbShowSensorGroup_clicked();

  void on_leSpF_LimitToObject_textChanged(const QString &arg1);

  void on_cbSpF_LimitToObject_toggled(bool checked);

  void on_pbUpdateGuiSettingsInJSON_clicked();

  void on_pbBlurReconstructedZ_clicked();

  void on_pbShowTruePositions_clicked();

  void on_pbClearPositions_clicked();

  void on_pbRecToTrue_clicked();

  void on_pbPrepareSignalHistograms_clicked();

  void on_pbFromPeaksToPreprocessing_clicked();

  void on_pbFromPeaksShow_clicked();

  void on_pbFrindPeaks_clicked();

  void on_sbFrompeakPM_valueChanged(int arg1);

  void on_pbFromPeaksPedestals_clicked();

protected:
    bool event(QEvent *event);

private:
  Ui::ReconstructionWindow *ui;

  //aliases
  EventsDataClass* EventsDataHub;
  ReconstructionManagerClass* ReconstructionManager; //only pointer!
  DetectorClass* Detector;
  APmGroupsManager* PMgroups;
  pms* PMs;

  QStandardItemModel *modelSpF_TV;

  QVector<TreeViewStruct> TreeViewHistory;

  Viewer2DarrayObject* vo;
  QDialog* dialog;
  int PMcolor, PMwidth, PMstyle;

  bool TableLocked;
  bool StopRecon; //stop button flag

  int ShiftZoldValue;
  bool ForbidUpdate;
  bool TMPignore;
  bool bFilteringStarted;

  double lastChi2;

  Double_t funcParams[4];

  QPolygonF polygon; //polygon for custom spatial filter

  QString FilterScript;

  int ManifestLineColor, ManifestLineStyle, ManifestLineWidth;

  void DrawPolygon(double z);  
  void PolygonToTable();
  void TableToPolygon();

  void VisualizeEnergyVector(int eventId);
  void VisualizeScan();

  void AnalyzeScanNode();
  bool BuildResolutionTree(int igroup);

  bool ShowVsXY(QString strIn);

  void MakeActivePassive(QString input, QString mode);

  void ReconstructTrack();

  bool startXextraction();
  bool start2DLineExtraction();
  bool start2DEllipseExtraction();
  bool start2DBoxExtraction();  
  bool start2DPolygonExtraction();

  void updateRedStatusOfRecOptions();
  void RefreshNumEventsIndication();

  void DrawCuts();
  TLine* CorrCutLine;
  TEllipse* CorrCutEllipse;
  TPolyLine* CorrCutPolygon;
  void FillUiFromTmpCorrFilter();

  QVector<TLine *> PrepareMarker(double x0, double y0, double len, Color_t color);
  void UpdatePMGroupCOBs();
  void UpdateOneGroupCOB(QComboBox *cob);

  void ShowIndividualSpectrum(bool focus); //true - window is focussed  

  void SetGuiBusyStatusOn();
  void SetGuiBusyStatusOff();

  double extractChPerPHEl(int ipm);
  bool IntersectionOfTwoLines(double A1, double B1, double C1, double A2, double B2, double C2, double* result);

  void showSensorGroup(int igroup);
public slots:
  void RefreshOnTimer(int Progress, double usPerEv); //update timer for reconstruction progress

  void onKNNreadyXchanged(bool ready, int events);
  void onKNNreadyYchanged(bool ready, int events);

  void ShowPositions(int Rec_True, bool fOnlyIfWindowVisible = false);
signals:

  void cbReconstructEnergyChanged(bool changed);
  void cb3DreconstructionChanged(bool changed);
  void StopRequested(); //to be used to request stop

public:
  bool isReconstructEnergy();
  bool isReconstructZ();
  double getSuggestedEnergy();
  double getSuggestedZ();

  void ReconstructAll(bool fShow = true);

  //for script mode:
  void SetShowFirst(bool fOn, int number = -1);
  void UpdateReconConfig();
};

#endif // RECONSTRUCTIONWINDOW_H
