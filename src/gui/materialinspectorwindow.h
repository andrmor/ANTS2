#ifndef MATERIALINSPECTORWINDOW_H
#define MATERIALINSPECTORWINDOW_H

#include "aguiwindow.h"

class QTextStream;
class MainWindow;
class DetectorClass;
class TGraph;
class AMatParticleConfigurator;
class QJsonObject;
class AElasticScatterElement;
class QTreeWidgetItem;
class AChemicalElement;
class ANeutronInteractionElement;
class ANeutronInfoDialog;

namespace Ui {
class MaterialInspectorWindow;
}

class MaterialInspectorWindow : public AGuiWindow
{
    Q_OBJECT

public:
    explicit MaterialInspectorWindow(QWidget* parent, MainWindow *mw, DetectorClass *detector);
    ~MaterialInspectorWindow();

    void InitWindow();
    void UpdateActiveMaterials();
    void UpdateIndicationTmpMaterial();
    void SetParticleSelection(int index);
    void SetMaterial(int index);
    void ShowTotalInteraction();

    void AddMatToCobs(QString str);
    void setLogLog(bool flag);

    void ConvertToStandardWavelengthes(QVector<double>* sp_x, QVector<double>* sp_y, QVector<double>* y);

    //void WriteElasticAutoToJson(QJsonObject& json);

    bool bClearInProgress;

protected:
    bool event(QEvent * e);

private slots:
    // both user and code control - potential problems
    void on_leName_textChanged(const QString &arg1);

    //on signals from delegates
    void onAddIsotope(AChemicalElement *element);
    void onRemoveIsotope(AChemicalElement* element, int isotopeIndexInElement);
    void IsotopePropertiesChanged(const AChemicalElement* element, int isotopeIndexInElement);
    void onRequestDraw(const QVector<double> & x, const QVector<double> & y, const QString & titleX, const QString & titleY);

    //on user input    
    void on_pbUpdateInteractionIndication_clicked();  // interaction indication update
    void on_pbShowTotalInteraction_clicked();
    void on_leName_editingFinished();
    void on_pbAddToActive_clicked();
    void on_cobActiveMaterials_activated(int index);
    void on_pbUpdateTmpMaterial_clicked();
    void on_pbLoadDeDr_clicked();
    void on_pbLoadThisScenarioCrossSection_clicked();
    void on_pbShowStoppingPower_clicked();
    void on_pbImportStoppingPowerFromTrim_clicked();
    void on_pbImportXCOM_clicked();
    void on_pbLoadPrimSpectrum_clicked();
    void on_pbShowPrimSpectrum_clicked();
    void on_pbDeletePrimSpectrum_clicked();
    void on_pbLoadSecSpectrum_clicked();
    void on_pbShowSecSpectrum_clicked();
    void on_pbDeleteSecSpectrum_clicked();
    void on_pbLoadNlambda_clicked();
    void on_pbShowNlambda_clicked();
    void on_pbDeleteNlambda_clicked();
    void on_pbLoadABSlambda_clicked();
    void on_pbShowABSlambda_clicked();
    void on_pbDeleteABSlambda_clicked();
    void on_pbWasModified_clicked();
    void on_pbGammaDiagnosticsPhComptRation_clicked();
    void on_ledGammaDiagnosticsEnergy_editingFinished();
    void on_pbComments_clicked();
    void on_ledRayleighWave_editingFinished();
    void on_ledRayleigh_editingFinished();
    void on_pbRemoveRayleigh_clicked();
    void on_pbShowUsage_clicked();
    void on_pbNistPage_clicked();
    void on_pbRename_clicked();
    void on_pbAddNewMaterial_clicked();
    void on_ledIntEnergyRes_editingFinished();
    void on_cbTransparentMaterial_clicked();
    void on_pbShowPhotoelectric_clicked();
    void on_pbShowCompton_clicked();
    void on_pbShowAllForGamma_clicked();
    void on_pbXCOMauto_clicked();
    void on_pbShowXCOMdata_clicked();
    void on_cobYieldForParticle_activated(int index);
    void on_pbShowPairProduction_clicked();
    void on_pbShowStatisticsOnElastic_clicked();
    void on_lePriT_raise_editingFinished();
    void on_pbNew_clicked();
    void on_pbComputeRangeCharged_clicked();
    void on_pbCopyPrYieldToAll_clicked();
    void on_cbTrackingAllowed_clicked();
    void on_pbModifyChemicalComposition_clicked();
    void on_cbShowIsotopes_clicked();
    void on_cbCapture_clicked();
    void on_cbEnableScatter_clicked();
    void on_pbMaterialInfo_clicked();
    void on_cbAllowAbsentCsData_clicked();
    void on_pbHelpNeutron_clicked();
    void on_pbAutoLoadMissingNeutronCrossSections_clicked();
    void on_trwChemicalComposition_doubleClicked(const QModelIndex &index);
    void on_pbShowReemProbLambda_clicked();
    void on_pbLoadReemisProbLambda_clicked();
    void on_pbDeleteReemisProbLambda_clicked();
    void on_lePriT_editingFinished();
    void on_pbPriThelp_clicked();
    void on_pbPriT_test_clicked();
    void on_pbShowNcmat_clicked();
    void on_pbLoadNcmat_clicked();
    void on_ledNCmatDcutoff_editingFinished();
    void on_ledNcmatPacking_editingFinished();
    void on_cbUseNCrystal_clicked(bool checked);

    //user or code controlled change - safe or only GUI
    void on_ledRayleigh_textChanged(const QString &arg1);
    void on_ledPrimaryYield_textChanged(const QString &arg1);
    void on_cbUseNCrystal_toggled(bool checked);

    //menu actions
    void on_actionSave_material_triggered();
    void on_actionLoad_material_triggered();
    void on_actionClear_Interaction_for_this_particle_triggered();
    void on_actionClear_interaction_for_all_particles_triggered();
    void on_actionUse_log_log_interpolation_triggered();
    void on_tabwNeutron_customContextMenuRequested(const QPoint &pos);
    void onTabwNeutronsActionRequest(int iEl, int iIso, const QString Action);
    void on_actionNeutrons_triggered();

    //new auto-generated, not cathegorized

    void on_pbSecScintHelp_clicked();

private:
    Ui::MaterialInspectorWindow *ui;
    MainWindow* MW;
    DetectorClass* Detector;

    AMatParticleConfigurator* OptionsConfigurator;

    ANeutronInfoDialog* NeutronInfoDialog;

    bool flagDisreguardChange;
    bool fLockTable;
    int LastSelectedParticle;
    bool bLockTmpMaterial = false;

    bool bMessageLock = false;

    void updateWaveButtons();
    void updateActionButtons();

    void showProcessIntCoefficient(int particleId, int TermScenario);
    TGraph* constructInterpolationGraph(QVector<double> X, QVector<double> Y);
    bool importXCOM(QTextStream &in, int particleId);

    bool autoLoadCrossSection(ANeutronInteractionElement *element, QString target); //target = "absorption" or "elastic scattering" - replace with dynamic_cast!!!
    bool doLoadCrossSection(ANeutronInteractionElement *element, QString fileName);
    void ShowTreeWithChemicalComposition();
    void FillNeutronTable();
    int autoloadMissingCrossSectionData(); //returns number of particles added to the collection

    void SetWasModified(bool flag);
    bool parseDecayOrRaiseTime(bool doParseDecay);
    void updateWarningIcons();
    int autoLoadReaction(ANeutronInteractionElement &element); //returns number of particles added to the collection
    void updateTmpMatOnPartCollChange(int newPartAdded);
    void updateEnableStatus();  // gui update for tracking allow / transparent
};

#endif // MATERIALINSPECTORWINDOW_H
