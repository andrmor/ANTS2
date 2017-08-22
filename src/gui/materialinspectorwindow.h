#ifndef MATERIALINSPECTORWINDOW_H
#define MATERIALINSPECTORWINDOW_H

#include <QMainWindow>

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

namespace Ui {
class MaterialInspectorWindow;
}

class MaterialInspectorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MaterialInspectorWindow(QWidget* parent, MainWindow *mw, DetectorClass *detector);
    ~MaterialInspectorWindow();

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

    //on user input    
    void on_pbUpdateInteractionIndication_clicked();  // interaction indication update
    void on_pbShowTotalInteraction_clicked();
    void on_leName_editingFinished();
    void on_pbAddToActive_clicked();
    void on_cobActiveMaterials_activated(int index);
    void on_pbUpdateTmpMaterial_clicked();
    void on_pbLoadDeDr_clicked();
    void on_pbLoadThisScenarioCrossSection_clicked();
    void on_pbAddNewTerminationScenario_clicked();
    void on_pbAddNewSecondary_clicked();
    void on_ledBranching_editingFinished();
    void on_pbShowStoppingPower_clicked();
    void on_pbImportStoppingPowerFromTrim_clicked();
    void on_pbImportXCOM_clicked();
    void on_pbLoadTotalInteractionCoefficient_clicked();
    void on_pbNeutronClear_clicked();
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
    void on_ledMFPenergy_editingFinished();
    void on_pbNistPage_clicked();
    void on_pbRemoveSecondary_clicked();
    void on_ledInitialEnergy_editingFinished();
    void on_cobSecondaryParticleToAdd_activated(int index);
    void on_pbRename_clicked();
    void on_ledMFPenergy_2_editingFinished();
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
    void on_pbShowTotalCapture_clicked();
    void on_cobTerminationScenarios_activated(int index);
    void on_ledMFPenergyEllastic_editingFinished();
    void on_pbShowTotalEllastic_clicked();
    void on_pbConfigureAutoElastic_clicked();
    void on_pbShowStatisticsOnElastic_clicked();

    //user or code controlled change - safe or only GUI
    void on_ledRayleigh_textChanged(const QString &arg1);
    void on_lwGeneratedParticlesEnergies_currentRowChanged(int currentRow);
    void on_cbTrackingAllowed_toggled(bool checked);
    void on_ledPrimaryYield_textChanged(const QString &arg1);
    void on_ledAtomicDensity_textChanged(const QString &arg1);

    //menu actions
    void on_actionSave_material_triggered();
    void on_actionLoad_material_triggered();
    void on_actionClear_Interaction_for_this_particle_triggered();
    void on_actionClear_interaction_for_all_particles_triggered();
    void on_actionUse_log_log_interpolation_triggered();

    //new auto-generated, not cathegorized

    void on_pbModifyChemicalComposition_clicked();

    void on_cbShowIsotopes_clicked();

    void on_tabwNeutron_customContextMenuRequested(const QPoint &pos);

    void on_pushButton_clicked();

    void on_cbCapture_clicked();

    void on_cbEnableScatter_clicked();

    void onTabwNeutronsActionRequest(int iEl, int iIso, const QString Action);

private:
    Ui::MaterialInspectorWindow *ui;
    MainWindow* MW;
    DetectorClass* Detector;

    AMatParticleConfigurator* MatParticleOptionsConfigurator;

    QIcon RedIcon;

    bool flagDisreguardChange;
    bool fLockTable;
    int LastSelectedParticle;

    void UpdateWaveButtons();
    void UpdateActionButtons();

    void showProcessIntCoefficient(int particleId, int TermScenario);
    TGraph* constructInterpolationGraph(QVector<double> X, QVector<double> Y);
    bool importXCOM(QTextStream &in, int particleId);
    bool isAllSameYield(double val);
    void updateNeutronReactionIndication();

    bool autoLoadCrossSection(ANeutronInteractionElement *element, QString target); //target = "absorption" or "elastic scattering"
    bool doLoadCrossSection(ANeutronInteractionElement *element, QString fileName);
    void ShowTreeWithChemicalComposition();
    void FillNeutronTable();

};

#endif // MATERIALINSPECTORWINDOW_H
