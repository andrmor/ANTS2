#ifndef DETECTORADDONSWINDOW_H
#define DETECTORADDONSWINDOW_H

#include "aguiwindow.h"

class MainWindow;
class DetectorClass;
class AGeo_SI;
class AGeoTreeWidget;
class AGeoObject;

namespace Ui {
  class DetectorAddOnsWindow;
}

class DetectorAddOnsWindow : public AGuiWindow
{
  Q_OBJECT
  
public:
  explicit DetectorAddOnsWindow(QWidget * parent, MainWindow * MW, DetectorClass* detector);
  ~DetectorAddOnsWindow();

  void UpdateGUI(); //update gui controls
  void SetTab(int tab);
  void UpdateDummyPMindication();
  void HighlightVolume(const QString & VolName);

  AGeo_SI* AddObjScriptInterface = 0;  // if created -> owned by the script manager
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
  bool GDMLtoTGeo(const QString &fileName);
  const QString loadGDML(const QString &fileName, QString &gdml);  //returns error string - empty if OK

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
