#ifndef AFARM_SI_H
#define AFARM_SI_H

#include "ascriptinterface.h"

class AGridRunner;

class AFarm_si : public AScriptInterface
{
    Q_OBJECT

public:
    AFarm_si(AGridRunner & GridRunner);
    ~AFarm_si();

public slots:
    void setTimeout(double Timeout_ms);

    int checkAvailableThreads();

private:
    AGridRunner & GridRunner;
};

#endif // AFARM_SI_H
