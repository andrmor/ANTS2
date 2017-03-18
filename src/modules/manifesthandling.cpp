#include "manifesthandling.h"

#include <QDebug>
#include <QFile>

#include "TEllipse.h"
#include "TPolyLine.h"
#include "TObject.h"

bool ManifestItemBaseClass::extractBaseProperties(QString item)
{
  QRegExp rx("(\\ |\\t)");
  QStringList fields = item.split(rx, QString::SkipEmptyParts);
  if (fields.size()<3)
    {
      qWarning() << "Unexpected length of manifest item line";
      return false;
    }

  FileName = fields[0];
  bool ok;
  X = fields[1].toDouble(&ok);
  if (!ok)
    {
      qWarning() << "Bad format of manifest base item line";
      return false;
    }
  Y = fields[2].toDouble(&ok);
  if (!ok)
    {
      qWarning() << "Bad format of manifest base item line";
      return false;
    }

  if (fields.size()>3)
    {
      LimitEvents = fields[3].toInt(&ok);
      if (!ok)
        {
          qWarning() << "Bad format of manifest base item line";
          return false;
        }
    }
  else LimitEvents = -1;

  return true;
}

bool ManifestItemHoleClass::extractProperties(QString item)
{
  bool ok = ManifestItemBaseClass::extractBaseProperties(item);
  if (!ok) return false;

  //qDebug() << "Extracted hole item with Name,X(raw),Y(raw),Diameter:" << FileName << X << Y << Diameter;
  return true;
}

TObject *ManifestItemHoleClass::makeTObject()
{
  TEllipse* e = new TEllipse(X, Y, 0.5*Diameter, 0.5*Diameter);
  e->SetFillStyle(0);
  e->SetLineColor(LineColor);
  e->SetLineWidth(LineWidth);
  e->SetLineStyle(LineStyle);
  return e;
}

bool ManifestItemSlitClass::extractProperties(QString item)
{
  bool ok = ManifestItemBaseClass::extractBaseProperties(item);
  if (!ok) return false;

  //qDebug() << "Extracted slit item with Name,Xcenter(raw),Ycenter(raw),Angle,Width:" << FileName << X << Y << Angle << Width;
  return true;
}

TObject *ManifestItemSlitClass::makeTObject()
{
  Double_t l = 0.5*Length;
  Double_t w = 0.5*Width;
  Double_t cX[5] = {-l, l,  l, -l, -l}, XX[5];
  Double_t cY[5] = { w, w, -w, -w,  w}, YY[5];

  Double_t angle = 3.1415926*Angle/180.0;
  Double_t sinA = sin(angle);
  Double_t cosA = cos(angle);
  for (int i=0; i<5; i++)
    {
      XX[i] = cX[i]*cosA - cY[i]*sinA + X;
      YY[i] = cX[i]*sinA + cY[i]*cosA + Y;
    }

  TPolyLine *p = new TPolyLine(5,XX,YY);
  p->SetFillStyle(0);
  p->SetLineColor(LineColor);
  p->SetLineWidth(LineWidth);
  p->SetLineStyle(LineStyle);
  return p;
}

ManifestProcessorClass::ManifestProcessorClass(QString ManifestFile)
{
  this->ManifestFile = ManifestFile;
  ManifestItem = 0;
  X0 = Y0 = 0;
  dX = dY = 1.0;
  Type = "hole";
  Diameter = 1.0;
  Angle = 0;
  Width = 1.0;
  Length = 30.0;
}

ManifestItemBaseClass *ManifestProcessorClass::makeManifestItem(QString FileName, QString& ErrorMessage)
{
  QFile file(ManifestFile);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      ErrorMessage = "Could not open manifest file!";
      qWarning() << ErrorMessage;
      return 0;
    }
  QTextStream in(&file);
  QRegExp rx("(\\ |\\t)");

  bool fFound = false;
  while(!in.atEnd())
   {
     QString line = in.readLine().trimmed();
     //qDebug() << line;
     QString tmp = line;
     bool ok;

     if ( line.startsWith("//"))
       {
         //this is a comment line, skip
         continue;
       }

     //check for control lines - they start from "#"
     //the latest control of the same type found before the item with FileName matching the target one will be used!
     if ( line.startsWith("#") )
       {
         //this is control line
         line.remove("#");
         QStringList fields = line.split(rx, QString::SkipEmptyParts);
         if (fields.size()<2)
           {
             ErrorMessage = "Bad control line in manifest line:\n"+tmp;
             qWarning() << ErrorMessage;
             return 0;
           }
         if (fields[0] == "Type" || fields[0] == "type") {Type = fields[1]; ok = true;}
         else if (fields[0] == "X0" || fields[0] == "x0" ) X0 = fields[1].toDouble(&ok);
         else if (fields[0] == "Y0" || fields[0] == "y0" ) Y0 = fields[1].toDouble(&ok);
         else if (fields[0] == "dX" || fields[0] == "dx" ) dX = fields[1].toDouble(&ok);
         else if (fields[0] == "dY" || fields[0] == "dy" ) dY = fields[1].toDouble(&ok);
         else if (fields[0] == "Diameter" || fields[0] == "diameter" ) Diameter = fields[1].toDouble(&ok);
         else if (fields[0] == "Angle" || fields[0] == "angle" ) Angle = fields[1].toDouble(&ok);
         else if (fields[0] == "Length" || fields[0] == "length" ) Length = fields[1].toDouble(&ok);
         else if (fields[0] == "Width" || fields[0] == "width" ) Width = fields[1].toDouble(&ok);
         else
           {
             ErrorMessage = "Unknown control tag at manifest line:\n" + tmp;
             qWarning() << ErrorMessage;
             return 0;
           }

         if (!ok)
           {
             ErrorMessage = "Format error in manifest line:\n" + tmp;
             qWarning() << ErrorMessage;
             return 0;
           }
         continue;
       }

     //this is not a control line
     QStringList fields = line.split(rx, QString::SkipEmptyParts);
     if (fields.isEmpty()) continue; //empty line

     if (fields.size()<3)
       {
         ErrorMessage = "Bad format (too short) of manifest item:\n" + line;
         qWarning() << ErrorMessage;
         return 0;
       }
     if (fields[0] != FileName) continue; //wrong file name, continue the search

     //found the proper line     
     if (Type == "hole") ManifestItem = new ManifestItemHoleClass(Diameter);
     else if (Type == "slit") ManifestItem = new ManifestItemSlitClass(Angle, Length, Width);
     else
       {
         ErrorMessage = "Attempt to use unknown type of manifest item ("+Type+") at line:\n"+line;
         qWarning() << ErrorMessage;
         return 0;
       }

     ok = ManifestItem->extractProperties(line);
     if (!ok)
       {
         ErrorMessage = "Failed to make manifestItem for line:\n"+line;
         qWarning() << ErrorMessage;
         delete ManifestItem;
         return 0;
       }
     fFound = true;
     break;
    }

  if (!fFound)
    {
      ErrorMessage = "Manifest file does not contain a line with the file name:\n" + FileName;
      qWarning() << ErrorMessage;
      return 0;
    }
  calculateCoordinates(); //shift to true coordinates

  return ManifestItem;
}

void ManifestProcessorClass::calculateCoordinates()
{
  ManifestItem->X = X0 + ManifestItem->X*dX;
  ManifestItem->Y = Y0 + ManifestItem->Y*dY;
}
