#include "amaterialloaderdialog.h"
#include "ui_amaterialloaderdialog.h"
#include "amaterialparticlecolection.h"
#include "ajsontools.h"
#include "amessage.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSet>
#include <QJsonArray>

AMaterialLoaderDialog::AMaterialLoaderDialog(const QString & fileName, AMaterialParticleCollection & MpCollection, QWidget *parentWidget) :
    QDialog(parentWidget), MpCollection(MpCollection),
    ui(new Ui::AMaterialLoaderDialog)
{
    ui->setupUi(this);
    ui->pbDummt->setVisible(false);
    ui->labForceByNeutron->setVisible(false);

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

    MaterialJsonFrom = json["Material"].toObject();
    parseJson(MaterialJsonFrom, "*MaterialName", NameInFile);
    ui->leMaterialNameInFile->setText(NameInFile);
    ui->leName->setText(NameInFile);

    generateParticleRecords();

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

    generateMatProperties();

    if (neutron) neutron->setChecked(true); //to update forced status
}

AMaterialLoaderDialog::~AMaterialLoaderDialog()
{
    delete ui;
}

const QVector<QString> AMaterialLoaderDialog::getSuppressedParticles() const
{
    QVector<QString> SuppressedParticles;

    for (int i = 0; i < ParticleRecords.size(); i++)
        if (!ParticleRecords.at(i)->isChecked())
            SuppressedParticles << ParticleRecords.at(i)->ParticleName;

    return SuppressedParticles;
}

void AMaterialLoaderDialog::generateParticleRecords()
{
    ParticlesForcedByNeutron = getForcedByNeutron();
    QVector<AParticleRecordForMerge*> SlaveParticlesForNeutron;

    const QVector<QString> UndefinedParticles = MpCollection.getUndefinedParticles(MaterialJsonFrom);
    for (const QString & str : UndefinedParticles)
    {
        AParticleRecordForMerge * rec = new AParticleRecordForMerge(str);
        ParticleRecords << rec;

        if (str == "neutron")
            neutron = rec;
        else if (ParticlesForcedByNeutron.contains(str))
            SlaveParticlesForNeutron << rec;
    }

    if (neutron) neutron->setForcedParticles(SlaveParticlesForNeutron);

    QWidget * w = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout(w);
    for (AParticleRecordForMerge * rec : ParticleRecords)
    {
        QCheckBox * CheckBox = new QCheckBox(rec->ParticleName);
        rec->connectCheckBox(CheckBox);

        connect(CheckBox, &QCheckBox::clicked, [this, rec](bool checked)
        {
            rec->setChecked(checked);
            ui->cbToggleAllParticles->setChecked(false);
        });

        lay->addWidget(CheckBox);
    }
    lay->addStretch();
    ui->scrollArea->setWidget(w);

    ui->cbToggleAllParticles->setVisible(ParticleRecords.size() > 1);
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
    if (ui->twMain->currentIndex() == 0)
    {
        if (isNameAlreadyExists())
        {
            message("Provide a unique name!", this);
            return;
        }
        MaterialJsonFrom["*MaterialName"] = ui->leName->text();
    }
    else
    {
        // target mat properties as base -> replacing only those which are selected by the user
        for (APropertyRecordForMerge * rec : PropertyRecords)
        {
            if (!rec->isChecked()) continue;
            MaterialJsonTarget[rec->key] = rec->value;
        }
        //same with MatPartiles, using QJsonObject to replace selected
        QJsonObject TmpMatParticlesJson;
        QJsonArray  arr = MaterialJsonTarget["MatParticles"].toArray();
        for (int i = 0; i < arr.size(); i++)
        {
            QJsonObject json = arr[i].toObject();
            QJsonObject jsonParticle = json["*Particle"].toObject();
            AParticle ParticleFrom;
            ParticleFrom.readFromJson(jsonParticle);
            TmpMatParticlesJson[ParticleFrom.ParticleName] = json;
        }
        //now override selected MatParticle properties
        for (APropertyRecordForMerge * rec : MatParticleRecords)
        {
            if (!rec->isChecked()) continue;
            TmpMatParticlesJson[rec->key] = rec->value;
        }
        //finally back to QJsonArray
        QJsonArray ArrMP;
        foreach (const QString & key, TmpMatParticlesJson.keys())
            ArrMP.append(TmpMatParticlesJson.value(key));

        MaterialJsonTarget["MatParticles"] = ArrMP;

        //MaterialJsonFrom will be returned to the loader
        MaterialJsonFrom = MaterialJsonTarget;
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

void AMaterialLoaderDialog::on_cbToggleAllParticles_clicked(bool checked)
{
    for (AParticleRecordForMerge * rec : ParticleRecords)
    {
        if (!checked) rec->setForced(false);
        rec->setChecked(checked);
    }
}

void AMaterialLoaderDialog::generateMatProperties()
{
    ui->lwProps->clear();
    clearPropertyRecords();

    int iMat = ui->cobMaterial->currentIndex();
    int numMat = DefinedMaterials.size();
    if (iMat < 0 || iMat >= numMat)
    {
        ui->labError->setText("Corrupted material record");
        ui->labError->setVisible(true);
        ui->tabMerge->setEnabled(false);
        return;
    }

    const AMaterial * matTo = MpCollection[iMat];
    matTo->writeToJson(MaterialJsonTarget, &MpCollection);

    QSet<QString> Ignore;
    Ignore << "*MaterialName" << "*Tags" << "Comments" << "TGeoP1" << "TGeoP2" << "TGeoP3" << "MatParticles";
    foreach (const QString & key, MaterialJsonFrom.keys())
    {
        if (Ignore.contains(key)) continue;

        QJsonValue valueFrom = MaterialJsonFrom.value(key);
        QJsonValue valueTo   = MaterialJsonTarget.value(key);
        if (valueFrom == valueTo) continue;

        APropertyRecordForMerge * rec = new APropertyRecordForMerge(key, valueFrom);
        PropertyRecords << rec;

        QListWidgetItem * item = new QListWidgetItem(ui->lwProps);
        QWidget * wid = new QWidget();
            QHBoxLayout * lay = new QHBoxLayout(wid);
                QCheckBox * cb = new QCheckBox(key);
                rec->connectGuiResources(cb);
                connect(cb, &QCheckBox::clicked, [this, rec](bool flag)
                {
                    rec->setChecked(flag);
                    ui->cbToggleAllProps->setChecked(false);
                    //updateParticleGui();
                });
            lay->addWidget(cb);
        item->setSizeHint(wid->sizeHint());
        ui->lwProps->setItemWidget(item, wid);
    }

    generateInteractionItems();

    int iDifProps = PropertyRecords.size() + MatParticleRecords.size();
    ui->cbToggleAllProps->setVisible(iDifProps > 1);
    ui->labAllMatch->setVisible(iDifProps == 0);
    ui->labSelectProp->setVisible(iDifProps != 0);
    ui->lwProps->setVisible(iDifProps != 0);
}

void AMaterialLoaderDialog::generateInteractionItems()
{
    QJsonArray ArrMpFrom = MaterialJsonFrom["MatParticles"].toArray();
    QJsonArray ArrMpTo   = MaterialJsonTarget["MatParticles"].toArray();

    for (int iFrom = 0; iFrom < ArrMpFrom.size(); iFrom++)
    {
        QJsonObject jsonFrom = ArrMpFrom[iFrom].toObject();
        QJsonObject jsonParticleFrom = jsonFrom["*Particle"].toObject();
        AParticle ParticleFrom;
        ParticleFrom.readFromJson(jsonParticleFrom);

        bool bFoundInTargetMat = false;
        QJsonObject jsonTo;
        QJsonObject jsonParticleTo;
        int iTo;
        for (iTo = 0; iTo < ArrMpTo.size(); iTo++)
        {
            jsonTo = ArrMpTo[iTo].toObject();
            jsonParticleTo = jsonTo["*Particle"].toObject();
            AParticle ParticleTo;
            ParticleTo.readFromJson(jsonParticleTo);
            if (ParticleFrom == ParticleTo)
            {
                bFoundInTargetMat = true;
                break;
            }
        }
        if (bFoundInTargetMat)
        {
            if (jsonFrom == jsonTo)
            {
                //qDebug() << "Records are identical, not creating a record";
                continue;
            }
            //qDebug() << "Records are different, adding";
        }
        //else qDebug() << "Interaction data in target material not found for this particle, adding";

        APropertyRecordForMerge * rec = new APropertyRecordForMerge(ParticleFrom.ParticleName, jsonFrom);
        MatParticleRecords << rec;

        AParticleRecordForMerge * particleRec = findParticleRecord(ParticleFrom.ParticleName);
        if (particleRec)
        {
            rec->connectParticleRecord(particleRec);
            particleRec->connectPropertyRecord(rec);
        }

        QListWidgetItem * item = new QListWidgetItem(ui->lwProps);
        QWidget * wid = new QWidget();
            QHBoxLayout * lay = new QHBoxLayout(wid);
                QCheckBox * cb = new QCheckBox("Interaction properties for " + ParticleFrom.ParticleName);
                rec->connectGuiResources(cb);
                connect(cb, &QCheckBox::clicked, [this, rec](bool flag)
                {
                    rec->setChecked(flag);
                    ui->cbToggleAllProps->setChecked(false);
                });
            lay->addWidget(cb);
        item->setSizeHint(wid->sizeHint());
        ui->lwProps->setItemWidget(item, wid);
    }
}

bool AMaterialLoaderDialog::isSuppressedParticle(const QString & ParticleName) const
{
    for (const AParticleRecordForMerge * rec : ParticleRecords)
        if (rec->ParticleName == ParticleName)
            return !rec->isChecked();
    return false;
}

const QVector<QString> AMaterialLoaderDialog::getForcedByNeutron() const
{
    QVector<QString> VecParticles;

    AMaterialParticleCollection FakeCollection;
    AMaterial Mat;
    Mat.readFromJson(MaterialJsonFrom, &FakeCollection);

    QStringList DefParticles = FakeCollection.getListOfParticleNames();
    int iNeutron = DefParticles.indexOf("neutron");
    if (iNeutron == -1)
        return VecParticles; //neutron not present

    const MatParticleStructure & neutron = Mat.MatParticle.at(iNeutron);

    for (const NeutralTerminatorStructure & term : neutron.Terminators)
        VecParticles << term.getSecondaryParticles(FakeCollection);

    return VecParticles;
}

AParticleRecordForMerge * AMaterialLoaderDialog::findParticleRecord(const QString & particleName)
{
    for (AParticleRecordForMerge * rec : ParticleRecords)
        if (particleName == rec->ParticleName)
            return rec;
    return nullptr;
}

void AMaterialLoaderDialog::clearParticleRecords()
{
    for (auto * r : ParticleRecords) delete r;
    ParticleRecords.clear();
}

void AMaterialLoaderDialog::clearPropertyRecords()
{
    //GUI components are automatically deleted

    for (auto * r : PropertyRecords) delete r;
    PropertyRecords.clear();

    for (auto * r : MatParticleRecords) delete r;
    MatParticleRecords.clear();
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

void AMaterialLoaderDialog::on_cbToggleAllProps_clicked(bool checked)
{
    for (APropertyRecordForMerge * rec : PropertyRecords)
        rec->setChecked(checked);
}

void AMaterialLoaderDialog::on_cobMaterial_activated(int)
{
    generateMatProperties();
}

void AParticleRecordForMerge::connectCheckBox(QCheckBox * cb)
{
    CheckBox = cb;
    updateIndication();
}

void AParticleRecordForMerge::setForcedParticles(QVector<AParticleRecordForMerge *> & vec)
{
    ForcedParticles = vec;
}

void AParticleRecordForMerge::setChecked(bool flag, bool bInduced)
{
    if (bForcedByNeutron) bChecked = true;
    else                  bChecked = flag;
    updateIndication();

    if (!bInduced && linkedPropertyRecord)
        linkedPropertyRecord->setChecked(bChecked, true);

    for (AParticleRecordForMerge * pr : ForcedParticles)
    {
        if (bChecked) pr->setChecked(true);
        pr->setForced(bChecked);
    }
}

void AParticleRecordForMerge::setForced(bool flag)
{
    bForcedByNeutron = flag;
    updateIndication();

    if (linkedPropertyRecord)
        linkedPropertyRecord->setDisabled(bForcedByNeutron);
}

void AParticleRecordForMerge::updateIndication()
{
    if (CheckBox)
    {
        CheckBox->setChecked(bChecked);
        CheckBox->setDisabled(bForcedByNeutron);
    }
}

void APropertyRecordForMerge::setChecked(bool flag, bool bInduced)
{
    if (!bInduced && bDisabled) return;

    bChecked = flag;
    updateIndication();

    if (!bInduced && linkedParticleRecord)
        linkedParticleRecord->setChecked(bChecked, true);
}

void APropertyRecordForMerge::setDisabled(bool flag)
{
    bDisabled = flag;
    updateIndication();
}

void APropertyRecordForMerge::connectGuiResources(QCheckBox *cb)
{
    CheckBox = cb;
    updateIndication();
}

void APropertyRecordForMerge::updateIndication()
{
    if (CheckBox)
    {
        CheckBox->setChecked(bChecked);
        CheckBox->setEnabled(!bDisabled);
    }
}
