#include "apadproperties.h"
#include "ajsontools.h"
#include <QDebug>

APadProperties::APadProperties()
{
    tPad = new TPad("tPad", "tPad", 0.05, 0.05, 0.95, 0.95);
}

APadProperties::APadProperties(TPad * pad)
{
    tPad = pad;
    updatePadGeometry();
}

void APadProperties::updatePadGeometry()
{
    padGeo.xLow = tPad->GetAbsXlowNDC();
    padGeo.yLow = tPad->GetAbsYlowNDC();

    double w = tPad->GetAbsWNDC();
    double h = tPad->GetAbsHNDC();
    padGeo.xHigh = padGeo.xLow + w;
    padGeo.yHigh = padGeo.yLow + h;
}

void APadProperties::applyPadGeometry()
{
    tPad->SetPad(padGeo.xLow, padGeo.yLow, padGeo.xHigh, padGeo.yHigh);
    //qDebug() <<"tpad geometry" <<tPad->GetAbsXlowNDC() <<tPad->GetAbsYlowNDC() <<tPad->GetAbsWNDC() <<tPad->GetAbsHNDC();
}

void APadProperties::writeToJson(QJsonObject &json) const
{
    padGeo.writeToJson(json);
}

void APadProperties::readFromJson(const QJsonObject &json)
{
    padGeo.readFromJson(json);
    applyPadGeometry();
}

QString APadProperties::toString() const
{
    QString output;
    output = QString::number(padGeo.xLow) + ", " + QString::number(padGeo.yLow) + ", "
            + QString::number(padGeo.xHigh) + ", "  + QString::number(padGeo.yHigh);
    return output;
}
