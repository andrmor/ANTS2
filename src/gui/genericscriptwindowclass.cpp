#include "genericscriptwindowclass.h"
#include "ui_genericscriptwindowclass.h"
#include "ahighlighters.h"
#include "completingtexteditclass.h"
#include "localscriptinterfaces.h"
#include "coreinterfaces.h"
#include "histgraphinterfaces.h"
#include "interfacetoglobscript.h"
#include "amessage.h"
#include "ascriptexampleexplorer.h"
#include "aconfiguration.h"
#include "ascriptmanager.h"

#include <QScriptEngine>
#include <QTextStream>
#include <QSplitter>
#include <QFileDialog>
#include <QDebug>
#include <QMetaMethod>
#include <QPainter>
#include <QPlainTextEdit>
#include <QStringListModel>
#include <QShortcut>
#include <QScriptValueIterator>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLineEdit>
#include <QMenu>
#include <QClipboard>
#include <QJsonParseError>
#include <QCompleter>
#include <QThread>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>

QStringList getCustomCommandsOfObject(QObject *obj, QString ObjName, bool fWithArguments = false)
{  
  QStringList commands;
  int methods = obj->metaObject()->methodCount();
  for (int i=0; i<methods; i++)
    {
      const QMetaMethod &m = obj->metaObject()->method(i);
      bool fSlot   = (m.methodType() == QMetaMethod::Slot);
      bool fPublic = (m.access() == QMetaMethod::Public);
      QString commCandidate;
      if (fSlot && fPublic)
        {
          if (m.name() == "deleteLater") continue;
          if (m.name() == "help") continue;
          if (ObjName.isEmpty()) commCandidate = m.name();
          else commCandidate = ObjName + "." + m.name();

          if (fWithArguments)
            {              
              commCandidate += "(";
              int args = m.parameterCount();
              for (int i=0; i<args; i++)
                {
                  //commCandidate += m.parameterTypes().at(i) + " " + m.parameterNames().at(i);
                  commCandidate += " " + m.parameterNames().at(i);
                  if (i!=args-1) commCandidate += ", ";
                }
              commCandidate += " )";
            }          
          if (commands.isEmpty() || commands.last() != commCandidate)
             commands << commCandidate;
        }
    }
  return commands;
}

GenericScriptWindowClass::GenericScriptWindowClass(TRandom2 *RandGen, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GenericScriptWindowClass)
{
    ScriptManager = new AScriptManager(RandGen);
    QObject::connect(ScriptManager, SIGNAL(showMessage(QString)), this, SLOT(ShowText(QString)));
    QObject::connect(ScriptManager, SIGNAL(clearText()), this, SLOT(ClearText()));
    //retranslators:
    QObject::connect(ScriptManager, SIGNAL(onStart()), this, SLOT(receivedOnStart()));
    QObject::connect(ScriptManager, SIGNAL(onAbort()), this, SLOT(receivedOnAbort()));
    QObject::connect(ScriptManager, SIGNAL(success(QString)), this, SLOT(receivedOnSuccess(QString)));

    tmpIgnore = false;
    fJsonTreeAlwayVisible = false;
    ShowEvalResult = true;
    ui->setupUi(this);
    ui->pbStop->setVisible(false);
    LocalScript = "//no external script provided!";    
    //interfacer = 0;
    Example = "";
    ExamplesDir = "";
    ScriptHistory.append("");
    HistoryPosition = 0;


    QPixmap rm(16, 16);
    rm.fill(Qt::transparent);
    QPainter b(&rm);
    b.setBrush(QBrush(Qt::red));
    b.drawEllipse(0, 0, 14, 14);
    RedIcon = new QIcon(rm);

    //creating text edit with completition for the script window
    completitionModel = 0;
    cteScript = new CompletingTextEditClass(this);
    cteScript->setLineWrapMode(QTextEdit::NoWrap);
    QObject::connect(cteScript, SIGNAL(requestHelp(QString)), this, SLOT(onF1pressed(QString)));
    completer = new QCompleter(this);
    QStringList words;
    completer->setModel(createCompletitionModel(words));
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    //completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setFilterMode(Qt::MatchContains);
    completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->setWrapAround(false);
    cteScript->setCompleter(completer);
    SetScript(&LocalScript); //to be reset from external
    highlighter = new AHighlighterScriptWindow(cteScript->document());

    QSplitter* sp = new QSplitter();  // upper + output with buttons
    sp->setOrientation(Qt::Vertical);
    sp->setChildrenCollapsible(false);
      //
    QSplitter* hor = new QSplitter(); //all upper widgets are here
    hor->setContentsMargins(0,0,0,0);
      cteScript->setMinimumHeight(25);
      hor->addWidget(cteScript); //already defined

      splHelp = new QSplitter();
      splHelp->setOrientation(Qt::Horizontal);
      splHelp->setChildrenCollapsible(false);
      splHelp->setContentsMargins(0,0,0,0);

        QFrame* fr1 = new QFrame();
        fr1->setContentsMargins(1,1,1,1);
        fr1->setFrameShape(QFrame::NoFrame);
        QVBoxLayout* vb1 = new QVBoxLayout();
        vb1->setContentsMargins(0,0,0,0);

          QSplitter* sh = new QSplitter();
          sh->setOrientation(Qt::Vertical);
          sh->setChildrenCollapsible(false);
          sh->setContentsMargins(0,0,0,0);

            trwHelp = new QTreeWidget();
            trwHelp->setColumnCount(1);
            trwHelp->setHeaderLabel("Unit.Function");
            QObject::connect(ui->pbHelp, SIGNAL(toggled(bool)), splHelp, SLOT(setVisible(bool)));
            QObject::connect(trwHelp, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onFunctionDoubleClicked(QTreeWidgetItem*,int)));
            QObject::connect(trwHelp, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onFunctionClicked(QTreeWidgetItem*,int)));
            //splHelp->addWidget(trwHelp);
            sh->addWidget(trwHelp);

            pteHelp = new QPlainTextEdit();
            pteHelp->setReadOnly(true);
            pteHelp->setMinimumHeight(20);
            //pteHelp->setMaximumHeight(50);
            QObject::connect(trwHelp, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(onHelpTWExpanded(QTreeWidgetItem*)));
            QObject::connect(trwHelp, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(onHelpTWCollapsed(QTreeWidgetItem*)));
            //splHelp->addWidget(pteHelp);
          sh->addWidget(pteHelp);
          QList<int> sizes;
          sizes << 800 << 175;
          sh->setSizes(sizes);

          vb1->addWidget(sh);

            leFind = new QLineEdit("Find");
            //splHelp->addWidget(leFind);
            leFind->setMinimumHeight(20);
            leFind->setMaximumHeight(20);
            QObject::connect(leFind, SIGNAL(textChanged(QString)), this, SLOT(onFindTextChanged(QString)));
            vb1->addWidget(leFind);

        fr1->setLayout(vb1);
        splHelp->addWidget(fr1);

        frJsonBrowser = new QFrame();
        frJsonBrowser->setContentsMargins(0,0,0,0);
        frJsonBrowser->setFrameShape(QFrame::NoFrame);
        QVBoxLayout* vbl = new QVBoxLayout();
        vbl->setContentsMargins(0,0,0,0);

          trwJson = new QTreeWidget();
          trwJson->setColumnCount(2);
          trwJson->setMinimumHeight(30);
          trwJson->setMinimumWidth(100);
          QStringList strl;
          strl << "Key" << "Value or type";
          trwJson->setHeaderLabels(strl);
          trwJson->setContextMenuPolicy(Qt::CustomContextMenu);
          QObject::connect(trwJson, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onKeyDoubleClicked(QTreeWidgetItem*,int)));
          QObject::connect(trwJson, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onKeyClicked(QTreeWidgetItem*,int)));
          QObject::connect(trwJson, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onContextMenuRequestedByJsonTree(QPoint)));
          QObject::connect(trwJson, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(onJsonTWExpanded(QTreeWidgetItem*)));
          QObject::connect(trwJson, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(onJsonTWCollapsed(QTreeWidgetItem*)));
          //splHelp->addWidget(trwJson);

        vbl->addWidget(trwJson);

          leFindJ = new QLineEdit("Find");
          //splHelp->addWidget(leFind);
          leFindJ->setMinimumHeight(20);
          leFindJ->setMaximumHeight(20);
          QObject::connect(leFindJ, SIGNAL(textChanged(QString)), this, SLOT(onFindTextJsonChanged(QString)));
        vbl->addWidget(leFindJ);
        frJsonBrowser->setLayout(vbl);

        splHelp->addWidget(frJsonBrowser);

    sizes.clear();
    sizes << 500 << 500 << 500;
    splHelp->setSizes(sizes);
    frJsonBrowser->setVisible(false);
    splHelp->setVisible(false);

    hor->addWidget(splHelp);

    sp->addWidget(hor);
      //
    pteOut = new QPlainTextEdit();
    pteOut->setMinimumHeight(25);
    pteOut->setReadOnly(true);
    QPalette p = pteOut->palette();
     p.setColor(QPalette::Active, QPalette::Base, QColor(240,240,240));
     p.setColor(QPalette::Inactive, QPalette::Base, QColor(240,240,240));
    pteOut->setPalette(p);
    pteHelp->setPalette(p);
    sizes.clear();
    sizes << 800 << 50;
    sp->setSizes(sizes);

    sp->addWidget(pteOut);
    ui->centralwidget->layout()->removeItem(ui->horizontalLayout);
    ui->centralwidget->layout()->addWidget(sp);
    ui->centralwidget->layout()->addItem(ui->horizontalLayout);

    trwJson->header()->resizeSection(0, 200);

    connect(cteScript, SIGNAL(textChanged()), this, SLOT(cteScript_textChanged()));

    //shortcuts
    QShortcut* Run = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(Run, SIGNAL(activated()), this, SLOT(on_pbRunScript_clicked()));

    QShortcut* HistBefore = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp), this);
    connect(HistBefore, SIGNAL(activated()), this, SLOT(onRequestHistoryBefore()));

    QShortcut* HistAfter = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown), this);
    connect(HistAfter, SIGNAL(activated()), this, SLOT(onRequestHistoryAfter()));
}

GenericScriptWindowClass::~GenericScriptWindowClass()
{
  //qDebug() << "Destructor of script window called";
  tmpIgnore = true;
  delete ui;
  //qDebug() << "ui deleted, deleting interface object";

  delete RedIcon;

  delete ScriptManager;
  //qDebug() << "Script manager deleted";
}

void GenericScriptWindowClass::SetInterfaceObject(QObject *interfaceObject, QString name)
{       
    ScriptManager->SetInterfaceObject(interfaceObject, name);

    // populating help
    if(name.isEmpty())
    { // empty name means the main module
        // populating help for main, math and core units
        trwHelp->clear();
        fillHelper(interfaceObject, "", "Global object functions");
        AInterfaceToCore core(0); //dummy to extract methods
        fillHelper(&core, "core", "Core object functions");
        AInterfaceToMath math(0); //dummy to extract methods
        fillHelper(&math, "math", "Basic mathematics: wrapper for std double functions");
        trwHelp->expandItem(trwHelp->itemAt(0,0));
    }
    else
        fillHelper(interfaceObject, name, "");

    // auto-read list of public slots for highlighter
    QStringList functions, constants;
    for (int i=0; i<ScriptManager->interfaces.size(); i++)
      functions << getCustomCommandsOfObject(ScriptManager->interfaces[i], ScriptManager->interfaceNames[i], false);
    highlighter->setCustomCommands(functions, constants);

    //filling autocompleter
    for (int i=0; i<functions.size(); i++) functions[i] += "()";
    completer->setModel(createCompletitionModel(functions+constants));

    //special "needs" of particular interface objects
    if ( dynamic_cast<AInterfaceToHist*>(interfaceObject) || dynamic_cast<AInterfaceToGraph*>(interfaceObject)) //"graph" or "hist"
       QObject::connect(interfaceObject, SIGNAL(RequestDraw(TObject*,QString,bool)), this, SLOT(onRequestDraw(TObject*,QString,bool)));
}

void GenericScriptWindowClass::SetScript(QString* text)
{
    Script = text;

    tmpIgnore = true;
      cteScript->clear();
      cteScript->append(*text);
    tmpIgnore = false;
}

void GenericScriptWindowClass::SetTitle(QString title)
{
    this->setWindowTitle(title);
}

void GenericScriptWindowClass::SetExample(QString example)
{
  if (example.startsWith("#Load "))
    {
      //Load example from file
      example.remove("#Load ");
      QString fileName = ExamplesDir + "/" + example;
      QFile file(fileName);
      if (!file.open(QIODevice::ReadOnly))
        {
          message("Failed to open example file:\n"+example, this);
          return;
        }
      QTextStream in(&file);
      Example = in.readAll();
    }
  else Example = example;
}

void GenericScriptWindowClass::SetJsonTreeAlwaysVisible(bool flag)
{
    fJsonTreeAlwayVisible = flag;
    if (flag) frJsonBrowser->setVisible(true);
}

//void GenericScriptWindowClass::SetRandomGenerator(TRandom2 *randGen)
//{
//    RandGen = randGen;
//    for (int i=0; i<interfaces.size(); i++)
//      {
//        MathInterfaceClass* m = dynamic_cast<MathInterfaceClass*>(interfaces[i]);
//        if (m) m->setRandomGen(randGen);
//      }
//}

void GenericScriptWindowClass::SetStarterDir(QString starter)
{
    LibScripts = starter;

    QString fileName = LibScripts + "/ScriptHistory.json";
  QFile file(fileName);
  if (file.open(QIODevice::ReadOnly))
    {
      QJsonParseError err;
      QJsonDocument loadDoc(QJsonDocument::fromJson(file.readAll(), &err));
      if (err.error == QJsonParseError::NoError)
        {
            QJsonObject json = loadDoc.object();
            QJsonArray ar =  json["ScriptHistory"].toArray();
            QStringList strl;
            for (int i=0; i<ar.size(); i++) strl<<ar[i].toString();

            if (strl.last().isEmpty()) strl.removeLast();
            if (!strl.isEmpty())
            {
                ScriptHistory = strl;
                HistoryPosition = ScriptHistory.size()-1;
            }
        }
    }
  ScriptHistory.append(*Script);

  ScriptManager->LibScripts = starter;
}

void GenericScriptWindowClass::SetStarterDir(QString ScriptDir, QString WorkingDir, QString ExamplesDir)
{
  LastOpenDir = WorkingDir;
  this->ExamplesDir = ExamplesDir;
  SetStarterDir(ScriptDir);

  ScriptManager->LibScripts = ScriptDir;
  ScriptManager->LastOpenDir = WorkingDir;
  ScriptManager->ExamplesDir = ExamplesDir;
}

void GenericScriptWindowClass::ReportError(QString error, int line)
{   
  //pteOut->appendHtml(error);
   error = "<font color=\"red\">Error:<br>" + error + "</font>";
   pteOut->appendHtml( error );
   //pteOut->moveCursor(QTextCursor::Start);
   if (line >= 0 ) HighlightErrorLine(line);
}

void GenericScriptWindowClass::HighlightErrorLine(int line)
{
  //highlight line with error
  QTextBlock block = cteScript->document()->findBlockByLineNumber(line-1);
  int loc = block.position();
  QTextCursor cur = cteScript->textCursor();
  cur.setPosition(loc);
  int length = block.text().split("\n").at(0).length();
  cur.movePosition(cur.Right, cur.KeepAnchor, length);

  QTextCharFormat tf = block.charFormat();
  tf.setBackground(QBrush(Qt::yellow));
  QTextEdit::ExtraSelection es;
  es.cursor = cur;
  es.format = tf;

  QList<QTextEdit::ExtraSelection> esList;
  esList << es;
  cteScript->setExtraSelections(esList);
}

void GenericScriptWindowClass::ShowText(QString text)
{
  pteOut->appendHtml(text);
  qApp->processEvents();
}

void GenericScriptWindowClass::ClearText()
{
  pteOut->clear();
  qApp->processEvents();
}

void GenericScriptWindowClass::on_pbRunScript_clicked()
{
   QString corrected = cteScript->document()->toPlainText();//.replace("\\","/");
   *Script = ScriptHistory.last() = corrected; //override in case one of the history items is selected and run button is clicked

   //qDebug() << "Init on Start done";
   pteOut->clear();
   GenericScriptWindowClass::ShowText("Processing script");

   //syntax check
   int errorLineNum = ScriptManager->FindSyntaxError(*Script);
   if (errorLineNum > -1)
     {
       GenericScriptWindowClass::ReportError("Syntax error!", errorLineNum);
       return;
     }

   //script history handling
   saveScriptHistory();
   ScriptHistory.append(ScriptHistory.last());  //this will be the next "current" item
   if (ScriptHistory.size()>50) ScriptHistory.removeFirst();
   HistoryPosition = ScriptHistory.size()-1;


   ui->pbStop->setVisible(true);
   ui->pbRunScript->setVisible(false);

   qApp->processEvents();
   QString result = ScriptManager->Evaluate(*Script);

   ui->pbStop->setVisible(false);
   ui->pbRunScript->setVisible(true);

   if (!ScriptManager->LastError.isEmpty())
   {
       GenericScriptWindowClass::ReportError("Script error: "+ScriptManager->LastError, -1);
   }
   else if (ScriptManager->engine->hasUncaughtException())
   {   //Script has uncaught exception
       int lineNum = ScriptManager->engine->uncaughtExceptionLineNumber();
       QString message = ScriptManager->engine->uncaughtException().toString();
       //qDebug() << "Error message:" << message;
       //QString backtrace = engine.uncaughtExceptionBacktrace().join('\n');
       //qDebug() << "backtrace:" << backtrace;
       GenericScriptWindowClass::ReportError("Script error: "+message, lineNum);
   }
   else
   {   //success
       //qDebug() << "Script returned:" << result;
       if (!ScriptManager->fAborted)
         {
            if (ShowEvalResult && result!="undefined") ShowText("Script evaluation result:\n"+result);
            else ShowText("Script evaluation finished");
         }
       else
         {
           //ShowText("Aborted!");
         }       
       ui->pbRunScript->setIcon(QIcon()); //clear red icon
     }

   ScriptManager->engine->collectGarbage();
}

void GenericScriptWindowClass::abortEvaluation(QString message)
{
//  if (fAborted || !ScriptManager->fEngineIsRunning) return;
//  fAborted = true;
//  emit onAbort();
//  ScriptManager->AbortEvaluation();
//  message = "<font color=\"red\">"+ message +"</font><br>";
//  ShowText(message);
}

void GenericScriptWindowClass::onF1pressed(QString text)
{
  //qDebug() << "F1 requested for:"<<text;
  ui->pbHelp->setChecked(true);

  trwHelp->collapseAll();
  trwHelp->clearSelection();

  QList<QTreeWidgetItem*> list;
  list = trwHelp->findItems(text, Qt::MatchContains | Qt::MatchRecursive, 0);

  for (int i=0; i<list.size(); i++)
    {
      QTreeWidgetItem* item = list[i];
      do
        {
          trwHelp->expandItem(item);
          item = item->parent();
        }
      while (item);
      trwHelp->setCurrentItem(list[i], 0, QItemSelectionModel::Select);
      trwHelp->setCurrentItem(list[i], 1, QItemSelectionModel::Select);
    }

  if (list.size() == 1)
    emit trwHelp->itemClicked(list.first(), 0);
}

void GenericScriptWindowClass::on_pbStop_clicked()
{    
  if (ScriptManager->fEngineIsRunning)
    {
      qDebug() << "Stop button pressed!";
      ShowText("Sending stop signal...");
      ScriptManager->AbortEvaluation("Aborted by user!");
      qApp->processEvents();
    }
}

void GenericScriptWindowClass::on_pbHelp_clicked()
{
    //message(HelpText, this);
}

void GenericScriptWindowClass::on_pbLoad_clicked()
{   
  QString starter = (LibScripts.isEmpty()) ? LastOpenDir : LibScripts;
  QString fileName = QFileDialog::getOpenFileName(this, "Load script", starter, "Script files (*.txt *.js);;All files (*.*)"); //""
  if (fileName.isEmpty()) return;

  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      message("Could not open: " + fileName, this);
      return;
    }

  QTextStream in(&file);
  *Script = in.readAll();
  file.close();

  tmpIgnore = true;
  cteScript->clear();
  cteScript->append(*Script);
  tmpIgnore = false;

  ui->pbRunScript->setIcon(*RedIcon);

  SavedName.clear();
  setWindowTitle("");
  ui->pbSave->setEnabled(false);
}

void GenericScriptWindowClass::on_pbAppendScript_clicked()
{
    QString starter = (LibScripts.isEmpty()) ? LastOpenDir : LibScripts;
    QString fileName = QFileDialog::getOpenFileName(this, "Append script", starter, "Script files (*.txt *.js);;All files (*.*)"); //""
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
      {
        message("Could not open: " + fileName, this);
        return;
      }

    QTextStream in(&file);
    QString t = in.readAll();
    file.close();

    *Script += "\n" + t;

    tmpIgnore = true;
    cteScript->clear();
    cteScript->append(*Script);
    tmpIgnore = false;

    ui->pbRunScript->setIcon(*RedIcon);

    SavedName.clear();
    setWindowTitle("");
    ui->pbSave->setEnabled(false);
}

void GenericScriptWindowClass::on_pbSave_clicked()
{
    if (SavedName.isEmpty()) return;

    QFile outputFile(SavedName);
    outputFile.open(QIODevice::WriteOnly);

    if(!outputFile.isOpen())
      {
        message("Unable to open file " +SavedName+ " for writing!", this);
        SavedName.clear();
        ui->pbSave->setEnabled(false);
        return;
      }

    QTextStream outStream(&outputFile);
    outStream << *Script;
    outputFile.close();

    ui->pbSave->setEnabled(true);
    setWindowTitle(QFileInfo(SavedName).fileName());
}

void GenericScriptWindowClass::on_pbSaveAs_clicked()
{
    QString starter = (LibScripts.isEmpty()) ? LastOpenDir : LibScripts;
    QString fileName = QFileDialog::getSaveFileName(this,"Save script", starter, "Script files (*.txt *.js);;All files (*.*)");
    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    if(fileInfo.suffix().isEmpty()) fileName += ".txt";

    SavedName = fileName;
    GenericScriptWindowClass::on_pbSave_clicked();
}

void GenericScriptWindowClass::cteScript_textChanged()
{
    if (tmpIgnore) return;

    *Script = cteScript->document()->toPlainText();
    ScriptHistory.last() = *Script;

    HighlightErrorLine(0);
    ui->pbRunScript->setIcon(*RedIcon);
}

void GenericScriptWindowClass::on_pbExample_clicked()
{
    if (Example.isEmpty())
    {
        //reading example database
        QString RecordsFilename = ExamplesDir + "/" + "ScriptExamples.cfg";
          //check it is found
        QFile file(RecordsFilename);
        if (!file.open(QIODevice::ReadOnly))
          {
            message("Failed to open example list file:\n"+RecordsFilename, this);
            return;
          }
        file.close();

        //starting explorer
        AScriptExampleExplorer* expl = new AScriptExampleExplorer(RecordsFilename, this);
        expl->setWindowModality(Qt::WindowModal);
        expl->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(expl, SIGNAL(LoadRequested(QString)), this, SLOT(onLoadRequested(QString)));
        expl->show();
        return;
    }

    tmpIgnore = true;
    if (Script->isEmpty()) *Script = Example;
    else
      {
        QMessageBox b;
        b.setText("Load / append example");
        //msgBox.setInformativeText("Do you want to save your changes?");
        QPushButton* append = b.addButton("Append", QMessageBox::AcceptRole);
        b.addButton("Replace", QMessageBox::AcceptRole);
        QPushButton* cancel = b.addButton("Cancel", QMessageBox::RejectRole);
        b.setDefaultButton(cancel);

        b.exec();

        if (b.clickedButton() == cancel) return;
        if (b.clickedButton() == append)
            *Script += "\n" + Example;
        else
            *Script = Example;
      }

    cteScript->clear();
    cteScript->append(*Script);
    tmpIgnore = false;

    ui->pbRunScript->setIcon(*RedIcon);

    SavedName.clear();
    setWindowTitle("");
    ui->pbSave->setEnabled(false);
}

void GenericScriptWindowClass::onLoadRequested(QString NewScript)
{
    tmpIgnore = true;
    *Script = NewScript;
    cteScript->clear();
    cteScript->append(NewScript);
    tmpIgnore = false;

    ui->pbRunScript->setIcon(*RedIcon);
    SavedName.clear();
    setWindowTitle("");
    ui->pbSave->setEnabled(false);
}

void GenericScriptWindowClass::fillHelper(QObject* obj, QString module, QString helpText)
{
  QStringList functions = getCustomCommandsOfObject(obj, module, true);
  functions.sort();
  //qDebug() << functions;
  if (module != "Math") cteScript->functionList << functions;

  QTreeWidgetItem *objItem = new QTreeWidgetItem(trwHelp);
  objItem->setText(0, module);
  QFont f = objItem->font(0);
  f.setBold(true);
  objItem->setFont(0, f);
  //objItem->setBackgroundColor(QColor(0, 0, 255, 80));
  objItem->setToolTip(0, helpText);
  for (int i=0; i<functions.size(); i++)
  {
      QTreeWidgetItem *fItem = new QTreeWidgetItem(objItem);
      fItem->setText(0, functions[i]);

      QString retVal = "Help not provided";
      QString funcNoArgs = functions[i].remove(QRegExp("\\((.*)\\)"));
      if (!module.isEmpty()) funcNoArgs.remove(0, module.length()+1); //remove name.
      if (obj->metaObject()->indexOfMethod("help(QString)") != -1)
        {
          QMetaObject::invokeMethod(obj, "help", Qt::DirectConnection,
                              Q_RETURN_ARG(QString, retVal),
                              Q_ARG(QString, funcNoArgs)
                              );
        }
      fItem->setToolTip(0, retVal);
    }
}

QString GenericScriptWindowClass::getFunctionReturnType(QString UnitFunction)
{
  QStringList f = UnitFunction.split(".");
  if (f.size() != 2) return "";

  QString unit = f.first();
  int unitIndex = ScriptManager->interfaceNames.indexOf(unit);
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
  int mi = ScriptManager->interfaces.at(unitIndex)->metaObject()->indexOfMethod(methodName.toLatin1().data());
  //qDebug() << "method index:"<<mi;
  if (mi == -1) return "";

  QString returnType = ScriptManager->interfaces.at(unitIndex)->metaObject()->method(mi).typeName();
  //qDebug() << returnType;
  return returnType;
}

//void GenericScriptWindowClass::fillHelper(QScriptValue &val, QString module, QString helpText)
//{
//  QStringList functions, constants;
//  QScriptValueIterator it(val);
//  while (it.hasNext())
//    {
//      it.next();
//      QString toAdd = "Math."+it.name();
//      if (it.value().isFunction()) functions << toAdd;
//      else constants << toAdd;
//    }

//  functions.sort();
//  constants.sort();
//  QTreeWidgetItem *objItem = new QTreeWidgetItem(trwHelp);
//  objItem->setText(0, module);
//  QFont f = objItem->font(0);
//  f.setBold(true);
//  objItem->setFont(0, f);
//  objItem->setToolTip(0, helpText);
//  for (int i=0; i<functions.size(); i++)
//    {
//      QTreeWidgetItem *fItem = new QTreeWidgetItem(objItem);
//      fItem->setText(0, functions[i]+"()");
//    }
//  for (int i=0; i<constants.size(); i++)
//    {
//      QTreeWidgetItem *fItem = new QTreeWidgetItem(objItem);
//      fItem->setText(0, constants[i]);
//    }
//}

QAbstractItemModel *GenericScriptWindowClass::createCompletitionModel(QStringList words)
{
  if (completitionModel) delete completitionModel;
  completitionModel = new QStringListModel(words, completer);
  return completitionModel;
}

void GenericScriptWindowClass::onJsonTWExpanded(QTreeWidgetItem *item)
{
   ExpandedItemsInJsonTW << item->text(0);
}

void GenericScriptWindowClass::onJsonTWCollapsed(QTreeWidgetItem *item)
{
    ExpandedItemsInJsonTW.remove(item->text(0));
}

void GenericScriptWindowClass::updateJsonTree()
{
  trwJson->clear();

  for (int i=0; i<ScriptManager->interfaces.size(); i++)
    {
      InterfaceToConfig* inter = dynamic_cast<InterfaceToConfig*>(ScriptManager->interfaces[i]);
      if (!inter) continue;

      QJsonObject json = inter->Config->JSON;
      QJsonObject::const_iterator it;
      for (it = json.begin(); it != json.end(); ++it)
        {
           QString key = it.key();
           QTreeWidgetItem *TopKey = new QTreeWidgetItem(trwJson);
           TopKey->setText(0, key);

           const QJsonValue &value = it.value();
           TopKey->setText(1, getDesc(value));

           if (value.isObject())
             fillSubObject(TopKey, value.toObject());
           else if (value.isArray())
             fillSubArray(TopKey, value.toArray());
        }
    }

  //restoring expanded status
  QSet<QString> expanded = ExpandedItemsInJsonTW;
  ExpandedItemsInJsonTW.clear();
  foreach (QString s, expanded)
    {
      QList<QTreeWidgetItem*> l = trwJson->findItems(s, Qt::MatchExactly | Qt::MatchRecursive);
      foreach (QTreeWidgetItem* item, l)
       item->setExpanded(true);
    }
  //qDebug() << "Expanded items:"<<ExpandedItemsInJsonTW.size();
}

void GenericScriptWindowClass::fillSubObject(QTreeWidgetItem *parent, const QJsonObject &obj)
{
  QJsonObject::const_iterator it;
  for (it = obj.begin(); it != obj.end(); ++it)
    {
      QTreeWidgetItem *item = new QTreeWidgetItem(parent);
      item->setText(0, it.key());
      QJsonValue value = it.value();
      item->setText(1, getDesc(value));

      if (value.isObject())
        fillSubObject(item, value.toObject());
      else if (value.isArray())
        fillSubArray(item, value.toArray());
    }
}

void GenericScriptWindowClass::fillSubArray(QTreeWidgetItem *parent, const QJsonArray &arr)
{
  for (int i=0; i<arr.size(); i++)
    {
      QTreeWidgetItem *item = new QTreeWidgetItem(parent);
      //QString str = "[" + QString::number(i)+"]";
      QString str = parent->text(0)+"[" + QString::number(i)+"]";
      item->setText(0, str);
      QJsonValue value = arr.at(i);
      item->setText(1, getDesc(value));

      if (value.isObject())
        fillSubObject(item, value.toObject());
      else if (value.isArray())
        fillSubArray(item, value.toArray());
    }
}

QString GenericScriptWindowClass::getDesc(const QJsonValue &ref)
{
  if (ref.isBool())
  {
      bool f = ref.toBool();
      return (f ? "true" : "false");
      //return "bool";
  }
  if (ref.isDouble())
  {
      double v = ref.toDouble();
      QString s = QString::number(v);
      return s;
      //return "number";
  }
  if (ref.isString())
  {
      QString s = ref.toString();
      s = "\"" + s + "\"";
      if (s.length()<100) return s;
      else return "string";
  }
  if (ref.isObject()) return "obj";
  if (ref.isArray())
    {
      int size = ref.toArray().size();
      QString ret = "array[" + QString::number(size)+"]";
      return ret;
    }
  return "undefined";
}

void GenericScriptWindowClass::onFunctionDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
  QString text = item->text(0);
//  int args;
//  if (text.endsWith("()")) args = 0;
//  else args = text.count(",") + 1;
//  text.remove(QRegExp("\\(.*\\)"));
//  QString addon = "";
//  if (args>0) addon += " __ ";
//  if (args>1)
//    for (int i=0; i<args-1; i++) addon += ", __ ";
//  text += "(" + addon + ")\n";
  cteScript->insertPlainText(text);
}

void GenericScriptWindowClass::onFunctionClicked(QTreeWidgetItem *item, int /*column*/)
{
  pteHelp->clear();
  QString returnType = getFunctionReturnType(item->text(0));
  pteHelp->appendPlainText(returnType+ "  " +item->text(0)+":");
  pteHelp->appendPlainText(item->toolTip(0));
}

void GenericScriptWindowClass::onKeyDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{  
  //QString str = getKeyPath(item);
  //cteScript->insertPlainText(str);
  if (!item) return;
  showContextMenuForJsonTree(item, trwJson->mapFromGlobal(cursor().pos()));
}

QString GenericScriptWindowClass::getKeyPath(QTreeWidgetItem *item)
{
  if (!item) return "";

  QString path;
  int SkipOnArray = 0;
  do
  {
      if (SkipOnArray != 0) SkipOnArray--;
      else
      {
          QString thisPart = item->text(0);
          SkipOnArray = thisPart.count('[');
          path = thisPart + "." + path;
      }
      item = item->parent();
  }
  while (item);

  path.chop(1);
  path = " \""+path+"\" ";
  return path;
}

void GenericScriptWindowClass::onKeyClicked(QTreeWidgetItem* /*item*/, int /*column*/)
{
  //trwJson->resizeColumnToContents(column);
}

void GenericScriptWindowClass::onFindTextChanged(const QString &arg1)
{
  QTreeWidget* tw = trwHelp;

  tw->collapseAll();
  tw->clearSelection();
  if (arg1.length()<3) return;

  QList<QTreeWidgetItem*> list;
  list = tw->findItems(arg1, Qt::MatchContains | Qt::MatchRecursive, 0);

  for (int i=0; i<list.size(); i++)
    {
      QTreeWidgetItem* item = list[i];
      do
        {
          tw->expandItem(item);
          item = item->parent();
        }
      while (item);
      tw->setCurrentItem(list[i], 0, QItemSelectionModel::Select);
      tw->setCurrentItem(list[i], 1, QItemSelectionModel::Select);
    }
}

void GenericScriptWindowClass::onFindTextJsonChanged(const QString &arg1)
{
  QTreeWidget* tw = trwJson;

  tw->collapseAll();
  tw->clearSelection();
  if (arg1.length()<3) return;

  QList<QTreeWidgetItem*> list;
  list = tw->findItems(arg1, Qt::MatchContains | Qt::MatchRecursive, 0);

  for (int i=0; i<list.size(); i++)
    {
      QTreeWidgetItem* item = list[i];
      do
        {
          tw->expandItem(item);
          item = item->parent();
        }
      while (item);
      tw->setCurrentItem(list[i], 0, QItemSelectionModel::Select);
      tw->setCurrentItem(list[i], 1, QItemSelectionModel::Select);
    }
}

void GenericScriptWindowClass::onContextMenuRequestedByJsonTree(QPoint pos)
{  
  QTreeWidgetItem *item = trwJson->itemAt(pos);
  if (!item) return;

  showContextMenuForJsonTree(item, pos);
}

void GenericScriptWindowClass::onHelpTWExpanded(QTreeWidgetItem *item)
{
    if (fJsonTreeAlwayVisible) return;

    QString txt = item->text(0);
    //qDebug() << "expanded!"<<txt;
    if (txt == "config")
    {
        frJsonBrowser->setVisible(true);
        updateJsonTree();
    }
}

void GenericScriptWindowClass::onHelpTWCollapsed(QTreeWidgetItem *item)
{
    if (fJsonTreeAlwayVisible) return;

    QString txt = item->text(0);
    //qDebug() << "collapsed!"<<txt;
    if (txt == "config")
        frJsonBrowser->setVisible(false);
}

void GenericScriptWindowClass::showContextMenuForJsonTree(QTreeWidgetItem *item, QPoint pos)
{
  QMenu menu;

  QAction* showVtoClipboard = menu.addAction("Add Key path to clipboard");
  //menu.addSeparator();

  QAction* selectedItem = menu.exec(trwJson->mapToGlobal(pos));
  if (!selectedItem) return; //nothing was selected

  if (selectedItem == showVtoClipboard)
    {
      QClipboard *clipboard = QApplication::clipboard();
      clipboard->setText(getKeyPath(item));
    }
}

void GenericScriptWindowClass::onRequestHistoryBefore()
{
  if (HistoryPosition == 0) return;

  HistoryPosition--;
  tmpIgnore = true;
  cteScript->clear();
  cteScript->setText(ScriptHistory[HistoryPosition]);
  tmpIgnore = false;
}

void GenericScriptWindowClass::onRequestHistoryAfter()
{
  if (HistoryPosition == ScriptHistory.size()-1) return;

  HistoryPosition++;
  tmpIgnore = true;
  cteScript->clear();
  cteScript->setText(ScriptHistory[HistoryPosition]);
  tmpIgnore = false;
}

void GenericScriptWindowClass::closeEvent(QCloseEvent *e)
{
  if (ScriptManager->fEngineIsRunning)
    {
      e->ignore();
      return;
    }

  ScriptManager->deleteMsgDialog();

  saveScriptHistory();
}

bool GenericScriptWindowClass::event(QEvent *e)
{
    switch(e->type())
       {
            case QEvent::WindowActivate :
                // gained focus
                //qDebug() << "Focussed!";
                updateJsonTree();
                break;

            case QEvent::WindowDeactivate :
                // lost focus
                break ;
            default:;
        };

    return QMainWindow::event(e) ;
}

void GenericScriptWindowClass::saveScriptHistory()
{
  QString fileName = LibScripts + "/ScriptHistory.json";
  //qDebug() << "-->Saving script history:"<<fileName;
  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly))
  {
      for (int i=0; i<ScriptHistory.size()-1; i++)
        if (ScriptHistory[i] == ScriptHistory.last())
          {
            ScriptHistory.removeAt(i);
            break;
          }
      QJsonArray ar = QJsonArray::fromStringList(ScriptHistory);
      QJsonObject json;
      json["ScriptHistory"] = ar;

      QJsonDocument saveDoc(json);
      file.write(saveDoc.toJson());
  }
}
