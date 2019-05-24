#ifndef ACONFIG_SI_H
#define ACONFIG_SI_H

#include "ascriptinterface.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector>

class AConfiguration;
class QJsonObject;
class QJsonValue;

class AConfig_SI : public AScriptInterface
{
  Q_OBJECT

public:
  AConfig_SI(AConfiguration* config);
  AConfig_SI(const AConfig_SI &other);
  ~AConfig_SI(){}

  virtual bool IsMultithreadCapable() const override {return true;}

  AConfiguration* Config;
  QString LastError;

public slots:
  bool Load(QString FileName);
  bool Save(QString FileName);

  const QVariant GetConfig() const;
  bool SetConfig(const QVariant& conf);

  void ExportToGDML(QString FileName);
  void ExportToROOT(QString FileName);

  bool Replace(QString Key, QVariant val);
  QVariant GetKeyValue(QString Key);

  QString GetLastError();

  void RebuildDetector();
  void UpdateGui();

signals:
  void requestReadRasterGeometry();

private:
  bool modifyJsonValue(QJsonObject& obj, const QString& path, const QJsonValue& newValue);
  void find(const QJsonObject &obj, QStringList Keys, QStringList& Found, QString Path = "");
  bool keyToNameAndIndex(QString Key, QString &Name, QVector<int> &Indexes);
  bool expandKey(QString &Key);

};

#endif // ACONFIG_SI_H
