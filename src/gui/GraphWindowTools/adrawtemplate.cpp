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
#include "TLegend.h"

ADrawTemplate::ADrawTemplate()
{
    InitSelection();
    DummyRecordSelected.bSelected = true;
}

ADrawTemplate::~ADrawTemplate()
{
    clearSelection();
}

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
    LegendIndex = -1;
    for (int iObj = 0; iObj < DrawObjects.size(); iObj++)
    {
        const ADrawObject & obj = DrawObjects.at(iObj);
        QJsonObject json;
        ARootJson::toJson(obj, json);
        ObjectAttributes << json;

        const TLegend * Leg = dynamic_cast<const TLegend*>(obj.Pointer);
        if (Leg) LegendIndex = iObj;
    }
}

void ADrawTemplate::applyTo(QVector<ADrawObject> & DrawObjects, QVector<QPair<double,double>> & XYZ_ranges, bool bAll)
{
    if (DrawObjects.isEmpty()) return;
    TObject * tobj = DrawObjects.first().Pointer;
    if (!tobj) return;

    bIgnoreSelection = bAll;

    //axes
    const ATemplateSelectionRecord * axes_rec = findRecord("Axes", &Selection);
    if (axes_rec && axes_rec->bSelected)
    {
        const ATemplateSelectionRecord * X_rec = findRecord("X axis", axes_rec);
        if (X_rec && X_rec->bSelected)
        {
            TAxis * axis = getAxis(tobj, 0);
            applyAxisProperties(0, axis);
        }
        const ATemplateSelectionRecord * Y_rec = findRecord("Y axis", axes_rec);
        if (Y_rec && Y_rec->bSelected)
        {
            TAxis * axis = getAxis(tobj, 1);  // will be special rules for vertical axis
            applyAxisProperties(1, axis);
        }
        const ATemplateSelectionRecord * Z_rec = findRecord("Y axis", axes_rec);
        if (Z_rec && Z_rec->bSelected)
        {
            TAxis * axis = getAxis(tobj, 2);  // might be special rule for Z axes
            applyAxisProperties(2, axis);
        }
    }

    //ranges
    const ATemplateSelectionRecord * range_rec = findRecord("Ranges", &Selection);
    if (range_rec && range_rec->bSelected)
        XYZ_ranges = XYZranges;

    //if template has a Legend, assure that the DrawObjects have TLegend, preferably at the same index
    int iLegend = -1;
    TLegend * Legend = nullptr;
    for (int iObj = 0; iObj < DrawObjects.size(); iObj++)
    {
        Legend = dynamic_cast<TLegend*>(DrawObjects[iObj].Pointer);
        if (Legend)
        {
            iLegend = iObj;
            break;
        }
    }
    bool bApplyLegend = false;
    if (LegendIndex != -1)
    {
        const ATemplateSelectionRecord * legend_rec = findRecord("Legend attributes", &Selection);
        if (legend_rec && legend_rec->bSelected)
        {
            bApplyLegend = true;

            if (!Legend) //paranoic -> Legend is created before calling this method
            {
                Legend = new TLegend(0.1,0.1, 0.5,0.5); //not fully functional if created here! position is not set by read properties method
                DrawObjects << ADrawObject(Legend, "same");
            }

            if (iLegend != LegendIndex)
            {
                if (iLegend > LegendIndex)
                {
                    DrawObjects.move(iLegend, LegendIndex);
                    iLegend = LegendIndex;
                }
            }
        }
    }

    //draw properties
    const ATemplateSelectionRecord * att_rec = findRecord("Drawn object attributes", &Selection);
    if (att_rec && att_rec->bSelected)
    {
        for (int i=0; i<ObjectAttributes.size(); i++)
        {
            if (i >= DrawObjects.size()) break;
            const QJsonObject & json = ObjectAttributes.at(i);

            if (i == LegendIndex)
            {
                /* now outside, since it is a separate option in the selection widget
                if (iLegend >= 0 && bApplyLegend)
                {
                    bool bOK = ARootJson::fromJson(DrawObjects[iLegend], json);
                    if (!bOK) break;
                    continue;
                }
                */
            }
            else
            {
                if (i == iLegend) continue;

                bool bOK = ARootJson::fromJson(DrawObjects[i], json);
                if (!bOK) break;
            }
        }
    }

    //legend properties
    if (iLegend >= 0 && bApplyLegend)
    {
        const QJsonObject & json = ObjectAttributes.at(LegendIndex);
        bool bOK = ARootJson::fromJson(DrawObjects[iLegend], json);
        if (!bOK) qWarning() << "Error applying TLegend properties from the template";
    }
}

void ADrawTemplate::InitSelection()
{
    clearSelection();

    Selection.Label = "All properties";
    Selection.bExpanded = true;

    ATemplateSelectionRecord * axes  = new ATemplateSelectionRecord("Axes", &Selection);
    ATemplateSelectionRecord * xAxis = new ATemplateSelectionRecord("X axis", axes);
    ATemplateSelectionRecord * yAxis = new ATemplateSelectionRecord("Y axis", axes);
    ATemplateSelectionRecord * zAxis = new ATemplateSelectionRecord("Z axis", axes);

    ATemplateSelectionRecord * ranges = new ATemplateSelectionRecord("Ranges", &Selection);
    ATemplateSelectionRecord * xRange = new ATemplateSelectionRecord("X range", ranges);
    ATemplateSelectionRecord * yRange = new ATemplateSelectionRecord("Y range", ranges);
    ATemplateSelectionRecord * zRange = new ATemplateSelectionRecord("Z range", ranges);

    ATemplateSelectionRecord * drawObj = new ATemplateSelectionRecord("Drawn object attributes", &Selection);

    ATemplateSelectionRecord * legend = new ATemplateSelectionRecord("Legend attributes", &Selection);
}

TAxis * ADrawTemplate::getAxis(TObject * tobj, int index) const
{
    TAxis * axis = nullptr;

    if (!tobj) return axis;

    TH1 * h = dynamic_cast<TH1*>(tobj);
    if (h)
    {
        //qDebug() << "Cast to TH1";
        if      (index == 0) axis = h->GetXaxis();
        else if (index == 1) axis = h->GetYaxis();
        else if (index == 2) axis = h->GetZaxis();

        return axis;
    }

    TGraph * g = dynamic_cast<TGraph*>(tobj);
    if (g)
    {
        //qDebug() << "Cast to TGraph";
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

void ADrawTemplate::clearSelection()
{
    for (ATemplateSelectionRecord * rec : Selection.Children) delete rec;
    Selection.Children.clear();
    Selection.Parent = nullptr;
}

const ATemplateSelectionRecord * ADrawTemplate::findRecord(const QString & Label, const ATemplateSelectionRecord * ParentRecord) const
{
    if (bIgnoreSelection) return &DummyRecordSelected;
    return ParentRecord->findChild(Label);
}

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
#include "TAttText.h"
void ARootJson::toJson(const ADrawObject & obj, QJsonObject & json)
{
    const TObject * tobj = obj.Pointer;
    json["Type"] = QString(tobj->ClassName());
    json["Options"] = obj.Options;

    const TAttLine * al = dynamic_cast<const TAttLine*>(tobj);
    if (al) ARootJson::toJson(al, json);

    const TAttMarker * am = dynamic_cast<const TAttMarker*>(tobj);
    if (am) ARootJson::toJson(am, json);

    const TAttFill * af = dynamic_cast<const TAttFill*>(tobj);
    if (af) ARootJson::toJson(af, json);

    const TAttText * aText = dynamic_cast<const TAttText*>(tobj);
    if (aText) ARootJson::toJson(aText, json);

    const TLegend * aLeg = dynamic_cast<const TLegend*>(tobj);
    if (aLeg) ARootJson::toJson(aLeg, json);
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
    if (al) ARootJson::fromJson(al, json);

    TAttMarker * am = dynamic_cast<TAttMarker*>(tobj);
    if (am) ARootJson::fromJson(am, json);

    TAttFill * af = dynamic_cast<TAttFill*>(tobj);
    if (af) ARootJson::fromJson(af, json);

    TAttText * aText = dynamic_cast<TAttText*>(tobj);
    if (aText) ARootJson::fromJson(aText, json);

    TLegend * aLeg = dynamic_cast<TLegend*>(tobj);
    if (aLeg) ARootJson::fromJson(aLeg, json);

    return true;
}

void ARootJson::toJson(const TAttLine * obj, QJsonObject & json)
{
    QJsonObject js;
    js["Color"] = obj->GetLineColor();
    js["Style"] = obj->GetLineStyle();
    js["Width"] = obj->GetLineWidth();

    json["Line attributes"] = js;
}

void ARootJson::fromJson(TAttLine * obj, const QJsonObject &json)
{
    QJsonObject js;
    if (parseJson(json, "Line attributes", js))
    {
        int Color = 1;
        parseJson(js, "Color", Color);
        obj->SetLineColor(Color);
        int Style = 1;
        parseJson(js, "Style", Style);
        obj->SetLineStyle(Style);
        int Width = 1;
        parseJson(js, "Width", Width);
        obj->SetLineWidth(Width);
    }
}

void ARootJson::toJson(const TAttMarker *obj, QJsonObject &json)
{
    QJsonObject js;
    js["Color"] = obj->GetMarkerColor();
    js["Style"] = obj->GetMarkerStyle();
    js["Size"]  = obj->GetMarkerSize();

    json["Marker attributes"] = js;
}

void ARootJson::fromJson(TAttMarker *obj, const QJsonObject &json)
{
    QJsonObject js;
    if (parseJson(json, "Marker attributes", js))
    {
        int Color = 1;
        parseJson(js, "Color", Color);
        obj->SetMarkerColor(Color);
        int Style = 1;
        parseJson(js, "Style", Style);
        obj->SetMarkerStyle(Style);
        float Size = 1.0;
        parseJson(js, "Size", Size);
        obj->SetMarkerSize(Size);
    }
}

void ARootJson::toJson(const TAttFill *obj, QJsonObject &json)
{
    QJsonObject js;
    js["Color"] = obj->GetFillColor();
    js["Style"] = obj->GetFillStyle();

    json["Fill attributes"] = js;
}

void ARootJson::fromJson(TAttFill *obj, const QJsonObject &json)
{
    QJsonObject js;
    if (parseJson(json, "Fill attributes", js))
    {
        int Color = 1;
        parseJson(js, "Color", Color);
        obj->SetFillColor(Color);
        int Style = 1001; //0
        parseJson(js, "Style", Style);
        obj->SetFillStyle(Style);
    }
}

void ARootJson::toJson(const TAttText * obj, QJsonObject &json)
{
    QJsonObject js;
    js["Align"] = obj->GetTextAlign();
    js["Color"] = obj->GetTextColor();
    js["Font"] = obj->GetTextFont();
    js["Size"] = obj->GetTextSize();

    json["Text attributes"] = js;
}

void ARootJson::fromJson(TAttText *obj, const QJsonObject &json)
{
    QJsonObject js;
    if (parseJson(json, "Text attributes", js))
    {
        int Align = 11;
        parseJson(js, "Align", Align);
        obj->SetTextAlign(Align);
        int Color = 1;
        parseJson(js, "Color", Color);
        obj->SetTextColor(Color);
        int Font = 2;
        parseJson(js, "Font", Font);
        obj->SetTextFont(Font);
        float Size;
        parseJson(js, "Size", Size);
        obj->SetTextSize(Size);
    }
}

void ARootJson::toJson(const TLegend *obj, QJsonObject &json)
{
    QJsonObject jsTop;

    QJsonObject jsGeo;
        jsGeo["X1"] = obj->GetX1NDC();
        jsGeo["X2"] = obj->GetX2NDC();
        jsGeo["Y1"] = obj->GetY1NDC();
        jsGeo["Y2"] = obj->GetY2NDC();
    jsTop["Geometry"] = jsGeo;

    json["Legend"] = jsTop;
}

void ARootJson::fromJson(TLegend *obj, const QJsonObject &json)
{
    QJsonObject jsTop;
    parseJson(json, "Legend", jsTop);

    QJsonObject jsGeo;
    parseJson(jsTop, "Geometry", jsGeo);
    double d = 0.5;
    parseJson(jsGeo, "X1", d);
    obj->SetX1NDC(d);
    parseJson(jsGeo, "X2", d);
    obj->SetX2NDC(d);
    parseJson(jsGeo, "Y1", d);
    obj->SetY1NDC(d);
    parseJson(jsGeo, "Y2", d);
    obj->SetY2NDC(d);

}
