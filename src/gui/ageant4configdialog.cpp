#include "ageant4configdialog.h"
#include "ui_ageant4configdialog.h"
#include "ag4simulationsettings.h"
#include "amessage.h"

#include <QListWidget>

AGeant4ConfigDialog::AGeant4ConfigDialog(AG4SimulationSettings & G4SimSet, QWidget *parent) :
    QDialog(parent), G4SimSet(G4SimSet),
    ui(new Ui::AGeant4ConfigDialog)
{
    ui->setupUi(this);

    ui->lePhysicsList->setText(G4SimSet.PhysicsList);

    ui->lwCommands->setDragDropMode(QAbstractItemView::InternalMove);
    ui->lwSensitiveVolumes->setDragDropMode(QAbstractItemView::InternalMove);

    for (auto& s : G4SimSet.Commands)
    {
        QListWidgetItem* item = new QListWidgetItem(s);
        item->setFlags (item->flags () | Qt::ItemIsEditable);
        ui->lwCommands->addItem(item);
    }

    for (auto& s : G4SimSet.SensitiveVolumes)
    {
        QListWidgetItem* item = new QListWidgetItem(s);
        item->setFlags (item->flags () | Qt::ItemIsEditable);
        ui->lwSensitiveVolumes->addItem(item);
    }
}

AGeant4ConfigDialog::~AGeant4ConfigDialog()
{
    delete ui;
}

void AGeant4ConfigDialog::on_pbAddCommand_clicked()
{
    int cr = ui->lwCommands->currentRow();
    int row = (cr == -1 ? ui->lwCommands->count() : cr+1);
    if (row < 0) row = 0;
    if (row > ui->lwCommands->count()) row = ui->lwCommands->count();
    QListWidgetItem* item = new QListWidgetItem("new_text");
    item->setFlags (item->flags () | Qt::ItemIsEditable);
    ui->lwCommands->insertItem(row, item);
    ui->lwCommands->editItem(ui->lwCommands->item(row));
}

void AGeant4ConfigDialog::on_pbRemoveCommand_clicked()
{
    int cr = ui->lwCommands->currentRow();
    if (cr == -1)
    {
        message("Select an item to remove", this);
        return;
    }
    delete ui->lwCommands->takeItem(cr);
}

void AGeant4ConfigDialog::on_pbAddVolume_clicked()
{
    int cr = ui->lwSensitiveVolumes->currentRow();
    int row = (cr == -1 ? ui->lwSensitiveVolumes->count() : cr+1);
    if (row < 0) row = 0;
    if (row > ui->lwSensitiveVolumes->count()) row = ui->lwSensitiveVolumes->count();
    QListWidgetItem* item = new QListWidgetItem("new_text");
    item->setFlags (item->flags () | Qt::ItemIsEditable);
    ui->lwSensitiveVolumes->insertItem(row, item);
    ui->lwSensitiveVolumes->editItem(ui->lwSensitiveVolumes->item(row));
}

void AGeant4ConfigDialog::on_pbRemoveVolume_clicked()
{
    int cr = ui->lwSensitiveVolumes->currentRow();
    if (cr == -1)
    {
        message("Select an item to remove", this);
        return;
    }
    delete ui->lwSensitiveVolumes->takeItem(cr);
}

void AGeant4ConfigDialog::on_pbAccept_clicked()
{
    QStringList com;
    for (int i=0; i<ui->lwCommands->count(); i++)
        com << ui->lwCommands->item(i)->text();
    G4SimSet.Commands = com;

    QStringList sv;
    for (int i=0; i<ui->lwSensitiveVolumes->count(); i++)
        sv << ui->lwSensitiveVolumes->item(i)->text();
    if (sv.isEmpty())
        message("Warning: no sensitive volumes are defined!\nNo deposition information will be collected in Geant4", this);

    G4SimSet.SensitiveVolumes = sv;

    accept();
}

void AGeant4ConfigDialog::on_pbSelectReferencePhList_clicked()
{
    G4SimSet.PhysicsList = ui->cobRefPhysLists->currentText();
    ui->lePhysicsList->setText(G4SimSet.PhysicsList);
}

void AGeant4ConfigDialog::on_lePhysicsList_editingFinished()
{
    G4SimSet.PhysicsList = ui->cobRefPhysLists->currentText();
}
