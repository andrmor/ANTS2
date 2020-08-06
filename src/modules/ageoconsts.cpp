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
    GeoConsts.clear();
    updateConsts();
}

void AGeoConsts::writeToJson(QJsonObject & json) const
{
    QJsonArray ar;

    QMapIterator<QString, double> iter(GeoConsts);
    while (iter.hasNext())
    {
        iter.next();
        QJsonArray el;
            el << iter.key() << iter.value();
        ar.push_back(el);
    }

    json["Map"] = ar;
}

void AGeoConsts::readFromJson(const QJsonObject & json)
{
    clearConstants();

    QJsonArray ar;
    parseJson(json, "Map", ar);

    for (int iA = 0; iA < ar.size(); iA++)
    {
        QJsonArray el = ar[iA].toArray();
        if (el.size() >= 2)
        {
            const QString key = el[0].toString();
            const double  val = el[1].toDouble();
            GeoConsts[key] = val;      // warnings if not unique?
        }
    }
    updateConsts();
}

bool AGeoConsts::evaluateFormula(QString str, double &returnValue) const
{
    for (int iQRE=0; iQRE < vQRegularExpression.size(); iQRE++)
    {
        QString num = QString("[%1]").arg(iQRE);
        str.replace(QRegExp(vQRegularExpression[iQRE]),num);
    }

    //qDebug() << str;
    TFormula * f = new TFormula("", str.toLocal8Bit().data());
    if (!f || !f->IsValid())
    {
        delete f;
        return false;
    }

    returnValue = f->EvalPar(nullptr, vConstValues.data());
    delete f;
    //qDebug() << "return value: "<< returnValue;
    return true;

}

void AGeoConsts::updateConsts()
{
    vQRegularExpression.clear();
    vConstValues.clear();

    QMapIterator<QString, double> iter(GeoConsts);
    while (iter.hasNext())
    {
        iter.next();

        QString newQRE ="\\b" + iter.key() + "\\b";
        double newConstVal = iter.value();
        //qDebug() << "new QRE" <<newQRE << "   newConstVal" << newConstVal;
        vQRegularExpression.append(newQRE);
        vConstValues.append(newConstVal);
    }
    //qDebug() <<"vQRegularExpression"<< vQRegularExpression;
    //qDebug() <<"vConstValues"<< vConstValues;


}

AGeoConsts::AGeoConsts()
{
//    geoConsts.insert("x", 50);
}
