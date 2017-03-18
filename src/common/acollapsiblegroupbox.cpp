#include "acollapsiblegroupbox.h"

#include <QCheckBox>
#include <QFrame>
#include <QVBoxLayout>
#include <QDebug>
#include <QTimer>

ACollapsibleGroupBox::ACollapsibleGroupBox(QString title)
{
    QVBoxLayout* lay = new QVBoxLayout();
    {
        ClientArea = new QWidget();
        ClientArea->setVisible(false);

        line1 = new QFrame(this);
        line1->setFrameShadow(QFrame::Sunken);
        line1->setFrameShape(QFrame::HLine);
        line1->setVisible(false);
        lay->addWidget(line1);

        cb = new QCheckBox(title, this);
        lay->addWidget(cb);

        lay->addWidget(ClientArea);

        line2 = new QFrame(this);
        line2->setFrameShadow(QFrame::Sunken);
        line2->setFrameShape(QFrame::HLine);
        line2->setVisible(false);
        lay->addWidget(line2);

        connect(cb, SIGNAL(toggled(bool)), this, SLOT(onToggle(bool)));
        connect(cb, SIGNAL(clicked(bool)), this, SLOT(onClicked()));
    }

    lay->setContentsMargins(0,0,0,0);
    lay->setSpacing(0);
    setLayout(lay);
}

void ACollapsibleGroupBox::setChecked(bool flag)
{
    cb->setChecked(flag);
}

bool ACollapsibleGroupBox::isChecked()
{
    return cb->isChecked();
}

void ACollapsibleGroupBox::updateLineVisibility()
{
    line1->setVisible(isChecked());
    line2->setVisible(isChecked());
    updateGeometry();
}

void ACollapsibleGroupBox::onToggle(bool flag)
{
      //qDebug() << "toggled";   
    ClientArea->setVisible(flag);
    updateLineVisibility();
    emit toggled(flag);
}

void ACollapsibleGroupBox::onClicked()
{
      //qDebug() << "clicked";
    emit clicked();
}
