#include "adrawobject.h"

#include "TNamed.h"

ADrawObject::ADrawObject(TObject *pointer, const char *options)
{
    Pointer = pointer;
    Options = options;
    extractName();
}

ADrawObject::ADrawObject(TObject *pointer, const QString &options)
{
    Pointer = pointer;
    Options = options;
    extractName();
}

void ADrawObject::extractName()
{
    if (!Pointer) return;

    TNamed * tn = dynamic_cast<TNamed*>(Pointer);
    if (tn) Name = tn->GetTitle();
}
