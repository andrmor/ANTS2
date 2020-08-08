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

    json["Map"] = ar;
}

void AGeoConsts::readFromJson(const QJsonObject & json)
{
    clearConstants();

    QJsonArray ar;
    parseJson(json, "Map", ar);

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
