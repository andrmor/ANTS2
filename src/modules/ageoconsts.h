#ifndef AGEOCONSTS_H
#define AGEOCONSTS_H

#include <QVector>
#include <QString>
#include <QRegExp>
#include <ageoobject.h>

class QJsonObject;

struct AGeoConstRecord
{
    AGeoConstRecord(){}
    AGeoConstRecord(const QString & Name) : Name(Name) {}

    QString Name;
    QString Expression;
    QString Comment;

    //runtime
    QRegExp RegExp;
    QString Index;
};

class AGeoConsts final
{
public:
    static       AGeoConsts & getInstance();
    static const AGeoConsts & getConstInstance();

    void    clearConstants();

    QString addNewConstant(const QString & name, double value, int index = -1);
    void    addNoNameConstant(int index);
    void    removeConstant(int index);

    bool    rename(int index, const QString & newName, AGeoObject * world, QString & errorStr);
    QString isNameValid(int index, const QString & newName);
    bool    setNewValue(int index, double newValue);
    QString setNewExpression(int index, const QString & newExpression);
    void    setNewComment(int index, const QString & txt);
    bool    isIndexValid(int index);

    QString checkifValidAndGetDoublefromExpression(int index);
    QString isGeoConstsBellowInUse(int index) const;

    QString isGeoConstInUse(const QRegExp & nameRegExp, int index) const;
    void    replaceGeoConstName(const QRegExp & nameRegExp, const QString & newName, int index);

    QString getName(int index) const;
    double  getValue(int index) const;
    QString getExpression(int index) const;
    QString getComment(int index) const;

    int     countConstants() const {return Records.size();}
    bool    evaluateConstExpression(int index);
    bool    isGeoConstInUseGlobal(const QRegExp & nameRegExp, const AGeoObject * obj) const;

    QString exportToJavaSript(const AGeoObject * obj) const;
    void    formulaToJavaScript(QString & str) const;

    void    writeToJson(QJsonObject & json) const;
    void    readFromJson(const QJsonObject & json);

    bool    evaluateFormula(QString str, double & returnValue, int to = -1) const;
    bool    updateParameter(QString & errorStr, QString & str, double & returnValue, bool bForbidZero = true, bool bForbidNegative = true, bool bMakeHalf = true) const;
    bool    updateParameter(QString & errorStr, QString & str, int    & returnValue, bool bForbidZero = true, bool bForbidNegative = true) const;

    const QVector<QString> & getTFormulaReservedWords() const {return FormulaReservedWords;}

public:
    const QString placeholderStr = "______";

private:
    AGeoConsts();

    AGeoConsts(const AGeoConsts&) = delete;            // Copy ctor
    AGeoConsts(AGeoConsts&&) = delete;                 // Move ctor
    AGeoConsts& operator=(const AGeoConsts&) = delete; // Copy assign
    AGeoConsts& operator=(AGeoConsts&) = delete;       // Move assign

    QVector<AGeoConstRecord> Records;
    QVector<double> GeoConstValues;                    // has to be always synchronized with Records !  GeoConstValues.data() is used by TFormula

    //misc
    QVector<QString> FunctionsToJS;
    QVector<QString> FormulaReservedWords;
    QVector<QRegExp> ForbiddenVarsRExp;

    void updateRunTimeProperties();
};

#endif // AGEOCONSTS_H
