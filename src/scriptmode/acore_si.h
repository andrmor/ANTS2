#ifndef ACORESCRIPTINTERFACE_H
#define ACORESCRIPTINTERFACE_H

#include "ascriptinterface.h"

#include <QVariant>
#include <QSet>
#include <QString>

class AScriptManager;
class CurveFit;

class ACore_SI : public AScriptInterface
{
  Q_OBJECT

public:
  explicit ACore_SI(AScriptManager *ScriptManager);
  explicit ACore_SI(const ACore_SI& other);

  virtual bool IsMultithreadCapable() const override {return true;}

  void SetScriptManager(AScriptManager *NewScriptManager) {ScriptManager = NewScriptManager;}

public slots:
  //abort execution of the script
  void abort(QString message = "Aborted!");

  QVariant evaluate(QString script);

  //time
  void          sleep(int ms);
  int           elapsedTimeInMilliseconds();

  //output part of the script window
  //void print(QString text);
  void print(QVariant message);
  void printHTML(QVariant message);
  void clearText();
  QString str(double value, int precision);
  bool strIncludes(QString str, QString pattern);

  //time stamps
  QString GetTimeStamp();
  QString GetDateTimeStamp();

  //save to file
  bool createFile(QString fileName, bool AbortIfExists = true);
  bool isFileExists(QString fileName);
  bool deleteFile(QString fileName);
  bool createDir(QString path);
  QString getCurrentDir();
  bool setCirrentDir(QString path);
  bool save(QString fileName, QString str);
  bool saveArray(QString fileName, QVariant array);
  bool saveObject(QString FileName, QVariant Object, bool CanOverride);

  //load from file
  QVariant loadColumn(QString fileName, int column = 0); //load column of doubles from file and return it as an array
  QVariant loadArray(QString fileName, int columns);
  QVariant loadArray(QString fileName);
  QVariantList loadArrayExtended(const QString & fileName, const QVariantList & format, int fromLine = 0, int untilLine = 1e6, bool bSkipComments = true);
  QVariantList loadArrayExtended3D(const QString & fileName, const QString & topSeparator, const QVariantList & format, int recordsFrom = 0, int recordsUntil = 1e6, bool bSkipComments = true);
  QVariantList loadArrayBinary(const QString & fileName, const QVariantList & format);
  QVariantList loadArrayExtended3Dbinary(const QString &fileName, char dataId, const QVariantList &dataFormat, char separatorId, const QVariantList &separatorFormat, int recordsFrom = 0, int recordsUntil = 1e6);
  QString  loadText(QString fileName);
  QVariant loadObject(QString fileName);

  QVariant loadArrayFromWeb(QString url, int msTimeout = 3000);

  //dirs
  QString GetWorkDir();
  QString GetScriptDir();
  QString GetExamplesDir();

  //file finder
  QVariant SetNewFileFinder(const QString dir, const QString fileNamePattern);
  QVariant GetNewFiles();

  //misc
  void processEvents();
  void reportProgress(int percents);

  void setCurveFitter(double min, double max, int nInt, QVariant x, QVariant y);
  double getFitted(double x);
  const QVariant getFittedArr(const QVariant array);

  const QString StartExternalProcess(QString command, QVariant arguments, bool waitToFinish, int milliseconds);

private:
  AScriptManager* ScriptManager;

  //file finder
  QSet<QString>   Finder_FileNames;
  QString         Finder_Dir;
  QString         Finder_NamePattern = "*.*";

  CurveFit* CurF = 0;

  void addQVariantToString(const QVariant & var, QString & string) const;

};

#endif // ACORESCRIPTINTERFACE_H
