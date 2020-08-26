#include "ageoconsts.h"
#include "ajsontools.h"
#include "TFormula.h"

#include <QDebug>

AGeoConsts::AGeoConsts()
{
    FunctionsToJS << "abs" << "acos" << "asin" << "atan" << "ceil" << "floor" << "cos" << "exp" << "log" << "pow" << "sin" << "sqrt" << "tan";

    FormulaReservedWords << "sqrt2" << "e" << "pi" << "ln10" << "infinity"
    << "pow" << "sin" << "cos" << "sqrt" << "exp" << "ceil" << "floor";
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

bool AGeoConsts::isGeoConstInUseGlobal(const QRegExp &nameRegExp, const AGeoObject * obj) const
{
    for (const QString & StrValue : Expressions)
    {
        if (StrValue.contains(nameRegExp)) return true;
    }

    if (obj->isGeoConstInUseRecursive(nameRegExp)) return true;

    return false;
}

QString AGeoConsts::exportToJavaSript(const AGeoObject * obj) const
{
    if (Names.isEmpty()) return "\n";
    QRegExp nameRegExp;
    QString GCScript;
    QString GCName;
    double GCValue;

    for (int i =0; i < Names.size(); i++)
    {
        GCName = Names.at(i);
        GCValue = Values.at(i);
        nameRegExp = QRegExp("\\b"+GCName+"\\b");
        if (isGeoConstInUseGlobal(nameRegExp, obj))
        {
            GCScript += (QString("var %1 = %2;\n").arg(GCName).arg(GCValue));
        }
    }
    GCScript += "\n";
    qDebug() << GCScript;
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

/*QVector<QString> &AGeoConsts::getGeoConstsInUse(const AGeoObject & obj) const
{
    //AGeoObject * obje = dynamic_cast<AGeoObject*> (obj);
    for (QString constName : Names)
    {

    }
}*/

void AGeoConsts::clearConstants()
{
    Names.clear();
    Values.clear();
    Expressions.clear();

    updateRegExpsAndIndexes();
    qDebug() <<"oooooop 0";

}

void AGeoConsts::writeToJson(QJsonObject & json) const
{
    QJsonArray ar;
    for (int i = 0; i < Names.size(); i++)
    {
        QJsonArray el;
            el << Names.at(i) << Values.at(i) << Expressions.at(i);
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
    Names.resize(size);
    Values.resize(size);
    Expressions.resize(size);

    for (int i = 0; i < size; i++)
    {
        QJsonArray el = ar[i].toArray();
        if (el.size() >= 2)
        {
            Names[i]  = el[0].toString();
            Values[i] = el[1].toDouble();
        }
        if (el.size() >= 3) Expressions[i] = el[2].toString();
        else Expressions[i] = QString();
    }
    updateRegExpsAndIndexes();
    qDebug() <<"1oooooop 1" <<Expressions;
    for (int i = 0; i < size; i++)
        {
            bool ok = evaluateConstExpression(i);
            if (!ok) qWarning() <<"something went really wrong";
        }
    qDebug() <<"2oooooop 1" <<Expressions;

}

bool AGeoConsts::evaluateFormula(QString str, double &returnValue, int to) const
{
    if (to == -1) to = RegExps.size();
    for (int i = 0; i < to; i++)
        str.replace(RegExps.at(i), Indexes.at(i));
    //qDebug() << str;

    TFormula * f = new TFormula("", str.toLocal8Bit().data());
    if (!f || !f->IsValid())
    {
        delete f;
        return false;
    }

    returnValue = f->EvalPar(nullptr, Values.data());
    delete f;
    //qDebug() << "return value: "<< returnValue;
    return true;
}
bool AGeoConsts::updateParameter(QString &errorStr, QString &str, double &returnValue, bool bForbidZero, bool bForbidNegative, bool bMakeHalf) const
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
        errorStr = "Unacceptable value zero";
        if (str !=0) errorStr += " in : " + str;
        return false;
    }
    if (bForbidNegative && returnValue < 0)
    {
        errorStr = "Unacceptable value negative in";
        errorStr += ": " + str;
        return false;
    }
    if (bMakeHalf) returnValue *= 0.5;
    return true;
}

QString AGeoConsts::getName(int index) const
{
    if (index < 0 || index >= Names.size()) return "";
    if (Names.at(index) == placeholderStr) return "";
    return Names.at(index);
}

bool AGeoConsts::evaluateConstExpression(int current)
{
    if (Expressions.at(current).isEmpty()) return true;
    QString strCopy = Expressions.at(current);


    bool ok;
    double val;
    val = strCopy.simplified().toDouble(&ok);
    if (ok) Values[current] = val;

    else
    {
        ok = evaluateFormula(strCopy, val, current);
        if (!ok)
        {
            return false;
        }
        Values[current] = val;
    }
    return true;
}

bool AGeoConsts::rename(int index, const QString & newName, AGeoObject *world)
{
    QString oldName = Names.at(index);
    if (oldName == newName) return true;

    if (index < 0 || index >= Names.size()) return false;

    for (int i = 0; i < Names.size(); i++)
    {
        if (i == index) continue;
        if (newName == Names.at(i)) return false;
    }

    Names[index] = newName;
    updateRegExpsAndIndexes();

    //const QRegExp OldNameRegExp("\\b" + oldName + "\\b");
    replaceGeoConstName(RegExps.at(index), newName, index);
    world->replaceGeoConstNameRecursive(RegExps.at(index), newName);

    return true;
}

bool AGeoConsts::setNewValue(int index, double newValue)
{
    if (index < 0 || index >= Names.size()) return false;

    Values[index] = newValue;
    Expressions[index].clear();
    return true;
}

QString AGeoConsts::setNewExpression(int index, const QString & newExpression)
{
    if (index < 0 || index >= Names.size()) return "wrong index";
    Expressions[index] = newExpression;

    QString err = checkifValidAndGetDoublefromExpression(index);
    if (!err.isEmpty()) Expressions[index].clear();

    return err;
}

QString AGeoConsts::checkifValidAndGetDoublefromExpression(int current)
{
    QString errorStr;
    if (!Expressions.at(current).isEmpty())
    {
        QString constInUseBellow = isGeoConstsBellowInUse(current);
        if (!constInUseBellow.isEmpty())
            errorStr = QString("Expression not valid:\n"
                               "%1\n\n"
                               "Expression uses a geometry constant defined bellow:\n%2")
                               .arg(Expressions.at(current)).arg(constInUseBellow);

        else
        {
            bool ok;
            ok = evaluateConstExpression(current);
            if (!ok) errorStr = QString("Expression not valid:\n\n%1\n\nSyntax error").arg(Expressions.at(current));
        }

    }
    return errorStr;
}

QString AGeoConsts::isGeoConstsBellowInUse(int current)
{
    current += 1;
    for (int i = current; i < Names.size(); i++)
        if (Expressions.at(current).contains(RegExps.at(i))) return Names.at(i);
    return "";
}

QString AGeoConsts::isGeoConstInUse(const QRegExp &nameRegExp, int index) const
{
    for (int i = index; i < Names.size(); i++)
        if (Expressions[i].contains(nameRegExp)) return Names.at(i);
    return "";
}

void AGeoConsts::replaceGeoConstName(const QRegExp &nameRegExp, const QString &newName, int index)
{
    qDebug() <<"aaaaaaaaaaaaaaaaaaaaaaaaaa";
    for (int i = index; i < Names.size(); i++)
    {
        Expressions[i].replace(nameRegExp, newName);
    }
    qDebug() <<"Expression" <<Expressions;
}

bool AGeoConsts::addNewConstant(const QString & name, double value, int index)
{
    if (name != placeholderStr)
    {
        for (int i = 0; i < Names.size(); i++)
            if (name == Names.at(i)) return false; //already in use
    }
    if (index == -1) index = Names.size();

    Names.insert(index, name);
    Values.insert(index, value);
    Expressions.insert(index, "");

    qDebug() <<"sizes" <<Names.size() <<Values.size()  <<Expressions.size();
    updateRegExpsAndIndexes();
    qDebug() <<"oooooop 3";

    return true;
}

void AGeoConsts::addNoNameConstant(int index)
{
    addNewConstant(placeholderStr, 0, index);
}

void AGeoConsts::removeConstant(int index)
{
    if (index < 0 || index >= Names.size()) return;

    Names.remove(index);
    Values.remove(index);
    Expressions.remove(index);
    qDebug() <<"oooooop 4";

    updateRegExpsAndIndexes();
}

void AGeoConsts::updateRegExpsAndIndexes()
{
    const int size = Names.size();

    RegExps.resize(size);
    Indexes.resize(size);
    for (int i = 0; i < size; i++)
    {
        RegExps[i] = QRegExp("\\b" + Names.at(i) + "\\b");
        qDebug() <<"reg exp" <<RegExps;
        Indexes[i] = QString("[%1]").arg(i);
    }
}
