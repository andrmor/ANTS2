#ifndef ANEUTRONREACTIONWIDGET_H
#define ANEUTRONREACTIONWIDGET_H

#include <QObject>
#include <QFrame>
#include <QStringList>

namespace Ui {
class ANeutronReactionWidget;
}

class ADecayScenario;
class AAbsorptionGeneratedParticle;
class QComboBox;
class QLineEdit;
class QCheckBox;

class ANeutronReactionWidget : public QFrame
{
    Q_OBJECT

public:
    explicit ANeutronReactionWidget(ADecayScenario* Reaction, QStringList DefinedParticles, QWidget *parent = 0);
    ~ANeutronReactionWidget();

private slots:
    void on_ledBranching_editingFinished();
    void on_pbAdd_clicked();
    void on_pbRemove_clicked();

    void on_ledBranching_textChanged(const QString &arg1);

private:
    Ui::ANeutronReactionWidget *ui;
    ADecayScenario* Reaction;
    QStringList DefinedParticles;

    void updateParticleList();

signals:
    void RequestParentResize();
};

class AGeneratedParticleDelegate : public QFrame
{
    Q_OBJECT
public:
    AGeneratedParticleDelegate(AAbsorptionGeneratedParticle* ParticleRecord, QStringList DefinedParticles, bool bEnableDir);

private:
    AAbsorptionGeneratedParticle* ParticleRecord;

    QComboBox* combP;
    QLineEdit* leE;
    QCheckBox* cbOp;

private slots:
    void onPropertiesChanged();

};

#endif // ANEUTRONREACTIONWIDGET_H
