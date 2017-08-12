#include "aelasticcrosssectionautoloadconfig.h"
#include "ui_aelasticcrosssectionautoloadconfig.h"
#include "ajsontools.h"

#include <QFileDialog>
#include <QDebug>

AElasticCrossSectionAutoloadConfig::AElasticCrossSectionAutoloadConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AElasticCrossSectionAutoloadConfig)
{
    ui->setupUi(this);

    ui->frame->setEnabled(false);
}

AElasticCrossSectionAutoloadConfig::~AElasticCrossSectionAutoloadConfig()
{
    delete ui;
}

const QString AElasticCrossSectionAutoloadConfig::getFileName(QString Element, QString Mass) const
{
    if (!ui->cbAuto->isEnabled()) return "";
    QString str = ui->leDir->text() + "/" + ui->lePreName->text() + Element + ui->leSeparatorInName->text() + Mass + ui->leEndName->text();
    return str;
}

bool AElasticCrossSectionAutoloadConfig::isAutoloadEnabled() const
{
    return ui->cbAuto->isEnabled();
}

void AElasticCrossSectionAutoloadConfig::writeToJson(QJsonObject &json) const
{
    json["Enabled"] = ui->cbAuto->isChecked();

    json["Dir"] = ui->leDir->text();

    json["PreName"] = ui->lePreName->text();
    json["MidName"] = ui->leSeparatorInName->text();
    json["EndName"] = ui->leEndName->text();
}

void AElasticCrossSectionAutoloadConfig::readFromJson(QJsonObject &json)
{
    JsonToCheckbox(json, "Enabled", ui->cbAuto);

    JsonToLineEditText(json, "Dir", ui->leDir);

    JsonToLineEditText(json, "PreName", ui->lePreName);
    JsonToLineEditText(json, "MidName", ui->leSeparatorInName);
    JsonToLineEditText(json, "EndName", ui->leEndName);
}

void AElasticCrossSectionAutoloadConfig::on_pbChangeDir_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select directory with cross-section data", Starter, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->leDir->setText(dir);
}

void AElasticCrossSectionAutoloadConfig::on_cbAuto_toggled(bool checked)
{
    emit AutoEnableChanged(checked);
}
