#ifndef AMATERIALLOADERDIALOG_H
#define AMATERIALLOADERDIALOG_H

#include <QDialog>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QJsonObject>

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

class AMaterialLoaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AMaterialLoaderDialog(const QString & fileName, AMaterialParticleCollection & MpCollection, QWidget * parentWidget = nullptr);
    ~AMaterialLoaderDialog();

    const QVector<QString> getSuppressedParticles() const;
    const QJsonObject      getMaterialJson() const {return MaterialJson;}

private slots:
    void on_pbDummt_clicked();
    void on_pbLoad_clicked();
    void on_pbCancel_clicked();
    void on_leName_textChanged(const QString &arg1);
    void on_twMain_currentChanged(int index);
    void on_cbToggleAllParticles_clicked(bool checked);
    void on_cbToggleAllProps_toggled(bool checked);
    void on_cobMaterial_activated(int index);

private:
    AMaterialParticleCollection & MpCollection;
    Ui::AMaterialLoaderDialog   * ui;

    QStringList DefinedMaterials;
    bool        bFileOK = true;

    QJsonObject MaterialJson;
    QString     NameInFile;

    QVector<AParticleRecordForMerge> NewParticles;

private:
    void generateParticleGui();
    void updateParticleGui();
    bool isNameAlreadyExists() const;
    void updateLoadEnabled();
    void updateMaterialPropertiesGui();
    int  getMatchValue(const QString & s1, const QString & s2) const;
    int  addInteractionItems(QJsonObject & MaterialTo);
    bool isSuppressedParticle(const QString & ParticleName) const;
    const QVector<QString> getForcedByNeutron() const;
};

#endif // AMATERIALLOADERDIALOG_H
