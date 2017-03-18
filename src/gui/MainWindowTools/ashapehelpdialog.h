#ifndef ASHAPEHELPDIALOG_H
#define ASHAPEHELPDIALOG_H

#include <QDialog>

namespace Ui {
  class AShapeHelpDialog;
}

class AGeoShape;

class AShapeHelpDialog : public QDialog
{
  Q_OBJECT

public:
  explicit AShapeHelpDialog(AGeoShape* thisShape, QList<AGeoShape*> shapes, QWidget *parent = 0);
  ~AShapeHelpDialog();

  QList<AGeoShape*> AvailableShapes;
  QString SelectedTemplate;

private slots:
  void on_pbSelect_clicked();

  void on_pbClose_clicked();

  void on_lwTypes_currentRowChanged(int currentRow);

private:
  Ui::AShapeHelpDialog *ui;
};

#endif // ASHAPEHELPDIALOG_H
