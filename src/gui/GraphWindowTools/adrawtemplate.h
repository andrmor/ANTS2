#ifndef ADRAWTEMPLATE_H
#define ADRAWTEMPLATE_H

#include <QVector>
#include <QString>

class ADrawObject;
class TObject;
class TAxis;

class ADrawTemplate_DrawAttributes
{
public:
    bool    bLineActive = false;
    int     LineColor;
    int     LineStyle;
    int     LineWidth;

    bool    bMarkerActive = false;
    int     MarkerColor;
    float   MarkerSize;
    int     MarkerStyle;

    bool    bFillActive = false;
    int     FillColor;
    int     FillStyle;

    void    fillProperties(const TObject * tobj);
    void    applyProperties(TObject * tobj) const;
};

class ADrawTemplate_Axis
{
public:
    bool    bActive = false;

    int     AxisColor;
    int     NumDiv;
    float   TickLength;
    QString TicksPosition;

    int     LabelColor;
    int     LabelFont;
    float   LabelSize;
    float   LabelOffset;

    QString Title;
    int     TitleColor;
    int     TitleFont;
    float   TitleOffset;
    float   TitleSize;
    bool    TitleCentered;

    void    fillProperties(const TAxis * axis);
    void    applyProperties(TAxis * axis) const;
};

class ADrawTemplate
{
public:
    void createFrom(const ADrawObject & Obj);
    void applyTo(ADrawObject & Obj) const;

private:
    QString   ObjType = "Undefined";

    QString DrawOption;
    ADrawTemplate_DrawAttributes DrawAttributes;
    ADrawTemplate_Axis  AxisProperties[3];


private:
    TAxis * getAxis(TObject *tobj, int index) const;
    void    fillAxisProperties (int index, TAxis * axis);
    void    applyAxisProperties(int index, TAxis * axis) const;
};

#endif // ADRAWTEMPLATE_H
