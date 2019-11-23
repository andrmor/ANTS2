#ifndef ATEXTPAVEDIALOG_H
#define ATEXTPAVEDIALOG_H

#include <QDialog>

namespace Ui {
class ATextPaveDialog;
}

class TPaveText;

class ATextPaveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ATextPaveDialog(TPaveText & Pave, QWidget *parent = 0);
    ~ATextPaveDialog();

private slots:
    void on_pbDummy_clicked();
    void on_pbConfirm_clicked();
    void on_pbTextAttributes_clicked();
    void on_pbConfigureFrame_clicked();

private:
    TPaveText & Pave;
    Ui::ATextPaveDialog *ui;

private slots:
    void updatePave();

signals:
    void requestRedraw();
};

#endif // ATEXTPAVEDIALOG_H
