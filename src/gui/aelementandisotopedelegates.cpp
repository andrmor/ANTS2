#include "aelementandisotopedelegates.h"
#include "amaterialcomposition.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

AChemicalElementDelegate::AChemicalElementDelegate(AChemicalElement *element, bool *bClearInProgress)
{
    pbAddIsotope = new QPushButton("Add isotope");
    //pbAddIsotope->setMaximumWidth(75);

    QHBoxLayout* lay = new QHBoxLayout();
    {
        lay->setContentsMargins(0,3,0,0);
        lay->setSpacing(2);
        lay->addWidget( new QLabel(element->Symbol) );
        lay->addWidget( new QLabel("  user proportion: " + QString::number(element->RelativeFraction, 'g', 4) ));
        //lay->addItem( new QSpacerItem(100, 0, QSizePolicy::Expanding, QSizePolicy::Expanding ) );
        lay->addWidget(pbAddIsotope);
    }
    setLayout(lay);

    QObject::connect(pbAddIsotope, &QPushButton::clicked, this, &AChemicalElementDelegate::onAddIsotopeClicked);
}

void AChemicalElementDelegate::onShowIsotopes(bool flag)
{
    pbAddIsotope->setVisible(flag);
}

void AChemicalElementDelegate::onAddIsotopeClicked()
{
    emit AddIsotopeClicked(element);
}
