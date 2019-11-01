#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "aguiwindow.h"
#include "ag4simulationsettings.h"

#include <QMainWindow>
#include <QVector>
#include <QThread>
#include <QTimer>
#include <QJsonObject>

// forward declarations
class AConfiguration;
class AGlobalSettings;
class GeoMarkerClass;
class AMaterialParticleCollection;
class EventsDataClass;
class GeometryWindowClass;
class GraphWindowClass;
class LRFwindow;
class ReconstructionWindow;
class ExamplesWindow;
class CheckUpWindowClass;
class DetectorAddOnsWindow;
class APmHub;
class AReconstructionManager;
class MaterialInspectorWindow;
class OutputWindow;
class QComboBox;
class TH1D;
class TH1I;
class WindowNavigatorClass;
class GlobalSettingsWindowClass;
class GainEvaluatorWindowClass;
class TApplication;
class Viewer2DarrayObject;
class DetectorClass;
class TmpObjHubClass;
class ASlabListWidget;
class InterfaceToPMscript;
class QMessageBox;
class QListWidgetItem;
class QFile;
class ASimulationManager;
class AScriptWindow;
class ALrfWindow;
class ANetworkModule;
struct AParticleSourceRecord;
class ARemoteWindow;
class AWebSocketServerDialog;
class AParticleGun;

#ifdef ANTS_FANN
class NeuralNetworksWindow;
#endif


namespace Ui {
class MainWindow;
}

class MainWindow : public AGuiWindow
{
    Q_OBJECT
    
public:
    MainWindow(DetectorClass *Detector,
               EventsDataClass *EventsDataHub,
               TApplication *RootApp,
               ASimulationManager *SimulationManager,
               AReconstructionManager *ReconstructionManager,
               ANetworkModule *Net,
               TmpObjHubClass *TmpHub);
    ~MainWindow();

    // Pointers to external resources
    DetectorClass *Detector = 0;
    APmHub* PMs = 0;
    AMaterialParticleCollection* MpCollection = 0;
    AConfiguration* Config = 0;
    EventsDataClass *EventsDataHub = 0;
    TApplication *RootApp = 0;
    ASimulationManager* SimulationManager = 0;
    AReconstructionManager *ReconstructionManager = 0;
    ANetworkModule* NetModule = 0;
    TmpObjHubClass *TmpHub = 0;
    AGlobalSettings& GlobSet;

    // ANTS2 windows
    GraphWindowClass *GraphWindow = 0;
    GeometryWindowClass *GeometryWindow = 0;
    OutputWindow *Owindow = 0;
    LRFwindow *lrfwindow = 0;                       //window of the v3 LRF module
    ReconstructionWindow *Rwindow = 0;
    MaterialInspectorWindow *MIwindow = 0;
    WindowNavigatorClass *WindowNavigator = 0;
    ExamplesWindow* ELwindow = 0;
    DetectorAddOnsWindow* DAwindow = 0;
    CheckUpWindowClass* CheckUpWindow = 0;
    GainEvaluatorWindowClass* GainWindow = 0;
    AScriptWindow* GenScriptWindow = 0;  //local script window
    GlobalSettingsWindowClass* GlobSetWindow = 0;
    AScriptWindow* ScriptWindow = 0;                //global script window
    ALrfWindow* newLrfWindow = 0;                   //window of the v3 LRF module
    AScriptWindow* PythonScriptWindow = 0;
    ARemoteWindow* RemoteWindow = 0;
    AWebSocketServerDialog* ServerDialog = 0;

#ifdef ANTS_FANN
    NeuralNetworksWindow* NNwindow = 0;
#endif

    // custom gui elements
    ASlabListWidget* lw = 0;

    //local data, just for GUI
    QVector<GeoMarkerClass*> GeoMarkers;

    InterfaceToPMscript* PMscriptInterface = 0;       // if created -> managed by the script manager

    //critical - updates
    void NumberOfPMsHaveChanged();

    //busy triggered on start of user action that can take long time and user should not be able
    //to change any settins
    void onBusyOn();
    void onBusyOff();

    void startRootUpdate();  // starts timer which in regular intervals process Root events
    void stopRootUpdate();   // stops timer which in regular intervals process Root events

    void ClearData(bool bKeepEnergyVector = false); //clear status, disconnect reconstruction

    //detector constructor
    void ReconstructDetector(bool fKeepData = false);

    void ListActiveParticles();

    void ShowGraphWindow();
    void UpdateMaterialListEdit();

    void UpdateTestWavelengthProperties(); //if material properties were updated, need to update indication in the Test tab

    void writeDetectorToJson(QJsonObject &json); //GDML is NOT here
    bool readDetectorFromJson(QJsonObject &json);
    void writeSimSettingsToJson(QJsonObject &json);
    bool readSimSettingsFromJson(QJsonObject &json);

    //save data to file - public due to batch mode usage
    int LoadSimulationDataFromTree(QString fileName, int maxEvents = -1);
    int LoadPMsignals(QString fileName);

    //configuration from outside
    void SetProgress(int val);

    //gains and ch per ph.el
    void SetMultipliersUsingGains(QVector<double> Gains);
    void SetMultipliersUsingChPhEl(QVector<double> ChPerPhEl);
    void CorrectPreprocessingAdds(QVector<double> Add);

    //public flags
    bool DoNotUpdateGeometry;  //if GUI is in bulk-update, we do not detector geometry be updated on each line
    bool GeometryDrawDisabled = false; //no drawing of the geometry or tracks
    bool fStartedFromGUI = false;          //flag indicating that an action was run from GUI, e.g. simulation

    bool isWavelengthResolved() const;
    double WaveFrom, WaveTo, WaveStep;
    int WaveNodes;

    TH1D *histSecScint = 0;

    int ScriptWinX, ScriptWinY, ScriptWinW, ScriptWinH;
    void recallGeometryOfLocalScriptWindow();
    void extractGeometryOfLocalScriptWindow();

    void LoadSecScintTheta(QString fileName);

    int PMArrayType(int ul);
    void SetPMarrayType(int ul, int itype);

    void LoadDummyPMs(QString DFile);

    void ShowGeoMarkers(); //Show dots on ALREADY PREPARED geometry window!

    void CheckPresenseOfSecScintillator();
    void DeleteLoadedEvents(bool KeepFileList = false);       
    void SavePreprocessingAddMulti(QString fileName);
    void LoadPreprocessingAddMulti(QString filename);

public slots:   
    void LoadEventsListContextMenu(const QPoint &pos);
    void LRF_ModuleReadySlot(bool ready);
    void on_pbSimulate_clicked();
    void on_pbParticleSourcesSimulate_clicked();    
    void RefreshOnProgressReport(int Progress, double msPerEv);
    void PMscriptSuccess();
    void onGDMLstatusChage(bool fGDMLactivated);
    void updateLoaded(int events, int progress);
    void on_pbSingleSourceShow_clicked();
    void ShowGeometrySlot();

private slots:
    void updateFileParticleGeneratorGui();
    void updateScriptParticleGeneratorGui();

    void on_pbRefreshMaterials_clicked();
    void on_cbXbyYarray_stateChanged(int arg1);
    void on_cbRingsArray_stateChanged(int arg1);
    void on_pbRefreshOverrides_clicked();
    void on_pbStartMaterialInspector_clicked();
    void on_pbRefreshParticles_clicked();
    void on_cbSecondAxis_toggled(bool checked);
    void on_cbThirdAxis_toggled(bool checked);
    void on_cobPMdeviceType_currentIndexChanged(int index);
    void on_cbUPM_toggled(bool checked);
    void on_cbLPM_toggled(bool checked);
    void on_pbRefreshPMArrayData_clicked();
    void on_pbShowPMsArrayRegularData_clicked();
    void on_pbRefreshPMproperties_clicked();
    void on_pbUpdatePMproperties_clicked();
    void on_sbPMtype_valueChanged(int arg1);
    void on_sbPMtypeForGroup_valueChanged(int arg1);
    void on_cbXbyYarray_clicked();
    void on_cbRingsArray_clicked();
    void on_cobUpperLowerPMs_currentIndexChanged(int index);
    void on_pbRemoveThisPMtype_clicked();
    void on_pbAddNewPMtype_clicked();
    void on_cbAreaSensitive_toggled(bool checked);
    void on_cbAngularSensitive_toggled(bool checked);
    void on_cbTimeResolved_toggled(bool checked);
    void on_pbSavePMtype_clicked();
    void on_pbLoadPMtype_clicked();
    void on_pbPMtypeShowCurrent_clicked();
    void on_cbWaveResolved_toggled(bool checked);
    void on_ledWaveFrom_editingFinished();
    void on_ledWaveTo_editingFinished();
    void on_ledWaveStep_editingFinished();
    void on_pbUpdateTestWavelengthProperties_clicked();
    void on_pbTestShowPrimary_clicked();
    void on_pbTestShowSecondary_clicked();
    void on_cobMaterialForWaveTests_currentIndexChanged(int index);
    void on_pbTestGeneratorPrimary_clicked();
    void on_pbTestGeneratorSecondary_clicked();
    void on_pbTestShowRefrIndex_clicked();
    void on_pbTestShowAbs_clicked();
    void on_sbFixedWaveIndexPointSource_valueChanged(int arg1);
    void on_pbShowPDE_clicked();
    void on_pbLoadPDE_clicked();
    void on_pbDeletePDE_clicked();
    void on_pbShowPDEbinned_clicked();
    void on_pbPMtypeLoadAngular_clicked();
    void on_pbPMtypeShowAngular_clicked();
    void on_pbPMtypeDeleteAngular_clicked();
    void on_pbPMtypeShowEffectiveAngular_clicked();
    void on_pbGunTest_clicked();
    void on_ledGunAverageNumPartperEvent_editingFinished();
    void on_ledMediumRefrIndex_editingFinished();
    void on_pbPMtypeLoadArea_clicked();
    void on_pbPMtypeShowArea_clicked();
    void on_pbPMtypeDeleteArea_clicked();
    void on_cobPMarrayRegularity_currentIndexChanged(int index);
    void on_sbIndPMnumber_valueChanged(int arg1);
    void on_pbIndPMshowInfo_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_pbUpdateToFullCustom_clicked();
    void on_pbUpdateToFixedZ_clicked();
    void on_pbIndPmRemove_clicked();
    void on_pbIndShowType_clicked();
    void on_pbIndShowDE_clicked();
    void on_pbIndShowDEbinned_clicked();
    void on_pbAddPM_clicked();
    void on_pbIndShowAngular_clicked();
    void on_pbIndShowEffectiveAngular_clicked();
    void on_pbIndShowArea_clicked();
    void on_cbGunAllowMultipleEvents_toggled(bool checked);
    void LoadPMsignalsRequested();
    void on_pbDeleteLoadedEvents_clicked();
    void on_pbPreprocessingLoad_clicked();
    void on_pbPreprocessingSave_clicked();
    void on_sbPreprocessigPMnumber_valueChanged(int arg1);
    void on_ledPreprocessingAdd_editingFinished();
    void on_pbReloadExpData_clicked();
    void on_cobPMshape_currentIndexChanged(int index);
    void on_pbElGainLoadDistr_clicked();
    void on_pbElGainShowDistr_clicked();
    void on_pbElTestGenerator_clicked();
    void on_sbElPMnumber_valueChanged(int arg1);
    void on_pbElUpdateIndication_clicked();
    void on_pbElCopyGainData_clicked();
    void on_cbEnableSPePHS_toggled(bool checked);
    void on_cbEnableADC_toggled(bool checked);
    void on_pbScanDistrLoad_clicked();
    void on_pbScanDistrShow_clicked();
    void on_pbScanDistrDelete_clicked();
    void on_actionWindow_navigator_triggered();
    void on_actionGeometry_triggered();
    void on_actionReconstructor_triggered();
    void on_actionOutput_triggered();
    void on_actionLRF_triggered();
    void on_actionGraph_triggered();
    void on_actionReset_position_of_windows_triggered();
    void on_actionSave_position_and_stratus_of_all_windows_triggered();
    void on_actionLoad_positions_and_status_of_all_windows_triggered();
    void on_actionMaterial_inspector_window_triggered();
    void on_cbEnableElNoise_toggled(bool checked);
    void on_actionExamples_triggered();
    void on_cobSecScintillationGenType_currentIndexChanged(int index);
    void on_pbSecScintShowProfile_clicked();
    void on_pbSecScintLoadProfile_clicked();
    void on_pbSecScintDeleteProfile_clicked();
    void on_lwMaterials_currentRowChanged(int currentRow);
    void on_lwMaterials_doubleClicked(const QModelIndex &index);
    void on_lwParticles_currentRowChanged(int currentRow);
    void on_pbReconstruction_clicked();
    void on_pbConfigureAddOns_clicked();
    void LoadSimTreeRequested();
    void on_pbRemoveSource_clicked();
    void on_pbAddSource_clicked();
    void on_pbUpdateSourcesIndication_clicked();
    void on_pbGunShowSource_toggled(bool checked);

protected:
    void closeEvent(QCloseEvent *event);
    bool event(QEvent *event);

private:
    Ui::MainWindow *ui;
    QTimer *RootUpdateTimer = 0; //root update timer
    QMessageBox *msBox = 0; //box to be used to confirm discard or save sim data on data clear; 0 if not activated

    //flags
    bool TriggerForbidden = false;
    bool BulkUpdate = false;

    TH1I* histScan = 0;

public:
    bool ShutDown = false; //when exiting ANTS2 by closing the main window

    bool fSimDataNotSaved = false;

    QJsonObject OvTesterSettings;

    void createScriptWindow();

    void SimGeneralConfigToJson(QJsonObject &jsonMaster);                              //Save to JSON general options of simulation
    void SimPointSourcesConfigToJson(QJsonObject &jsonMaster);  //Save to JSON config for PointSources simulation
    void SimParticleSourcesConfigToJson(QJsonObject &json);     //Save to JSON config for ParticleSources simulation
    void updatePMArrayDataIndication();
    void writeLoadExpDataConfigToJson(QJsonObject &json);
    bool readLoadExpDataConfigFromJson(QJsonObject &json);
    void clearGeoMarkers(int All_Rec_True = 0);
    void setFontSizeAllWindows(int size);
    void writeExtraGuiToJson(QJsonObject &json);
    void readExtraGuiFromJson(QJsonObject &json);
    void SaveSimulationDataTree();
    void SaveSimulationDataAsText();
    void setFloodZposition(double Z);
    void ShowSource(const AParticleSourceRecord *p, bool clear = true);
    void TestParticleGun(AParticleGun *ParticleSources, int numParticles);

private:
    bool startupDetector();  //on ANTS start load/create detector
    void CheckSetMaterial(const QString name, QComboBox* cob, QVector<QString>* vec);
    void ToggleUpperLowerPMs();
    void PopulatePMarray(int ul, double z, int istart);
    void AddDefaultPMtype();
    void CorrectWaveTo();
    void RefreshAngularButtons();
    void RefreshAreaButtons();
    void ShowPMcount();
    void initOverridesAfterLoad();  //after detector is loaded, show first non-empty optical override
    void LoadScanPhotonDistribution(QString fileName);

    int LoadAreaResponse(QString fileName, QVector<QVector<double> >* tmp, double* xStep, double* yStep);       ///see MainWindowDiskIO.cpp
    int LoadSPePHSfile(QString fileName, QVector<double>* SPePHS_x, QVector<double>* SPePHS);                   ///see MainWindowDiskIO.cpp    
    QStringList LoadedEventFiles, LoadedTreeFiles;

//    QString CheckerScript; //obsolete?
    QString PreprocessingFileName;

    int PreviousNumberPMs = 0;
    bool fConfigGuiLocked = false;
    int timesTriedToExit = 0;

    bool populateTable; //for SimLoadConfig - compatability check

    bool fStopLoadRequested;

    bool bOptOvDialogPositioned = false;
    QSize OptOvDialogSize;
    QPoint OptOvDialogPosition;

    AG4SimulationSettings G4SimSet;

    void clearPreprocessingData();
    void updateCOBsWithPMtypeNames();

private slots:
    void timerTimeout(); //timer-based update of Root events

    void on_pbShowCheckUpWindow_clicked();
    void on_lePreprocessingMultiply_editingFinished();
    void on_extractPedestals_clicked();   
    void on_sbLoadedEnergyChannelNumber_editingFinished();
    void on_pbPositionScript_clicked();
    void on_cobPMtypes_currentIndexChanged(int index);
    void on_pbShowMaterialInPMtypes_clicked();
    void on_cobPMtypeInArrays_currentIndexChanged(int index);
    void on_pbLoadPMcenters_clicked();
    void on_pbSavePMcenters_clicked();
    void on_pbSetPMtype_clicked();    
    void on_pbSetSPEfactors_clicked();
    void on_actionSave_configuration_triggered();
    void on_actionLoad_configuration_triggered();
    void on_pbRemoveParticle_clicked();
    void on_pbSaveParticleSource_clicked();
    void on_pbLoadParticleSource_clicked();
    void on_pbSaveResults_clicked();
    void on_pbRemoveMaterial_clicked();
    void on_lwLoadedEventsFiles_itemChanged(QListWidgetItem *item);
    void on_pobTest_clicked();
    void on_actionGain_evaluation_triggered();
    void on_cbLRFs_toggled(bool checked);  
    void on_pbClearAdd_clicked();
    void on_pbClearMulti_clicked();
    void on_actionCredits_triggered();
    void on_actionVersion_triggered();
    void on_actionLicence_triggered();
    void on_pnShowHideAdvanced_toggled(bool checked);
    void on_pbYellow_clicked();
    void on_pbReconstruction_2_clicked();
    void on_cobXYtype_activated(int index);
    void on_cobTOP_activated(int index);
    void on_actionNew_detector_triggered();
    void on_pbReloadTreeData_clicked();
    void on_pbStopLoad_clicked();
    void on_cobFixedDirOrCone_currentIndexChanged(int index);
    void on_pbShowComptonAngles_clicked();
    void on_pbShowComptonEnergies_clicked();
    void on_pbCheckRandomGen_clicked();

    /************************* Simulation *************************/
public:
    void startSimulation(QJsonObject &json);
signals:
    void StopRequested();
private slots:
    void simulationFinished();
    /**************************************************************/

private slots:
    void on_pbGDML_clicked();
    void on_cobPMdeviceType_activated(const QString &arg1);
    void on_pbShowColorCoding_pressed();
    void on_pbShowColorCoding_released();
    void on_actionOpen_settings_triggered();
    void on_actionSave_Load_windows_status_on_Exit_Init_toggled(bool arg1);
    void on_pbUpdateElectronics_clicked();
    void on_pbOverlay_clicked();
    void on_pbScalePDE_clicked();
    void on_pbLoadManifestFile_clicked();
    void on_pbDeleteManifestFile_clicked();
    void on_pbManifestFileHelp_clicked();
    void on_pbLoadAppendFiles_clicked();
    void on_sbLoadASCIIpositionXchannel_valueChanged(int arg1);
    void on_pbPDEFromWavelength_clicked();
    void on_cobLoadDataType_customContextMenuRequested(const QPoint &pos);
    void on_pbManuscriptExtractNames_clicked();
    void on_actionGlobal_script_triggered();
    void on_pbLockGui_clicked();
    void on_pbUnlockGui_clicked();
    void on_actionNewLRFModule_triggered();
    void on_cobPMampGainModel_currentIndexChanged(int index);
    void on_pbAddCellMCcrosstalk_clicked();
    void on_pbShowMCcrosstalk_clicked();
    void on_pbLoadMCcrosstalk_clicked();
    void on_tabMCcrosstalk_cellChanged(int row, int column);
    void on_cbEnableMCcrosstalk_toggled(bool checked);
    void on_pbRemoveCellMCcrosstalk_clicked();
    void on_pbMCnormalize_clicked();
    void on_leLimitNodesObject_textChanged(const QString &arg1);
    void on_cbLimitNodesOutsideObject_toggled(bool checked);
    void on_bpResults_clicked();
    void on_pobTest_2_clicked();
    void on_actionScript_window_triggered();
    void on_actionQuick_save_1_triggered();
    void on_actionQuick_save_2_triggered();
    void on_actionQuick_save_3_triggered();
    void on_actionQuick_load_1_triggered();
    void on_actionQuick_load_2_triggered();
    void on_actionQuick_load_3_triggered();
    void on_actionLoad_last_config_triggered();
    void on_actionQuick_load_1_hovered();
    void on_actionQuick_save_1_hovered();
    void on_actionQuick_save_2_hovered();
    void on_actionQuick_save_3_hovered();
    void on_actionQuick_load_2_hovered();
    void on_actionQuick_load_3_hovered();
    void on_actionLoad_last_config_hovered();
    void on_cobPartPerEvent_currentIndexChanged(int index);
    void on_actionOpen_Python_window_triggered();
    void on_ledElNoiseSigma_Norm_editingFinished();
    void on_cbDarkCounts_Enable_toggled(bool checked);
    void on_pbDarkCounts_Show_clicked();
    void on_pbDarkCounts_Load_clicked();
    void on_pbDarkCounts_Delete_clicked();
    void on_pushButton_clicked();
    void on_cobDarkCounts_Model_currentIndexChanged(int index);
    void on_cobDarkCounts_LoadOptions_currentIndexChanged(int index);
    void on_actionServer_window_triggered();
    void on_actionServer_settings_triggered();
    void on_actionGrid_triggered();
    void on_pbOpenTrackProperties_Phot_clicked();
    void on_pbTrackOptionsGun_clicked();
    void on_pbQEacceleratorWarning_clicked();
    void on_ledSphericalPMAngle_editingFinished();
    void on_pbEditOverride_clicked();
    void on_lwOverrides_itemDoubleClicked(QListWidgetItem *item);
    void on_lwOverrides_itemSelectionChanged();

    void on_pbShowOverrideMap_clicked();

    void on_pbStopScan_clicked();

    void on_pbPMtypeHelp_clicked();

    void on_pbEditParticleSource_clicked();

    void on_lwDefinedParticleSources_itemDoubleClicked(QListWidgetItem *item);

    void on_lwDefinedParticleSources_itemClicked(QListWidgetItem *item);

    void on_pbGenerateFromFile_Help_clicked();

    void on_pbGenerateFromFile_Change_clicked();

    void on_leGenerateFromFile_FileName_editingFinished();

    void on_pbGenerateFromFile_Check_clicked();

    void on_pbTrackOptionsGun_customContextMenuRequested(const QPoint &pos);

    void on_pbOpenTrackProperties_Phot_customContextMenuRequested(const QPoint &pos);

    void on_pbParticleGenerationScript_clicked();

    void on_pteParticleGenerationScript_customContextMenuRequested(const QPoint &pos);

    void on_pbSetPDEfactors_clicked();

    void on_pnShowHideAdvanced_Particle_toggled(bool checked);

    void on_pbGoToConfigueG4ants_clicked();

    void on_cbGeant4ParticleTracking_toggled(bool checked);

    void on_pbG4Settings_clicked();

    void on_pbAddNewParticle_clicked();

    void on_pbAddIon_clicked();

    void on_pbRenameParticle_clicked();

    void on_pbLoadExampleFileFromFileGen_clicked();

    void on_pbNodesFromFileHelp_clicked();

    void on_pbNodesFromFileChange_clicked();

    void on_pbNodesFromFileCheckShow_clicked();

    void on_pbConvertToIon_clicked();

    void on_actionExit_triggered();

    void on_pbShowPDEfactors_clicked();

    void on_pbGainsUpdateGUI_clicked();

    void on_pbShowSPEfactors_clicked();

    void on_pbRandomizePDEfactors_clicked();

    void on_pbRandomizeSPEfactors_clicked();

    void on_pbCearOverridePDEscalar_clicked();

    void on_pbCearOverridePDEwave_clicked();

    void on_pbCearOverrideAngular_clicked();

    void on_pbCearOverrideArea_clicked();

    void on_pbHelpDetectionEfficiency_clicked();

    void on_pbOpenLogOptions_clicked();

    void on_pbOpenLogOptions2_clicked();


public slots:
    void on_pbRebuildDetector_clicked();
    void onRequestDetectorGuiUpdate();     // called to update GUI related to Detector
    void onRequestSimulationGuiUpdate();   // called to update GUI related to simulations
    void onRequestUpdateGuiForClearData(); // called to clear data indication after EventsDataHub.clear() is triggered
    void UpdatePreprocessingSettingsIndication();  // called when preprocessing settings were modified in EventsDataHub
    void onGlobalScriptStarted();
    void onGlobalScriptFinished();
    void on_pbUpdatePreprocessingSettings_clicked(); //updates preprocessing settings in Config
    void on_pbUpdateSimConfig_clicked();   // updates simulation-related properies in Config from GUI
    void selectFirstActiveParticleSource(); //trigger after load new config to select the first source with non-zero activity

private:
    void initDetectorSandwich();
    void onGuiEnableStatus(bool fLocked);
    void ShowParticleSource_noFocus();
    void showPDEorSPEfactors(bool bShowPDE);
    void randomizePDEorSPEfactors(bool bDoPDE, bool bUniform, double min, double max, double mean, double sigma);

#ifdef __USE_ANTS_PYTHON__
    void createPythonScriptWindow();
#endif

public slots:
    //new sandwich
    void UpdateSandwichGui();
    void OnWarningMessage(QString text);
    void OnDetectorColorSchemeChanged(int scheme, int matId);    
    void OnSlabDoubleClicked(QString SlabName);
    void onNewConfigLoaded();    
    void onOpticalOverrideDialogAccepted();
signals:
    void RequestStopLoad();
    void RequestUpdateSimConfig(); //used to delay execution (quesued connection have to be used). Used in Lambda expression!
};

#endif // MAINWINDOW_H
