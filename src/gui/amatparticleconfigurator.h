#ifndef AMATPARTICLECONFIGURATOR_H
#define AMATPARTICLECONFIGURATOR_H

#include <QDialog>
#include <QString>

class QJsonObject;

namespace Ui {
class AMatParticleConfigurator;
}

class AMatParticleConfigurator : public QDialog
{
    Q_OBJECT

public:
    explicit AMatParticleConfigurator(QWidget *parent = 0);
    ~AMatParticleConfigurator();

    const QString getElasticScatteringFileName(QString Element, QString Mass) const;
    const QString getAbsorptionFileName(QString Element, QString Mass) const;

    int           getCrossSectionLoadOption() const; //meV=0, eV, keV, MeV

    bool          isAutoloadEnabled() const;

    bool          isEnergyRangeLimited() const;
    double        getMinEnergy() const;
    double        getMaxEnergy() const;

    const QString getNatAbundFileName() const;

    const QString getCrossSectionFirstDataDir() const; //For GUI only
    const QString getHeaderLineId() const;
    int           getNumCommentLines() const;

    const QVector<QPair<int, double> > getIsotopes(QString ElementName) const; //empty vector - element not found; otherewise QVector<mass, abund>

    void setStarterDir(const QString starter) {StarterDir = starter;}

    void writeToJson(QJsonObject& json) const;
    void readFromJson(QJsonObject& json);

private slots:
    void on_pbChangeDir_clicked();
    void on_pbUpdateGlobSet_clicked();

    void on_pbShowSystemDir_clicked();

    void on_pbChangeDir_customContextMenuRequested(const QPoint &pos);

private:
    Ui::AMatParticleConfigurator *ui;
    QString StarterDir;

    QString CrossSectionSystemDir;
};

#endif // AMATPARTICLECONFIGURATOR_H
