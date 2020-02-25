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

    connect(ui->tw, &QTreeWidget::itemExpanded, this,  &ATemplateSelectionDialog::onItemExpanded);
    connect(ui->tw, &QTreeWidget::itemCollapsed, this, &ATemplateSelectionDialog::onItemColapsed);
}

void ATemplateSelectionDialog::addItem(ATemplateSelectionRecord * rec, ATemplateSelectionRecord * parent)
{
    rec->Item = new QTreeWidgetItem();

    if (parent) parent->Item->addChild(rec->Item);
    else ui->tw->addTopLevelItem(rec->Item);

    QCheckBox * cb = new QCheckBox(rec->Label);
    cb->setTristate(true);
    ui->tw->setItemWidget(rec->Item, 0, cb);
    connect(cb, &QCheckBox::clicked, [rec, this]()
    {
        bool bChecked = !rec->bSelected;
        setCheckedStatusRecursive(rec, bChecked);
        if (bChecked) setCheckedForParentsRecursive(rec);
        updateCheckStateIndication(&Selection);
    });

    rec->Item->setExpanded(rec->bExpanded);

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
    return dynamic_cast<QCheckBox*>(ui->tw->itemWidget(rec->Item, 0));
}

void ATemplateSelectionDialog::onItemExpanded(QTreeWidgetItem * item)
{
    ATemplateSelectionRecord * rec = Selection.findRecordByItem(item);
    if (rec) rec->bExpanded = true;
}

void ATemplateSelectionDialog::onItemColapsed(QTreeWidgetItem * item)
{
    ATemplateSelectionRecord * rec = Selection.findRecordByItem(item);
    if (rec) rec->bExpanded = false;
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
