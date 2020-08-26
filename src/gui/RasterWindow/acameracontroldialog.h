#ifndef ACAMERACONTROLDIALOG_H
#define ACAMERACONTROLDIALOG_H

#include <QDialog>

class TCanvas;
class QLineEdit;

namespace Ui {
class ACameraControlDialog;
}

class ACameraControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ACameraControlDialog(TCanvas * Canvas, QWidget * parent = nullptr);
    ~ACameraControlDialog();

    void showAndUpdate();

private slots:
    void on_pbClose_clicked();
    void on_pbUpdateRange_clicked();
    void on_ledCenterX_editingFinished();
    void on_ledCenterY_editingFinished();
    void on_ledCenterZ_editingFinished();

    void on_pbUpdateWindow_clicked();

protected:
    void closeEvent(QCloseEvent *);

private:
    TCanvas * Canvas = nullptr;
    Ui::ACameraControlDialog * ui;

    double xPos = 0;
    double yPos = 0;

    void updateGui();
    void setCenter(int index, QLineEdit * led);

};

#endif // ACAMERACONTROLDIALOG_H
