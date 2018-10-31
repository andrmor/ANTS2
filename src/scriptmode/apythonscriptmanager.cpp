#include "PythonQt.h"
//#include "PythonQt_QtAll.h"

#include "apythonscriptmanager.h"
#include "ascriptinterface.h"
#include "acorescriptinterface.h"

APythonScriptManager::APythonScriptManager(TRandom2 *RandGen) :
  AScriptManager(RandGen)
{
  PythonQt::init(PythonQt::RedirectStdOut);
  //PythonQt_QtAll::init();

  connect(PythonQt::self(), SIGNAL(pythonStdOut(const QString&)), this, SLOT(stdOut(const QString&)));
  connect(PythonQt::self(), SIGNAL(pythonStdErr(const QString&)), this, SLOT(stdErr(const QString&)));
}

/*
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
      QString mathName = "MATH";
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
*/

void APythonScriptManager::RegisterInterfaceAsGlobal(AScriptInterface *)
{
    qDebug() << "Registering as global is not implemented for python scripting!";
}

void APythonScriptManager::RegisterCoreInterfaces(bool bCore, bool bMath)
{
    if (bCore)
    {
        ACoreScriptInterface* coreObj = new ACoreScriptInterface(this);
        RegisterInterface(coreObj, "core");
    }

    if (bMath)
    {
        QObject* mathObj = new AInterfaceToMath(RandGen);
        RegisterInterface(mathObj, "MATH");
    }
}

void APythonScriptManager::RegisterInterface(AScriptInterface *interface, const QString &name)
{
    PythonQt::self()->addObject(PythonQt::self()->getMainModule(), name, interface);
    interface->setObjectName(name);
    interfaces.append(interface);
    QObject::connect(interface, &AScriptInterface::AbortScriptEvaluation, this, &APythonScriptManager::AbortEvaluation);
}

QString APythonScriptManager::Evaluate(const QString &Script)
{
  LastError.clear();
  fAborted = false;

  emit onStart();

  fEngineIsRunning = true;

  PythonQtObjectPtr mainModule = PythonQt::self()->getMainModule();
  //_PyModule_Clear(mainModule);

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
      //p.setNewRef(PyRun_String(Script.toLatin1().data(), Py_file_input, dict, dict));//Py_single_input

      //dict = PyDict_Copy(dict);
      //p.setNewRef(PyRun_String(Script.toLatin1().data(), Py_file_input, dict, dict));

      GlobalDict.setNewRef(PyDict_Copy(dict)); //decref for the old one (if not NULL)
      //should do Py_XINCREF(GlobalDict.object())? seems not - no memory leak
      //do not do decref for dict -> crash
      p.setNewRef(PyRun_String(Script.toLatin1().data(), Py_file_input, GlobalDict, GlobalDict));
    }

  if (!p) handleError();

  fEngineIsRunning = false;

  emit onFinish("");

  return "";
}

void APythonScriptManager::handleError()
{
      PythonQt::self()->handleError();

  /*

      // Work in progress!
      // no guarantie that direct conversion of traceback will work with other versions
      //does not report error in attempt to import non-existing module, while StdErr shows it properly

      PyObject *ptype = NULL, *pvalue = NULL, *ptraceback = NULL;
      PyErr_Fetch(&ptype, &pvalue, &ptraceback);

      QString str = "Error";

      if (pvalue && PyString_Check(pvalue)) str += QString(": ") + PyString_AsString(pvalue);
      else str += ": Unknown error type";
      qDebug() << str;

      if (ptraceback)
        {
           PyTracebackObject* traceback = (PyTracebackObject*)ptraceback; //dynamic cast does not work - not polymorphic type
           int line = traceback->tb_lineno;
           qDebug() << "Error in line #:"<<line;
           requestHighlightErrorLine(line);
           str += QString("<br>in line #" + QString::number(line));
        }

      Py_XDECREF(ptype);
      Py_XDECREF(pvalue);
      Py_XDECREF(ptraceback);

      PyErr_Clear();

      AbortEvaluation(str);

 */


      /*
    PyObject* err = PyErr_Occurred();
    if (err != NULL) {
        PyObject *ptype, *pvalue, *ptraceback;
        PyObject *pystr, *module_name, *pyth_module, *pyth_func;
        char *str;

        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        pystr = PyObject_Str(pvalue);
        str = PyString_AsString(pystr);
        QString error_description = strdup(str);
        qDebug() << "error descr:"<<error_description;

        // See if we can get a full traceback
        module_name = PyString_FromString("traceback");
        pyth_module = PyImport_Import(module_name);
        Py_DECREF(module_name);

        if (pyth_module == NULL) {
            full_backtrace = NULL;
            return;
        }

        pyth_func = PyObject_GetAttrString(pyth_module, "format_exception");
        if (pyth_func && PyCallable_Check(pyth_func)) {
            PyObject *pyth_val;

            pyth_val = PyObject_CallFunctionObjArgs(pyth_func, ptype, pvalue, ptraceback, NULL);

            pystr = PyObject_Str(pyth_val);
            str = PyString_AsString(pystr);
            QString full_backtrace = strdup(str);
            qDebug() << "full backtrace:"<<full_backtrace;
            Py_DECREF(pyth_val);
        }
    }
    */
}

QVariant APythonScriptManager::EvaluateScriptInScript(const QString &script)
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
        p.setNewRef(PyRun_String(script.toLatin1().data(), Py_file_input, dict, dict));//Py_single_input
      }

    if (!p) handleError();

    return "";
}

void APythonScriptManager::abortEvaluation()
{
  // no need to do anything - python interpreter already stopped evaluation
}

void APythonScriptManager::stdOut(const QString &s)
{
  if (s == "\n") return;
  //    qDebug() << "StdOut>>>" << s;
  emit showMessage(s);
}

void APythonScriptManager::stdErr(const QString &s)
{
  QString str = s.simplified();
  str.remove("\n");
  //    qDebug() << "ErrOut>>>" << str;

  AbortEvaluation("Aborted!");

  if (str.isEmpty()) return;
  if (str.startsWith("Traceback")) return;
  if (str == ":") return;
  if (str == "^") return;
  if (str == "<string>") return;
  if (str == "File \"") return;
  if (str == "\", line") return;

  int lineNumber = -1;
  bool bOK;
  if (str.startsWith("File \"<string>\", "))
  {
      str.remove("File \"<string>\", ");
      str.remove(", in <module>");
      QStringList sl = str.split(" ", QString::SkipEmptyParts);
      if (sl.size() > 1)
          if (sl.first() == "line")
          {
              QString lns = sl.at(1);
              lineNumber = lns.toInt(&bOK);
              if (bOK)
              {
                  qDebug() << "Error line number:"<<lineNumber;
                  requestHighlightErrorLine(lineNumber);
              }
          }
  }

  lineNumber = str.toInt(&bOK);
  if (bOK)
  {
      str = "line " + str;
      requestHighlightErrorLine(lineNumber);
  }

  QString message = "<font color=\"red\">"+ str +"</font>";
  //    qDebug() << "---->"<<message;
  emit showMessage(message);
}
