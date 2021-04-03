#include "apadproperties.h"
#include "ajsontools.h"
#include <QDebug>

APadProperties::APadProperties()
{
    tPad = new TPad("tPad", "tPad", 0.05, 0.05, 0.95, 0.95);
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
//    double x1 = tPad->XtoAbsPixel(padGeo.xLow);
//    double x2 = tPad->XtoAbsPixel(padGeo.xHigh);
//    double y1 = tPad->YtoAbsPixel(padGeo.yLow);
//    double y2 = tPad->YtoAbsPixel(padGeo.yHigh);
//    tPad->SetBBoxX1(x1);
//    tPad->SetBBoxX2(x2);
//    tPad->SetBBoxY1(y1);
//    tPad->SetBBoxY2(y2);
    tPad->SetPad(padGeo.xLow, padGeo.yLow, padGeo.xHigh, padGeo.yHigh);
    qDebug() <<"tpad geometry" <<tPad->GetAbsXlowNDC() <<tPad->GetAbsYlowNDC() <<tPad->GetAbsWNDC() <<tPad->GetAbsHNDC();
//    qDebug() <<"tpad geometry" <<x1 <<y1 <<x2 <<y2;
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
