#include "ascriptmanager.h"
#include "ascriptinterface.h"
#include "ascriptinterface.h"

#ifdef GUI
#include "amsg_si.h"
#endif

#include <QDebug>
#include <QElapsedTimer>
#include <QMetaMethod>
#include <QFile>

#include "TRandom2.h"

AScriptManager::AScriptManager(TRandom2 *RandGen)
    : RandGen(RandGen) {}

AScriptManager::~AScriptManager()
{
    for (int i = 0; i < interfaces.size(); i++) delete interfaces[i];
    interfaces.clear();

    delete timer;

    if (bOwnRandomGen) delete RandGen;
}

#ifdef GUI
void AScriptManager::hideMsgDialogs()
{
    for (int i=0; i<interfaces.size(); i++)
    {
        AMsg_SI* t = dynamic_cast<AMsg_SI*>(interfaces[i]);
        if (t) t->HideWidget();
    }
}

void AScriptManager::restoreMsgDialogs()
{
    for (int i=0; i<interfaces.size(); i++)
    {
        AMsg_SI* t = dynamic_cast<AMsg_SI*>(interfaces[i]);
        if (t) t->RestorelWidget();
    }
}

void AScriptManager::deleteMsgDialogs()
{
    for (int i=0; i<interfaces.size(); i++)
    {
        AMsg_SI* t = dynamic_cast<AMsg_SI*>(interfaces[i]);
        if (t)
        {
            // *** !!! t->deleteDialog(); //need by GenScriptWindow ?
            return;
        }
    }
}
#endif

qint64 AScriptManager::getElapsedTime()
{
  if (timer) return timer->elapsed();
  return timerEvalTookMs;
}

QString AScriptManager::getFunctionReturnType(const QString &UnitFunction)
{
  QStringList f = UnitFunction.split(".");
  if (f.size() != 2) return "";

  QString unit = f.first();
  int unitIndex = -1;
  for (int i=0; i<interfaces.size(); i++)
    if (interfaces.at(i)->objectName() == unit)
      {
        unitIndex = i;
        break;
      }
  if (unitIndex == -1) return "";

  //qDebug() << "Found unit"<<unit<<" with index"<<unitIndex;
  QString met = f.last();
  //qDebug() << met;
  QStringList skob = met.split("(", QString::SkipEmptyParts);
  if (skob.size()<2) return "";
  QString funct = skob.first();
  QString args = skob[1];
  args.chop(1);
  //qDebug() << funct << args;

  QString insert;
  if (!args.isEmpty())
    {
      QStringList argl = args.split(",");
      for (int i=0; i<argl.size(); i++)
        {
          QStringList a = argl.at(i).simplified().split(" ");
          if (!insert.isEmpty()) insert += ",";
          insert += a.first();
        }
    }
  //qDebug() << insert;

  QString methodName = funct + "(" + insert + ")";
  //qDebug() << "method name" << methodName;
  int mi = interfaces.at(unitIndex)->metaObject()->indexOfMethod(methodName.toLatin1().data());
  //qDebug() << "method index:"<<mi;
  if (mi == -1) return "";

  QString returnType = interfaces.at(unitIndex)->metaObject()->method(mi).typeName();
  //qDebug() << returnType;
  return returnType;
}

void AScriptManager::ifError_AbortAndReport()
{
    if (isUncaughtException())
    {
        int lineNum = getUncaughtExceptionLineNumber();
        QString err = getUncaughtExceptionString();
        //err += "<br>Line #" + QString::number(lineNum) + ", call originating from function: " + functionName;
        requestHighlightErrorLine(lineNum);
        AbortEvaluation(err);
    }
}

void AScriptManager::AbortEvaluation(QString message)
{
  //  qDebug() << "ScriptManager: Abort requested!"<<fAborted<<fEngineIsRunning;

  //if (fAborted || !fEngineIsRunning) return;
  if (fAborted)
  {
      //  qDebug() << "...ignoring, already aborted";
      return;
  }
  fAborted = true;

  abortEvaluation();
  fEngineIsRunning = false;

  // going through registered units and requesting abort
  for (int i=0; i<interfaces.size(); i++)
    {
      AScriptInterface* bi = dynamic_cast<AScriptInterface*>(interfaces[i]);
      if (bi) bi->ForceStop();
    }

  if (!message.isEmpty() && bShowAbortMessageInOutput)
  {
      message = "<font color=\"red\">"+ message +"</font><br>";
      emit showMessage(message);
  }
  emit onAbort();
}

bool AScriptManager::expandScript(const QString & OriginalScript, QString & ExpandedScript)
{
    ExpandedScript = OriginalScript;
    const int OriginalSize = OriginalScript.count('\n') + 1;

    LineNumberMapper.resize(OriginalSize + 1);
    LineNumberMapper[0] = -1;    // just paranoic, line number 0 will never be requested
    for (int i = 1; i <= OriginalSize; i++)
        LineNumberMapper[i] = i; // line numbers start from 1

    bool bWasExpanded;
    bool bInsideBlockComments;
    int iCycleCounter = 0;
    do
    {
        if ( !ExpandedScript.contains("#include") ) break;

        bWasExpanded = false;
        bInsideBlockComments = false;
        QVector<int> LineNumberMapperCopy = LineNumberMapper;
        int iInsertLine = 0;

        QString WorkScript;

        const QStringList SL = ExpandedScript.split('\n', QString::KeepEmptyParts);
        for (int iLine = 0; iLine < SL.size(); iLine++)
        {
            const QString Line = SL.at(iLine);

            if ( bInsideBlockComments || !Line.simplified().startsWith("#include") )
            {
                WorkScript += Line + '\n';
                iInsertLine++;
                updateBlockCommentStatus(Line, bInsideBlockComments);
            }
            else
            {
                const QStringList tmp = Line.split('\"', QString::KeepEmptyParts);
                if (tmp.size() < 3)
                {
                    LastError = "Format error in #include (could be in #include of included file)";
                    LastErrorLineNumber = LineNumberMapperCopy[iLine + 1];
                    return false;
                }
                const QString FileName = tmp.at(1);

                QFile file(FileName);
                if (!file.exists() || !file.open(QIODevice::ReadOnly | QFile::Text))
                {
                    LastError = "Cannot find or open file in #include (could be in #include of included file)";
                    LastErrorLineNumber = LineNumberMapperCopy[iLine + 1];
                    return false;
                }

                QTextStream in(&file);

                int LineCounter = 0;
                while(!in.atEnd())
                {
                    in.readLine();
                    LineCounter++;
                }
                in.seek(0);
                QString IncludedScript = in.readAll();
                file.close();
                WorkScript += IncludedScript + "\n";

                int old = LineNumberMapperCopy[iLine + 1];
                LineNumberMapper.insert(iInsertLine+1, LineCounter-1, old);
                //qDebug() << old << LineNumberMapperCopy << iLine << LineCounter;

                iInsertLine += LineCounter;
                bWasExpanded = true;
                bScriptExpanded = true;
            }
        }
        ExpandedScript = WorkScript;

        iCycleCounter++;
        //qDebug() << iCycleCounter;
        if (iCycleCounter > 100)
        {
            LastError = "Suspect infinite loop in #includes";
            return false;
        }
    }
    while (bWasExpanded);

    /*
    const QStringList SL = ExpandedScript.split('\n', QString::KeepEmptyParts);
    for (int iLine = 0; iLine < SL.size(); iLine++)
        qDebug() << iLine + 1 << " "<< SL.at(iLine);
    qDebug() << "-----\n"<< LineNumberMapper;
    */

    return true;
}

void AScriptManager::correctLineNumber(int & iLineNumber) const
{
    if (!bScriptExpanded) return;

    if (iLineNumber < 0 || iLineNumber >= LineNumberMapper.size()) return;

    iLineNumber = LineNumberMapper[iLineNumber]; // line numbers start from 1 !
}

