#include "apadproperties.h"
#include "ajsontools.h"
#include <QDebug>

APadProperties::APadProperties()
{
    tPad = new TPad("tPad", "tPad",0.05, 0.05, 0.95, 0.95);
}

APadProperties::APadProperties(TPad *pad)
{
    tPad = pad;
    updatePadGeometry();
}

void APadProperties::updatePadGeometry() //actually should I keep "const TPad *pad" as a parameter because it shoudnt be able to change it?
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
    tPad->SetBBoxX1(tPad->XtoAbsPixel(padGeo.xLow));
    tPad->SetBBoxX2(tPad->XtoAbsPixel(padGeo.xHigh));
    tPad->SetBBoxY1(tPad->XtoAbsPixel(padGeo.yLow));
    tPad->SetBBoxY2(tPad->XtoAbsPixel(padGeo.yHigh));

    qDebug() <<"tpad geometry" <<tPad->GetAbsXlowNDC() <<tPad->GetAbsYlowNDC() <<tPad->GetAbsXlowNDC() + tPad->GetAbsWNDC() <<tPad->GetAbsYlowNDC() +tPad->GetAbsHNDC();
}

void APadProperties::writeToJson(QJsonObject &json) const
{
    padGeo.writeToJson(json);
    json["basketIndex"] = basketIndex;
}

void APadProperties::readFromJson(const QJsonObject &json)
{
    parseJson(json, "basketIndex", basketIndex);
    padGeo.readFromJson(json);
    applyPadGeometry();
}
