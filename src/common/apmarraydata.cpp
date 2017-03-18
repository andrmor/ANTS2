#include "apmarraydata.h"
#include "ajsontools.h"

#include <QJsonObject>
#include <QDebug>

void APmArrayData::writeToJson(QJsonObject &json)
{
  QJsonObject js;
  js["Activated"] = fActive;
  //if (fActive)
    {
      js["Regularity"] = Regularity;   //0-regular, 1-auto_z, 2-custom
      switch (Regularity)
        {
         case 0:
          js["PMtype"] = PMtype;
          js["Packing"] = Packing;
          js["CenterToCenter"] = CenterToCenter;
          js["UseRings"] = fUseRings;
          if (fUseRings) js["NumRings"] = NumRings;
          else
            {
              js["NumX"] = NumX;
              js["NumY"] = NumY;
            }
          break;
        case 1:
          {
            js["PMtype"] = PMtype;
            QJsonArray Arr;
            for (int ipm=0; ipm<PositionsAnglesTypes.size(); ipm++)
              Arr.append(PositionsAnglesTypes[ipm].writeToJArrayAutoZ());
            js["PositionsTypes"] = Arr;
            break;
          }
        case 2:
          {
            QJsonArray Arr;
            for (int ipm=0; ipm<PositionsAnglesTypes.size(); ipm++)
              Arr.append(PositionsAnglesTypes[ipm].writeToJArrayFull());
            js["PositionsAnglesTypes"] = Arr;
            break;
          }
        default:
          qWarning()<<"Unknown array regularity type!";
        }
      if (!PositioningScript.isEmpty()) js["PositioningScript"] = PositioningScript;
    }
  json["PMarrayData"] = js;
}

bool APmArrayData::readFromJson(QJsonObject &json)
{
  if (!json.contains("PMarrayData")) return false;

  QJsonObject js = json["PMarrayData"].toObject();
  parseJson(js, "Activated", fActive);

  parseJson(js, "Regularity", Regularity);
  parseJson(js, "PMtype", PMtype);
  parseJson(js, "Packing", Packing);
  parseJson(js, "CenterToCenter", CenterToCenter);
  parseJson(js, "UseRings", fUseRings);
  parseJson(js, "NumRings", NumRings);
  parseJson(js, "NumX", NumX);
  parseJson(js, "NumY", NumY);
  if (Regularity == 1)
    {
      if (!js.contains("PositionsTypes"))
        {
          qWarning() << "Missing positions xy data!";
          return false;
        }
      QJsonArray Parr = QJsonArray();
      parseJson(js, "PositionsTypes", Parr);
      PositionsAnglesTypes.resize(0);
      for (int i=0; i<Parr.size(); i++)
        {
          QJsonArray el = Parr[i].toArray();
          PositionsAnglesTypes.append( APmPosAngTypeRecord() );
          PositionsAnglesTypes.last().readFromJArrayAutoZ(el);
        }
    }
  if (Regularity==2)
    {
      if (!js.contains("PositionsAnglesTypes"))
        {
          qWarning() << "Missing PM position/angle/type data!";
          return false;
        }
      QJsonArray Parr = QJsonArray();
      parseJson(js, "PositionsAnglesTypes", Parr);
      PositionsAnglesTypes.resize(0);
      for (int i=0; i<Parr.size(); i++)
        {
          QJsonArray el = Parr[i].toArray();
          PositionsAnglesTypes.append( APmPosAngTypeRecord() );
          PositionsAnglesTypes.last().readFromJArrayFull(el);
        }
    }
  parseJson(js, "PositioningScript", PositioningScript);
  //qDebug() << "--> Loaded PM array with regularity"<<Regularity;
  return true;
}

