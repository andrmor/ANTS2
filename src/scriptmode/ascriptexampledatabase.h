#ifndef ASCRIPTEXAMPLEDATABASE_H
#define ASCRIPTEXAMPLEDATABASE_H

#include "ascriptexample.h"

#include <QString>
#include <QVector>
#include <QStringList>

class AScriptExampleDatabase
{
public:
    AScriptExampleDatabase(QString ConfigText);

    void Select(QStringList tags);
    void UnselectAll();
    int size() {return Examples.size();}
    int Find(QString fileName);

    QVector<AScriptExample> Examples;
    QStringList Tags;
};

#endif // ASCRIPTEXAMPLEDATABASE_H
