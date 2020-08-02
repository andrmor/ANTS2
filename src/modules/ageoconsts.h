#ifndef AGEOCONSTS_H
#define AGEOCONSTS_H

#include <QMap>
#include <QVector>
#include <QString>

class QJsonObject;

class AGeoConsts final
{
public:
    static       AGeoConsts& getInstance();
    static const AGeoConsts& getConstInstance();

    QMap<QString, double> GeoConsts;

    void clearConstants();

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
    bool evaluateFormula(QString str, double &returnValue) const;
    void updateConsts();

private:
    AGeoConsts();
    //~AGeoConsts();

    AGeoConsts(const AGeoConsts&) = delete;           // Copy ctor
    AGeoConsts(AGeoConsts&&) = delete;                // Move ctor
    AGeoConsts& operator=(const AGeoConsts&) = delete;// Copy assign
    AGeoConsts& operator=(AGeoConsts&) = delete;     // Move assign

    QVector<QString> vQRegularExpression;
    QVector<double>  vConstValues;

};

#endif // AGEOCONSTS_H

