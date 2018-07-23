#ifndef AMATPARTICLECONFIGURATOR_H
#define AMATPARTICLECONFIGURATOR_H

#include <QDialog>
#include <QString>

class QJsonObject;
class GlobalSettingsClass;

namespace Ui {
class AMatParticleConfigurator;
}

class AMatParticleConfigurator : public QDialog
{
    Q_OBJECT

public:
    explicit AMatParticleConfigurator(GlobalSettingsClass *GlobSet, QWidget *parent = 0);
    ~AMatParticleConfigurator();

    const QString getElasticScatteringFileName(QString Element, QString Mass) const;
    const QString getAbsorptionFileName(QString Element, QString Mass) const;
    int           getCrossSectionLoadOption() const;
    bool          isAutoloadEnabled() const;
    bool          isEnergyRangeLimited() const;
    double        getMinEnergy() const;
    double        getMaxEnergy() const;
    const QString getNatAbundFileName() const;
    const QString getCrossSectionDataDir() const;
    const QString getHeaderLineId() const;
    int           getNumCommentLines() const;

    const QVector<QPair<int, double> > getIsotopes(QString ElementName) const; //empty vector - element not found; otherewise QVector<mass, abund>

    void setStarterDir(const QString starter) {StarterDir = starter;}

    void writeToJson(QJsonObject& json) const;
    void readFromJson(QJsonObject& json);

private slots:
    void on_pbChangeDir_clicked();
    void on_pbUpdateGlobSet_clicked();

private:
    Ui::AMatParticleConfigurator *ui;
    GlobalSettingsClass* GlobSet;
    QString StarterDir;
};

#endif // AMATPARTICLECONFIGURATOR_H
