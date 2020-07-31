#include "ageoconsts.h"

AGeoConsts &AGeoConsts::getInstance()
{
    static AGeoConsts instance;
    return instance;
}

const AGeoConsts &AGeoConsts::getConstInstance()
{
    return getInstance();
}

AGeoConsts::AGeoConsts()
{

    geoConsts.insert("x", 50);
}
