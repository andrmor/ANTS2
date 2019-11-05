#ifndef ATLEGENDENTRY_H
#define ATLEGENDENTRY_H

#include "TLegendEntry.h"

class ATLegendEntry : public TLegendEntry
{
public:
    virtual void SetObject(TObject* obj ) override;
};

#endif // ATLEGENDENTRY_H
