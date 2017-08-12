#ifndef AELASTICCROSSSECTIONAUTOLOADCONFIG_H
#define AELASTICCROSSSECTIONAUTOLOADCONFIG_H

#include <QDialog>
#include <QString>

class QJsonObject;

namespace Ui {
class AElasticCrossSectionAutoloadConfig;
}

class AElasticCrossSectionAutoloadConfig : public QDialog
{
    Q_OBJECT

public:
    explicit AElasticCrossSectionAutoloadConfig(QWidget *parent = 0);
    ~AElasticCrossSectionAutoloadConfig();

    const QString getFileName(QString Element, QString Mass) const;
    bool isAutoloadEnabled() const;
    void setStarter(const QString starter) {Starter = starter;}

    void writeToJson(QJsonObject& json) const;
    void readFromJson(QJsonObject& json);

private slots:
    void on_pbChangeDir_clicked();

    void on_cbAuto_toggled(bool checked);

private:
    Ui::AElasticCrossSectionAutoloadConfig *ui;
    QString Starter;

signals:
    void AutoEnableChanged(bool);
};

#endif // AELASTICCROSSSECTIONAUTOLOADCONFIG_H
