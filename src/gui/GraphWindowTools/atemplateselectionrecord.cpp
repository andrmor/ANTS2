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
