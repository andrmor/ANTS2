#include "ajsontools.h"
#include "amessage.h"

#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QJsonDocument>
#include <QDebug>
#include <QFile>

#include "TH1D.h"
#include "TH1I.h"

bool parseJson(const QJsonObject &json, const QString &key, bool &var)
{
  if (json.contains(key))
    {
      var = json[key].toBool();
      return true;
    }
  else return false;
}
bool parseJson(const QJsonObject &json, const QString &key, int &var)
{
  if (json.contains(key))
    {
      //var = json[key].toInt();
      double val = json[key].toDouble();
      var = std::round(val);
      return true;
    }
  else return false;
}
bool parseJson(const QJsonObject &json, const QString &key, double &var)
{
  if (json.contains(key))
    {
      var = json[key].toDouble();
      return true;
    }
  else return false;
}
bool parseJson(const QJsonObject &json, const QString &key, float &var)
{
  if (json.contains(key))
    {
      var = json[key].toDouble();
      return true;
    }
  else return false;
}
bool parseJson(const QJsonObject &json, const QString &key, QString &var)
{
  if (json.contains(key))
    {
      var = json[key].toString();
      return true;
    }
  else return false;
}

bool parseJson(const QJsonObject &json, const QString &key, QJsonArray &var)
{
  if (json.contains(key))
    {
      var = json[key].toArray();
      return true;
    }
  else return false;
}

bool parseJson(const QJsonObject &json, const QString &key, QJsonObject &obj)
{
    if (json.contains(key))
      {
        obj = json[key].toObject();
        return true;
      }
    else return false;
}

void JsonToCheckbox(QJsonObject &json, QString key, QCheckBox *cb)
{
  if (json.contains(key))
    cb->setChecked(json[key].toBool());
}

void JsonToSpinBox(QJsonObject &json, QString key, QSpinBox *sb)
{
  if (json.contains(key))
    {
       double val = json[key].toDouble();
       int ival = std::round(val);
       sb->setValue(ival);
    }
}

void JsonToLineEditDouble(QJsonObject &json, QString key, QLineEdit *le)
{
  if (json.contains(key))
    le->setText( QString::number(json[key].toDouble()) );
}

void JsonToLineEditText(QJsonObject &json, QString key, QLineEdit *le)
{
    if (json.contains(key))
      le->setText( json[key].toString() );
}

void JsonToComboBox(QJsonObject &json, QString key, QComboBox *qb)
{
  if (json.contains(key))
    {
      int index = json[key].toInt();
      if (index >= qb->count()) index = -1;
      qb->setCurrentIndex(index);
    }
}

bool LoadJsonFromFile(QJsonObject &json, QString fileName)
{
    QFile loadFile(fileName);
    if (loadFile.open(QIODevice::ReadOnly))
      {
        QByteArray saveData = loadFile.readAll();
        QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
        json = loadDoc.object();
        loadFile.close();
        //qDebug()<<"  Loaded Json from file:"<<fileName;
      }
    else
      {
        message("Cannot open file: "+fileName);
        return false;
      }
    return true;
}

bool SaveJsonToFile(QJsonObject &json, QString fileName)
{
  QJsonDocument saveDoc(json);

  QFile saveFile(fileName);
  if (saveFile.open(QIODevice::WriteOnly))
    {
      saveFile.write(saveDoc.toJson());
      saveFile.close();
      //  qDebug()<<"  Saved Json to file:"<<fileName;
    }
  else
    {
      message("Couldn't save json to file: "+fileName);
      return false;
    }

  return true;
}

bool writeTH1ItoJsonArr(TH1I* hist, QJsonArray &ja)
{
  if (hist)
    {
      for (int i=1; i<hist->GetSize(); i++)
        {
          QJsonArray el;
          el.append(hist->GetBinLowEdge(i));
          el.append(hist->GetBinContent(i));
          ja.append(el);
        }
      return true;
    }
  else return false;
}

bool writeTH1DtoJsonArr(TH1D* hist, QJsonArray &ja)
{
  if (hist)
    {
      for (int i=1; i<hist->GetSize(); i++)
        {
          QJsonArray el;
          el.append(hist->GetBinLowEdge(i));
          el.append(hist->GetBinContent(i));
          ja.append(el);
        }
      return true;
    }
  else return false;
}

bool writeTwoQVectorsToJArray(QVector<double> &x, QVector<double> &y, QJsonArray &ar)
{
  if (x.size() != y.size())
    {
      qWarning() << "Vectors mismatch!";
      return false;
    }

  for (int i=0; i<x.size(); i++)
    {
      QJsonArray el;
      el.append(x[i]);
      el.append(y[i]);
      ar.append(el);
    }
  return true;
}

void readTwoQVectorsFromJArray(QJsonArray &ar, QVector<double> &x, QVector<double> &y)
{
  for (int i=0; i<ar.size(); i++)
    {
      double X = ar[i].toArray()[0].toDouble();
      x.append(X);
      double Y = ar[i].toArray()[1].toDouble();
      y.append(Y);
    }
}

bool write2DQVectorToJArray(QVector<QVector<double> > &xy, QJsonArray &ar)
{
  for (int i1=0; i1<xy.size(); i1++)
    {
      QJsonArray el;
      for (int i2=0; i2<xy[i1].size(); i2++) el.append(xy[i1][i2]);
      ar.append(el);
    }
  return true;
}

void read2DQVectorFromJArray(QJsonArray &ar, QVector<QVector<double> > &xy)
{
  xy.resize(ar.size());
  for (int i1=0; i1<xy.size(); i1++)
    {
      QJsonArray el = ar[i1].toArray();
      for (int i2=0; i2<el.size(); i2++)
        xy[i1].append( el[i2].toDouble());
    }
}

bool isContainAllKeys(QJsonObject json, QStringList keys)
{
    for (QString key : keys)
        if (!json.contains(key)) return false;
    return true;
}
