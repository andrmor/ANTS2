#include "acommonfunctions.h"
#include "amessage.h"

#include <QDebug>

#include "TMath.h"

bool NormalizeVector(double *arr)
{
    double Norm =0;
    for (int i=0;i<3;i++) Norm += arr[i]*arr[i];
    Norm = TMath::Sqrt(Norm);
    if (Norm < 1e-20)
      {
        // qDebug()<<"fail norm";
        message("Error normalizing to unit vector!");
        return false;
      }

    for (int i=0; i<3; i++) arr[i] = arr[i]/Norm;
    // qDebug()<<arr[0]<<arr[1]<<arr[2]<<Norm;
    return true;
}

double InteractionValue(double energy, QVector<double>* X, QVector<double>* F, bool LogLog)
{
//  qDebug()<<"data point in arrays X and F:"<<X->size()<<F->size()<<"Min X:"<<X->first()<<"Max X:"<<X->last();
  //if (degree == 1)   {

    if (energy < X->first())
    {
        qWarning()<<"Interpolation: value is out of the data range:"<<energy<< " < " << X->first();
        return F->first();
    }
    if (energy > X->last())
    {
        qWarning()<<"Interpolation: value is out of the data range:"<<energy<< " > " << X->last();
        return F->last();
    }

    //linear interpolation of IterationData for current energy
    QVector<double>::iterator it;
    it = qLowerBound(X->begin(), X->end(), energy);
    int index = X->indexOf(*it);
    //qDebug()<<"energy:"<<energy<<"index"<<index;//<<*it;
    if (index < 1)
    {
        //qWarning()<<"Interpolation: value out (or on the border) of the interaction data range!";
        return F->first();
    }

    //interpolating:
    double Less = F->at(index-1);
    double More = F->at(index);
    //qDebug()<<" Less/More"<<Less<<More;
    double EnergyLess = X->at(index-1);
    double EnergyMore = X->at(index);
    // qDebug()<<" Energy Less/More"<<EnergyLess<<EnergyMore;
    double InteractValue;
    if (EnergyLess == EnergyMore) {InteractValue = More;}   //*** ==  To change to safer
    else
      {
        if (LogLog)
          {
            double LogLess = log(Less);
            double LogMore = log(More);
            double LogEnergyLess = log(EnergyLess);
            double LogEnergyMore = log(EnergyMore);
            InteractValue = LogLess + (LogMore-LogLess)*(log(energy)-LogEnergyLess)/(LogEnergyMore-LogEnergyLess);
            InteractValue = exp(InteractValue);
          }
        else InteractValue = Less + (More-Less)*(energy-EnergyLess)/(EnergyMore-EnergyLess);
      }
//     qDebug()<<"energy / interValue"<<energy<<InteractValue;
    return InteractValue;
// }

  /*
  else
   {
     return PolyInterpolation(energy, X, F, degree+1);
   }
   */
}

/*
double PolyInterpolation(double x, QVector<double>* xi, QVector<double>* yi, int order)  //with degenerate nodes
{
  qDebug()<<"-----------";
  int isize = xi->size();

  QVector<double> lambda;
  lambda.resize(isize);
  double y;
  int j, is, il;

  // check order of interpolation
  if (order > isize) order = isize;

  // if x is ouside the xi[] interval
  if (x <= xi->at(0)) return yi->at(0);
  if (x >= xi->at(isize-1)) return yi->at(isize-1);

  // loop to find j so that x[j-1] < x < x[j]
  j = 0;
  while (j <= isize-1)
    {
      if (xi->at(j) >= x) break;
      j++;
    }
  qDebug()<<"x="<<x<<"start from"<<j;
  qDebug()<<"set size"<<isize<<"order"<<order;

//  // shift j to correspond to (order-1)th interpolation
//  j = j - order/2;
//  // if j is ouside of the range [0, ... isize-1]
//  if (j < 0) j=0;
//  if (j+order-1 > isize-1 ) j = isize-order;

    //tring to go left
  int leftBoundLimited = false;
  int leftToGo = order/2;
  int leftBound = j;
  for (int itmp=1; itmp<leftToGo+1; itmp++)
    {
      if (j-itmp<0)
        {
          leftBoundLimited = true;
          break;
        }
      if (xi->at(j-itmp) == xi->at(j-itmp+1))
        {
          //dgenerate node found
          leftBoundLimited = true;
          break;
        }
      leftBound--;
    }
   qDebug()<<"left bound"<<leftBound<<leftBoundLimited;
     //trying to go right
   int rightBoundLimited = false;
   int rightToGo = order-1;
   int rightBound = leftBound;
   for (int itmp=1; itmp<rightToGo+1; itmp++)
     {
       if (rightBound+1 > isize-1)
         {
           rightBoundLimited = true;
           break;
         }
       if (xi->at(rightBound+1) == xi->at(rightBound))
         {
           //degenerate node found
           rightBoundLimited = true;
           break;
         }
       rightBound++;
     }
   qDebug()<<"right bound"<<rightBound<<rightBoundLimited;
     //if rightBound and not leftBound, attempt to extend left
   if (rightBoundLimited && !leftBoundLimited)
     {
       qDebug()<< " attempt to correct left bound";
       leftToGo = order-1 - (rightBound-leftBound);
       qDebug()<<"  want to make"<<leftToGo<<"extra steps left";
       for (int itmp = 1; itmp<leftToGo+1; itmp++)
         {
           if (leftBound-1<0)
             {
               leftBoundLimited = true;
               break;
             }
           if (xi->at(leftBound-1) == xi->at(leftBound))
             {
               //dgenerate node found
               leftBoundLimited = true;
               break;
             }
           leftBound--;
         }
       qDebug()<<"left bound corrected"<<leftBound<<leftBoundLimited;
     }

   //check if have to adjust degree for this iteration
   if (order > rightBound-leftBound+1)
     {
       order = rightBound-leftBound+1;
       qDebug()<<"corrected order"<<order;
     }

//  y = 0.0;
//  for (is = j; is <= j+order-1; is = is+1)
//   {
//     lambda[is] = 1.0;
//     for (il = j; il <= j+ order-1; il++)
//        if(il != is)
//             lambda[is] = lambda[is]*(x - xi->at(il))/(xi->at(is) - xi->at(il));

//     y = y + yi->at(is)*lambda[is];
//   }

   y = 0.0;
   for (is = leftBound; is <= rightBound; is = is+1)
    {
      lambda[is] = 1.0;
      for (il = leftBound; il <= rightBound; il++)
         if(il != is)
              lambda[is] = lambda[is]*(x - xi->at(il))/(xi->at(is) - xi->at(il));

      y = y + yi->at(is)*lambda[is];
    }
  return y;
}
*/

void ConvertToStandardWavelengthes(QVector<double>* sp_x, QVector<double>* sp_y, double WaveFrom, double WaveStep, int WaveNodes, QVector<double>* y)
{
  y->resize(0);

  //qDebug()<<"Data range:"<<sp_x->at(0)<<sp_x->at(sp_x->size()-1);
  double xx, yy;
  int points = WaveNodes;
  for (int i=0; i<points; i++)
    {
      xx = WaveFrom + WaveStep*i;
      if (xx <= sp_x->at(0)) yy = sp_y->at(0);
      else
        {
          if (xx >= sp_x->at(sp_x->size()-1)) yy = sp_y->at(sp_x->size()-1);
          else
            {
              //general case
              yy = InteractionValue(xx, sp_x, sp_y); //reusing interpolation function
            }
        }
//      qDebug()<<xx<<yy;
      y->append(yy);
    }
}

bool ExtractNumbersFromQString(QString input, QList<int> *ToAdd)
{
  ToAdd->clear();

  QRegExp rx("(\\,|\\-)"); //RegEx for ' ' and '-'

  QStringList fields = input.split(rx, QString::SkipEmptyParts);

  if (fields.size() == 0 )
    {
      //message("Nothing to add!");
      return false;
    }

  fields = input.split(",", QString::SkipEmptyParts);
  //  qDebug()<<"found "<<fields.size()<<" records";

  for (int i=0; i<fields.size(); i++)
    {
      QString thisField = fields[i];

      //are there "-" separated fields?
      QStringList subFields = thisField.split("-", QString::SkipEmptyParts);
      //bool error = false;

      if (subFields.size() > 2 || subFields.size() == 0) return false;
      else if (subFields.size() == 1)
        {
          //just one number
          bool ok;
          int pm;
          pm = subFields[0].toInt(&ok);

          if (ok) ToAdd->append(pm);
          else return false;
        }
      else //range - two subFields
        {
          bool ok1, ok2;
          int pm1, pm2;
          pm1 = subFields[0].toInt(&ok1);
          pm2 = subFields[1].toInt(&ok2);
          if (ok1 && ok2)
            {
               if (pm2<pm1) return false;//error = true;
               else
                 {
                   for (int j=pm1; j<=pm2; j++) ToAdd->append(j);
                 }
            }
          else return false;//error = true;

        }
  /*
      if (error)
        {
          message("Input error!");
          return;
        }
 */
    }

  return true;
}

void UpdateMax(double &max, double value)
{
    if (value>max) max=value;
}

QPolygonF MakeClosedPolygon(double side, double angle, double x0, double y0)
{
    QPolygonF poly;
    for (int i=0; i<6; i++)
      {
        double phi = angle + 60.0*i;
        phi = phi/180.0*3.1415926536;
        poly << QPointF(x0+side*sin(phi), y0+side*cos(phi));
      }
    poly << QPointF(x0+side*sin(angle/180.0*3.1415926536), y0+side*cos(angle/180.0*3.1415926536)); //to close it
    return poly;
}
