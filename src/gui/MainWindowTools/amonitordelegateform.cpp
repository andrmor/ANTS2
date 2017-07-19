#include "amonitordelegateform.h"
#include "ui_amonitordelegateform.h"
#include "ageoobject.h"

#include <QDebug>

AMonitorDelegateForm::AMonitorDelegateForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AMonitorDelegateForm)
{
    ui->setupUi(this);

    ui->pbContentChanged->setVisible(false);

    //installing double validators for edit boxes
    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list)
        if (w->objectName().startsWith("led"))
            w->setValidator(dv);
}

AMonitorDelegateForm::~AMonitorDelegateForm()
{
    delete ui;
}

void AMonitorDelegateForm::Update(const AGeoObject *obj)
{
    ATypeMonitorObject* mon = dynamic_cast<ATypeMonitorObject*>(obj->ObjectType);
    if (!mon)
    {
        qWarning() << "Attempt to use non-monitor object to update monitor delegate!";
        return;
    }

    ui->leName->setText(obj->Name);

    if (mon->shape == 0) ui->cobShape->setCurrentIndex(0);
    else ui->cobShape->setCurrentIndex(1);

    ui->ledSize1->setText( QString::number(mon->size1));
    ui->ledSize2->setText( QString::number(mon->size2));

    ui->ledX->setText(QString::number(obj->Position[0]));
    ui->ledY->setText(QString::number(obj->Position[1]));
    ui->ledZ->setText(QString::number(obj->Position[2]));

    ui->ledPhi->setText(QString::number(obj->Orientation[0]));
    ui->ledTheta->setText(QString::number(obj->Orientation[1]));
    ui->ledPsi->setText(QString::number(obj->Orientation[2]));
}

void AMonitorDelegateForm::UpdateVisibility()
{
    on_cobShape_currentIndexChanged(ui->cobShape->currentIndex());
    on_cobMonitoring_currentIndexChanged(ui->cobMonitoring->currentIndex());
}

void AMonitorDelegateForm::on_cobShape_currentIndexChanged(int index)
{
    bool bRectangular = (index == 0);

    ui->labSize2->setVisible(bRectangular);
    ui->labSize2mm->setVisible(bRectangular);
    ui->ledSize2->setVisible(bRectangular);
    ui->labSizeX->setText( (bRectangular ? "Size X:" : "Diameter:") );
}

void AMonitorDelegateForm::on_cobMonitoring_currentIndexChanged(int index)
{
    ui->frParticle->setVisible(index == 1);

    ui->labWave->setVisible(index == 0);
    ui->labWaveBins->setVisible(index == 0);
    ui->labWaveFrom->setVisible(index == 0);
    ui->labWaveTo->setVisible(index == 0);
    ui->ledWaveFrom->setVisible(index == 0);
    ui->ledWaveTo->setVisible(index == 0);
    ui->sbWaveBins->setVisible(index == 0);

    ui->labEnergy->setVisible(index == 1);
    ui->labEnergyBins->setVisible(index == 1);
    ui->labEnergyFrom->setVisible(index == 1);
    ui->labEnergyTo->setVisible(index == 1);
    ui->ledEnergyFrom->setVisible(index == 1);
    ui->ledEnergyTo->setVisible(index == 1);
    ui->sbEnergyBins->setVisible(index == 1);
}

void AMonitorDelegateForm::on_pbContentChanged_clicked()
{
    emit contentChanged();
}
