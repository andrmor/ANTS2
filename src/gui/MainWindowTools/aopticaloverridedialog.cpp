#include "aopticaloverridedialog.h"
#include "ui_aopticaloverridedialog.h"
#include "aopticaloverride.h"
#include "amaterialparticlecolection.h"
#include "amessage.h"
#include "aopticaloverridetester.h"
#include "graphwindowclass.h"
#include "geometrywindowclass.h"

#include <QJsonObject>
#include <QVBoxLayout>
#include <QDebug>

AOpticalOverrideDialog::AOpticalOverrideDialog(AMaterialParticleCollection * MatCollection, int matFrom, int matTo,
                                               GraphWindowClass * GraphWindow, GeometryWindowClass *GeometryWindow, QWidget * parent) :
    QDialog(parent), ui(new Ui::AOpticalOverrideDialog), GraphWindow(GraphWindow), GeometryWindow(GeometryWindow),
    MatCollection(MatCollection), matFrom(matFrom), matTo(matTo), matNames(MatCollection->getListOfMaterialNames())
{
    ui->setupUi(this);
    ui->pbInterceptorForEnter->setVisible(false);
    ui->pbInterceptorForEnter->setDefault(true);
    setWindowTitle("Photon tracing rules for material interface");

    ui->leMatFrom->setText(matNames.at(matFrom));
    ui->leMatTo->setText(matNames.at(matTo));
    ui->cobType->addItem("No special rule");
    QStringList avOv = ListOvAllOpticalOverrideTypes();
    ui->cobType->addItems(avOv);

    AOpticalOverride* ov = (*MatCollection)[matFrom]->OpticalOverrides[matTo];
    if (ov)
    {
        ovLocal = OpticalOverrideFactory( ov->getType(), MatCollection, matFrom, matTo);
        QJsonObject json; ov->writeToJson(json); ovLocal->readFromJson(json);
    }

    updateGui();

    TesterWindow = new AOpticalOverrideTester(&ovLocal, GraphWindow, GeometryWindow, MatCollection, matFrom, matTo, this);
}

AOpticalOverrideDialog::~AOpticalOverrideDialog()
{
    delete TesterWindow;
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

        QStringList avOv = ListOvAllOpticalOverrideTypes();
        int index = avOv.indexOf(ovLocal->getType()); //TODO -> if not found?
        ui->cobType->setCurrentIndex(index+1);

        QVBoxLayout* l = static_cast<QVBoxLayout*>(layout());
        customWidget = ovLocal->getEditWidget(this, GraphWindow);
        l->insertWidget(customWidgetPositionInLayout, customWidget);
    }
    else
    {
        ui->frNoOverride->setVisible(true);
        ui->pbTestOverride->setVisible(false);
    }
}

AOpticalOverride *AOpticalOverrideDialog::findInOpended(const QString &ovType)
{
    for (AOpticalOverride* ov : openedOVs)
        if (ov->getType() == ovType) return ov;
    return 0;
}

void AOpticalOverrideDialog::clearOpenedExcept(AOpticalOverride *keepOV)
{
    for (AOpticalOverride* ov : openedOVs)
        if (ov != keepOV) delete ov;
    openedOVs.clear();
}

void AOpticalOverrideDialog::on_pbAccept_clicked()
{
    if (ovLocal)
    {
        QString err = ovLocal->checkOverrideData();
        if (!err.isEmpty())
        {
            message(err, this);
            return;
        }
    }

    clearOpenedExcept(ovLocal);

    delete (*MatCollection)[matFrom]->OpticalOverrides[matTo];
    (*MatCollection)[matFrom]->OpticalOverrides[matTo] = ovLocal;
    accept();
}

void AOpticalOverrideDialog::on_pbCancel_clicked()
{
    clearOpenedExcept(ovLocal);
    delete ovLocal; ovLocal = 0;
    reject();
}

void AOpticalOverrideDialog::on_cobType_activated(int index)
{
    if (ovLocal) openedOVs << ovLocal;
    ovLocal = 0;

    if (index != 0)
    {
         QString selectedType = ui->cobType->currentText();
         ovLocal = findInOpended(selectedType);
         if (!ovLocal)
            ovLocal = OpticalOverrideFactory(ui->cobType->currentText(), MatCollection, matFrom, matTo);
    }
    updateGui();
}

void AOpticalOverrideDialog::on_pbTestOverride_clicked()
{
    TesterWindow->show();
}
