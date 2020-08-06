#include "agridelementdelegate.h"
#include "ageoobject.h"
#include "atypegeoobject.h"

#include <QDebug>
#include <QWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFont>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDoubleValidator>
#include <QMessageBox>

AGridElementDelegate::AGridElementDelegate(QWidget * ParentWidget)
    : AGeoBaseDelegate(ParentWidget)
{
    QFrame * frMainFrame = new QFrame();
    frMainFrame->setFrameShape(QFrame::Box);

    Widget = frMainFrame;

    QPalette palette = frMainFrame->palette();
    palette.setColor( frMainFrame->backgroundRole(), QColor( 255, 255, 255 ) );
    frMainFrame->setPalette( palette );
    frMainFrame->setAutoFillBackground( true );
    //frMainFrame->setMinimumHeight(150);
    //frMainFrame->setMaximumHeight(150);

    QVBoxLayout* vl = new QVBoxLayout();

    //object type
    QLabel * labType = new QLabel("Elementary block of the grid");
    labType->setAlignment(Qt::AlignCenter);
    QFont font = labType->font();
    font.setBold(true);
    labType->setFont(font);
    vl->addWidget(labType);

    QGridLayout *lay = new QGridLayout();
    lay->setContentsMargins(20, 5, 20, 5);
    lay->setVerticalSpacing(3);

      QLabel *la = new QLabel("Shape:");
      lay->addWidget(la, 0, 0);
      lSize1 = new QLabel("dX, mm:");
      lay->addWidget(lSize1, 1, 0);
      la = new QLabel("    dZ, mm:");
      la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      lay->addWidget(la, 0, 2);
      lSize2 = new QLabel("    dY, mm:");
      lSize2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      lay->addWidget(lSize2, 1, 2);

      cobShape = new QComboBox();
      cobShape->addItem("Rectangular");
      cobShape->addItem("Hexagonal");
      connect(cobShape, SIGNAL(activated(int)), this, SLOT(onContentChanged()));
      lay->addWidget(cobShape, 0, 1);

      ledDZ = new QLineEdit();
      connect(ledDZ, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      lay->addWidget(ledDZ, 0, 3);
      ledDX = new QLineEdit();
      connect(ledDX, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      lay->addWidget(ledDX, 1, 1);
      ledDY = new QLineEdit();
      connect(ledDY, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      lay->addWidget(ledDY, 1, 3);

    vl->addLayout(lay);
    QPushButton* pbInstructions = new QPushButton();
    connect(pbInstructions, SIGNAL(clicked(bool)), this, SLOT(onInstructionsForGridRequested()));
    pbInstructions->setText("Read me");
    pbInstructions->setMinimumWidth(200);
    pbInstructions->setMaximumWidth(200);
    vl->addWidget(pbInstructions);
    vl->setAlignment(pbInstructions, Qt::AlignHCenter);

    QPushButton* pbAuto = new QPushButton();
    connect(pbAuto, SIGNAL(clicked(bool)), this, SLOT(StartDialog()));
    pbAuto->setText("Open generation/edit dialog");
    pbAuto->setMinimumWidth(200);
    pbAuto->setMaximumWidth(200);
    vl->addWidget(pbAuto);
    vl->setAlignment(pbAuto, Qt::AlignHCenter);

    frMainFrame->setLayout(vl);

    //installing double validators for edit boxes
    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    ledDX->setValidator(dv);
    ledDY->setValidator(dv);
    ledDZ->setValidator(dv);
}

const QString AGridElementDelegate::getName() const
{
    if (!CurrentObject) return "Undefined";
    return CurrentObject->Name;
}

bool AGridElementDelegate::isValid(AGeoObject * /*obj*/)
{
    return true;
}

bool AGridElementDelegate::updateObject(AGeoObject * obj) const
{
    if (!obj) return false;
    if (!obj->ObjectType->isGridElement()) return false;

    ATypeGridElementObject* GE = static_cast<ATypeGridElementObject*>(obj->ObjectType);
    int shape = cobShape->currentIndex();
    if (shape == 0)
    {
        if (GE->shape == 2) GE->shape = 1;
    }
    else
    {
        if (GE->shape != 2) GE->shape = 2;
    }
    GE->dz =    ledDZ->text().toDouble();
    GE->size1 = ledDX->text().toDouble();
    GE->size2 = ledDY->text().toDouble();

    obj->updateGridElementShape();

    return true;
}

void AGridElementDelegate::updateVisibility()
{
    if (cobShape->currentIndex() == 0)
    {  //rectangular
       lSize1->setText("dX, mm:");
       lSize2->setVisible(true);
       ledDY->setVisible(true);
    }
    else
    {
        lSize1->setText("dR, mm:");
        lSize2->setVisible(false);
        ledDY->setVisible(false);
    }
}

void AGridElementDelegate::Update(const AGeoObject *obj)
{
    ATypeGridElementObject* GE = dynamic_cast<ATypeGridElementObject*>(obj->ObjectType);
    if (!GE)
    {
        qWarning() << "Attempt to use non-grid element object to update grid element delegate!";
        return;
    }

    CurrentObject = obj;

    if (GE->shape==0 || GE->shape==1) cobShape->setCurrentIndex(0);
    else cobShape->setCurrentIndex(1);
    updateVisibility();

    ledDZ->setText( QString::number(GE->dz));
    ledDX->setText( QString::number(GE->size1));
    ledDY->setText( QString::number(GE->size2));
}

void AGridElementDelegate::onContentChanged()
{
    updateVisibility();
    emit ContentChanged();
}

void AGridElementDelegate::StartDialog()
{
    if (!CurrentObject->Container) return;
    emit RequestReshapeGrid(CurrentObject->Container->Name);
}

void AGridElementDelegate::onInstructionsForGridRequested()
{
    QString s = "There are two requirements which MUST be fullfiled!\n\n"
                "1. Photons are not allowed to enter objects (\"wires\")\n"
                "   placed within the grid element:\n"
                "   the user has to properly define the optical overrides\n"
                "   for the bulk -> wires interface.\n\n"
                "2. Photons are allowed to leave the grid element only\n"
                "   when they exit the grid object:\n"
                "   the user has to properly position the wires inside the\n"
                "   grid element, so photons cannot cross the border \"sideways\"\n"
                "   \n"
                "Grid generation dialog generates a correct grid element automatically\n"
                "   \n "
                "For modification of all properties of the grid except\n"
                "  - the position/orientation/XYsize of the grid bulk\n"
                "  - materials of the bulk and the wires\n"
                "it is _strongly_ recommended to use the auto-generation dialog.\n"
                "\n"
                "Press the \"Open generation/edit dialog\" button";


    QMessageBox::information(Widget, "", s);
}
