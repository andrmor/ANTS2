#ifndef AMATERIALLOADERDIALOG_H
#define AMATERIALLOADERDIALOG_H

#include <QDialog>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

class AMaterialParticleCollection;
class QCheckBox;

namespace Ui {
class AMaterialLoaderDialog;
}

class AParticleRecordForMerge
{
public:
    AParticleRecordForMerge(const QString & name) : ParticleName(name){}
    AParticleRecordForMerge(){}

    QString ParticleName;

    void connectCheckBox(QCheckBox * cb);

    void setChecked(bool flag);
    void setForced(bool flag);

    bool isChecked() const {return bChecked;}
    bool isForced() const  {return bForcedByNeutron;}

private:
    bool bChecked         = true;
    bool bForcedByNeutron = false;

    QCheckBox * CheckBox  = nullptr;

private:
    void updateIndication();
};

class APropertyRecordForMerge
{
public:
    APropertyRecordForMerge(const QString & key, const QJsonValue & value) : key(key), value(value) {}
    APropertyRecordForMerge(){}

    QString    key;
    QJsonValue value;

    void setChecked(bool flag);
    bool isChecked() const {return bChecked;}

    void setDisabled(bool flag);
    bool isDisabled() const {return bDisabled;}

    void connectGuiResources(QCheckBox * cb);

private:
    bool bChecked        = true;
    bool bDisabled       = false;
    QCheckBox * CheckBox = nullptr;

private:
    void updateIndication();
};

class AMaterialLoaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AMaterialLoaderDialog(const QString & fileName, AMaterialParticleCollection & MpCollection, QWidget * parentWidget = nullptr);
    ~AMaterialLoaderDialog();

    const QVector<QString> getSuppressedParticles() const;
    const QJsonObject      getMaterialJson() const {return MaterialJsonFrom;}

private slots:
    void on_pbDummt_clicked();
    void on_pbLoad_clicked();
    void on_pbCancel_clicked();
    void on_leName_textChanged(const QString &arg1);
    void on_twMain_currentChanged(int index);
    void on_cbToggleAllParticles_clicked(bool checked);
    void on_cbToggleAllProps_clicked(bool checked);
    void on_cobMaterial_activated(int index);

private:
    AMaterialParticleCollection & MpCollection;
    Ui::AMaterialLoaderDialog   * ui;

    QStringList DefinedMaterials;
    bool        bFileOK = true;

    QJsonObject MaterialJsonFrom;
    QJsonObject MaterialJsonTarget;
    QString     NameInFile;

    QVector<AParticleRecordForMerge> ParticleRecords;
    QVector<APropertyRecordForMerge> PropertyRecords;
    QVector<APropertyRecordForMerge> MatParticleRecords;

private:
    void generateParticleGui();
    void updateParticleGui();

    void generateMatProperties();
    void generateInteractionItems();
    void updateMatPropertiesGui();

    bool isNameAlreadyExists() const;
    void updateLoadEnabled();
    int  getMatchValue(const QString & s1, const QString & s2) const;
    bool isSuppressedParticle(const QString & ParticleName) const;
    const QVector<QString> getForcedByNeutron() const;
};

#endif // AMATERIALLOADERDIALOG_H
