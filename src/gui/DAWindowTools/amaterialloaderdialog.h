#ifndef AMATERIALLOADERDIALOG_H
#define AMATERIALLOADERDIALOG_H

#include <QDialog>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QJsonObject>

class AMaterialParticleCollection;
class QCheckBox;
class QWidget;

namespace Ui {
class AMaterialLoaderDialog;
}

class AParticleRecordForMerge;
class APropertyRecordForMerge;

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

    AParticleRecordForMerge * neutron = nullptr;
    QVector<QString> ParticlesForcedByNeutron;

    QVector<AParticleRecordForMerge*> ParticleRecords;
    QVector<APropertyRecordForMerge*> PropertyRecords;
    QVector<APropertyRecordForMerge*> MatParticleRecords;

private:
    void generateParticleRecords();
    void generateMatPropRecords();
    void generateInteractionRecords();

    bool isNameAlreadyExists(const QString & newName) const;
    void updateLoadEnabled();
    int  getMatchValue(const QString & s1, const QString & s2) const;
    bool isSuppressedParticle(const QString & ParticleName) const;
    const QVector<QString> getForcedByNeutron() const;
    AParticleRecordForMerge * findParticleRecord(const QString & particleName);
    QWidget * createComparisonWidget(const QString & key, const QJsonValue & valueFrom, const QJsonValue & valueTo);
    QWidget * createComparisonWidgetMatParticle(const QJsonObject & jsonFrom, const QJsonObject & jsonTo);
    const QString convertJsonNameToReadable(const QString & key) const;

    void clearParticleRecords();
    void clearPropertyRecords();

    QString addAsNew(const QString &newName);
    void mergeWithExistentMaterial();
};

class AParticleRecordForMerge
{
public:
    AParticleRecordForMerge(const QString & name) : ParticleName(name){}
    AParticleRecordForMerge(){}

    QString ParticleName;

    void connectCheckBox(QCheckBox * cb);
    void connectPropertyRecord(APropertyRecordForMerge * rec) {linkedPropertyRecord = rec;}

    void setForcedParticles(QVector<AParticleRecordForMerge*> & vec);

    void setChecked(bool flag, bool bInduced = false);
    void setForced(bool flag);

    bool isChecked() const {return bChecked;}
    bool isForced() const  {return bForcedByNeutron;}

private:
    bool bChecked         = true;
    bool bForcedByNeutron = false;

    QVector<AParticleRecordForMerge*> ForcedParticles;  // can be non-empty only for neutron

    QCheckBox * CheckBox  = nullptr;

    APropertyRecordForMerge * linkedPropertyRecord = nullptr;  //defined only if the target material does not have MatParticle property for this particle

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

    void setChecked(bool flag, bool bInduced = false);
    bool isChecked() const {return bChecked;}

    void setDisabled(bool flag);
    bool isDisabled() const {return bDisabled;}

    void connectGuiResources(QCheckBox * cb);
    void connectParticleRecord(AParticleRecordForMerge * rec) {linkedParticleRecord = rec;}

private:
    bool bChecked        = true;
    bool bDisabled       = false;

    QCheckBox * CheckBox = nullptr;

    AParticleRecordForMerge * linkedParticleRecord = nullptr;  // used only by MatParticle properties.
                                                               // nullptr indicates that this particle is defined in the target material

private:
    void updateIndication();
};

#endif // AMATERIALLOADERDIALOG_H
