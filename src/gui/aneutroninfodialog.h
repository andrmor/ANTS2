#ifndef ANEUTRONINFODIALOG_H
#define ANEUTRONINFODIALOG_H

#include <QDialog>
#include <QVector>

namespace Ui {
class ANeutronInfoDialog;
}

class AMaterial;
class GraphWindowClass;
class TString;

class ANeutronInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ANeutronInfoDialog(const AMaterial *mat, int ipart, bool bLogLog, bool bShowAbs, bool bShowScat, GraphWindowClass *GraphWindow, QWidget *parent = 0);
    ~ANeutronInfoDialog();

private slots:
    void on_ledEnergy_textChanged(const QString &arg1);

    void on_pbClose_clicked();

    void on_pbTotal_clicked();

    void on_pbAbs_clicked();

    void on_pbScatter_clicked();

private:
    Ui::ANeutronInfoDialog *ui;
    const AMaterial* mat;
    int ipart;
    bool bLogLog;
    bool bShowAbs;
    bool bShowScat;
    GraphWindowClass *GraphWindow;

    void update();
    void updateIsotopeTable();
    void drawCrossSection(const QVector<double> &energy, const QVector<double> &cs, TString& xTitle);
};

#endif // ANEUTRONINFODIALOG_H
