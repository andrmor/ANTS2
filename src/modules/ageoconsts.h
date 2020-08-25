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

    bool addNewConstant(const QString & name, double value, int index = -1);
    void removeConstant(int index);

    bool rename(int index, const QString & newName);
    bool setNewValue(int index, double newValue);
    QString setNewExpression(int &index, const QString & newExpression, AGeoObject *worldObj);

    QString isGeoConstInUse(const QRegExp & nameRegExp, int index) const ;
    void replaceGeoConstName(const QRegExp & nameRegExp, const QString & newName, int index);

    QString getName(int index) const;
    const QVector<QString> & getNames()  const {return Names;}
    const QVector<double>  & getValues() const {return Values;}
    const QVector<QString> & getExpressions() const {return Expressions;}

    int  countConstants() const {return Names.size();}
    bool evaluateConstExpression(const int to, double &returnValue, const QString &str) const;
    bool isGeoConstInUseGlobal(const QRegExp & nameRegExp, const AGeoObject * obj) const;

    QString exportToJavaSript(const AGeoObject * obj) const;
    void formulaToJavaScript(QString & str) const;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    bool evaluateFormula(QString str, double & returnValue) const;
    bool updateParameter(QString &errorStr, QString & str, double & returnValue, bool bForbidZero = true, bool bForbidNegative = true, bool bMakeHalf = true) const;

private:
    AGeoConsts();

    AGeoConsts(const AGeoConsts&) = delete;            // Copy ctor
    AGeoConsts(AGeoConsts&&) = delete;                 // Move ctor
    AGeoConsts& operator=(const AGeoConsts&) = delete; // Copy assign
    AGeoConsts& operator=(AGeoConsts&) = delete;       // Move assign

    QVector<QString> Names;
    QVector<double>  Values;
    QVector<QString> Expressions;
    //runtime
    QVector<QRegExp> RegExps;
    QVector<QString> Indexes;

    //misc
    QVector<QString> FunctionsToJS;

    void update();
    QString updateExpression(const QString &Expression, int &index);
    void clearConstants();

public:
    static QVector<QString> getTFormulaReservedWords();
};

#endif // AGEOCONSTS_H

