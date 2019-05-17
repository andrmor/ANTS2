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
}

AGeant4ConfigDialog::~AGeant4ConfigDialog()
{
    delete ui;
}

void AGeant4ConfigDialog::on_pbAccept_clicked()
{
    const QRegularExpression rx = QRegularExpression("(\\ |\\,|\\n|\\t)"); //separators: ' ' or ',' or 'n' or '\t'
    QString t = ui->pteSensitiveVolumes->document()->toPlainText();
    QStringList sl = t.split(rx, QString::SkipEmptyParts);
    if (sl.isEmpty())
        if (!confirm("Warning: no sensitive volumes are defined!\nNo deposition information will be collected in Geant4", this)) return;

    t = ui->pteCommands->document()->toPlainText();
    G4SimSet.Commands = t.split('\n', QString::SkipEmptyParts);

    G4SimSet.SensitiveVolumes = sl;
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
