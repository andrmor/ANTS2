#include "adrawtemplate.h"
#include "adrawobject.h"

#include <QDebug>

#include "TObject.h"
#include "TGraph.h"
#include "TH1.h"
#include "TAxis.h"
#include "TAttLine.h"
#include "TAttMarker.h"
#include "TAttFill.h"

void ADrawTemplate::createFrom(const QVector<ADrawObject> & DrawObjects, const QVector<QPair<double, double> > & XYZ_ranges)
{
    if (DrawObjects.isEmpty()) return;

    TObject * tobj = DrawObjects.first().Pointer;
    if (!tobj) return;

    //draw options and attibutes
    DrawOption = DrawObjects.first().Options;
    //DrawAttributes.fillProperties(tobj);

    //axes
    for (int i = 0; i < 3; i++)
    {
        TAxis * axis = getAxis(tobj, i);
        fillAxisProperties(i, axis);
    }

    //ranges
    XYZranges = XYZ_ranges;

    //draw properties
    ObjectAttributes.clear();
    for (const ADrawObject & obj : DrawObjects)
    {
        QJsonObject json;
        ARootJson::toJson(obj, json);
        ObjectAttributes << json;
    }
}

void ADrawTemplate::applyTo(QVector<ADrawObject> & DrawObjects, QVector<QPair<double,double>> & XYZ_ranges) const
{
    if (DrawObjects.isEmpty()) return;
    TObject * tobj = DrawObjects.first().Pointer;
    if (!tobj) return;

    //draw options and attibutes
    DrawObjects.first().Options = DrawOption;
    //DrawAttributes.applyProperties(tobj);

    //axes
    for (int i = 0; i < 3; i++)
    {
        TAxis * axis = getAxis(tobj, i);
        applyAxisProperties(i, axis);
    }

    //ranges
    XYZ_ranges = XYZranges;

    //draw properties
    for (int i=0; i<ObjectAttributes.size(); i++)
    {
        if (i >= DrawObjects.size()) break;
        const QJsonObject & json = ObjectAttributes.at(i);

        bool bOK = ARootJson::fromJson(DrawObjects[i], json);
        if (!bOK) break;
    }
}

TAxis * ADrawTemplate::getAxis(TObject * tobj, int index) const
{
    TAxis * axis = nullptr;

    if (!tobj) return axis;

    TH1 * h = dynamic_cast<TH1*>(tobj);
    if (h)
    {
        qDebug() << "Cast to TH1";
        if      (index == 0) axis = h->GetXaxis();
        else if (index == 1) axis = h->GetYaxis();
        else if (index == 2) axis = h->GetZaxis();

        return axis;
    }

    TGraph * g = dynamic_cast<TGraph*>(tobj);
    if (g)
    {
        qDebug() << "Cast to TGraph";
        if      (index == 0) axis = g->GetXaxis();
        else if (index == 1) axis = g->GetYaxis();

        return axis;
    }

    return axis;
}

void ADrawTemplate::fillAxisProperties(int index, TAxis * axis)
{
    if (index < 0 || index > 2) return;
    ADrawTemplate_Axis & ap = AxisProperties[index];
    ap.fillProperties(axis);
}

void ADrawTemplate::applyAxisProperties(int index, TAxis * axis) const
{
    if (!axis) return;
    if (index < 0 || index > 2) return;
    const ADrawTemplate_Axis & ap = AxisProperties[index];
    ap.applyProperties(axis);
}

/*
void ADrawTemplate_DrawAttributes::fillProperties(const TObject * tobj)
{
    const TAttLine * al = dynamic_cast<const TAttLine*>(tobj);
    bLineActive = (bool)al;
    if (al)
    {
        LineColor = al->GetLineColor();
        LineStyle = al->GetLineStyle();
        LineWidth = al->GetLineWidth();
    }

    const TAttMarker * am = dynamic_cast<const TAttMarker*>(tobj);
    bMarkerActive = (bool)am;
    if (am)
    {
        MarkerColor = am->GetMarkerColor();
        MarkerStyle = am->GetMarkerStyle();
        MarkerSize  = am->GetMarkerSize();
    }

    const TAttFill * af = dynamic_cast<const TAttFill*>(tobj);
    bFillActive = (bool)af;
    if (af)
    {
        FillColor = af->GetFillColor();
        FillStyle = af->GetFillStyle();
    }
}

void ADrawTemplate_DrawAttributes::applyProperties(TObject * tobj) const
{
    if (bLineActive)
    {
        TAttLine * al = dynamic_cast<TAttLine*>(tobj);
        if (al)
        {
            al->SetLineColor(LineColor);
            al->SetLineStyle(LineStyle);
            al->SetLineWidth(LineWidth);
        }
    }

    if (bMarkerActive)
    {
        TAttMarker * am = dynamic_cast<TAttMarker*>(tobj);
        if (am)
        {
            am->SetMarkerColor(MarkerColor);
            am->SetMarkerStyle(MarkerStyle);
            am->SetMarkerSize(MarkerSize);
        }
    }

    if (bFillActive)
    {
        TAttFill * af = dynamic_cast<TAttFill*>(tobj);
        if (af)
        {
            af->SetFillColor(FillColor);
            af->SetFillStyle(FillStyle);
        }
    }
}
*/

void ADrawTemplate_Axis::fillProperties(const TAxis *axis)
{
    bActive = (bool)axis;
    if (!axis) return;

    AxisColor     = axis->GetAxisColor();
    NumDiv        = axis->GetNdivisions();
    TickLength    = axis->GetTickLength();
    TicksPosition = QString(axis->GetTicks());

    LabelColor    = axis->GetLabelColor();
    LabelFont     = axis->GetLabelFont();
    LabelSize     = axis->GetLabelSize();
    LabelOffset   = axis->GetLabelOffset();

    Title         = QString(axis->GetTitle());
    TitleColor    = axis->GetTitleColor();
    TitleFont     = axis->GetTitleFont();
    TitleOffset   = axis->GetTitleOffset();
    TitleSize     = axis->GetTitleSize();
    TitleCentered = axis->GetCenterTitle();
}

void ADrawTemplate_Axis::applyProperties(TAxis * axis) const
{
    if (!axis) return;
    if (!bActive) return;

    axis->SetAxisColor(AxisColor);
    axis->SetNdivisions(NumDiv);
    axis->SetTickLength(TickLength);
    //axis->SetTicks(TicksPosition.toLatin1().data());

    axis->SetLabelColor(LabelColor);
    axis->SetLabelFont(LabelFont);
    axis->SetLabelSize(LabelSize);
    axis->SetLabelOffset(LabelOffset);

    axis->SetTitle(Title.toLatin1().data());
    axis->SetTitleColor(TitleColor);
    axis->SetTitleFont(TitleFont);
    axis->SetTitleOffset(TitleOffset);
    axis->SetTitleSize(TitleSize);
    axis->CenterTitle(TitleCentered);
}

#include "ajsontools.h"
void ARootJson::toJson(const ADrawObject & obj, QJsonObject & json)
{
    const TObject * tobj = obj.Pointer;
    json["Type"] = QString(tobj->ClassName());
    json["Options"] = obj.Options;

    const TAttLine * al = dynamic_cast<const TAttLine*>(tobj);
    if (al)
    {
        QJsonObject js;
        js["Color"] = al->GetLineColor();
        js["Style"] = al->GetLineStyle();
        js["Width"] = al->GetLineWidth();

        json["Line"] = js;
    }

    const TAttMarker * am = dynamic_cast<const TAttMarker*>(tobj);
    if (am)
    {
        QJsonObject js;
        js["Color"] = am->GetMarkerColor();
        js["Style"] = am->GetMarkerStyle();
        js["Size"]  = am->GetMarkerSize();

        json["Marker"] = js;
    }

    const TAttFill * af = dynamic_cast<const TAttFill*>(tobj);
    if (af)
    {
        QJsonObject js;
        js["Color"] = af->GetFillColor();
        js["Style"] = af->GetFillStyle();

        json["Fill"] = js;
    }
}

bool ARootJson::fromJson(ADrawObject & obj, const QJsonObject & json)
{
    TObject * tobj = obj.Pointer;

    QString SavedType;
    parseJson(json, "Type", SavedType);
    QString Type = QString(tobj->ClassName());
    if (Type != SavedType) return false;

    parseJson(json, "Options", obj.Options);

    TAttLine * al = dynamic_cast<TAttLine*>(tobj);
    if (al)
    {
        QJsonObject js;
        if (parseJson(json, "Line", js))
        {
            int Color = 1;
            parseJson(js, "Color", Color);
            al->SetLineColor(Color);
            int Style = 1;
            parseJson(js, "Style", Style);
            al->SetLineStyle(Style);
            int Width = 1;
            parseJson(js, "Width", Width);
            al->SetLineWidth(Width);
        }
    }

    TAttMarker * am = dynamic_cast<TAttMarker*>(tobj);
    if (am)
    {
        QJsonObject js;
        if (parseJson(json, "Marker", js))
        {
            int Color = 1;
            parseJson(js, "Color", Color);
            am->SetMarkerColor(Color);
            int Style = 1;
            parseJson(js, "Style", Style);
            am->SetMarkerStyle(Style);
            float Size = 1.0;
            parseJson(js, "Size", Size);
            am->SetMarkerSize(Size);
        }
    }

    TAttFill * af = dynamic_cast<TAttFill*>(tobj);
    if (af)
    {
        QJsonObject js;
        if (parseJson(json, "Fill", js))
        {
            int Color = 1;
            parseJson(js, "Color", Color);
            af->SetFillColor(Color);
            int Style = 1001; //0
            parseJson(js, "Style", Style);
            af->SetFillStyle(Style);
        }
    }

    return true;
}
