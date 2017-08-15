#ifndef AELASTICCROSSSECTIONAUTOLOADCONFIG_H
#define AELASTICCROSSSECTIONAUTOLOADCONFIG_H

#include <QDialog>
#include <QString>

class QJsonObject;
class GlobalSettingsClass;

namespace Ui {
class AElasticCrossSectionAutoloadConfig;
}

class AElasticCrossSectionAutoloadConfig : public QDialog
{
    Q_OBJECT

public:
    explicit AElasticCrossSectionAutoloadConfig(GlobalSettingsClass *GlobSet, QWidget *parent = 0);
    ~AElasticCrossSectionAutoloadConfig();

    const QString getFileName(QString Element, QString Mass) const;
    int           getCrossSectionLoadOption() const;
    bool          isAutoloadEnabled() const;
    const QVector<QPair<int, double> > getIsotopes(QString ElementName) const; //empty vector - element not found; otherewise QVector<mass, abund>

    void setStarterDir(const QString starter) {StarterDir = starter;}

    void writeToJson(QJsonObject& json) const;
    void readFromJson(QJsonObject& json);

private slots:
    void on_pbChangeDir_clicked();

    void on_pbUpdateGlobSet_clicked();

    void on_pbChangeNatAbFile_clicked();

private:
    Ui::AElasticCrossSectionAutoloadConfig *ui;
    GlobalSettingsClass* GlobSet;
    QString StarterDir;

};

#endif // AELASTICCROSSSECTIONAUTOLOADCONFIG_H
