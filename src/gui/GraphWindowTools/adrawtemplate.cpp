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
    for (const ADrawObject & obj : DrawObjects)
    {
        QJsonObject json;
        ARootJson::toJson(obj, json);
        ObjectAttributes << json;
    }
}

void ADrawTemplate::applyTo(QVector<ADrawObject> & DrawObjects, QVector<QPair<double,double>> & XYZ_ranges, bool bAll)
{
    if (DrawObjects.isEmpty()) return;
    TObject * tobj = DrawObjects.first().Pointer;
    if (!tobj) return;

    bIgnoreSelection = bAll;

    //draw options and attibutes
    //DrawObjects.first().Options = DrawOption;
    //DrawAttributes.applyProperties(tobj);

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
    {
        XYZ_ranges = XYZranges;
        /*
        const ATemplateSelectionRecord * X_rec = findRecord("X range", range_rec);
        if (X_rec && X_rec->bSelected) XYZ_ranges[0] = XYZranges[0];
        const ATemplateSelectionRecord * Y_rec = findRecord("Y range", range_rec);
        if (Y_rec && Y_rec->bSelected) XYZ_ranges[1] = XYZranges[1];
        const ATemplateSelectionRecord * Z_rec = findRecord("Z range", range_rec);
        if (Z_rec && Z_rec->bSelected) XYZ_ranges[2] = XYZranges[2];
        */
    }

    //draw properties
    const ATemplateSelectionRecord * att_rec = findRecord("Drawn object attributes", &Selection);
    if (att_rec && att_rec->bSelected)
    {
        for (int i=0; i<ObjectAttributes.size(); i++)
        {
            if (i >= DrawObjects.size()) break;
            const QJsonObject & json = ObjectAttributes.at(i);

            bool bOK = ARootJson::fromJson(DrawObjects[i], json);
            if (!bOK) break;
        }
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
