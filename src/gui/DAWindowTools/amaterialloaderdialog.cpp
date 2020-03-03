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

    const QVector<QString> UndefinedParticles = MpCollection.getUndefinedParticles(MaterialJsonFrom);
    for (const QString & str : UndefinedParticles)
    {
        AParticleRecordForMerge pr(str);
        ParticleRecords << pr;
    }
    generateParticleGui();
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

    generateMatProperties();
}

AMaterialLoaderDialog::~AMaterialLoaderDialog()
{
    delete ui;
}

const QVector<QString> AMaterialLoaderDialog::getSuppressedParticles() const
{
    QVector<QString> SuppressedParticles;

    for (int i = 0; i < ParticleRecords.size(); i++)
        if (!ParticleRecords.at(i).isChecked())
            SuppressedParticles << ParticleRecords.at(i).ParticleName;

    return SuppressedParticles;
}

void AMaterialLoaderDialog::generateParticleGui()
{
    ui->cbToggleAllParticles->setVisible(ParticleRecords.size() > 1);

    QWidget * w = new QWidget();
    QVBoxLayout * lay = new QVBoxLayout(w);
    for (AParticleRecordForMerge & rec : ParticleRecords)
    {
        QCheckBox * CheckBox = new QCheckBox(rec.ParticleName);
        rec.connectCheckBox(CheckBox);
            connect(CheckBox, &QCheckBox::clicked, [this, &rec](bool checked)
            {
                rec.setChecked(checked);
                if (rec.ParticleName == "neutron") updateParticleGui();
                updateMatPropertiesGui();
                ui->cbToggleAllParticles->setChecked(false);
            });
        lay->addWidget(CheckBox);
    }
    lay->addStretch();
    ui->scrollArea->setWidget(w);
}

void AMaterialLoaderDialog::updateParticleGui()
{
    const QVector<QString> ParticlesForcedByNeutron = getForcedByNeutron();

    for (AParticleRecordForMerge & pr : ParticleRecords)
        pr.setForced( ParticlesForcedByNeutron.contains(pr.ParticleName) );

    ui->labForceByNeutron->setVisible( !ParticlesForcedByNeutron.isEmpty() );
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
        //message("Not implemented yet!", this);
        //return;

        // target mat properties as base -> replacing only those which are selected by the user
        for (APropertyRecordForMerge & rec : PropertyRecords)
        {
            if (!rec.isChecked()) continue;
            MaterialJsonTarget[rec.key] = rec.value;
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
        for (APropertyRecordForMerge & rec : MatParticleRecords)
        {
            if (!rec.isChecked()) continue;
            TmpMatParticlesJson[rec.key] = rec.value;
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
    for (AParticleRecordForMerge & rec : ParticleRecords)
    {
        if (!checked) rec.setForced(false);
        rec.setChecked(checked);
    }

    updateParticleGui();
    updateMatPropertiesGui();
}

void AMaterialLoaderDialog::generateMatProperties()
{
    ui->lwProps->clear();
    PropertyRecords.clear();
    MatParticleRecords.clear();

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

        APropertyRecordForMerge rec(key, valueFrom);

        QListWidgetItem * item = new QListWidgetItem(ui->lwProps);
        QWidget * wid = new QWidget();
            QHBoxLayout * lay = new QHBoxLayout(wid);
                QCheckBox * cb = new QCheckBox(key);
                rec.connectGuiResources(cb);
                connect(cb, &QCheckBox::clicked, [this, &rec](bool flag)
                {
                    rec.setChecked(flag);
                    ui->cbToggleAllProps->setChecked(false);
                });
            lay->addWidget(cb);
        item->setSizeHint(wid->sizeHint());
        ui->lwProps->setItemWidget(item, wid);

        PropertyRecords << rec;
    }

    generateInteractionItems();

    int iDifProps = PropertyRecords.size() + MatParticleRecords.size();
    ui->cbToggleAllProps->setVisible(iDifProps > 1);
    ui->labAllMatch->setVisible(iDifProps == 0);
    ui->labSelectProp->setVisible(iDifProps != 0);
    ui->lwProps->setVisible(iDifProps != 0);

    updateMatPropertiesGui();
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
        qDebug() << "\nParticle" << ParticleFrom.ParticleName;


        if (isSuppressedParticle(ParticleFrom.ParticleName))
        {
            qDebug() << "This particle will not be imported, skipping";
            continue;
        }

        bool bFound = false;
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
                bFound = true;
                break;
            }
        }

        if (bFound)
        {
            if (jsonFrom == jsonTo)
            {
                qDebug() << "Records are identical, skipping";
                continue;
            }
            qDebug() << "Records are different, adding";
        }
        else qDebug() << "Interaction data in target material not found for this particle, adding";

        APropertyRecordForMerge rec(ParticleFrom.ParticleName, jsonFrom);

        QListWidgetItem * item = new QListWidgetItem(ui->lwProps);
        QWidget * wid = new QWidget();
            QHBoxLayout * lay = new QHBoxLayout(wid);
                QCheckBox * cb = new QCheckBox("Interaction properties for " + ParticleFrom.ParticleName);
                rec.connectGuiResources(cb);
                connect(cb, &QCheckBox::clicked, [this, &rec](bool flag)
                {
                    rec.setChecked(flag);
                    ui->cbToggleAllProps->setChecked(false);
                });
            lay->addWidget(cb);
        item->setSizeHint(wid->sizeHint());
        ui->lwProps->setItemWidget(item, wid);

        MatParticleRecords << rec;
    }
}

void AMaterialLoaderDialog::updateMatPropertiesGui()
{
    for (APropertyRecordForMerge & rec : MatParticleRecords)
        rec.setDisabled( isSuppressedParticle(rec.key) );
}

bool AMaterialLoaderDialog::isSuppressedParticle(const QString & ParticleName) const
{
    for (const AParticleRecordForMerge & rec : ParticleRecords)
        if (rec.ParticleName == ParticleName)
            return !rec.isChecked();
    return false;
}

const QVector<QString> AMaterialLoaderDialog::getForcedByNeutron() const
{
    QVector<QString> VecParticles;

    AMaterialParticleCollection FakeCollection;
    AMaterial Mat;
    Mat.readFromJson(MaterialJsonFrom, &FakeCollection, getSuppressedParticles());

    QStringList DefParticles = FakeCollection.getListOfParticleNames();
    int iNeutron = DefParticles.indexOf("neutron");
    if (iNeutron == -1)
    {
        qDebug() << "Neutron is not found";
        return VecParticles;
    }

    const MatParticleStructure & neutron = Mat.MatParticle.at(iNeutron);

    for (const NeutralTerminatorStructure & term : neutron.Terminators)
        VecParticles << term.getSecondaryParticles(FakeCollection);

    return VecParticles;
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
    for (APropertyRecordForMerge & rec : PropertyRecords)
        rec.setChecked(checked);
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

void AParticleRecordForMerge::setChecked(bool flag)
{
    if (bForcedByNeutron) bChecked = true;
    else                  bChecked = flag;
    updateIndication();
}

void AParticleRecordForMerge::setForced(bool flag)
{
    bForcedByNeutron = flag;
    if (flag) bChecked = true;
    updateIndication();
}

void AParticleRecordForMerge::updateIndication()
{
    if (CheckBox)
    {
        CheckBox->setChecked(bChecked);
        CheckBox->setEnabled(!bForcedByNeutron);
    }
}

void APropertyRecordForMerge::setChecked(bool flag)
{
    bChecked = flag;
    updateIndication();
}

void APropertyRecordForMerge::setDisabled(bool flag)
{
    bDisabled = flag;
    if (flag) bChecked = false;
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
        CheckBox->setDisabled(bDisabled);
    }
}
