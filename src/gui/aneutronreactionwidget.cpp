#include "aneutronreactionwidget.h"
#include "ui_aneutronreactionwidget.h"
#include "aneutroninteractionelement.h"
#include "aglobalsettings.h"
#include "afiletools.h"

#include <QFileDialog>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDebug>

ANeutronReactionWidget::ANeutronReactionWidget(ADecayScenario* Reaction, QStringList DefinedParticles, QWidget *parent) :
    QFrame(parent), ui(new Ui::ANeutronReactionWidget), Reaction(Reaction), DefinedParticles(DefinedParticles)
{
    ui->setupUi(this);
    //setFrameShape(QFrame::Box);
    //setContentsMargins(0,0,0,0);

    QDoubleValidator* val = new QDoubleValidator();
    ui->ledBranching->setValidator(val);
    ui->ledConstDepo->setValidator(val);
    ui->ledAvDepo->setValidator(val);
    ui->ledSigma->setValidator(val);

    ui->ledBranching->setText( QString::number( 100.0 * Reaction->Branching) );

    int MainMode = static_cast<int>(Reaction->InteractionModel);
    if (MainMode >= 0 && MainMode < 3) ui->cobModel->setCurrentIndex(MainMode);
    else qWarning() << "Bad main mode!";

    int GenMode = static_cast<int>(Reaction->DirectModel);
    if (GenMode >= 0 && GenMode < 3) ui->cobGenModel->setCurrentIndex(GenMode);

    ui->ledConstDepo->setText( QString::number(Reaction->DirectConst) );
    ui->ledAvDepo->setText( QString::number(Reaction->DirectAverage) );
    ui->ledSigma->setText( QString::number(Reaction->DirectSigma) );

    ParLay = new QVBoxLayout(ui->frParticles);

    updateGUI();
}

ANeutronReactionWidget::~ANeutronReactionWidget()
{
    clearDelegates();
    delete ui;
}

void ANeutronReactionWidget::on_ledBranching_editingFinished()
{
    Reaction->Branching = 0.01 * ui->ledBranching->text().toDouble();
}

void ANeutronReactionWidget::on_ledBranching_textChanged(const QString &arg1)
{
    bool bOK;
    double newVal = arg1.toDouble(&bOK);
    if (bOK) Reaction->Branching = 0.01 * newVal;
}

void ANeutronReactionWidget::on_pbAdd_clicked()
{
    Reaction->GeneratedParticles << AAbsorptionGeneratedParticle();
    updateGUI();
}

void ANeutronReactionWidget::on_pbRemove_clicked()
{
    if (Reaction->GeneratedParticles.isEmpty()) return;
    if (Reaction->GeneratedParticles.size() == 1)
        Reaction->GeneratedParticles.clear();
    else
        Reaction->GeneratedParticles.removeLast();

    updateGUI();
}

AGeneratedParticleDelegate::AGeneratedParticleDelegate(AAbsorptionGeneratedParticle *ParticleRecord, QStringList DefinedParticles, bool bEnableDir) :
    ParticleRecord(ParticleRecord)
{
    //setFrameShape(QFrame::Box);
    QHBoxLayout* l = new QHBoxLayout(this);
    l->setContentsMargins(6,2,6,2);
        combP = new QComboBox();
        combP->setMinimumHeight(20);
        combP->addItems(DefinedParticles);
        combP->setCurrentIndex(ParticleRecord->ParticleId);
        l->addWidget(combP);
        leE = new QLineEdit(QString::number(ParticleRecord->Energy));
        leE->setMaximumWidth(55);
        QDoubleValidator* val = new QDoubleValidator();
        val->setBottom(0);
        leE->setValidator(val);
        l->addWidget(leE);
        QLabel* labE = new QLabel("keV");
        l->addWidget(labE);
        cbOp = new QCheckBox("Opposite direction");
        cbOp->setChecked(ParticleRecord->bOpositeDirectionWithPrevious);
        cbOp->setEnabled(bEnableDir);
        cbOp->setToolTip("If checked, this particle is generated with the direction opposite to the one of the particle above");
        l->addWidget(cbOp);

    QObject::connect(combP, SIGNAL(activated(int)), this, SLOT(onPropertiesChanged()));
    QObject::connect(leE,   &QLineEdit::editingFinished,     this, &AGeneratedParticleDelegate::onPropertiesChanged);
    QObject::connect(cbOp,  &QCheckBox::clicked,             this, &AGeneratedParticleDelegate::onPropertiesChanged);
}

void AGeneratedParticleDelegate::onPropertiesChanged()
{
     ParticleRecord->ParticleId = combP->currentIndex();
     ParticleRecord->Energy = leE->text().toDouble();
     ParticleRecord->bOpositeDirectionWithPrevious = cbOp->isChecked();
}

void ANeutronReactionWidget::updateGUI()
{
    int MainModel = ui->cobModel->currentIndex(); //0-Abs, 1-Particles, 2-direct

    ui->frDirect->setVisible(MainModel == 2);

    ui->pbAdd->setVisible(MainModel == 1);
    ui->pbRemove->setVisible(MainModel == 1);
    ui->frParticles->setVisible(MainModel == 1);

    if (MainModel == 2)
    {
        int GenModel = ui->cobGenModel->currentIndex(); //0-const, 1-Gauss, 2-custom

        ui->swGenModel->setCurrentIndex(GenModel);
        ui->frAverage->setVisible(GenModel != 2);
        ui->pbShowCustom->setEnabled(!Reaction->DirectCustomEn.isEmpty());
    }

    clearDelegates();
    if (MainModel == 1)
    {
        for (int i=0; i<Reaction->GeneratedParticles.size(); i++)
        {
            AGeneratedParticleDelegate* del = new AGeneratedParticleDelegate(&Reaction->GeneratedParticles[i], DefinedParticles, (i!=0));
            del->setFocusPolicy(Qt::StrongFocus);
            ParLay->addWidget(del);
        }
    }

    emit RequestParentResize();
}

void ANeutronReactionWidget::clearDelegates()
{
    for (QWidget * d : Delegates)
    {
        ParLay->removeWidget(d);
        delete d;
    }
    Delegates.clear();
}

void ANeutronReactionWidget::on_cobModel_activated(int index)
{
    Reaction->InteractionModel = static_cast<ADecayScenario::eInteractionModels>(index);
    updateGUI();
}

void ANeutronReactionWidget::on_cobGenModel_activated(int index)
{
    Reaction->DirectModel = static_cast<ADecayScenario::eDirectModels>(index);
    updateGUI();
}

void ANeutronReactionWidget::on_ledConstDepo_editingFinished()
{
    Reaction->DirectConst = ui->ledConstDepo->text().toDouble();
}

void ANeutronReactionWidget::on_ledAvDepo_editingFinished()
{
    Reaction->DirectAverage = ui->ledAvDepo->text().toDouble();
}

void ANeutronReactionWidget::on_ledSigma_editingFinished()
{
    Reaction->DirectSigma = ui->ledSigma->text().toDouble();
}

void ANeutronReactionWidget::on_pbLoadCustom_clicked()
{
    AGlobalSettings & gs = AGlobalSettings::getInstance();
    QString fileName = QFileDialog::getOpenFileName(this, "Load custom distribution of energy deposition per neutron."
                                                          "\nFormat: every line should contain Energy_keV Probability",
                                                    gs.LastOpenDir, "Data files (*.dat *.txt);;All files(*)");
    if (fileName.isEmpty()) return;
    gs.LastOpenDir = QFileInfo(fileName).absolutePath();

    LoadDoubleVectorsFromFile(fileName, &Reaction->DirectCustomEn, &Reaction->DirectCustomProb);

    updateGUI();
}

void ANeutronReactionWidget::on_pbShowCustom_clicked()
{
    emit RequestDraw(Reaction->DirectCustomEn, Reaction->DirectCustomProb, "Energy, keV", "");
}
