#include "ageoconsts.h"
#include "ajsontools.h"
#include "TFormula.h"

#include <QDebug>

AGeoConsts::AGeoConsts()
{
    FunctionsToJS << "abs" << "acos" << "asin" << "atan" << "ceil" << "floor" << "cos" << "exp" << "log" << "pow" << "sin" << "sqrt" << "tan";

    FormulaReservedWords << "sqrt2" << "pi" << "ln10" << "infinity"
                         << "pow" << "sin" << "cos" << "sqrt" << "exp" << "ceil" << "floor";

    ForbiddenVarsRExp << QRegExp("\\bg\\b") << QRegExp("\\bh\\b") << QRegExp("\\bt\\b") << QRegExp("\\bk\\b") << QRegExp("\\bx\\b")
                      << QRegExp("\\by\\b") << QRegExp("\\bz\\b") << QRegExp("\\bc\\b") << QRegExp("\\br\\b") << QRegExp("\\be\\b");
}

AGeoConsts &AGeoConsts::getInstance()
{
    static AGeoConsts instance;
    return instance;
}

const AGeoConsts &AGeoConsts::getConstInstance()
{
    return getInstance();
}

bool AGeoConsts::isGeoConstInUseGlobal(const QRegExp & nameRegExp, const AGeoObject * obj) const
{
    for (const AGeoConstRecord & r : Records)
        if (r.Expression.contains(nameRegExp)) return true;

    if (obj->isGeoConstInUseRecursive(nameRegExp)) return true;

    return false;
}

QString AGeoConsts::exportToJavaSript(const AGeoObject * obj) const
{
    QString GCScript;

    if (!Records.isEmpty())
    {
        for (int i = 0; i < Records.size(); i++)
        {
            const AGeoConstRecord & r = Records.at(i);
            QRegExp nameRegExp("\\b" + r.Name + "\\b");
            if (isGeoConstInUseGlobal(nameRegExp, obj))
                GCScript += (QString("var %1 = %2%3\n").arg(r.Name)
                             .arg(r.Expression.isEmpty()? QString::number(GeoConstValues[i]) : r.Expression))
                             .arg(r.Comment.isEmpty()? r.Comment : QString("                   //%1").arg(r.Comment));
        }
        formulaToJavaScript(GCScript);
    }

    GCScript += "\n";
    return GCScript;
}

void AGeoConsts::formulaToJavaScript(QString & str) const
{
    for (const QString & s : FunctionsToJS)
    {
        QString pat(s + '(');
        str.replace(pat, "Math." + pat);
    }
}

void AGeoConsts::clearConstants()
{
    Records.clear();
    GeoConstValues.clear();
}

void AGeoConsts::writeToJson(QJsonObject & json) const
{
    QJsonArray ar;
    for (int i = 0; i < Records.size(); i++)
    {
        const AGeoConstRecord & r = Records.at(i);
        QJsonArray el;
            el << r.Name << GeoConstValues.at(i) << r.Expression << r.Comment;
        ar.push_back(el);
    }
    json["GeoConsts"] = ar;
}

void AGeoConsts::readFromJson(const QJsonObject & json)
{
    clearConstants();

    QJsonArray ar;
    parseJson(json, "GeoConsts", ar);

    const int size = ar.size();
    Records.resize(size);
    GeoConstValues.resize(size);

    for (int i = 0; i < size; i++)
    {
        AGeoConstRecord & r = Records[i];
        QJsonArray el = ar[i].toArray();
        if (el.size() >= 2)
        {
            r.Name            = el[0].toString();
            GeoConstValues[i] = el[1].toDouble();
        }
        if (el.size() >= 3) r.Expression = el[2].toString();
        if (el.size() >= 4) r.Comment    = el[3].toString();
    }

    updateRunTimeProperties();

    for (int i = 0; i < size; i++)
    {
        bool ok = evaluateConstExpression(i);
        if (!ok) qWarning() <<"something went really wrong";
    }
}

bool AGeoConsts::evaluateFormula(QString str, double & returnValue, int to) const
{
    if (to == -1) to = Records.size();

    for (int i = 0; i < to; i++)
        str.replace(Records.at(i).RegExp, Records.at(i).Index);

    for (int ir = 0; ir < ForbiddenVarsRExp.size(); ir++)
    {
        if (str.contains(ForbiddenVarsRExp.at(ir)) )
            return false;
    }

    TFormula * f = new TFormula("", str.toLocal8Bit().data());
    if (!f || !f->IsValid())
    {
        delete f;
        return false;
    }

    returnValue = f->EvalPar(nullptr, GeoConstValues.data());
    delete f;
    return true;
}

bool AGeoConsts::updateParameter(QString & errorStr, QString & str, double & returnValue, bool bForbidZero, bool bForbidNegative, bool bMakeHalf) const
{
    if (str.isEmpty()) return true;

    bool ok;
    returnValue = str.simplified().toDouble(&ok);
    if (ok) str.clear();
    else
    {
        ok = evaluateFormula(str, returnValue);
        if (!ok)
        {
            errorStr = QString("Syntax error:\n%1").arg(str);
            return false;
        }
    }

    if (bForbidZero && returnValue == 0)
    {
        errorStr = "Unacceptable zero value";
        if (!str.isEmpty()) errorStr += " in: " + str;
        return false;
    }
    if (bForbidNegative && returnValue < 0)
    {
        errorStr = "Unacceptable negative value in";
        if (!str.isEmpty()) errorStr += ": " + str;
        else errorStr += ": " + QString::number(returnValue);
        return false;
    }
    if (bMakeHalf) returnValue *= 0.5;
    return true;
}

bool AGeoConsts::updateParameter(QString & errorStr, QString & str, int & returnValue, bool bForbidZero, bool bForbidNegative) const
{
    if (str.isEmpty()) return true;

    bool ok;
    returnValue = str.simplified().toInt(&ok);
    if (ok) str.clear();
    else
    {
        double dRetVal;
        ok = evaluateFormula(str, dRetVal);
        if (!ok)
        {
            errorStr = QString("Syntax error:\n%1").arg(str);
            return false;
        }
        returnValue = dRetVal;
    }

    if (bForbidZero && returnValue == 0)
    {
        errorStr = "Unacceptable zero value";
        if (!str.isEmpty()) errorStr += " in: " + str;
        return false;
    }
    if (bForbidNegative && returnValue < 0)
    {
        errorStr = "Unacceptable negative value in";
        if (!str.isEmpty()) errorStr += ": " + str;
        else errorStr += ": " + QString::number(returnValue);
        return false;
    }
    return true;
}

QString AGeoConsts::getName(int index) const
{
    if (index < 0 || index >= Records.size()) return "";
    if (Records.at(index).Name == placeholderStr) return "";
    return Records.at(index).Name;
}

double AGeoConsts::getValue(int index) const
{
    if (index < 0 || index >= GeoConstValues.size()) return 0;
    return GeoConstValues.at(index);
}

QString AGeoConsts::getExpression(int index) const
{
    if (index < 0 || index >= Records.size()) return "";
    return Records.at(index).Expression;
}

QString AGeoConsts::getComment(int index) const
{
    if (index < 0 || index >= Records.size()) return "";
    return Records.at(index).Comment;
}

bool AGeoConsts::evaluateConstExpression(int index)
{
    AGeoConstRecord & rec = Records[index];

    if (rec.Expression.isEmpty()) return true;
    QString strCopy = rec.Expression;

    bool ok;
    double val = strCopy.simplified().toDouble(&ok);
    if (ok)
        rec.Expression.clear();
    else
    {
        ok = evaluateFormula(strCopy, val, index);
        if (!ok) return false;
    }
    GeoConstValues[index] = val;
    return true;
}

bool AGeoConsts::rename(int index, const QString & newName, AGeoObject * world, QString & errorStr)
{
    if (index < 0 || index >= Records.size()) return false;
    AGeoConstRecord & rec = Records[index];

    if (newName == rec.Name) return true;
    errorStr = isNameValid(index, newName);
    if (!errorStr.isEmpty()) return false;
    rec.Name = newName;

    replaceGeoConstName(rec.RegExp, newName, index);
    world->replaceGeoConstNameRecursive(rec.RegExp, newName);
    updateRunTimeProperties();
    return true;
}

QString AGeoConsts::isNameValid(int index, const QString & newName)
{
    if (newName.isEmpty())                return "Name cannot be empty";
    if (newName.at(0).isDigit())          return "Name cannot start with a digit";
    if (newName.contains(QRegExp("\\s"))) return "Name cannot contain whitespace charachters eg:\" \" or \"\\n\" ";
    if (newName.contains(QRegExp("\\W"))) return "Name can only contain word characters: [0-9], [A-Z], [a-z], _";

    for (int i = 0; i < Records.size(); i++)
    {
        if (i == index) continue;
        if (newName == Records.at(i).Name) return "This name is already in use";
    }

    QRegExp reservedQRegExp;
    for (const QString & word : FormulaReservedWords)
    {
        reservedQRegExp = QRegExp("\\b" + word + "\\b");
        if (newName.contains(reservedQRegExp)) return QString("Name contains a TFormula reserved word: %1").arg(word);
    }
    return "";
}

bool AGeoConsts::setNewValue(int index, double newValue)
{
    if (index < 0 || index >= Records.size()) return false;

    GeoConstValues[index] = newValue;
    Records[index].Expression.clear();
    return true;
}

QString AGeoConsts::setNewExpression(int index, const QString & newExpression)
{
    if (index < 0 || index >= Records.size()) return "Wrong index";

    AGeoConstRecord & rec = Records[index];
    rec.Expression = newExpression;

    QString err = checkifValidAndGetDoublefromExpression(index);
    if (!err.isEmpty()) rec.Expression.clear();
    return err;
}

void AGeoConsts::setNewComment(int index, const QString & txt)
{
    if (index < 0 || index >= Records.size()) return;
    Records[index].Comment = txt;
}

bool AGeoConsts::isIndexValid(int index)
{
    if (index < 0 || index >= Records.size()) return false;
    return true;
}

QString AGeoConsts::checkifValidAndGetDoublefromExpression(int index)
{
    QString errorStr;
    const AGeoConstRecord & rec = Records[index];
    if (!rec.Expression.isEmpty())
    {
        QString constInUseBellow = isGeoConstsBellowInUse(index);
        if (!constInUseBellow.isEmpty())
            errorStr = QString("Expression not valid:\n"
                               "%1\n\n"
                               "Expression uses a geometry constant defined bellow:\n%2")
                               .arg(rec.Expression).arg(constInUseBellow);
        else
        {
            bool ok;
            ok = evaluateConstExpression(index);
            if (!ok) errorStr = QString("Expression not valid:\n\n%1\n\nSyntax error").arg(rec.Expression);
        }
    }
    return errorStr;
}

QString AGeoConsts::isGeoConstsBellowInUse(int index) const
{
    for (int i = index+1; i < Records.size(); i++)
        if (Records.at(index).Expression.contains(Records.at(i).RegExp)) return Records.at(i).Name;
    return "";
}

QString AGeoConsts::isGeoConstInUse(const QRegExp & nameRegExp, int index) const
{
    for (int i = index; i < Records.size(); i++)
        if (Records.at(i).Expression.contains(nameRegExp)) return Records.at(i).Name;
    return "";
}

void AGeoConsts::replaceGeoConstName(const QRegExp & nameRegExp, const QString & newName, int index)
{
    for (int i = index; i < Records.size(); i++)
        Records[i].Expression.replace(nameRegExp, newName);
}

QString AGeoConsts::addNewConstant(const QString & name, double value, int index)
{
    if (index < -1 || index > Records.size()) return "Bad index for the new geo constant";

    QString errorStr;
    if (name != placeholderStr)
    {
        errorStr = isNameValid(index, name);
        if (!errorStr.isEmpty()) return errorStr;
    }
    if (index == -1) index = Records.size();

    Records.insert(index, AGeoConstRecord(name));
    GeoConstValues.insert(index, value);

    updateRunTimeProperties();

    return errorStr;
}

void AGeoConsts::addNoNameConstant(int index)
{
    addNewConstant(placeholderStr, 0, index);
}

void AGeoConsts::removeConstant(int index)
{
    if (index < 0 || index >= Records.size()) return;

    Records.remove(index);
    GeoConstValues.remove(index);

    updateRunTimeProperties();
}

void AGeoConsts::updateRunTimeProperties()
{
    const int size = Records.size();

    for (int i = 0; i < size; i++)
    {
        Records[i].RegExp = QRegExp("\\b" + Records.at(i).Name + "\\b");
        Records[i].Index  = QString("[%1]").arg(i);
    }
}
