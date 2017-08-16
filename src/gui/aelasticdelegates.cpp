#include "aelasticdelegates.h"
#include "amaterial.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QDebug>

AElasticElementDelegate::AElasticElementDelegate(AElasticScatterElement *element, bool *bClearInProgress) :
    element(element), bClearInProgress(bClearInProgress)
{
    leName = new QLineEdit(element->Name);
    ledFraction = new QLineEdit(QString::number(element->Fraction, 'g', 4));
    pbAuto = new QPushButton("Auto");
    pbDel = new QPushButton("X");
    pbDel->setMaximumWidth(25);

    QHBoxLayout* lay = new QHBoxLayout();
    {
        lay->setContentsMargins(0,3,0,0);
        lay->setSpacing(2);
        lay->addWidget( new QLabel("Element:") );
        lay->addWidget(leName);
        lay->addWidget( new QLabel("  Molar proportion:") );
        lay->addWidget(ledFraction);
        lay->addItem( new QSpacerItem(100, 0, QSizePolicy::Expanding, QSizePolicy::Expanding ) );
        lay->addWidget(pbAuto);
        lay->addWidget(pbDel);
    }
    setLayout(lay);

    QDoubleValidator* valD = new QDoubleValidator(this);
    valD->setBottom(0);
    ledFraction->setValidator(valD);

    QObject::connect(pbAuto, &QPushButton::clicked, this, &AElasticElementDelegate::onAutoClicked);
    QObject::connect(pbDel,  &QPushButton::clicked, this, &AElasticElementDelegate::onDelClicked);

    QObject::connect(leName,      &QLineEdit::editingFinished, this, &AElasticElementDelegate::onChanged);
    QObject::connect(ledFraction, &QLineEdit::editingFinished, this, &AElasticElementDelegate::onChanged);
}

void AElasticElementDelegate::onAutoClicked()
{
    if (*bClearInProgress) return;
    emit AutoClicked(element);
}

void AElasticElementDelegate::onDelClicked()
{
    if (*bClearInProgress) return;
    emit DelClicked(element);
}

void AElasticElementDelegate::onChanged()
{
    if (*bClearInProgress) return;
    QString newName = leName->text();
    double newFraction = ledFraction->text().toDouble();

    if (newName == element->Name && newFraction == element->Fraction ) return; //nothing changed

    emit RequestUpdateIsotopes(element, newName, newFraction);
}

AElasticIsotopeDelegate::AElasticIsotopeDelegate(AElasticScatterElement *element, bool* bClearInProgress) :
    element(element), bClearInProgress(bClearInProgress)
{
    leiMass = new QLineEdit( QString::number(element->Mass) );
    ledAbund = new QLineEdit( QString::number(element->Abundancy, 'g', 4) );
    pbShow = new QPushButton("Show");
    pbLoad = new QPushButton("Load");
    QPushButton* pbDel = new QPushButton("X");
    pbDel->setMaximumWidth(25);

    updateButtons();

    QHBoxLayout* lay = new QHBoxLayout();
    {
        lay->setContentsMargins(0,0,0,0);
        lay->setSpacing(2);
        lay->addWidget(new QLabel(element->Name));
        lay->addWidget(new QLabel("-"));
        lay->addWidget(leiMass);
        lay->addWidget(new QLabel("   "));
        lay->addWidget(ledAbund);
        lay->addWidget(new QLabel("%") );
        lay->addItem( new QSpacerItem(1000, 0, QSizePolicy::Expanding, QSizePolicy::Expanding ) );
        lay->addWidget(pbShow);
        lay->addWidget(pbLoad);
        lay->addWidget(pbDel);
    }
    setLayout(lay);

    QDoubleValidator* valD = new QDoubleValidator(this);
    valD->setBottom(0);
    ledAbund->setValidator(valD);
    QIntValidator* valI = new QIntValidator(this);
    valI->setBottom(1);
    leiMass->setValidator(valI);

    QObject::connect(leiMass,  &QLineEdit::editingFinished, this, &AElasticIsotopeDelegate::onChanged);
    QObject::connect(ledAbund, &QLineEdit::editingFinished, this, &AElasticIsotopeDelegate::onChanged);

    QObject::connect(pbShow, &QPushButton::clicked, this, &AElasticIsotopeDelegate::onShowClicked);
    QObject::connect(pbLoad, &QPushButton::clicked, this, &AElasticIsotopeDelegate::onLoadClicked);
    QObject::connect(pbDel,  &QPushButton::clicked, this, &AElasticIsotopeDelegate::onDelClicked);
}

void AElasticIsotopeDelegate::updateButtons()
{
    QString toRed = "QPushButton {color: red;}";
    QString s = pbLoad->styleSheet();

    if (element->CrossSection.size()<2)
    {
        pbShow->setEnabled(false);
        if (!s.contains(toRed)) s += toRed;
    }
    else
    {
        pbShow->setEnabled(true);
        if (s.contains(toRed)) s.remove(toRed);
    }

    pbLoad->setStyleSheet(s);
}

void AElasticIsotopeDelegate::onShowClicked()
{
    if (*bClearInProgress) return;
    emit ShowClicked(element);
}

void AElasticIsotopeDelegate::onLoadClicked()
{
    if (*bClearInProgress) return;
    emit LoadClicked(element);
    updateButtons();
}

void AElasticIsotopeDelegate::onDelClicked()
{
    if (*bClearInProgress) return;
    emit DelClicked(element);
}

void AElasticIsotopeDelegate::onChanged()
{
    if (*bClearInProgress) return;
    int newMass = leiMass->text().toInt();
    double newAb = ledAbund->text().toDouble();

    if (newMass == element->Mass && newAb == element->Abundancy ) return; //nothing changed

    if (element->Mass != newMass)
    {
      element->Mass = newMass;
      element->CrossSection.clear();
      element->Energy.clear();
      updateButtons();
    }
    element->Abundancy = newAb;

    emit RequestActivateModifiedStatus();
}


