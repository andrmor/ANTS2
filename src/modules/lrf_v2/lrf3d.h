#ifndef LRF3D_H
#define LRF3D_H

#include "lrf2.h"

class LRF3d : public LRF2
{
public:
    LRF3d() : LRF2() {}
    virtual double getZmin() const = 0;
    virtual double getZmax() const = 0;
};

#endif // LRF3D_H
