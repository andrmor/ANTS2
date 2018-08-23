#include "aelementandisotopedelegates.h"
#include "amaterialcomposition.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QDebug>
#include <QMenu>
#include <QDoubleValidator>
#include <QIntValidator>

AChemicalElementDelegate::AChemicalElementDelegate(AChemicalElement *element, bool *bClearInProgress, bool IsotopesShown) :
    element(element), bClearInProgress(bClearInProgress), bIsotopesShown(IsotopesShown)
{
    QHBoxLayout* lay = new QHBoxLayout();
    {
        lay->setContentsMargins(0,3,0,0);
        lay->setSpacing(2);
        QLabel* l = new QLabel("  " + element->Symbol);
        l->setMinimumWidth(50);
        lay->addWidget(l);
        l =  new QLabel("  Molar fraction: " + QString::number(element->MolarFraction, 'g', 4) );
        l->setMinimumWidth(200);
        lay->addWidget(l);
        //lay->addItem( new QSpacerItem(100, 0, QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    }
    setLayout(lay);

    setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, &AChemicalElementDelegate::customContextMenuRequested, this, &AChemicalElementDelegate::onMenuRequested);
}

void AChemicalElementDelegate::onMenuRequested(const QPoint &pos)
{
    QMenu menu;

    QAction* addIsotope = menu.addAction("Add new isotope");
    addIsotope->setEnabled(bIsotopesShown);
    //menu.addSeparator();

    QAction* selectedItem = menu.exec(this->mapToGlobal(pos));
    if (!selectedItem) return; //nothing was selected

    if (selectedItem == addIsotope)
      {
        emit AddIsotopeActivated(element);
        return;
      }
}

// ======================= Isotope ===========================

AIsotopeDelegate::AIsotopeDelegate(AChemicalElement *element, int isotopeIndexInElement, bool *bClearInProgress) :
    element(element), isotopeIndexInElement(isotopeIndexInElement), bClearInProgress(bClearInProgress)
{
    QHBoxLayout* lay = new QHBoxLayout();
    {
        lay->setContentsMargins(0,0,0,0);
        lay->setSpacing(2);
        lay->addWidget(new QLabel("    " + element->Symbol + "-"));
        leiMass = new QLineEdit( QString::number(element->Isotopes.at(isotopeIndexInElement).Mass) );
        leiMass->setMinimumWidth(50);
        lay->addWidget(leiMass);
        lay->addWidget(new QLabel("   "));
        ledAbund = new QLineEdit( QString::number(element->Isotopes.at(isotopeIndexInElement).Abundancy, 'g', 4) );
        ledAbund->setMinimumWidth(50);
        lay->addWidget(ledAbund);
        lay->addWidget(new QLabel("%") );
        lay->addItem( new QSpacerItem(100, 0, QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    }
    setLayout(lay);

    QDoubleValidator* valD = new QDoubleValidator(this);
    valD->setBottom(0);
    valD->setTop(100.0);
    ledAbund->setValidator(valD);
    QIntValidator* valI = new QIntValidator(this);
    valI->setBottom(1);
    leiMass->setValidator(valI);

    QObject::connect(leiMass,  &QLineEdit::editingFinished, this, &AIsotopeDelegate::onChanged);
    QObject::connect(ledAbund, &QLineEdit::editingFinished, this, &AIsotopeDelegate::onChanged);

    setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, &AIsotopeDelegate::customContextMenuRequested, this, &AIsotopeDelegate::onMenuRequested);
}

void AIsotopeDelegate::onChanged()
{
    if (*bClearInProgress) return;
    int newMass = leiMass->text().toInt();
    double newAb = ledAbund->text().toDouble();

    if (newMass == element->Isotopes.at(isotopeIndexInElement).Mass && newAb == element->Isotopes.at(isotopeIndexInElement).Abundancy ) return; //nothing changed

    element->Isotopes[isotopeIndexInElement].Mass = newMass;
    element->Isotopes[isotopeIndexInElement].Abundancy = newAb;

    emit IsotopePropertiesChanged(element, isotopeIndexInElement);
}

void AIsotopeDelegate::onMenuRequested(const QPoint &pos)
{
    QMenu menu;
    QAction* removeIsotope = menu.addAction("Remove isotope");

    QAction* selectedItem = menu.exec(this->mapToGlobal(pos));
    if (!selectedItem) return; //nothing was selected

    if (selectedItem == removeIsotope)
      {
        emit RemoveIsotope(element, isotopeIndexInElement);
        return;
      }
}
