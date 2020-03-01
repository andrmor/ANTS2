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

class AMaterialLoaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AMaterialLoaderDialog(const QString & fileName, AMaterialParticleCollection & MpCollection, QWidget * parentWidget = nullptr);
    ~AMaterialLoaderDialog();

    const QVector<QString> getSuppressParticles() const {return SuppressParticles;}
    const QJsonObject      getMaterialJson() const      {return MaterialJson;}

private slots:
    void onCheckBoxClicked();
    void on_pbDummt_clicked();
    void on_pbLoad_clicked();
    void on_pbCancel_clicked();
    void on_leName_textChanged(const QString &arg1);
    void on_twMain_currentChanged(int index);
    void on_cbToggleAll_toggled(bool checked);

private:
    AMaterialParticleCollection & MpCollection;
    Ui::AMaterialLoaderDialog   * ui;

    bool bFileOK = true;
    QJsonObject MaterialJson;
    QString NameInFile;
    QVector<QString> NewParticles;
    QStringList DefinedMaterials;

    QVector<QString> SuppressParticles;

    QVector<QCheckBox*> cbVec;

private:
    void updateParticleGui();
    bool isNameAlreadyExists() const;
    void updateLoadEnabled();

};

#endif // AMATERIALLOADERDIALOG_H
