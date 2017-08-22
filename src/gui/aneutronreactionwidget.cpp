#include "aneutronreactionwidget.h"
#include "ui_aneutronreactionwidget.h"

#include "aneutroninteractionelement.h"

#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDebug>

ANeutronReactionWidget::ANeutronReactionWidget(ACaptureReaction* Reaction, QStringList DefinedParticles, QWidget *parent) :
    QFrame(parent), ui(new Ui::ANeutronReactionWidget), Reaction(Reaction), DefinedParticles(DefinedParticles)
{
    ui->setupUi(this);
    setFrameShape(QFrame::Box);

    QDoubleValidator* val = new QDoubleValidator();
    val->setBottom(0);
    val->setTop(1.0);
    ui->ledBranching->setValidator(val);

    ui->ledBranching->setText( QString::number( 100.0 * Reaction->Branching) );
    updateParticleList();
}

ANeutronReactionWidget::~ANeutronReactionWidget()
{
    delete ui;
}

void ANeutronReactionWidget::on_ledBranching_editingFinished()
{
    // not triggered - this is how QDialog works...
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
    updateParticleList();
    emit RequestParentResize();
}

void ANeutronReactionWidget::on_pbRemove_clicked()
{
    if (Reaction->GeneratedParticles.isEmpty()) return;
    if (Reaction->GeneratedParticles.size() == 1)
        Reaction->GeneratedParticles.clear();
    else
        Reaction->GeneratedParticles.removeLast();
    updateParticleList();
    emit RequestParentResize();
}

void ANeutronReactionWidget::updateParticleList()
{
    ui->frNoParticles->setVisible(Reaction->GeneratedParticles.isEmpty());
    ui->pbRemove->setEnabled(!Reaction->GeneratedParticles.isEmpty());

    for (int i=0; i<Reaction->GeneratedParticles.size(); i++)
    {
        AGeneratedParticleDelegate* del = new AGeneratedParticleDelegate(&Reaction->GeneratedParticles[i], DefinedParticles, (i!=0));
        ui->ReactionsLayout->addWidget(del);
        del->setFocusPolicy(Qt::StrongFocus);
    }
}

AGeneratedParticleDelegate::AGeneratedParticleDelegate(AAbsorptionGeneratedParticle *ParticleRecord, QStringList DefinedParticles, bool bEnableDir) :
    ParticleRecord(ParticleRecord)
{
    //setFrameShape(QFrame::Box);
    QHBoxLayout* l = new QHBoxLayout(this);
    l->setContentsMargins(6,2,6,2);
        combP = new QComboBox();
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
        cbOp->setToolTip("This particle is generated with the direction opposite of this of the previously defined pareticle");
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
