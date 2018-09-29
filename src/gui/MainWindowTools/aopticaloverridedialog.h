#ifndef AOPTICALOVERRIDEDIALOG_H
#define AOPTICALOVERRIDEDIALOG_H

#include <QDialog>
#include <QStringList>

namespace Ui {
class AOpticalOverrideDialog;
}

class AOpticalOverride;
class AMaterialParticleCollection;

class AOpticalOverrideDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AOpticalOverrideDialog(AMaterialParticleCollection * MatCollection, int matFrom, int matTo, QWidget *parent = 0);
    ~AOpticalOverrideDialog();

private slots:
    void on_pbAccept_clicked();
    void on_pbCancel_clicked();
    void on_cobType_activated(int index);
    void on_pbTestOverride_clicked();

private:
    Ui::AOpticalOverrideDialog *ui;
    AOpticalOverride ** pOV;
    AOpticalOverride * ovLocal = 0;
    AMaterialParticleCollection* MatCollection;
    int matFrom;
    int matTo;
    QStringList matNames; //need?

    int customWidgetPositionInLayout = 5;
    QWidget* customWidget = 0;

    void updateGui();
};

#endif // AOPTICALOVERRIDEDIALOG_H
