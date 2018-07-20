#include "amatparticleconfigurator.h"
#include "ui_amatparticleconfigurator.h"
#include "ajsontools.h"
#include "globalsettingsclass.h"
#include "afiletools.h"

#include <QDoubleValidator>
#include <QFileDialog>
#include <QDebug>

AMatParticleConfigurator::AMatParticleConfigurator(GlobalSettingsClass *GlobSet, QWidget *parent) :
    QDialog(parent), ui(new Ui::AMatParticleConfigurator), GlobSet(GlobSet)
{
    ui->setupUi(this);
    ui->frame->setEnabled(false);
    ui->pbUpdateGlobSet->setVisible(false);
    ui->cobUnitsForEllastic->setCurrentIndex(1);

    QDoubleValidator* val = new QDoubleValidator(this);
    val->setBottom(0);
    ui->ledMinEnergy->setValidator(val);
    ui->ledMaxEnergy->setValidator(val);

    if (!GlobSet->MaterialsAndParticlesSettings.isEmpty())
        readFromJson(GlobSet->MaterialsAndParticlesSettings);

    if (ui->leNatAbundFile->text().isEmpty())
        ui->leNatAbundFile->setText(GlobSet->ExamplesDir + "/IsotopeNaturalAbundances.txt");
}

AMatParticleConfigurator::~AMatParticleConfigurator()
{
    delete ui;
}

const QString AMatParticleConfigurator::getElasticScatteringFileName(QString Element, QString Mass) const
{
    if (!ui->cbAuto->isEnabled()) return "";
    QString str = ui->leDir->text() + "/" + ui->lePreName->text() + Element + ui->leSeparatorInName->text() + Mass + ui->leEndName->text();
    return str;
}

const QString AMatParticleConfigurator::getAbsorptionFileName(QString Element, QString Mass) const
{
    if (!ui->cbAuto->isEnabled()) return "";
    QString str = ui->leDir->text() + "/" + ui->lePreNameAbs->text() + Element + ui->leSeparatorInNameAbs->text() + Mass + ui->leEndNameAbs->text();
    return str;
}

int AMatParticleConfigurator::getCrossSectionLoadOption() const
{
    return ui->cobUnitsForEllastic->currentIndex();
}

const QVector<QPair<int, double> > AMatParticleConfigurator::getIsotopes(QString ElementName) const
{
    QVector<QPair<int, double> > tmp;
    QString Table;
    bool bOK = LoadTextFromFile(ui->leNatAbundFile->text(), Table);
    if (!bOK) return tmp;

    QMap<QString, QVector<QPair<int, double> > > IsotopeMap;  //Key - element name, contains QVector<mass, abund>
    QStringList SL = Table.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
    for (QString s : SL)
    {
        QStringList f = s.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        if (f.size() != 2) continue;
        bool bOK = true;
        double Abundancy = (f.last()).toDouble(&bOK);
        if (!bOK) continue;
        QString mass = f.first();
        mass.remove(QRegExp("[a-zA-Z]"));
        int Mass = mass.toInt(&bOK);
        if (!bOK) continue;
        QString Element = f.first();
        Element.remove(QRegExp("[0-9]"));
        //qDebug() << Element << Mass << Abundancy;
        tmp.clear();
        tmp << QPair<int, double>(Mass, Abundancy);
        if (IsotopeMap.contains(Element)) (IsotopeMap[Element]) << tmp;
        else IsotopeMap[Element] = tmp;
    }
    //qDebug() << data;

    tmp.clear();
    if (!IsotopeMap.contains(ElementName)) return tmp;
    return IsotopeMap[ElementName];
}

bool AMatParticleConfigurator::isAutoloadEnabled() const
{
    return ui->cbAuto->isChecked();
}

bool AMatParticleConfigurator::isEnergyRangeLimited() const
{
    return ui->cbOnlyInRange->isChecked();
}

double AMatParticleConfigurator::getMinEnergy() const
{
    return ui->ledMinEnergy->text().toDouble();
}

double AMatParticleConfigurator::getMaxEnergy() const
{
    return ui->ledMaxEnergy->text().toDouble();
}

const QString AMatParticleConfigurator::getNatAbundFileName() const
{
    return ui->leNatAbundFile->text();
}

const QString AMatParticleConfigurator::getCrossSectionDataDir() const
{
    return ui->leDir->text();
}

const QString AMatParticleConfigurator::getHeaderLineId() const
{
    return ui->leHeaderId->text();
}

int AMatParticleConfigurator::getNumCommentLines() const
{
    return ui->sbNumHeaderLines->value();
}

void AMatParticleConfigurator::writeToJson(QJsonObject &json) const
{
    json["CSunits"] = ui->cobUnitsForEllastic->currentIndex();
    json["OnlyLoadEnergyInRange"] = ui->cbOnlyInRange->isChecked();
    json["MinEnergy"] = ui->ledMinEnergy->text().toDouble();
    json["MaxEnergy"] = ui->ledMaxEnergy->text().toDouble();

    json["NaturalAbundanciesFileName"] = ui->leNatAbundFile->text();

    json["EnableAutoLoad"] = ui->cbAuto->isChecked();
    json["CrossSectionsDir"] = ui->leDir->text();
    json["PreName"] = ui->lePreName->text();
    json["MidName"] = ui->leSeparatorInName->text();
    json["EndName"] = ui->leEndName->text();
    json["PreNameAbs"] = ui->lePreNameAbs->text();
    json["MidNameAbs"] = ui->leSeparatorInNameAbs->text();
    json["EndNameAbs"] = ui->leEndNameAbs->text();

    json["HeaderId"] = ui->leHeaderId->text();
    json["HeaderNumLines"] = ui->sbNumHeaderLines->value();
}

void AMatParticleConfigurator::readFromJson(QJsonObject &json)
{
    JsonToComboBox(json, "CSunits", ui->cobUnitsForEllastic);
    JsonToCheckbox (json, "OnlyLoadEnergyInRange", ui->cbOnlyInRange);
    JsonToLineEditDouble(json, "MinEnergy", ui->ledMinEnergy);
    JsonToLineEditDouble(json, "MaxEnergy", ui->ledMaxEnergy);

    JsonToLineEditText(json, "NaturalAbundanciesFileName", ui->leNatAbundFile);

    JsonToCheckbox(json, "EnableAutoLoad", ui->cbAuto);
    JsonToLineEditText(json, "CrossSectionsDir", ui->leDir);
    JsonToLineEditText(json, "PreName", ui->lePreName);
    JsonToLineEditText(json, "MidName", ui->leSeparatorInName);
    JsonToLineEditText(json, "EndName", ui->leEndName);
    JsonToLineEditText(json, "PreNameAbs", ui->lePreNameAbs);
    JsonToLineEditText(json, "MidNameAbs", ui->leSeparatorInNameAbs);
    JsonToLineEditText(json, "EndNameAbs", ui->leEndNameAbs);

    JsonToLineEditText(json, "HeaderId", ui->leHeaderId);
    JsonToSpinBox(json, "HeaderNumLines", ui->sbNumHeaderLines);
}

void AMatParticleConfigurator::on_pbChangeDir_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select directory with neutron cross-section data", StarterDir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) return;
    ui->leDir->setText(dir);
    on_pbUpdateGlobSet_clicked();
}

void AMatParticleConfigurator::on_pbUpdateGlobSet_clicked()
{
    writeToJson(GlobSet->MaterialsAndParticlesSettings);
}

void AMatParticleConfigurator::on_pbChangeNatAbFile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select file listing isotope natural abundances", StarterDir, "Data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    ui->leNatAbundFile->setText(fileName);
    on_pbUpdateGlobSet_clicked();
}
