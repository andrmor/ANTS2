#ifndef ASCRIPTINTERFACE_H
#define ASCRIPTINTERFACE_H

#include <QString>
#include <QHash>
#include <QObject>

class AScriptInterface : public QObject
{
  Q_OBJECT

public:
  AScriptInterface() {}
  virtual bool InitOnRun() {return true;}   // automatically called before script evaluation
  virtual void ForceStop() {}               // called when abort was triggered - used to e.g. abort simulation or reconstruction

  QString Description;  // used as help for the unit
  QString UnitName;     // used as default name of the unit

public slots:
  const QString help(QString method) const  //automatically requested to obtain help strings
  {
    if (method.endsWith("()")) method.remove("()");
    if (method.endsWith("(")) method.remove("(");
    if (!H.contains(method)) return "";
    return H[method];
  }

signals:
  void AbortScriptEvaluation(QString);      //abort request is automatically linked to abort slot of core unit
  void PrintText(QString);

protected:
  QHash<QString, QString> H;

  void abort(QString message = "Aborted!") {emit AbortScriptEvaluation(message);}
  void requestPrint(QString text) {emit PrintText(text);} // print this text using "core" module
};

#endif // ASCRIPTINTERFACE_H
