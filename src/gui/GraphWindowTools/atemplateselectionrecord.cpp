#include "atemplateselectionrecord.h"

ATemplateSelectionRecord::ATemplateSelectionRecord(const QString & Label, ATemplateSelectionRecord * Parent) :
    Label(Label), Parent(Parent)
{
    Parent->addChild(this);
}

ATemplateSelectionRecord::~ATemplateSelectionRecord()
{
    for (ATemplateSelectionRecord * rec : Children) delete rec;
    Children.clear();
}

void ATemplateSelectionRecord::addChild(ATemplateSelectionRecord * rec)
{
    Children << rec;
}

ATemplateSelectionRecord *ATemplateSelectionRecord::findChild(const QString & Label) const
{
    for (ATemplateSelectionRecord * rec : Children)
        if (rec->Label == Label) return rec;
    return nullptr;
}

ATemplateSelectionRecord * ATemplateSelectionRecord::findRecordByItem(const QTreeWidgetItem *item)
{
    if (Item == item) return this;

    for (ATemplateSelectionRecord * rec : Children)
    {
        ATemplateSelectionRecord * found = rec->findRecordByItem(item);
        if (found) return found;
    }
    return nullptr;
}
