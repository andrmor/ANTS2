#ifndef ANEUTRONINFODIALOG_H
#define ANEUTRONINFODIALOG_H

#include <QDialog>
#include <QVector>
#include <QTableWidgetItem>

namespace Ui {
class ANeutronInfoDialog;
}

class AMaterial;
class GraphWindowClass;
class TString;
class TH1D;

class ANeutronInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ANeutronInfoDialog(const AMaterial *mat, int ipart, bool bLogLog, bool bShowAbs, bool bShowScat, GraphWindowClass *GraphWindow, QWidget *parent = 0);
    ~ANeutronInfoDialog();

private slots:
    void on_pbClose_clicked();

    void on_pbTotal_clicked();

    void on_pbAbs_clicked();

    void on_pbScatter_clicked();

    void onIsotopeTable_ColumnSelected(int column);

    void on_pbNCrystal_Angle_clicked();

    void on_pbNCrystal_Energy_clicked();

    void on_ledEnergy_editingFinished();

    void on_ledWave_editingFinished();

    void on_cbNCrystalSkipBragg_toggled(bool checked);

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
    void drawCrossSection(const QVector<double> &energy, const QVector<double> &cs, const TString &xTitle);
    void RunNCrystal(bool bAngle);

protected:
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

signals:
    void requestDraw(TH1D* histCopy);
};

class ATableWidgetDoubleItem : public QTableWidgetItem
{
public:
    ATableWidgetDoubleItem(QString text);

    public:
        bool operator< (const QTableWidgetItem &other) const;
};

#endif // ANEUTRONINFODIALOG_H
