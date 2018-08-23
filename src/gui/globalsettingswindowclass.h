#ifndef GLOBALSETTINGSWINDOWCLASS_H
#define GLOBALSETTINGSWINDOWCLASS_H

#include <QMainWindow>

class MainWindow;
class GlobalSettingsClass;
class ANetworkModule;
class AInterfaceToGStyleScript;

namespace Ui {
  class GlobalSettingsWindowClass;
}

class GlobalSettingsWindowClass : public QMainWindow
{
  Q_OBJECT

public:
  explicit GlobalSettingsWindowClass(MainWindow *parent);
  ~GlobalSettingsWindowClass();

  void updateGUI();
  void SetTab(int iTab);

  AInterfaceToGStyleScript* GStyleInterface = 0;  // if created -> owned by the script manager

public slots:
  void updateNetGui();

private slots:    
  void on_pbgStyleScript_clicked();


  void on_pbChoosePMtypeLib_clicked();
  void on_leLibPMtypes_editingFinished();

  void on_pbChooseMaterialLib_clicked();
  void on_leLibMaterials_editingFinished();

  void on_pbChooseParticleSourcesLib_clicked();
  void on_leLibParticleSources_editingFinished();

  void on_pbChooseScriptsLib_clicked();
  void on_leLibScripts_editingFinished();

  void on_pbChangeWorkingDir_clicked();
  void on_leWorkingDir_editingFinished();

  void on_sbFontSize_editingFinished();

  void on_rbAlways_toggled(bool checked);
  void on_rbNever_toggled(bool checked);

  void on_cbOpenImageExternalEditor_clicked(bool checked);

  void on_sbLogPrecision_editingFinished();

  void on_sbNumBinsHistogramsX_editingFinished();

  void on_sbNumBinsHistogramsY_editingFinished();

  void on_sbNumBinsHistogramsZ_editingFinished();

  void on_pbChoosePMtypeLib_customContextMenuRequested(const QPoint &pos);

  void on_pbChooseMaterialLib_customContextMenuRequested(const QPoint &pos);

  void on_pbChooseParticleSourcesLib_customContextMenuRequested(const QPoint &pos);

  void on_pbChooseScriptsLib_customContextMenuRequested(const QPoint &pos);

  void on_pbOpen_clicked();

  void on_sbNumSegments_editingFinished();

  void on_sbNumPointsFunctionX_editingFinished();

  void on_sbNumPointsFunctionY_editingFinished();

  void on_sbNumTreads_valueChanged(int arg1);

  void on_sbNumTreads_editingFinished();

  void on_cbSaveRecAsTree_IncludePMsignals_clicked(bool checked);

  void on_cbSaveRecAsTree_IncludeTrue_clicked(bool checked);

  void on_cbSaveRecAsTree_IncludeRho_clicked(bool checked);

  void on_leWebSocketPort_editingFinished();

  void on_cbAutoRunRootServer_clicked();

  void on_leRootServerPort_editingFinished();

  void on_leJSROOT_editingFinished();

  void on_cbRunWebSocketServer_clicked(bool checked);

  void on_cbRunRootServer_clicked(bool checked);

  void on_leWebSocketIP_editingFinished();

  void on_cbRunWebSocketServer_toggled(bool checked);

  void on_cbSaveSimAsText_IncludeNumPhotons_clicked(bool checked);

  void on_cbSaveSimAsText_IncludePositions_clicked(bool checked);

private:
  Ui::GlobalSettingsWindowClass *ui;

  MainWindow* MW;
  GlobalSettingsClass* GlobSet; //just an alias, read from MW

};

#endif // GLOBALSETTINGSWINDOWCLASS_H
