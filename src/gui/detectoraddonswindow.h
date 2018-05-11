#ifndef DETECTORADDONSWINDOW_H
#define DETECTORADDONSWINDOW_H

#include <QMainWindow>

class MainWindow;
class DetectorClass;
class AInterfaceToAddObjScript;
class AGeoTreeWidget;
class AGeoObject;

namespace Ui {
  class DetectorAddOnsWindow;
}

class DetectorAddOnsWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit DetectorAddOnsWindow(MainWindow *parent, DetectorClass* detector);
  ~DetectorAddOnsWindow();

  void UpdateGUI(); //update gui controls
  void SetTab(int tab);
  void UpdateDummyPMindication();
  void HighlightVolume(QString VolName);

  AInterfaceToAddObjScript* AddObjScriptInterface = 0;  // if created -> owned by the script manager
  AGeoTreeWidget* twGeo;  // WorldTree widget

private slots:
  void onReconstructDetectorRequest();

  void on_pbConvertToDummies_clicked();

  void on_sbDummyPMindex_valueChanged(int arg1);

  void on_pbDeleteDummy_clicked();

  void on_pbConvertDummy_clicked();

  void on_pbUpdateDummy_clicked();

  void on_pbCreateNewDummy_clicked();

  void on_sbDummyType_valueChanged(int arg1);

  void on_pbLoadDummyPMs_clicked();

  void on_pbConvertAllToPMs_clicked();

  void on_pbUseScriptToAddObj_clicked();

  void on_pbSaveTGeo_clicked();

  void on_pbLoadTGeo_clicked();

  void on_pbBackToSandwich_clicked();

  void on_pbRootWeb_clicked();

  void on_pbCheckGeometry_clicked();

  void on_cbAutoCheck_clicked(bool checked);

  void on_pbRunTestParticle_clicked();

  void on_cbAutoCheck_stateChanged(int arg1);

  void on_pmParseInGeometryFromGDML_clicked();

  void on_pbConvertToScript_clicked();

private:
  Ui::DetectorAddOnsWindow *ui;
  MainWindow* MW;
  DetectorClass* Detector;
  QString ObjectScriptTarget;

  void ConvertDummyToPM(int idpm);  
  QString makeScriptString_basicObject(AGeoObject *obj);
  QString makeScriptString_arrayObject(AGeoObject *obj);
  QString makeScriptString_stackObjectStart(AGeoObject *obj);
  QString makeScriptString_groupObjectStart(AGeoObject *obj);
  QString makeScriptString_stackObjectEnd(AGeoObject *obj);
  QString makeLinePropertiesString(AGeoObject *obj);
  void objectMembersToScript(AGeoObject *Master, QString &script, int ident);

public slots:
  void UpdateGeoTree(QString name = "");
  void ShowTab(int tab);
  void AddObjScriptSuccess();
  void ReportScriptError(QString ErrorMessage);
  void ShowObject(QString name = "");
  void ShowObjectRecursive(QString name);
  void OnrequestShowMonitor(const AGeoObject* mon);
};

#endif // DETECTORADDONSWINDOW_H
