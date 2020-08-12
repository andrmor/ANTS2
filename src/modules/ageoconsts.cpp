#include "ageoconsts.h"
#include "ajsontools.h"
#include "TFormula.h"

#include <QDebug>

AGeoConsts &AGeoConsts::getInstance()
{
    static AGeoConsts instance;
    return instance;
}

const AGeoConsts &AGeoConsts::getConstInstance()
{
    return getInstance();
}

void AGeoConsts::clearConstants()
{
    Names.clear();
    Values.clear();

    update();
}

void AGeoConsts::writeToJson(QJsonObject & json) const
{
    QJsonArray ar;
    for (int i = 0; i < Names.size(); i++)
    {
        QJsonArray el;
            el << Names.at(i) << Values.at(i);
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
    Names .resize(size);
    Values.resize(size);

    for (int i = 0; i < size; i++)
    {
        QJsonArray el = ar[i].toArray();
        if (el.size() >= 2)
        {
            Names[i]  = el[0].toString();
            Values[i] = el[1].toDouble();
        }
    }
    update();
}

bool AGeoConsts::evaluateFormula(QString str, double &returnValue) const
{
    for (int i = 0; i < RegExps.size(); i++)
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

bool AGeoConsts::updateParameter(QString &errorStr, QString &str, double &returnValue, bool bForbidZero, bool bForbidNegative, bool bMakeHalf)
{
    if (str.isEmpty()) return true;    //should it return true?

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
            errorStr = "Unacceptable value: zero";
            return false;
        }
        if (bForbidNegative && returnValue < 0)
        {
            errorStr = "Unacceptable value: negative";
            return false;
        }
        if (bMakeHalf) returnValue *= 0.5;
        return true;
}

QString AGeoConsts::getName(int index) const
{
    if (index < 0 || index >= Names.size()) return "";
    return Names.at(index);
}

bool AGeoConsts::rename(int index, const QString & newName)
{
    if (index < 0 || index >= Names.size()) return false;

    for (int i = 0; i < Names.size(); i++)
    {
        if (i == index) continue;
        if (newName == Names.at(i)) return false;
    }

    Names[index] = newName;
    update();
    return true;
}

bool AGeoConsts::setNewValue(int index, double newValue)
{
    if (index < 0 || index >= Names.size()) return false;

    Values[index] = newValue;
    return true;
}

bool AGeoConsts::addNewConstant(const QString & name, double value)
{
    for (int i = 0; i < Names.size(); i++)
        if (name == Names.at(i)) return false; //already in use

    Names. append(name);
    Values.append(value);
    update();
    return true;
}

void AGeoConsts::remove(int index)
{
    if (index < 0 || index >= Names.size()) return;

    Names .remove(index);
    Values.remove(index);
    update();
}

void AGeoConsts::update()
{
    const int size = Names.size();

    RegExps.resize(size);
    Indexes.resize(size);
    for (int i = 0; i < size; i++)
    {
        RegExps[i] = QRegExp("\\b" + Names.at(i) + "\\b");
        Indexes[i] = QString("[%1]").arg(i);
    }
}

QVector<QString> AGeoConsts::getTFormulaReservedWords()
{
    QVector<QString> v;
    v << "sqrt2" << "e" << "pi" << "ln10" << "infinity";
    v << "pow" << "sin" << "cos" << "sqrt" << "exp";
    return v;
}
