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
    ui->labLibMaterial->setText(NameInFile);
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

    generateMatPropRecords();

    if (neutron) neutron->setChecked(true); //to update forced status

    ui->cobMaterial->setCurrentIndex(0);
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

bool AMaterialLoaderDialog::isNameAlreadyExists(const QString & newName) const
{
    return DefinedMaterials.contains(newName);
}

void AMaterialLoaderDialog::updateLoadEnabled()
{
    const bool bNameExists = isNameAlreadyExists(ui->leName->text());
    ui->labAlreadyExists->setVisible(bNameExists);

    bool bLoadActive = true;
    if (ui->cobMode->currentIndex()==0 && bNameExists) bLoadActive = false;
    ui->pbLoad->setEnabled(bLoadActive);
}

void AMaterialLoaderDialog::on_pbDummt_clicked()
{
    //dummy
}

void AMaterialLoaderDialog::on_pbLoad_clicked()
{
    if (ui->cobMode->currentIndex() == 0)
    {
        const QString err = addAsNew(ui->leName->text());
        if (!err.isEmpty())
        {
            message(err, this);
            return;
        }
    }
    else
        mergeWithExistentMaterial();

    accept();
}

QString AMaterialLoaderDialog::addAsNew(const QString & newName)
{
    if (isNameAlreadyExists(newName))
        return "Provide a unique name!";

    MaterialJsonFrom["*MaterialName"] = ui->leName->text();
    return "";
}

void AMaterialLoaderDialog::mergeWithExistentMaterial()
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

    //comments
    QString MatFromComments = MaterialJsonFrom["Comments"].toString();
    QString MatToComments   = MaterialJsonTarget["Comments"].toString();
    MaterialJsonTarget["Comments"] = ">>>>>> Original material comments:\n" + MatToComments + QString("\n\n>>>>>> Comments from merged-in material (%1):\n").arg(NameInFile) + MatFromComments;

    //MaterialJsonFrom will be returned to the loader
    MaterialJsonFrom = MaterialJsonTarget;
}

void AMaterialLoaderDialog::on_pbCancel_clicked()
{
    reject();
}

void AMaterialLoaderDialog::on_leName_textChanged(const QString &)
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

void AMaterialLoaderDialog::generateMatPropRecords()
{
    ui->lwProps->clear();
    clearPropertyRecords();

    int iMat = ui->cobMaterial->currentIndex();
    int numMat = DefinedMaterials.size();
    if (iMat < 0 || iMat >= numMat)
    {
        ui->labError->setText("Corrupted material record");
        ui->labError->setVisible(true);
        ui->sw->setEnabled(false);
        return;
    }

    const AMaterial * matTo = MpCollection[iMat];
    matTo->writeToJson(MaterialJsonTarget, &MpCollection);

    QSet<QString> Ignore;
    Ignore << "*MaterialName" << "Comments" << "TGeoP1" << "TGeoP2" << "TGeoP3" << "MatParticles";
    foreach (const QString & key, MaterialJsonFrom.keys())
    {
        if (Ignore.contains(key)) continue;

        QJsonValue valueFrom = MaterialJsonFrom.value(key);
        QJsonValue valueTo   = MaterialJsonTarget.value(key);
        if (valueFrom == valueTo) continue;

        APropertyRecordForMerge * rec = new APropertyRecordForMerge(key, valueFrom);
        PropertyRecords << rec;

        AListWidgetItem * item = new AListWidgetItem(ui->lwProps);
        QWidget * wid = new QWidget();
            QHBoxLayout * lay = new QHBoxLayout(wid);
            lay->setContentsMargins(6,2,6,2);
                item->cb = new QCheckBox(convertJsonNameToReadable(key));
                rec->connectGuiResources(item->cb);
                connect(item->cb, &QCheckBox::clicked, [this, rec](bool flag)
                {
                    rec->setChecked(flag);
                    ui->cbToggleAllProps->setChecked(false);
                    //updateParticleGui();
                });
            lay->addWidget(item->cb);
                QWidget * comparisonWidget = createComparisonWidget(key, valueFrom, valueTo);
            if (comparisonWidget)
            {
                lay->addWidget(new QLabel("     "));
                lay->addWidget(comparisonWidget);
                wid->setToolTip(comparisonWidget->toolTip());
                lay->addStretch();
            }
        item->setSizeHint(wid->sizeHint());
        ui->lwProps->setItemWidget(item, wid);
    }

    generateInteractionRecords();

    ui->lwProps->sortItems();

    int iDifProps = PropertyRecords.size() + MatParticleRecords.size();
    ui->cbToggleAllProps->setVisible(iDifProps > 1);
    ui->labAllMatch->setVisible(iDifProps == 0);
    ui->labSelectProp->setVisible(iDifProps != 0);
    ui->lwProps->setVisible(iDifProps != 0);
}

void AMaterialLoaderDialog::generateInteractionRecords()
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

        AListWidgetItem * item = new AListWidgetItem(ui->lwProps);
        QWidget * wid = new QWidget();
            QHBoxLayout * lay = new QHBoxLayout(wid);
            lay->setContentsMargins(6,2,6,2);
                item->cb = new QCheckBox("Interaction for " + ParticleFrom.ParticleName);
                rec->connectGuiResources(item->cb);
                connect(item->cb, &QCheckBox::clicked, [this, rec](bool flag)
                {
                    rec->setChecked(flag);
                    ui->cbToggleAllProps->setChecked(false);
                });
            lay->addWidget(item->cb);
            lay->addWidget(new QLabel("     "));
                QWidget * comparisonWidget = createComparisonWidgetMatParticle(jsonFrom, jsonTo);
            lay->addWidget(comparisonWidget);
            lay->addStretch();
        //wid->setToolTip(comparisonWidget->toolTip());
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

QWidget * makeWidget(const QString & s1, const QString & s2)
{
    QWidget * w = new QWidget();
    QHBoxLayout * lay = new QHBoxLayout(w);
    //lay->setContentsMargins(6,2,6,2);
    lay->setContentsMargins(0,0,0,0);
        QLabel * l = new QLabel(s1);
        lay->addWidget(l);
        l = new QLabel(QChar(0x21D0));//8594//8678
        lay->addWidget(l);
        l = new QLabel(QString("<p style='color:#909090;'>%1</p>").arg(s2));
        lay->addWidget(l);

    return w;
}

QWidget *AMaterialLoaderDialog::createComparisonWidget(const QString & key, const QJsonValue &valueFrom, const QJsonValue &valueTo)
{
    QWidget * w = nullptr;

    if (key == "ChemicalComposition")
    {
        QJsonObject from = valueFrom.toObject();
        QJsonObject to   =   valueTo.toObject();

        QString sfrom = from["ElementCompositionString"].toString();
        QString sto   = to  ["ElementCompositionString"].toString();

        if (sfrom.isEmpty()) sfrom = "Undefined";
        if (sto  .isEmpty()) sto   = "Undefined";

        w = makeWidget(sfrom, sto);
    }
    else if (valueFrom.isDouble() && valueTo.isDouble())
    {
        w = makeWidget(QString::number(valueFrom.toDouble()), QString::number(valueTo.toDouble()));
    }
    else if (valueFrom.isArray() && valueTo.isArray())
    {
        QJsonArray from = valueFrom.toArray();
        QJsonArray to   =   valueTo.toArray();

        int sizeFrom = from.size();
        int sizeTo   = to.size();

        QString sfrom = QString("[%1]").arg(sizeFrom);
        QString sto   = QString("[%1]").arg(sizeTo);

        w = makeWidget(sfrom, sto);

        if (key == "*Tags")
        {
            QString sFrom;
            QString sTo;
            for (int i=0; i<sizeFrom; i++) sFrom += from.at(i).toString() + ", ";
            for (int i=0; i<sizeTo;   i++)   sTo +=   to.at(i).toString() + ", ";
            if (sFrom.size() > 1)  sFrom.chop(2);
            if (sTo.size()   > 1)    sTo.chop(2);
            if (sFrom.isEmpty()) sFrom = "Undefined";
            if (sTo.isEmpty())     sTo = "Undefined";
            w->setToolTip(sFrom + "\n\nwill replace\n\n" + sTo);
        }
    }

    return w;
}

const QString makeStringForMatParticleComparison(const QJsonObject &json)
{
    if ( !json["TrackingAllowed"].toBool() ) return "No tracking";
    if (json["MatIsTransparent"].toBool()) return "Transparent";

    QString s;
    if (json.contains("TotalInteraction"))
    {
        QJsonArray ar = json["TotalInteraction"].toArray();
        s += QString("Data[%1]").arg(ar.size());
    }
    if (json.contains("Terminators"))
    {
        QJsonArray ar = json["Terminators"].toArray();
        s += QString("Term[%1]").arg(ar.size());
    }
    return s;
}

QWidget *AMaterialLoaderDialog::createComparisonWidgetMatParticle(const QJsonObject &jsonFrom, const QJsonObject &jsonTo)
{
    const QString sfrom = makeStringForMatParticleComparison(jsonFrom);
    const QString sto   = makeStringForMatParticleComparison(jsonTo);
    return makeWidget(sfrom, sto);
}

const QString AMaterialLoaderDialog::convertJsonNameToReadable(const QString & key) const
{
    QJsonObject js;

    js["ChemicalComposition"]       = "Composition";
    js["RefractiveIndex"]           = "Refractive index";
    js["BulkAbsorption"]            = "Absorption coefficient";
    js["RayleighMFP"]               = "Rayleigh mean free path";
    js["RayleighWave"]              = "Rayleigh wavelength";
    js["ReemissionProb"]            = "Reemission probability";
    js["PhotonYieldDefault"]        = "Photon yield (prim scint, default)";
    js["PrimScintRaise"]            = "Rise time (prim scint)";
    js["W"]                         = "Energy per e/ion pair";
    js["SecScint_PhYield"]          = "Photon yield (sec scint)";
    js["SecScint_Tau"]              = "Decay time (sec scint)";
    js["ElDriftVelo"]               = "Electron drift speed";
    js["ElDiffusionL"]              = "Electron diffusion coeff (L)";
    js["ElDiffusionT"]              = "Electron diffusion coeff (T)";

    js["RefractiveIndexWave"]       = "Refractive index (wave)";
    js["BulkAbsorptionWave"]        = "Absorption coefficient (wave)";
    js["ReemissionProbabilityWave"] = "Reemission probability (wave)";
    js["PrimScintSpectrum"]         = "Emission spectrum (prim scint)";
    js["SecScintSpectrum"]          = "Emission spectrum (sec scint)";
    js["*Tags"]                     = "Material tags";

    if (js.contains(key)) return js[key].toString();
    return key;
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
    generateMatPropRecords();
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

bool AListWidgetItem::operator<(const QListWidgetItem & other) const
{
    const AListWidgetItem * pOther = dynamic_cast<const AListWidgetItem*>(&other);
    if (!pOther) return QListWidgetItem::operator<(other);

    if (!cb || !pOther->cb) return QListWidgetItem::operator<(other);

    return cb->text() < pOther->cb->text();
}

void AMaterialLoaderDialog::on_sw_currentChanged(int arg1)
{
    ui->pbLoad->setText( arg1 == 0 ? "Add new material"
                                   : "Merge with existent");
    updateLoadEnabled();
}
