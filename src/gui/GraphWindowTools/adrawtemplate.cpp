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

    ADrawTemplate_Axis & a = AxisProperties[index];
    a.bActive = (bool)axis;
    if (!axis) return;

    a.AxisColor     = axis->GetAxisColor();
    a.NumDiv        = axis->GetNdivisions();
    a.TickLength    = axis->GetTickLength();

    a.LabelColor    = axis->GetLabelColor();
    a.LabelFont     = axis->GetLabelFont();
    a.LabelSize     = axis->GetLabelSize();
    a.LabelOffset   = axis->GetLabelOffset();

    a.Title         = QString(axis->GetTitle());
    a.TitleColor    = axis->GetTitleColor();
    a.TitleFont     = axis->GetTitleFont();
    a.TitleOffset   = axis->GetTitleOffset();
    a.TitleSize     = axis->GetTitleSize();
}

void ADrawTemplate::applyAxisProperties(int index, TAxis * axis) const
{
    if (!axis) return;
    if (index < 0 || index > 2) return;

    const ADrawTemplate_Axis & a = AxisProperties[index];

    axis->SetAxisColor(a.AxisColor);
    axis->SetNdivisions(a.NumDiv);
    axis->SetTickLength(a.TickLength);

    axis->SetLabelColor(a.LabelColor);
    axis->SetLabelFont(a.LabelFont);
    axis->SetLabelSize(a.LabelSize);
    axis->SetLabelOffset(a.LabelOffset);

    axis->SetTitle(a.Title.toLatin1().data());
    axis->SetTitleColor(a.TitleColor);
    axis->SetTitleFont(a.TitleFont);
    axis->SetTitleOffset(a.TitleOffset);
    axis->SetTitleSize(a.TitleSize);
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
