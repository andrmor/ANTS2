#include "atemplateselectiondialog.h"
#include "ui_atemplateselectiondialog.h"
#include "atemplateselectionrecord.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QDebug>

ATemplateSelectionDialog::ATemplateSelectionDialog(ATemplateSelectionRecord & Selection, QWidget *parent) :
    QDialog(parent),
    Selection(Selection),
    ui(new Ui::ATemplateSelectionDialog)
{
    ui->setupUi(this);
    addItem(&Selection, nullptr);
    updateCheckStateIndication(&Selection);
}

void ATemplateSelectionDialog::addItem(ATemplateSelectionRecord * rec, ATemplateSelectionRecord * parent)
{
    rec->item = new QTreeWidgetItem();

    if (parent) parent->item->addChild(rec->item);
    else ui->tw->addTopLevelItem(rec->item);

    QCheckBox * cb = new QCheckBox(rec->Label);
    cb->setTristate(true);
    ui->tw->setItemWidget(rec->item, 0, cb);
    connect(cb, &QCheckBox::clicked, [rec, this](bool checked)
    {
        bool bChecked = !rec->bSelected;
        setCheckedStatusRecursive(rec, bChecked);
        if (bChecked) setCheckedForParentsRecursive(rec);
        updateCheckStateIndication(&Selection);
    });

    rec->item->setExpanded(rec->bExpanded);

    for (ATemplateSelectionRecord * r : rec->Children)
        addItem(r, rec);
}

void ATemplateSelectionDialog::setCheckedStatusRecursive(ATemplateSelectionRecord *rec, bool flag)
{
    rec->bSelected = flag;
    for (ATemplateSelectionRecord * r : rec->Children)
        setCheckedStatusRecursive(r, flag);
}

void ATemplateSelectionDialog::setCheckedForParentsRecursive(ATemplateSelectionRecord *rec)
{
    ATemplateSelectionRecord * Parent = rec->Parent;
    while (Parent)
    {
        Parent->bSelected = true;
        Parent = Parent->Parent;
    }
}

bool ATemplateSelectionDialog::containsCheckedChild(ATemplateSelectionRecord *rec) const
{
    for (ATemplateSelectionRecord * r : rec->Children)
    {
        if (r->bSelected) return true;
        if (containsCheckedChild(r)) return true;
    }
    return false;
}

bool ATemplateSelectionDialog::containsUncheckedChild(ATemplateSelectionRecord *rec) const
{
    for (ATemplateSelectionRecord * r : rec->Children)
    {
        if (!r->bSelected) return true;
        if (containsUncheckedChild(r)) return true;
    }
    return false;
}

void ATemplateSelectionDialog::updateCheckStateIndication(ATemplateSelectionRecord * rec)
{
    QCheckBox * cb = getCheckBox(rec);
    if (!cb) return;          //paranoic

    Qt::CheckState state;
    if (rec->bSelected)
    {
        if ( containsUncheckedChild(rec) ) state = Qt::PartiallyChecked;
        else                               state = Qt::Checked;
    }
    else
    {
        if ( containsCheckedChild(rec) )   state = Qt::PartiallyChecked;
        else                               state = Qt::Unchecked;
    }
    cb->setCheckState(state);

    for (ATemplateSelectionRecord * r : rec->Children)
        updateCheckStateIndication(r);
}

QCheckBox * ATemplateSelectionDialog::getCheckBox(ATemplateSelectionRecord * rec)
{
    return dynamic_cast<QCheckBox*>(ui->tw->itemWidget(rec->item, 0));
}

ATemplateSelectionDialog::~ATemplateSelectionDialog()
{
    delete ui;
}

void ATemplateSelectionDialog::on_pbAccept_clicked()
{
    accept();
}

void ATemplateSelectionDialog::on_pbCancel_clicked()
{
    reject();
}
