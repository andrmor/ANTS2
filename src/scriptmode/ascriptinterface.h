#ifndef ASCRIPTINTERFACE_H
#define ASCRIPTINTERFACE_H

#include <QString>
#include <QHash>
#include <QObject>

class AScriptInterface : public QObject
{
  Q_OBJECT

public:
  AScriptInterface() : QObject() {}
  AScriptInterface(const AScriptInterface& other) : QObject(), H(other.H), Description(other.Description), bGuiThread(false) {}

  virtual bool InitOnRun() {return true;}   // automatically called before script evaluation
  virtual void ForceStop() {}               // called when abort was triggered - used to e.g. abort simulation or reconstruction

  virtual bool IsMultithreadCapable() const {return false;}

  const QString getDescription() const
      {return Description + (IsMultithreadCapable()?"\nMultithread-capable":"");}  // description text for the unit in GUI

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

protected:
  QHash<const QString, QString> H;
  QString Description;
  bool bGuiThread = true;

  void abort(QString message = "Aborted!") {emit AbortScriptEvaluation(message);}
};

#endif // ASCRIPTINTERFACE_H
