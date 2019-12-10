#ifndef ADRAWTEMPLATE_H
#define ADRAWTEMPLATE_H

#include "atemplateselectionrecord.h"

#include <QVector>
#include <QPair>
#include <QString>
#include <QJsonObject>

class ADrawObject;
class TObject;
class TAxis;
class TAttText;

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
    ATemplateSelectionRecord Selection;

    ADrawTemplate();
    virtual ~ADrawTemplate();

    void createFrom(const QVector<ADrawObject> &DrawObjects, const QVector<QPair<double,double>> & XYZ_ranges);
    void applyTo(QVector<ADrawObject> & DrawObjects, QVector<QPair<double,double>> & XYZ_ranges, bool bAll);

    const ATemplateSelectionRecord * findRecord(const QString & Label, const ATemplateSelectionRecord * ParentRecord) const;
    bool hasLegend() const {return (LegendIndex != -1);}

private:
    QString DrawOption;
    ADrawTemplate_Axis  AxisProperties[3];
    QVector<QPair<double, double>> XYZranges;
    QVector<QJsonObject> ObjectAttributes;

    int LegendIndex = -1;

    bool bIgnoreSelection = true;
    ATemplateSelectionRecord DummyRecordSelected;  //used when "apply template" is triggered in non-selective mode

private:
    void    InitSelection();
    TAxis * getAxis(TObject *tobj, int index) const;
    void    fillAxisProperties (int index, TAxis * axis);
    void    applyAxisProperties(int index, TAxis * axis) const;
    void    clearSelection();
};

class TAttLine;
class TAttMarker;
class TAttFill;
class TLegend;

namespace ARootJson
{
    void toJson(const ADrawObject & obj, QJsonObject & json);
    bool fromJson(ADrawObject & obj, const QJsonObject & json);


    void toJson(const TAttLine * obj, QJsonObject & json);
    void fromJson(TAttLine * obj, const QJsonObject & json);

    void toJson(const TAttMarker * obj, QJsonObject & json);
    void fromJson(TAttMarker * obj, const QJsonObject & json);

    void toJson(const TAttFill * obj, QJsonObject & json);
    void fromJson(TAttFill * obj, const QJsonObject & json);

    void toJson(const TAttText * obj, QJsonObject & json);
    void fromJson(TAttText * obj, const QJsonObject & json);

    void toJson(const TLegend * obj, QJsonObject & json);
    void fromJson(TLegend * obj, const QJsonObject & json);
}

#endif // ADRAWTEMPLATE_H
