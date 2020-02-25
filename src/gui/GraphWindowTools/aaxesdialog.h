#ifndef AAXESDIALOG_H
#define AAXESDIALOG_H

#include <QDialog>
#include <QVector>

namespace Ui {
class AAxesDialog;
}

class TAxis;
class QLayout;

class AAxesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AAxesDialog(QVector<TAxis*> & axes, int axisIndex, QWidget *parent = nullptr);
    ~AAxesDialog();

    int exec() override;

    void addLayout(QLayout * lay);

private slots:
    void on_pbDummy_clicked();
    void on_pbTitleProperties_clicked();
    void on_pbAccept_clicked();
    void on_leTitle_editingFinished();
    void on_ledOffset_editingFinished();
    void on_pbLabelProperties_clicked();
    void on_cobTickInsideOutside_activated(int index);
    void on_ledTickLength_editingFinished();
    void on_pbConfigureDivisions_clicked();
    void on_ledLabelOffset_editingFinished();

private:
    void updateGui();

private:
    Ui::AAxesDialog * ui;
    QVector<TAxis*> & Axes;
    int AxisIndex = 0;
    TAxis * Axis = nullptr;

signals:
    void requestRedraw();
};

#endif // AAXESDIALOG_H
