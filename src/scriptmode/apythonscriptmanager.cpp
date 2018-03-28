#include "PythonQt.h"
#include "PythonQt_QtAll.h"

#include "apythonscriptmanager.h"
#include "coreinterfaces.h"

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
      p.setNewRef(PyRun_String(Script.toLatin1().data(), Py_file_input, dict, dict));//Py_single_input
    }

  if (!p)
    {
      handleError();
    }

  fEngineIsRunning = false;

  emit onFinish("");

  return "";
}

void APythonScriptManager::handleError()
{
//    PyObject *ptype;
//    PyObject *pvalue;
//    PyObject *ptraceback;
//    PyErr_Fetch( &ptype, &pvalue, &ptraceback);

//      Py_XDECREF(ptype);
//      Py_XDECREF(pvalue);
//      Py_XDECREF(ptraceback);

    PythonQt::self()->handleError();
    return;

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
            //full_backtrace = NULL;

            fEngineIsRunning = false;
            return "";
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

    bool flag = false;
    if (PyErr_Occurred()) {

      if (_p->_systemExitExceptionHandlerEnabled &&
          PyErr_ExceptionMatches(PyExc_SystemExit)) {
        int exitcode = custom_system_exit_exception_handler();
        Q_EMIT PythonQt::self()->systemExitExceptionRaised(exitcode);
        }
      else
        {
        // currently we just print the error and the stderr handler parses the errors
        PyErr_Print();


        // EXTRA: the format of the ptype and ptraceback is not really documented, so I use PyErr_Print() above
//        PyObject *ptype;
//        PyObject *pvalue;
//        PyObject *ptraceback;
//        PyErr_Fetch( &ptype, &pvalue, &ptraceback);

//          Py_XDECREF(ptype);
//          Py_XDECREF(pvalue);
//          Py_XDECREF(ptraceback);

        PyErr_Clear();
        }
      flag = true;
    }
    _p->_hadError = flag;
    return flag;

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

    if (!p)
      {
        handleError();
      }

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
