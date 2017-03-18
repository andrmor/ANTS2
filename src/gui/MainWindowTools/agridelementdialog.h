#ifndef AGRIDELEMENTDIALOG_H
#define AGRIDELEMENTDIALOG_H

#include <QDialog>

namespace Ui {
class AGridElementDialog;
}

class AGridElementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AGridElementDialog(QWidget *parent = 0);
    ~AGridElementDialog();

    void setValues(int shape, double p0, double p1, double p2);

    int shape();
    double pitch();
    double length();
    double pitchX();
    double pitchY();
    double diameter();
    double inner();
    double outer();
    double height();

private slots:
    void on_cobGridType_currentIndexChanged(int index);
    void on_pbAccept_clicked();
    void on_pbCancel_clicked();

private:
    Ui::AGridElementDialog *ui;
};

#endif // AGRIDELEMENTDIALOG_H
