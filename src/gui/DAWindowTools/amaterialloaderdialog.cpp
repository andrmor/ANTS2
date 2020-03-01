#include "amaterialloaderdialog.h"
#include "ui_amaterialloaderdialog.h"
#include "amaterialparticlecolection.h"
#include "ajsontools.h"
#include "amessage.h"

#include <QVBoxLayout>
#include <QCheckBox>

AMaterialLoaderDialog::AMaterialLoaderDialog(const QString & fileName, AMaterialParticleCollection & MpCollection, QWidget *parentWidget) :
    QDialog(parentWidget), MpCollection(MpCollection),
    ui(new Ui::AMaterialLoaderDialog)
{
    ui->setupUi(this);

    ui->pbDummt->setVisible(false);
    DefinedMaterials = MpCollection.getListOfMaterialNames();

    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setWidgetResizable(true);

    QJsonObject json;
    bFileOK = LoadJsonFromFile(json, fileName);
    if (!bFileOK)
    {
        ui->labError->setText("Cannot open file: "+fileName);
        return;
    }
    if (!json.contains("Material"))
    {
        bFileOK = false;
        ui->labError->setText("File format error: Json with material settings not found");
        return;
    }
    ui->labError->setVisible(false);

    MaterialJson = json["Material"].toObject();
    parseJson(MaterialJson, "*MaterialName", NameInFile);
    ui->leMaterialNameInFile->setText(NameInFile);
    ui->leName->setText(NameInFile);

    NewParticles = MpCollection.getUndefinedParticles(MaterialJson);

    updateParticleGui();
}

AMaterialLoaderDialog::~AMaterialLoaderDialog()
{
    delete ui;
}

void AMaterialLoaderDialog::onCheckBoxClicked()
{
    ui->cbToggleAll->blockSignals(true);
    ui->cbToggleAll->setChecked(false);
    ui->cbToggleAll->blockSignals(false);
}

void AMaterialLoaderDialog::updateParticleGui()
{
    ui->cbToggleAll->setVisible(NewParticles.size() > 1);

    QWidget * w = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout(w);
    for (const QString & name : NewParticles)
    {
        QCheckBox * cb = new QCheckBox(name);
        cb->setChecked(true);
        cbVec << cb;
        lay->addWidget(cb);
    }
    lay->addStretch();
    ui->scrollArea->setWidget(w);
}

bool AMaterialLoaderDialog::isNameAlreadyExists() const
{
    return DefinedMaterials.contains(ui->leName->text());
}

void AMaterialLoaderDialog::updateLoadEnabled()
{
    bool bLoadActive = true;
    if (ui->twMain->currentIndex() == 0 && isNameAlreadyExists()) bLoadActive = false;
    ui->pbLoad->setEnabled(bLoadActive);
}

void AMaterialLoaderDialog::on_pbDummt_clicked()
{
    //dummy
}

void AMaterialLoaderDialog::on_pbLoad_clicked()
{
    for (int i = 0; i < cbVec.size(); i++)
        if (!cbVec.at(i)->isChecked())
            SuppressParticles << NewParticles.at(i);

    if (ui->twMain->currentIndex() == 0)
    {
        if (isNameAlreadyExists())
        {
            message("Provide a unique name!", this);
            return;
        }
        MaterialJson["*MaterialName"] = ui->leName->text();
    }
    else
    {
        message("Not implemented yet!", this);
        return;
    }
    accept();
}

void AMaterialLoaderDialog::on_pbCancel_clicked()
{
    reject();
}

void AMaterialLoaderDialog::on_leName_textChanged(const QString &)
{
    ui->labAlreadyExists->setVisible(isNameAlreadyExists());
    updateLoadEnabled();
}

void AMaterialLoaderDialog::on_twMain_currentChanged(int index)
{
    updateLoadEnabled();
}

void AMaterialLoaderDialog::on_cbToggleAll_toggled(bool checked)
{
    for (QCheckBox * cb : cbVec) cb->setChecked(checked);
}
