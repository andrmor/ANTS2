#include "aeventfilteringsettings.h"
#include "acommonfunctions.h"
#include "ajsontools.h"
#include "CorrelationFilters.h"
#include "apmgroupsmanager.h"
#include "eventsdataclass.h"

#include <QDebug>
#include <QJsonArray>

//bool AEventFilteringSettings::readFromJson(QJsonObject &js, DetectorClass *Detector, EventsDataClass *EventsDataHub)
bool AEventFilteringSettings::readFromJson(QJsonObject &js, APmGroupsManager* PMgroups, int ThisPMgroup, EventsDataClass *EventsDataHub)
{
    ErrorString.clear();

    fEventNumberFilter = js.contains("EventNumber");
    if (fEventNumberFilter)
      {
        QJsonObject njson = js["EventNumber"].toObject();
        parseJson(njson, "Active", fEventNumberFilter);
        parseJson(njson, "Min", EventNumberFilterMin);
        parseJson(njson, "Max", EventNumberFilterMax);
      }

    fMultipleScanEvents = false;
    if (js.contains("MultipleScanEvents"))
        parseJson(js, "MultipleScanEvents", fMultipleScanEvents);

    fSumCutOffFilter = js.contains("SumSignal");
    if (fSumCutOffFilter)
      {
        QJsonObject ssjson = js["SumSignal"].toObject();
        parseJson(ssjson, "Active", fSumCutOffFilter);
        parseJson(ssjson, "UseGains", fSumCutUsesGains);
        parseJson(ssjson, "UsePassives", fCutOffsForPassivePMs);
        parseJson(ssjson, "Min", SumCutOffMin);
        parseJson(ssjson, "Max", SumCutOffMax);
      }

    fCutOffFilter = js.contains("IndividualPMSignal");
    if (fCutOffFilter)
      {
        QJsonObject sjson = js["IndividualPMSignal"].toObject();
        parseJson(sjson, "Active", fCutOffFilter);
        parseJson(sjson, "UsePassives", fCutOffsForPassivePMs);
      }

    fEnergyFilter = js.contains("ReconstructedEnergy");
    if (fEnergyFilter)
      {
        QJsonObject rejson = js["ReconstructedEnergy"].toObject();
        parseJson(rejson, "Active", fEnergyFilter);
        parseJson(rejson, "Min", EnergyFilterMin);
        parseJson(rejson, "Max", EnergyFilterMax);
      }

    fLoadedEnergyFilter = js.contains("LoadedEnergy");
    if (fLoadedEnergyFilter)
      {
        QJsonObject lejson = js["LoadedEnergy"].toObject();
        parseJson(lejson, "Active", fLoadedEnergyFilter);
        parseJson(lejson, "Min", LoadedEnergyFilterMin);
        parseJson(lejson, "Max", LoadedEnergyFilterMax);
      }

    fChi2Filter = js.contains("Chi2");
    if (fChi2Filter)
      {
        QJsonObject cjson = js["Chi2"].toObject();
        parseJson(cjson, "Active", fChi2Filter);
        parseJson(cjson, "Min", Chi2FilterMin);
        parseJson(cjson, "Max", Chi2FilterMax);
      }

    SpF_RecOrScan = 0; //compatibility
    fSpF_LimitToObj = false;
    bool fOn = true;
    QString LimitCandidate;
    if (js.contains("SpatialToPrimary"))
        LimitCandidate = "PrScint";
    if (js.contains("SpatialToObject"))
    {
        QJsonObject jj = js["SpatialToObject"].toObject();
        parseJson(jj, "Active", fOn);
        parseJson(jj, "RecOrScan", SpF_RecOrScan);
        parseJson(jj, "LimitObject", LimitCandidate);

        if (SpF_RecOrScan == 1 && EventsDataHub->isScanEmpty())
        {
            qWarning() << "Attempt to set spatial filter by Scan, but it is empty";
            LimitCandidate.clear();
        }
    }
    if (!LimitCandidate.isEmpty())
    {
        //if (Detector->Sandwich->World->findObjectByName(LimitCandidate))
        //{
          fSpF_LimitToObj = fOn;
          SpF_LimitToObj = LimitCandidate.toLocal8Bit().data();
        //}
    }

    fSpF_custom = js.contains("SpatialCustom");
    if (fSpF_custom)
      {
        QJsonObject spfCjson = js["SpatialCustom"].toObject();
        parseJson(spfCjson, "Active", fSpF_custom);
        parseJson(spfCjson, "RecOrScan", SpF_RecOrScan);
        parseJson(spfCjson, "Shape", SpF_shape);
        parseJson(spfCjson, "SizeX", SpF_SizeX);
        parseJson(spfCjson, "SizeY", SpF_SizeY);
        parseJson(spfCjson, "Diameter", SpF_Diameter);
        parseJson(spfCjson, "Side", SpF_Side);
        parseJson(spfCjson, "Angle", SpF_Angle);
        parseJson(spfCjson, "AllZ", fSpF_allZ);
        parseJson(spfCjson, "Zfrom", SpF_Zfrom);
        parseJson(spfCjson, "Zto", SpF_Zto);
        SpF_X0 = 0; //compatibility
        parseJson(spfCjson, "X0", SpF_X0);
        SpF_Y0 = 0; //compatibility
        parseJson(spfCjson, "Y0", SpF_Y0);
        SpF_CutOutsideInside = 0; //compatibility
        parseJson(spfCjson, "CutOutsideInside", SpF_CutOutsideInside);

        SpF_polygon.clear();
        if (spfCjson.contains("Polygon"))
          {
            QJsonArray polyarr = spfCjson["Polygon"].toArray();
            for (int i=0; i<polyarr.size(); i++)
              {
                double x = polyarr[i].toArray()[0].toDouble();
                double y = polyarr[i].toArray()[1].toDouble();
                SpF_polygon.append(QPointF(x,y));
              }
          }
        //need to calculate..
        switch (SpF_shape)
          {
          case 0:
            SpF_halfSizeX = 0.5*SpF_SizeX;
            SpF_halfSizeY = 0.5*SpF_SizeY;
            break;
          case 1:
            SpF_radius2 = 0.5*SpF_Diameter;
            SpF_radius2 *= SpF_radius2;
            //qDebug() << "Rad2"<<SpF_radius2;
            break;
          case 2:
            SpF_polygon = MakeClosedPolygon(SpF_Side, SpF_Angle, SpF_X0, SpF_Y0);
            break;
          case 3:
            if (SpF_polygon.size()<4)
              {
                ErrorString = "Custom spatial filter: polygon not provided or not valid";
                return false;
              }
            break;
          default:
            ErrorString = "Unknown shape of custom spatial filter";
            return false;
          }
      }

    fCorrelationFilters = js.contains("Correlation");
    if (fCorrelationFilters)
      {
        QJsonObject corjson = js["Correlation"].toObject();
        parseJson(corjson, "Active", fCorrelationFilters);
        for (int i=0; i<CorrelationFilters.size(); i++) delete CorrelationFilters[i];
        CorrelationFilters.clear();

        if (fCorrelationFilters && corjson.contains("Set") )
        {
            QJsonArray jsonArray = corjson["Set"].toArray();
            if (jsonArray.isEmpty())
              {
                ErrorString = "Correlation filters active, but json does not contain their config";
                return false;
              }
            for (int i=0; i<jsonArray.size(); i++)
              {
                QJsonObject js = jsonArray[i].toObject();
                CorrelationFilterStructure* tmp = CorrelationFilterStructure::createFromJson(js, PMgroups, ThisPMgroup, EventsDataHub);
                if (!tmp)
                  {
                    ErrorString = "Error in json config of correlation filter";
                    return false;
                  }
                CorrelationFilters.append(tmp);
              }
        }
      }

     fKNNfilter = js.contains("kNN");
     if (fKNNfilter)
       {
         QJsonObject knnjson = js["kNN"].toObject();
         parseJson(knnjson, "Active", fKNNfilter);
         parseJson(knnjson, "Min", KNNfilterMin);
         parseJson(knnjson, "Max", KNNfilterMax);
         parseJson(knnjson, "AverageOver", KNNfilterAverageOver);
       }
     return true;
}
