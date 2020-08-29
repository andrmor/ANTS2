#ifndef ACAMERACONTROLDIALOG_H
#define ACAMERACONTROLDIALOG_H

#include <QDialog>

class RasterWindowBaseClass;
class QLineEdit;

namespace Ui {
class ACameraControlDialog;
}

class ACameraControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ACameraControlDialog(RasterWindowBaseClass * RasterWin, QWidget * parent = nullptr);
    ~ACameraControlDialog();

    void showAndUpdate();

    void setView();

public slots:
    void updateGui();

private slots:
    void on_pbClose_clicked();

    void on_ledCenterX_editingFinished();
    void on_ledCenterY_editingFinished();
    void on_ledCenterZ_editingFinished();

    void on_pbSetView_clicked();

    void on_pbStepXM_clicked();

    void on_pbStepXP_clicked();

    void on_pbStepYM_clicked();

    void on_pbStepYP_clicked();

    void on_pbStepZM_clicked();

    void on_pbStepZP_clicked();

protected:
    void closeEvent(QCloseEvent *);

private:
    RasterWindowBaseClass * RW = nullptr;
    Ui::ACameraControlDialog * ui;

    double xPos = 0;
    double yPos = 0;

    void setCenter(int index, QLineEdit * led);
    void makeStep(int index, double step);

};

#endif // ACAMERACONTROLDIALOG_H
