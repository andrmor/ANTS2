#include "aopticaloverridedialog.h"
#include "ui_aopticaloverridedialog.h"
#include "aopticaloverride.h"
#include "amaterialparticlecolection.h"
#include "amessage.h";

#include <QJsonObject>
#include <QVBoxLayout>

AOpticalOverrideDialog::AOpticalOverrideDialog(AMaterialParticleCollection * MatCollection,
                                               int matFrom, int matTo, QWidget *parent) :
    QDialog(parent), ui(new Ui::AOpticalOverrideDialog),
    MatCollection(MatCollection), matFrom(matFrom), matTo(matTo), matNames(MatCollection->getListOfMaterialNames())
{
    ui->setupUi(this);
    ui->pbInterceptorForEnter->setVisible(false);
    ui->pbInterceptorForEnter->setDefault(true);
    setWindowTitle("Photon tracing rules for material interface");

    ui->leMatFrom->setText(matNames.at(matFrom));
    ui->leMatTo->setText(matNames.at(matTo));
    ui->cobType->addItem("No special rule");
    QStringList avOv = GetListOvAvailableOverrides();
    ui->cobType->addItems(avOv);

    AOpticalOverride* ov = (*MatCollection)[matFrom]->OpticalOverrides[matTo];
    if (ov)
    {
        ovLocal = OpticalOverrideFactory( ov->getType(), MatCollection, matFrom, matTo);
        QJsonObject json; ov->writeToJson(json); ovLocal->readFromJson(json);
    }

    updateGui();
}

AOpticalOverrideDialog::~AOpticalOverrideDialog()
{
    delete ui;
}

void AOpticalOverrideDialog::updateGui()
{
    if (customWidget)
    {
        QVBoxLayout* l = static_cast<QVBoxLayout*>(layout());
        l->removeWidget(customWidget);
        delete customWidget; customWidget = 0;
    }

    if (ovLocal)
    {
        ui->frNoOverride->setVisible(false);
        ui->pbTestOverride->setVisible(true);

        QStringList avOv = GetListOvAvailableOverrides();
        int index = avOv.indexOf(ovLocal->getType()); //TODO -> if not found?
        ui->cobType->setCurrentIndex(index+1);

        QVBoxLayout* l = static_cast<QVBoxLayout*>(layout());
        customWidget = ovLocal->getEditWidget();
        l->insertWidget(customWidgetPositionInLayout, customWidget);
    }
    else
    {
        ui->frNoOverride->setVisible(true);
        ui->pbTestOverride->setVisible(false);
    }
}

void AOpticalOverrideDialog::on_pbAccept_clicked()
{
    if (ovLocal)
    {
        QString err = ovLocal->checkValidity();
        if (!err.isEmpty())
        {
            message(err, this);
            return;
        }
    }

    delete (*MatCollection)[matFrom]->OpticalOverrides[matTo];
    (*MatCollection)[matFrom]->OpticalOverrides[matTo] = ovLocal;
    accept();
}

void AOpticalOverrideDialog::on_pbCancel_clicked()
{
    delete ovLocal; ovLocal = 0;
    reject();
}

void AOpticalOverrideDialog::on_cobType_activated(int index)
{
    delete ovLocal; ovLocal = 0;

    if (index != 0)
         ovLocal = OpticalOverrideFactory(ui->cobType->currentText(), MatCollection, matFrom, matTo);

    updateGui();
}

void AOpticalOverrideDialog::on_pbTestOverride_clicked()
{

}
