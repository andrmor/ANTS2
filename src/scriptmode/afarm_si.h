#ifndef AFARM_SI_H
#define AFARM_SI_H

#include "ascriptinterface.h"

#include <QVariant>
#include <QVariantList>

class QJsonObject;
class AGridRunner;

class AFarm_si : public AScriptInterface
{
    Q_OBJECT

public:
    AFarm_si(const QJsonObject & Config, AGridRunner & GridRunner);

public slots:
    void setTimeout(double Timeout_ms);

    int checkAvailableThreads();

    QVariantList evaluateScript(QString Script, QVariantList Resources, QVariantList FileNames);


private:
    const QJsonObject & Config;
    AGridRunner & GridRunner;
};

#endif // AFARM_SI_H
