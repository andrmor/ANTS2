#ifndef ASCRIPTEXAMPLE_H
#define ASCRIPTEXAMPLE_H

#include <QString>
#include <QVector>

class AScriptExample
{
public:
    explicit AScriptExample() {fSelected = true;}

    bool ReadFromRecord(QString Text);

    QString FileName;
    QVector<QString> Tags;
    QString Description;

    bool fSelected;

    QString ErrorString;
};

#endif // ASCRIPTEXAMPLE_H
