#ifndef EXAMPLESWINDOW_H
#define EXAMPLESWINDOW_H

#include <QMainWindow>

class MainWindow;

namespace Ui {
  class ExamplesWindow;
}

struct DetectorExampleStructure
{
  int MainType;
  int SubType;
  QString Description;
  QString filename;

  DetectorExampleStructure(const int mainT, const int subT, const QString file, const QString desc){Description = desc; MainType = mainT; filename = file; SubType = subT;}
  DetectorExampleStructure(){;}
};

class ExamplesWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit ExamplesWindow(QWidget* parent, MainWindow *mw);
  ~ExamplesWindow();

  void SaveConfig(QString fileName, bool DetConstructor = true, bool SimSettings = true, bool ReconstrSettings = true);

  void QuickSave(); //save configuration to LAST/QuickSave.conf
  void QuickLoad();

private slots:
  void on_cbDoNotShowExamplesOnStart_toggled(bool checked);

  void on_lwMainType_currentRowChanged(int currentRow);

  void on_lwSubType_currentRowChanged(int currentRow);

  void on_lwExample_currentRowChanged(int currentRow);

  void on_pbLoadExample_clicked();

  void on_lwExample_doubleClicked(const QModelIndex &index);

  void on_pbSaveSessings_clicked();

  void on_pbQuickSave_clicked();

  void on_pbLoadSettings_clicked();

  void on_pbQuickLoad_clicked();

  void on_pbNew_clicked();

protected:
    bool event(QEvent *event);

private:
  Ui::ExamplesWindow *ui;
  MainWindow* MW;

  int MainPointer, SubPointer, ExamplePointer;
  QVector<QString> MainTypeNames;
  QVector< QVector <QString> > SubTypeNames;

  QVector<DetectorExampleStructure> Examples;
  QVector<int> ListedExamples; //to find the proper one from the list index

  void BuildExampleRecord();
  void AddNewMainType(QString name);
  void AddNewSubType(QString maintype, QString subtype);
  void AddNewExample(QString maintype, QString subtype, QString filename, QString description);

};

#endif // EXAMPLESWINDOW_H
