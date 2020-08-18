#ifndef AGEOCONSTS_H
#define AGEOCONSTS_H

#include <QVector>
#include <QString>
#include <QRegExp>
#include <ageoobject.h>

class QJsonObject;

class AGeoConsts final
{
public:
    static       AGeoConsts & getInstance();
    static const AGeoConsts & getConstInstance();

    const QVector<QString> & getNames()  const {return Names;}
    const QVector<double>  & getValues() const {return Values;}

    int  countConstants() const {return Names.size();}
    //QVector<QString> getGeoConstsInUse(const AGeoObject &obj) const;
    bool isGeoConstInUse(const QRegExp & nameRegExp, const AGeoObject * obj) const;

    QString exportToJavaSript(const AGeoObject * obj) const;
    void formulaToJavaScript(QString & str) const;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    bool evaluateFormula(QString str, double & returnValue) const;
    bool updateParameter(QString &errorStr, QString & str, double & returnValue, bool bForbidZero = true, bool bForbidNegative = true, bool bMakeHalf = true) const;

    QString getName(int index) const;
    bool rename(int index, const QString & newName);
    bool setNewValue(int index, double newValue);
    bool addNewConstant(const QString & name, double value);

    void remove(int index);

private:
    AGeoConsts();

    AGeoConsts(const AGeoConsts&) = delete;            // Copy ctor
    AGeoConsts(AGeoConsts&&) = delete;                 // Move ctor
    AGeoConsts& operator=(const AGeoConsts&) = delete; // Copy assign
    AGeoConsts& operator=(AGeoConsts&) = delete;       // Move assign

    QVector<QString> Names;
    QVector<double>  Values;
    QVector<QString> StrValues;
    //runtime
    QVector<QRegExp> RegExps;
    QVector<QString> Indexes;

    //misc
    QVector<QString> FunctionsToJS;

    void update();
    void clearConstants();

public:
    static QVector<QString> getTFormulaReservedWords();
};

#endif // AGEOCONSTS_H

