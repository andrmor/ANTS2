#include "amaterialloaderdialog.h"
#include "ui_amaterialloaderdialog.h"
#include "amaterialparticlecolection.h"
#include "ajsontools.h"
#include "amessage.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSet>

AMaterialLoaderDialog::AMaterialLoaderDialog(const QString & fileName, AMaterialParticleCollection & MpCollection, QWidget *parentWidget) :
    QDialog(parentWidget), MpCollection(MpCollection),
    ui(new Ui::AMaterialLoaderDialog)
{
    ui->setupUi(this);

    ui->pbDummt->setVisible(false);
    DefinedMaterials = MpCollection.getListOfMaterialNames();
    ui->cobMaterial->addItems(DefinedMaterials);

    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setWidgetResizable(true);

    QJsonObject json;
    bFileOK = LoadJsonFromFile(json, fileName);
    if (!bFileOK)
    {
        ui->labError->setText("Cannot open file: "+fileName);
        ui->labError->setVisible(true);
        return;
    }
    if (!json.contains("Material"))
    {
        bFileOK = false;
        ui->labError->setText("File format error: Json with material settings not found");
        ui->labError->setVisible(true);
        return;
    }
    ui->labError->setVisible(false);

    MaterialJson = json["Material"].toObject();
    parseJson(MaterialJson, "*MaterialName", NameInFile);
    ui->leMaterialNameInFile->setText(NameInFile);
    ui->leName->setText(NameInFile);

    NewParticles = MpCollection.getUndefinedParticles(MaterialJson);

    updateParticleGui();

    if (DefinedMaterials.size() != 0)
    {
        int iBest = 0;
        int BestVal = -1;
        for (int i=0; i<DefinedMaterials.size(); i++)
        {
            const int match = getMatchValue(NameInFile, DefinedMaterials.at(i));
            if (match > BestVal)
            {
                iBest = i;
                BestVal = match;
            }
        }
        ui->cobMaterial->setCurrentIndex(iBest);
    }
    updatePropertiesGui();
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

void AMaterialLoaderDialog::on_twMain_currentChanged(int)
{
    updateLoadEnabled();
}

void AMaterialLoaderDialog::on_cbToggleAll_toggled(bool checked)
{
    for (QCheckBox * cb : cbVec) cb->setChecked(checked);
}

void AMaterialLoaderDialog::updatePropertiesGui()
{
    ui->lwProps->clear();

    int numMat = DefinedMaterials.size();
    ui->tabMerge->setEnabled(numMat > 0);
    if (numMat == 0) return;

    int iMat = ui->cobMaterial->currentIndex();
    if (iMat < 0 || iMat >= numMat)
    {
        ui->labError->setText("Corrupted material record");
        ui->labError->setVisible(true);
        return;
    }
    ui->labError->setVisible(false);

    const AMaterial * matTo = MpCollection[iMat];

    QJsonObject MaterialTo;
    matTo->writeToJson(MaterialTo, &MpCollection);

    qDebug() << "Merging" << NameInFile << " to " << matTo->name;
    QSet<QString> Ignore;
    Ignore << "*MaterialName" << "*Tags" << "Comments"
           << "TGeoP1" << "TGeoP2" << "TGeoP3"
           << "MatParticles";

    int iDifProps = 0;
    foreach(const QString & key, MaterialJson.keys())
    {
        qDebug() << "\nKey = " << key;
        if (Ignore.contains(key)) continue;

        QJsonValue valueFrom = MaterialJson.value(key);
        QJsonValue valueTo   = MaterialTo.value(key);
        if (valueFrom == valueTo) continue;

        iDifProps++;
        QListWidgetItem * item = new QListWidgetItem(ui->lwProps);

        QWidget * wid = new QWidget();
            QHBoxLayout * lay = new QHBoxLayout(wid);
                QCheckBox * cb = new QCheckBox(key);
                cb->setChecked(true);
            lay->addWidget(cb);
        item->setSizeHint(wid->sizeHint());
        ui->lwProps->setItemWidget(item, wid);
    }

    ui->cbToggleAllProps->setVisible(iDifProps > 1);
    ui->labAllMatch->setVisible(iDifProps == 0);
    ui->labSelectProp->setVisible(iDifProps != 0);
    ui->lwProps->setVisible(iDifProps != 0);
}

int AMaterialLoaderDialog::getMatchValue(const QString & s1, const QString & s2) const
{
    int imatch = 0;
    for (int i = 0; (i < s1.size()) && (i < s2.size()); i++)
    {
        if (s1.at(i) == s2.at(i)) imatch++;
        else break;
    }
    return imatch;
}

void AMaterialLoaderDialog::on_cbToggleAllProps_toggled(bool checked)
{

}

void AMaterialLoaderDialog::on_cobMaterial_activated(int)
{
    updatePropertiesGui();
}
