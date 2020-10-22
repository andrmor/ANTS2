#include "amonitordelegate.h"
#include "amonitordelegateform.h"

#include <QWidget>
#include <QFrame>
#include <QPalette>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

AMonitorDelegate::AMonitorDelegate(const QStringList &definedParticles, QWidget *ParentWidget) :
    AGeoBaseDelegate(ParentWidget)
{
    QFrame * frMainFrame = new QFrame();
    frMainFrame->setFrameShape(QFrame::Box);

    Widget = frMainFrame;

    QPalette palette = frMainFrame->palette();
    palette.setColor( frMainFrame->backgroundRole(), QColor( 255, 255, 255 ) );
    frMainFrame->setPalette( palette );
    frMainFrame->setAutoFillBackground( true );
    //frMainFrame->setMinimumHeight(380);
    //frMainFrame->setMaximumHeight(380);
    //frMainFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    QVBoxLayout* vl = new QVBoxLayout();
    vl->setContentsMargins(5,5,5,5);

    //object type
    QLabel * labType = new QLabel("Monitor");
    labType->setAlignment(Qt::AlignCenter);
    QFont font = labType->font();
    font.setBold(true);
    labType->setFont(font);
    vl->addWidget(labType);

    del = new AMonitorDelegateForm(definedParticles, Widget);
    del->UpdateVisibility();
    connect(del, &AMonitorDelegateForm::contentChanged, this, &AMonitorDelegate::onContentChanged);
    connect(del, &AMonitorDelegateForm::showSensDirection, this, &AMonitorDelegate::requestShowSensitiveFaces);
    vl->addWidget(del);

    // bottom line buttons
    QHBoxLayout * abl = createBottomButtons();
    vl->addLayout(abl);

    frMainFrame->setLayout(vl);
}

QString AMonitorDelegate::getName() const
{
    return del->getName();
}

bool AMonitorDelegate::updateObject(AGeoObject *obj) const
{
    return del->updateObject(obj);
}

void AMonitorDelegate::Update(const AGeoObject *obj)
{
    bool bOK = del->updateGUI(obj);
    if (!bOK) return;
}

void AMonitorDelegate::onContentChanged()
{
    emit ContentChanged();
    Widget->layout()->activate();
}
