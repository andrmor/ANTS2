#include "apythonscriptmanager.h"
#include "coreinterfaces.h"

#include "PythonQt.h"
#include "PythonQt_QtAll.h"

APythonScriptManager::APythonScriptManager(TRandom2 *RandGen) :
  AScriptManager(RandGen)
{
  PythonQt::init(PythonQt::RedirectStdOut);
  PythonQt_QtAll::init();

  connect(PythonQt::self(), SIGNAL(pythonStdOut(const QString&)), this, SLOT(stdOut(const QString&)));
  connect(PythonQt::self(), SIGNAL(pythonStdErr(const QString&)), this, SLOT(stdErr(const QString&)));
}

void APythonScriptManager::SetInterfaceObject(QObject *interfaceObject, QString name)
{
  //qDebug() << "Registering:" << interfaceObject << name;

  if (name.isEmpty())
    { // empty name means the main module
      // registering service object
      AInterfaceToCore* coreObj = new AInterfaceToCore(this);
      QString coreName = "core";
      coreObj->setObjectName(coreName);
      PythonQt::self()->addObject(PythonQt::self()->getMainModule(), coreName, coreObj);
      interfaces.append(coreObj);
      //registering math module
      QObject* mathObj = new AInterfaceToMath(RandGen);
      QString mathName = "math";
      mathObj->setObjectName(mathName);
      PythonQt::self()->addObject(PythonQt::self()->getMainModule(), mathName, mathObj);
      interfaces.append(mathObj);
    }

  if (interfaceObject)
    {
      interfaceObject->setObjectName(name);
      PythonQt::self()->addObject(PythonQt::self()->getMainModule(), name, interfaceObject);
      interfaces.append(interfaceObject);

      //connecting abort request from main interface to serviceObj
      int index = interfaceObject->metaObject()->indexOfSignal("AbortScriptEvaluation(QString)");
      if (index != -1)
          QObject::connect(interfaceObject, "2AbortScriptEvaluation(QString)", this, SLOT(AbortEvaluation(QString)));  //1-slot, 2-signal
    }

}

QString APythonScriptManager::Evaluate(const QString &Script)
{
  PythonQtObjectPtr mainModule = PythonQt::self()->getMainModule();

  PythonQtObjectPtr p;
  PyObject* dict = NULL;
  if (PyModule_Check(mainModule))
    {
      dict = PyModule_GetDict(mainModule);
    }
  else if (PyDict_Check(mainModule))
    {
      dict = mainModule;
    }
  if (dict)
    {
      p.setNewRef(PyRun_String(Script.toLatin1().data(), Py_file_input, dict, dict));//Py_single_input
    }

  if (!p)
    {
      PythonQt::self()->handleError();
    }
  else
    {

    }

  return "";
}

QVariant APythonScriptManager::EvaluateScriptInScript(const QString & /*script*/)
{
  return "Not yet implemented";
}

void APythonScriptManager::abortEvaluation()
{
  qDebug() << "Not implemented!";
}

void APythonScriptManager::stdOut(const QString &s)
{
  //  QString _stdOut = s;
  //  int idx;
  //  while ((idx = _stdOut.indexOf('\n'))!=-1) {
  //    consoleMessage(_stdOut.left(idx));
  //    std::cout << _stdOut.left(idx).toLatin1().data() << std::endl;
  //    _stdOut = _stdOut.mid(idx+1);
  //  }

  if (s == "\n") return;
  //ui->pteOut->appendPlainText(s);
  qDebug() << "StdOut>>>" << s;
}

void APythonScriptManager::stdErr(const QString &s)
{
  //  _hadError = true;
  //  _stdErr += s;
  //  int idx;
  //  while ((idx = _stdErr.indexOf('\n'))!=-1) {
  //    consoleMessage(_stdErr.left(idx));
  //    std::cerr << _stdErr.left(idx).toLatin1().data() << std::endl;
  //    _stdErr = _stdErr.mid(idx+1);
  //  }

  if (s == "\n") return;
  if (s.startsWith("Traceback")) return;
  if (s.startsWith("  File")) return;
  if (s.startsWith("NameError")) return;
  if (s == ": ") return;

  //ui->pteOut->appendPlainText(s);
  qDebug() << "ErrOut>>>" << s;
}
