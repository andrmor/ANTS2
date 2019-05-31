#include "ageant4configdialog.h"
#include "ui_ageant4configdialog.h"
#include "ag4simulationsettings.h"
#include "amessage.h"

#include <QListWidget>
#include <QDebug>

AGeant4ConfigDialog::AGeant4ConfigDialog(AG4SimulationSettings & G4SimSet, QWidget *parent) :
    QDialog(parent), G4SimSet(G4SimSet),
    ui(new Ui::AGeant4ConfigDialog)
{
    //setWindowFlags(Qt::Dialog);
    //setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);

    ui->setupUi(this);

    ui->lePhysicsList->setText(G4SimSet.PhysicsList);
    ui->cobRefPhysLists->setCurrentIndex(-1);

    for (auto& s : G4SimSet.Commands)
        ui->pteCommands->appendPlainText(s);

    for (auto& s : G4SimSet.SensitiveVolumes)
        ui->pteSensitiveVolumes->appendPlainText(s);

    ui->pteStepLimits->clear();
    for (auto & key : G4SimSet.StepLimits.keys())
        ui->pteStepLimits->appendPlainText( QString("%1 %2").arg(key).arg(G4SimSet.StepLimits.value(key)) );

    ui->sbPositionPrecision->setValue(G4SimSet.PositionPrecision);
}

AGeant4ConfigDialog::~AGeant4ConfigDialog()
{
    delete ui;
}

void AGeant4ConfigDialog::on_pbAccept_clicked()
{
    const QRegularExpression rx = QRegularExpression("(\\ |\\,|\\n|\\t)"); //separators: ' ' or ',' or 'n' or '\t'
    const QRegularExpression rx2 = QRegularExpression("(\\ |\\t)"); //separators: ' ' or '\t'

    QString t = ui->pteSensitiveVolumes->document()->toPlainText();
    QStringList sl = t.split(rx, QString::SkipEmptyParts);
    if (sl.isEmpty())
        if (!confirm("Warning: no sensitive volumes are defined!\nNo deposition information will be collected in Geant4", this)) return;
    G4SimSet.SensitiveVolumes = sl;

    t = ui->pteCommands->document()->toPlainText();
    G4SimSet.Commands = t.split('\n', QString::SkipEmptyParts);

    G4SimSet.StepLimits.clear();
    t = ui->pteStepLimits->document()->toPlainText();
    sl = t.split('\n', QString::SkipEmptyParts);
    for (const QString & str : sl)
    {
        QStringList f = str.split(rx2, QString::SkipEmptyParts);
        if (f.size() != 2)
        {
            message("Bad format of step limits, it should be (new line for each):\nVolume_name Step_Limit");
            return;
        }
        QString vol = f[0];
        bool bOK;
        double step = f[1].toDouble(&bOK);
        if (!bOK)
        {
            message("Bad format of step limits: failed to convert to double value: " + f[1]);
            return;
        }
        G4SimSet.StepLimits[vol] = step;
    }

    G4SimSet.PositionPrecision = ui->sbPositionPrecision->value();

    accept();
}

void AGeant4ConfigDialog::on_lePhysicsList_editingFinished()
{
    G4SimSet.PhysicsList = ui->lePhysicsList->text();
}

void AGeant4ConfigDialog::on_cobRefPhysLists_activated(int index)
{
     ui->lePhysicsList->setText( ui->cobRefPhysLists->itemText(index) );
     on_lePhysicsList_editingFinished();
}

void AGeant4ConfigDialog::on_pbCancel_clicked()
{
    reject();
}
