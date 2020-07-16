#ifndef AFARM_SI_H
#define AFARM_SI_H

#include "ascriptinterface.h"

class AGridRunner;

class AFarm_si : public AScriptInterface
{
public:
    AFarm_si();
    ~AFarm_si();

private:
    AGridRunner * GridRunner = nullptr; // !*! now local, consider make external later
};

#endif // AFARM_SI_H
