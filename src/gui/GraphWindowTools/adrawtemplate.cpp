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

void ADrawTemplate::createFrom(const ADrawObject & Obj)
{
    TObject * tobj = Obj.Pointer;
    if (!tobj) return;

    ObjType = Obj.Pointer->ClassName();

    //draw options and attibutes
    DrawOption = Obj.Options;
    DrawAttributes.fillProperties(tobj);

    //axes
    for (int i = 0; i < 3; i++)
    {
        TAxis * axis = getAxis(tobj, i);
        fillAxisProperties(i, axis);
    }

}

void ADrawTemplate::applyTo(ADrawObject & Obj) const
{
    TObject * tobj = Obj.Pointer;
    if (!tobj) return;

    //draw options and attibutes
    Obj.Options = DrawOption;
    DrawAttributes.applyProperties(tobj);

    //axes
    for (int i = 0; i < 3; i++)
    {
        TAxis * axis = getAxis(tobj, i);
        applyAxisProperties(i, axis);
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
    axis->SetTicks(TicksPosition.toLatin1().data());

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
