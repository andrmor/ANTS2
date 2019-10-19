#include "amonitordelegateform.h"
#include "ui_amonitordelegateform.h"
#include "ageoobject.h"

#include <QDebug>

AMonitorDelegateForm::AMonitorDelegateForm(QStringList particles, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AMonitorDelegateForm)
{
    ui->setupUi(this);

    particles.insert(0, "All particles");
    ui->cobParticle->addItems(particles);
    ui->pbContentChanged->setVisible(false);

    ui->cobEnergyUnits->setCurrentIndex(2);

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

bool AMonitorDelegateForm::updateGUI(const AGeoObject *obj)
{
    ATypeMonitorObject* mon = dynamic_cast<ATypeMonitorObject*>(obj->ObjectType);
    if (!mon)
    {
        qWarning() << "Attempt to use non-monitor object to update monitor delegate!";
        return false;
    }
    const AMonitorConfig& config = mon->config;

    ui->leName->setText(obj->Name);
    if (config.shape == 0) ui->cobShape->setCurrentIndex(0);
    else ui->cobShape->setCurrentIndex(1);
    ui->ledSize1->setText( QString::number(2.0*config.size1));
    ui->ledSize2->setText( QString::number(2.0*config.size2));

    ui->ledX->setText(QString::number(obj->Position[0]));
    ui->ledY->setText(QString::number(obj->Position[1]));
    ui->ledZ->setText(QString::number(obj->Position[2]));
    ui->ledPhi->setText(QString::number(obj->Orientation[0]));
    ui->ledTheta->setText(QString::number(obj->Orientation[1]));
    ui->ledPsi->setText(QString::number(obj->Orientation[2]));

    int sens = 0;
    if (config.bLower && !config.bUpper) sens = 1;
    else if (config.bLower && config.bUpper) sens = 2;
    ui->cobSensitiveDirection->setCurrentIndex(sens);

    ui->cobMonitoring->setCurrentIndex(config.PhotonOrParticle);

    ui->cbStopTracking->setChecked(config.bStopTracking);

    const int effIndex = config.ParticleIndex + 1; // shifting -1 (is for "all particles") to 0
    if (effIndex > -1 && effIndex < ui->cobParticle->count())
        ui->cobParticle->setCurrentIndex(effIndex);

    int prsec = 0;
    if (config.bSecondary && !config.bPrimary) prsec = 1;
    else if (config.bSecondary && config.bPrimary) prsec = 2;
    ui->cobPrimarySecondary->setCurrentIndex(prsec);

    int dirin = 0;
    if (config.bIndirect && !config.bDirect) dirin = 1;
    else if (config.bIndirect &&  config.bDirect) dirin = 2;
    ui->cobDirectIndirect->setCurrentIndex(dirin);

    ui->sbXbins->setValue(config.xbins);
    ui->sbYbins->setValue(config.ybins);
    ui->sbTimeBins->setValue(config.timeBins);
    ui->sbAngleBins->setValue(config.angleBins);
    ui->sbWaveBins->setValue(config.waveBins);
    ui->sbEnergyBins->setValue(config.energyBins);

    ui->ledTimeFrom->setText( QString::number(config.timeFrom) );
    ui->ledAngleFrom->setText( QString::number(config.angleFrom) );
    ui->ledWaveFrom->setText( QString::number(config.waveFrom) );
    ui->ledEnergyFrom->setText( QString::number(config.energyFrom) );

    ui->ledTimeTo->setText( QString::number(config.timeTo) );
    ui->ledAngleTo->setText( QString::number(config.angleTo) );
    ui->ledWaveTo->setText( QString::number(config.waveTo) );
    ui->ledEnergyTo->setText( QString::number(config.energyTo) );
    ui->cobEnergyUnits->setCurrentIndex( config.energyUnitsInHist );

    return true;
}

const QString AMonitorDelegateForm::getName() const
{
    return ui->leName->text();
}

void AMonitorDelegateForm::updateObject(AGeoObject *obj) const
{
    obj->Name = ui->leName->text();

    ATypeMonitorObject* mon = dynamic_cast<ATypeMonitorObject*>(obj->ObjectType);
    AMonitorConfig& config = mon->config;
    config.shape = ui->cobShape->currentIndex();
    config.size1 = 0.5*ui->ledSize1->text().toDouble();
    config.size2 = 0.5*ui->ledSize2->text().toDouble();
    obj->updateMonitorShape();

    obj->Position[0] = ui->ledX->text().toDouble();
    obj->Position[1] = ui->ledY->text().toDouble();
    obj->Position[2] = ui->ledZ->text().toDouble();
    obj->Orientation[0] = ui->ledPhi->text().toDouble();
    obj->Orientation[1] = ui->ledTheta->text().toDouble();
    obj->Orientation[2] = ui->ledPsi->text().toDouble();

    int sens = ui->cobSensitiveDirection->currentIndex();
    switch (sens)
    {
      case 0: config.bUpper = true; config.bLower = false; break;
      case 1: config.bUpper = false; config.bLower = true; break;
      case 2: config.bUpper = true; config.bLower = true; break;
      default: qWarning() << "Bad sensitive directions!";
    }

    config.PhotonOrParticle = ui->cobMonitoring->currentIndex();

    config.bStopTracking = ui->cbStopTracking->isChecked();

    if (ui->cobMonitoring->currentIndex() == 1)
    {
        config.ParticleIndex = ui->cobParticle->currentIndex() - 1; // shift!

        int prsec =  ui->cobPrimarySecondary->currentIndex();
        switch (prsec)
        {
        case 0: config.bPrimary = true; config.bSecondary = false; break;
        case 1: config.bPrimary = false; config.bSecondary = true; break;
        case 2: config.bPrimary = true; config.bSecondary = true; break;
        default: qWarning() << "Bad primary/secondary selector";
        }

        int dirin =  ui->cobDirectIndirect->currentIndex();
        switch (dirin)
        {
        case 0: config.bDirect = true;  config.bIndirect = false; break;
        case 1: config.bDirect = false; config.bIndirect = true;  break;
        case 2: config.bDirect = true;  config.bIndirect = true;  break;
        default: qWarning() << "Bad direct/indirect selector";
        }
    }

    config.xbins = ui->sbXbins->value();
    config.ybins = ui->sbYbins->value();
    config.timeBins = ui->sbTimeBins->value();
    config.angleBins = ui->sbAngleBins->value();
    config.waveBins = ui->sbWaveBins->value();
    config.energyBins = ui->sbEnergyBins->value();

    config.timeFrom = ui->ledTimeFrom->text().toDouble();
    config.angleFrom = ui->ledAngleFrom->text().toDouble();
    config.waveFrom = ui->ledWaveFrom->text().toDouble();
    config.energyFrom = ui->ledEnergyFrom->text().toDouble();

    config.timeTo = ui->ledTimeTo->text().toDouble();
    config.angleTo = ui->ledAngleTo->text().toDouble();
    config.waveTo = ui->ledWaveTo->text().toDouble();
    config.energyTo = ui->ledEnergyTo->text().toDouble();
    config.energyUnitsInHist = ui->cobEnergyUnits->currentIndex();
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
    ui->labWaveUnits->setVisible(index == 0);
    ui->sbWaveBins->setVisible(index == 0);

    ui->labEnergy->setVisible(index == 1);
    ui->labEnergyBins->setVisible(index == 1);
    ui->labEnergyFrom->setVisible(index == 1);
    ui->labEnergyTo->setVisible(index == 1);
    ui->ledEnergyFrom->setVisible(index == 1);
    ui->ledEnergyTo->setVisible(index == 1);
    ui->cobEnergyUnits->setVisible(index == 1);
    ui->sbEnergyBins->setVisible(index == 1);
}

void AMonitorDelegateForm::on_pbContentChanged_clicked()
{
    emit contentChanged();
}

void AMonitorDelegateForm::on_pbShowSensitiveDirection_clicked()
{
    emit showSensDirection();
}
