#include "ascriptwindow.h"
#include "ui_ascriptwindow.h"
#include "ahighlighters.h"
#include "completingtexteditclass.h"
#include "scriptinterfaces.h"
#include "interfacetoglobscript.h"
#include "amessage.h"
#include "ascriptexampleexplorer.h"
#include "aconfiguration.h"
#include "ascriptmanager.h"
#include "globalsettingsclass.h"
#include "afiletools.h"

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

AScriptWindow::AScriptWindow(GlobalSettingsClass *GlobSet, TRandom2 *RandGen, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AScriptWindow)
{
    ScriptManager = new AScriptManager(RandGen);
    QObject::connect(ScriptManager, SIGNAL(showMessage(QString)), this, SLOT(ShowText(QString)));
    QObject::connect(ScriptManager, SIGNAL(clearText()), this, SLOT(ClearText()));
    //retranslators:
    QObject::connect(ScriptManager, SIGNAL(onStart()), this, SLOT(receivedOnStart()));
    QObject::connect(ScriptManager, SIGNAL(onAbort()), this, SLOT(receivedOnAbort()));
    QObject::connect(ScriptManager, SIGNAL(success(QString)), this, SLOT(receivedOnSuccess(QString)));

    this->GlobSet = GlobSet;
    //SetStarterDir(GlobSet->LibScripts);
    ScriptManager->LibScripts = GlobSet->LibScripts;
    ScriptManager->LastOpenDir = GlobSet->LastOpenDir;
    ScriptManager->ExamplesDir = GlobSet->ExamplesDir;

    tmpIgnore = false;
    ShowEvalResult = true;
    ui->setupUi(this);
    ui->pbStop->setVisible(false);
    LocalScript = "//no external script provided!";
    this->setWindowTitle("ANTS2 script");

    QPixmap rm(16, 16);
    rm.fill(Qt::transparent);
    QPainter b(&rm);
    b.setBrush(QBrush(Qt::red));
    b.drawEllipse(0, 0, 14, 14);
    RedIcon = new QIcon(rm);

    completitionModel = new QStringListModel(QStringList());

    //more GUI
    QSplitter* sp = new QSplitter();  // upper + output with buttons
    sp->setOrientation(Qt::Vertical);
    sp->setChildrenCollapsible(false);
      //
    twScriptTabs = new QTabWidget();
    connect(twScriptTabs, SIGNAL(currentChanged(int)), this, SLOT(onCurrentTabChanged(int)));
    twScriptTabs->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(twScriptTabs, SIGNAL(customContextMenuRequested(QPoint)), SLOT(onRequestTabWidgetContextMenu(QPoint)));
    connect(twScriptTabs->tabBar(), SIGNAL(tabMoved(int,int)), SLOT(onScriptTabMoved(int,int)));
    twScriptTabs->setMovable(true);
    //twScriptTabs->setTabShape(QTabWidget::Triangular);
    twScriptTabs->setMinimumHeight(25);
    AddNewTab();
      //
    QSplitter* hor = new QSplitter(); //all upper widgets are here
    hor->setContentsMargins(0,0,0,0);
      //cteScript->setMinimumHeight(25);
      //hor->addWidget(cteScript); //already defined
      hor->addWidget(twScriptTabs); //already defined

      splHelp = new QSplitter();
      splHelp->setOrientation(Qt::Horizontal);
      splHelp->setChildrenCollapsible(false);
      splHelp->setContentsMargins(0,0,0,0);

        frHelper = new QFrame();
        frHelper->setContentsMargins(1,1,1,1);
        frHelper->setFrameShape(QFrame::NoFrame);
        QVBoxLayout* vb1 = new QVBoxLayout();
        vb1->setContentsMargins(0,0,0,0);

          QSplitter* sh = new QSplitter();
          sh->setOrientation(Qt::Vertical);
          sh->setChildrenCollapsible(false);
          sh->setContentsMargins(0,0,0,0);

            trwHelp = new QTreeWidget();
            trwHelp->setContextMenuPolicy(Qt::CustomContextMenu);
            trwHelp->setColumnCount(1);
            trwHelp->setHeaderLabel("Unit.Function");
            //QObject::connect(trwHelp, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onFunctionDoubleClicked(QTreeWidgetItem*,int)));
            QObject::connect(trwHelp, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onFunctionClicked(QTreeWidgetItem*,int)));
            QObject::connect(trwHelp, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onContextMenuRequestedByHelp(QPoint)));
            //splHelp->addWidget(trwHelp);
            sh->addWidget(trwHelp);

            pteHelp = new QPlainTextEdit();
            pteHelp->setReadOnly(true);
            pteHelp->setMinimumHeight(20);
            //pteHelp->setMaximumHeight(50);
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

        frHelper->setLayout(vb1);
        splHelp->addWidget(frHelper);
        frHelper->setVisible(false);

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
    //splHelp->setVisible(false);

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
    hor->setSizes(sizes);  // sizes of Script / Help / Config
    sizes.clear();
    sizes << 800 << 50;
    sp->setSizes(sizes);

    sp->addWidget(pteOut);
    ui->centralwidget->layout()->removeItem(ui->horizontalLayout);
    ui->centralwidget->layout()->addWidget(sp);
    ui->centralwidget->layout()->addItem(ui->horizontalLayout);

    trwJson->header()->resizeSection(0, 200);

    //shortcuts
    QShortcut* Run = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(Run, SIGNAL(activated()), this, SLOT(on_pbRunScript_clicked()));

    if (!GlobSet->ScriptWindowJson.isEmpty())
        ReadFromJson(GlobSet->ScriptWindowJson);
}

AScriptWindow::~AScriptWindow()
{
  //qDebug() << "Destructor of script window called";  
  tmpIgnore = true;
  clearAllTabs();
  delete ui;
  delete RedIcon;
  delete ScriptManager;
  //qDebug() << "Script manager deleted";
  delete completitionModel;
  //qDebug() << "Completition model deleted";
}

void AScriptWindow::SetInterfaceObject(QObject *interfaceObject, QString name)
{
    ScriptManager->SetInterfaceObject(interfaceObject, name);

    // populating help
    QStringList newFunctions;
    if(name.isEmpty())
    { // empty name means the main module
        // populating help for main, math and core units
        trwHelp->clear();
        //fillHelper(interfaceObject, "", "Global object functions"); //forbidden to override master object now.
        CoreInterfaceClass core(0); //dummy to extract methods
        fillHelper(&core, "core", "Core object functions");
        newFunctions << getCustomCommandsOfObject(&core, "core", false);
        MathInterfaceClass math(0); //dummy to extract methods
        fillHelper(&math, "math", "Basic mathematics: wrapper for std double functions");
        newFunctions << getCustomCommandsOfObject(&math, "math", false);
        trwHelp->expandItem(trwHelp->itemAt(0,0));
    }
    else
    {
        fillHelper(interfaceObject, name, "");
        newFunctions << getCustomCommandsOfObject(interfaceObject, name, false);
    }

    // auto-read list of public slots for highlighter    
    for (int i=0; i<ScriptTabs.size(); i++)
        ScriptTabs[i]->highlighter->setCustomCommands(newFunctions);

    //filling autocompleter
    for (int i=0; i<newFunctions.size(); i++)
        newFunctions[i] += "()";
    functions << newFunctions;
    completitionModel->setStringList(functions);

    //special "needs" of particular interface objects
    if ( dynamic_cast<InterfaceToHistD*>(interfaceObject) || dynamic_cast<InterfaceToGraphs*>(interfaceObject)) //"graph" or "hist"
       QObject::connect(interfaceObject, SIGNAL(RequestDraw(TObject*,QString,bool)), this, SLOT(onRequestDraw(TObject*,QString,bool)));

    trwHelp->collapseAll();
}

void AScriptWindow::SetScript(QString* text)
{
    Script = text;

    tmpIgnore = true;
      ScriptTabs[CurrentTab]->TextEdit->clear();
      ScriptTabs[CurrentTab]->TextEdit->append(*text);
      tmpIgnore = false;
}

void AScriptWindow::ReportError(QString error, int line)
{
  //pteOut->appendHtml(error);
   error = "<font color=\"red\">Error:</font><br>" + error;
   pteOut->appendHtml( error );
   //pteOut->moveCursor(QTextCursor::Start);
   if (line >= 0 ) HighlightErrorLine(line);
}

void AScriptWindow::HighlightErrorLine(int line)
{
  //highlight line with error
  QTextBlock block = ScriptTabs[CurrentTab]->TextEdit->document()->findBlockByLineNumber(line-1);
  int loc = block.position();
  QTextCursor cur = ScriptTabs[CurrentTab]->TextEdit->textCursor();
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
  ScriptTabs[CurrentTab]->TextEdit->setExtraSelections(esList);
}

void AScriptWindow::WriteToJson(QJsonObject &json)
{
    json = QJsonObject(); //clear

    QJsonArray ar;
    for (int i=0; i<ScriptTabs.size(); i++)
    {
        QJsonObject js;
        ScriptTabs.at(i)->WriteToJson(js);
        ar << js;
    }
    json["ScriptTabs"] = ar;
    json["CurrentTab"] = CurrentTab;
}

void AScriptWindow::ReadFromJson(QJsonObject &json)
{
    if (!json.contains("ScriptTabs")) return;

    clearAllTabs();
    QJsonArray ar = json["ScriptTabs"].toArray();
    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar[i].toObject();
        AddNewTab();
        AScriptWindowTabItem* st = ScriptTabs.last();
        st->ReadFromJson(js);
        if (!st->FileName.isEmpty())
        {
           QString ScriptInFile;
           if ( LoadTextFromFile(st->FileName, ScriptInFile) )
           {
               QTextEdit te;
               te.append(ScriptInFile);
               if (te.document()->toPlainText() == st->TextEdit->document()->toPlainText())
                   twScriptTabs->setTabText(twScriptTabs->count()-1, QFileInfo(st->FileName).fileName());
           }
        }
    }
    if (ScriptTabs.isEmpty()) AddNewTab();

    CurrentTab = json["CurrentTab"].toInt();
    if (CurrentTab<0 || CurrentTab>ScriptTabs.size()-1) CurrentTab = 0;
    twScriptTabs->setCurrentIndex(CurrentTab);
}

void AScriptWindow::UpdateHighlight()
{
   for (int i=0; i<ScriptTabs.size(); i++)
       ScriptTabs[i]->UpdateHighlight();
}

void AScriptWindow::ShowText(QString text)
{
  pteOut->appendHtml(text);
  qApp->processEvents();
}

void AScriptWindow::ClearText()
{
  pteOut->clear();
  qApp->processEvents();
}

void AScriptWindow::on_pbRunScript_clicked()
{
   WriteToJson(GlobSet->ScriptWindowJson);
   GlobSet->SaveANTSconfiguration();

   QString Script = ScriptTabs[CurrentTab]->TextEdit->document()->toPlainText();

   //qDebug() << "Init on Start done";
   pteOut->clear();
   AScriptWindow::ShowText("Processing script");

   //syntax check
   int errorLineNum = ScriptManager->FindSyntaxError(Script);
   if (errorLineNum > -1)
     {
       AScriptWindow::ReportError("Syntax error!", errorLineNum);
       return;
     }

   ui->pbStop->setVisible(true);
   ui->pbRunScript->setVisible(false);

   qApp->processEvents();
   QString result = ScriptManager->Evaluate(Script);

   ui->pbStop->setVisible(false);
   ui->pbRunScript->setVisible(true);

   if (!ScriptManager->LastError.isEmpty())
   {
       AScriptWindow::ReportError("Script error: "+ScriptManager->LastError, -1);
   }
   else if (ScriptManager->engine->hasUncaughtException())
   {   //Script has uncaught exception
       int lineNum = ScriptManager->engine->uncaughtExceptionLineNumber();
       QString message = ScriptManager->engine->uncaughtException().toString();
       //qDebug() << "Error message:" << message;
       //QString backtrace = engine.uncaughtExceptionBacktrace().join('\n');
       //qDebug() << "backtrace:" << backtrace;
       AScriptWindow::ReportError("Script error: "+message, lineNum);
   }
   else
   {   //success
       //qDebug() << "Script returned:" << result;
       if (!ScriptManager->fAborted)
         {
            if (ShowEvalResult && result!="undefined") ShowText("Script evaluation result:\n"+result);
            else ShowText("Script evaluation: success");
         }
       else
         {
           //ShowText("Aborted!");
         }
       ui->pbRunScript->setIcon(QIcon()); //clear red icon
     }

   ScriptManager->engine->collectGarbage();
}

//void AScriptWindow::abortEvaluation(QString message)
//{
////  if (fAborted || !ScriptManager->fEngineIsRunning) return;
////  fAborted = true;
////  emit onAbort();
////  ScriptManager->AbortEvaluation();
////  message = "<font color=\"red\">"+ message +"</font><br>";
////  ShowText(message);
//}

void AScriptWindow::onF1pressed(QString text)
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

void AScriptWindow::on_pbStop_clicked()
{
  if (ScriptManager->fEngineIsRunning)
    {
      qDebug() << "Stop button pressed!";
      ShowText("Sending stop signal...");
      ScriptManager->AbortEvaluation("Aborted by user!");
      qApp->processEvents();
    }
}

void AScriptWindow::on_pbLoad_clicked()
{
  QString starter = (GlobSet->LibScripts.isEmpty()) ? GlobSet->LastOpenDir : GlobSet->LibScripts;
  QString fileName = QFileDialog::getOpenFileName(this, "Load script", starter, "Script files (*.txt *.js);;All files (*.*)"); //""
  if (fileName.isEmpty()) return;

  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      message("Could not open: " + fileName, this);
      return;
    }
  QTextStream in(&file);
  QString Script = in.readAll();
  file.close();

  onLoadRequested(Script);

  ScriptTabs[CurrentTab]->FileName = fileName;
  twScriptTabs->setTabText(CurrentTab, QFileInfo(fileName).fileName());
}

void AScriptWindow::onLoadRequested(QString NewScript)
{
    if (!ScriptTabs[CurrentTab]->TextEdit->document()->isEmpty()) AddNewTab();
    twScriptTabs->setTabText(CurrentTab, "__123456789");
    twScriptTabs->setTabText(CurrentTab, createNewTabName());

    tmpIgnore = true;
    ScriptTabs[CurrentTab]->TextEdit->clear();
    ScriptTabs[CurrentTab]->TextEdit->append(NewScript);
    tmpIgnore = false;

    ScriptTabs[CurrentTab]->FileName.clear();

    //ui->pbRunScript->setIcon(*RedIcon);
    //ui->pbSave->setEnabled(false);
}

void AScriptWindow::on_pbSave_clicked()
{
    if (ScriptTabs.isEmpty()) return;

    QString SavedName = ScriptTabs[CurrentTab]->FileName;
    if (SavedName.isEmpty())
      {
       on_pbSaveAs_clicked();
       return;
      }

    QFile outputFile(SavedName);
    outputFile.open(QIODevice::WriteOnly);
    if(!outputFile.isOpen())
      {
        message("Unable to open file " +SavedName+ " for writing!", this);
        return;
      }

    QString Script = ScriptTabs[CurrentTab]->TextEdit->document()->toPlainText();
    QTextStream outStream(&outputFile);
    outStream << Script;
    outputFile.close();

    twScriptTabs->setTabText(CurrentTab, QFileInfo(SavedName).fileName());

    //ui->pbSave->setEnabled(true);
}

void AScriptWindow::on_pbSaveAs_clicked()
{
    if (ScriptTabs.isEmpty()) return;
    QString starter = (GlobSet->LibScripts.isEmpty()) ? GlobSet->LastOpenDir : GlobSet->LibScripts;
    if (!ScriptTabs[CurrentTab]->FileName.isEmpty()) starter = ScriptTabs[CurrentTab]->FileName;
    QString fileName = QFileDialog::getSaveFileName(this,"Save script", starter, "Script files (*.txt *.js);;All files (*.*)");
    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    if(fileInfo.suffix().isEmpty()) fileName += ".txt";

    ScriptTabs[CurrentTab]->FileName = fileName;
    AScriptWindow::on_pbSave_clicked();
}

void AScriptWindow::on_pbExample_clicked()
{
    //reading example database
    QString RecordsFilename = GlobSet->ExamplesDir + "/" + "ScriptExamples.cfg";
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
}

void AScriptWindow::fillHelper(QObject* obj, QString module, QString helpText)
{
  QStringList functions = getCustomCommandsOfObject(obj, module, true);
  functions.sort();

  QTreeWidgetItem *objItem = new QTreeWidgetItem(trwHelp);
  objItem->setText(0, module);
  QFont f = objItem->font(0);
  f.setBold(true);
  objItem->setFont(0, f);
  //objItem->setBackgroundColor(QColor(0, 0, 255, 80));
  objItem->setToolTip(0, helpText);
  for (int i=0; i<functions.size(); i++)
  {
      QStringList sl = functions.at(i).split("_:_");
      QString Fshort = sl.first();
      QString Flong  = sl.last();
      ScriptTabs[CurrentTab]->TextEdit->functionList << Fshort;

      QTreeWidgetItem *fItem = new QTreeWidgetItem(objItem);
      fItem->setText(0, Fshort);
      fItem->setText(1, Flong);

      QString retVal = "Help not provided";
      QString funcNoArgs = Fshort.remove(QRegExp("\\((.*)\\)"));
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

QString AScriptWindow::getFunctionReturnType(QString UnitFunction)
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

void AScriptWindow::onJsonTWExpanded(QTreeWidgetItem *item)
{
   ExpandedItemsInJsonTW << item->text(0);
}

void AScriptWindow::onJsonTWCollapsed(QTreeWidgetItem *item)
{
    ExpandedItemsInJsonTW.remove(item->text(0));
}

void AScriptWindow::updateJsonTree()
{
  trwJson->clear();

  for (int i=0; i<ScriptManager->interfaces.size(); i++)
    {
      InterfaceToConfig* inter = dynamic_cast<InterfaceToConfig*>(ScriptManager->interfaces[i]);
      if (!inter) continue;

      QJsonObject json = inter->Config->JSON;
      QJsonObject::const_iterator it;
      for (it = json.begin(); it != json.end(); it++)
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

void AScriptWindow::fillSubObject(QTreeWidgetItem *parent, const QJsonObject &obj)
{
  QJsonObject::const_iterator it;
  for (it = obj.begin(); it != obj.end(); it++)
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

void AScriptWindow::fillSubArray(QTreeWidgetItem *parent, const QJsonArray &arr)
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

QString AScriptWindow::getDesc(const QJsonValue &ref)
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

void AScriptWindow::onFunctionClicked(QTreeWidgetItem *item, int /*column*/)
{
  pteHelp->clear();
  //qDebug() << item->text(1);
  //QString returnType = getFunctionReturnType(item->text(0));
  //pteHelp->appendPlainText(returnType+ "  " +item->text(0)+":");

  //pteHelp->appendHtml("<b>" + item->text(1) + "</b>");
  pteHelp->appendHtml("<p style=\"color:blue;\"> " + item->text(1) + "</p>");
  pteHelp->appendPlainText(item->toolTip(0));
}

void AScriptWindow::onKeyDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
  //QString str = getKeyPath(item);
  //cteScript->insertPlainText(str);
  if (!item) return;
  showContextMenuForJsonTree(item, trwJson->mapFromGlobal(cursor().pos()));
}

QString AScriptWindow::getKeyPath(QTreeWidgetItem *item)
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

void AScriptWindow::onKeyClicked(QTreeWidgetItem* /*item*/, int /*column*/)
{
  //trwJson->resizeColumnToContents(column);
}

void AScriptWindow::onFindTextChanged(const QString &arg1)
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

void AScriptWindow::onFindTextJsonChanged(const QString &arg1)
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

void AScriptWindow::onContextMenuRequestedByJsonTree(QPoint pos)
{
  QTreeWidgetItem *item = trwJson->itemAt(pos);
  if (!item) return;

  showContextMenuForJsonTree(item, pos);
}

void AScriptWindow::showContextMenuForJsonTree(QTreeWidgetItem *item, QPoint pos)
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

void AScriptWindow::onContextMenuRequestedByHelp(QPoint pos)
{
  QTreeWidgetItem *item = trwHelp->itemAt(pos);
  if (!item) return;
  QString str = item->text(0);

  QMenu menu;
  QAction* toClipboard = menu.addAction("Add function to clipboard");

  QAction* selectedItem = menu.exec(trwHelp->mapToGlobal(pos));
  if (!selectedItem) return; //nothing was selected

  if (selectedItem == toClipboard)
    {
      QClipboard *clipboard = QApplication::clipboard();
      clipboard->setText(str);
    }
}

//void AScriptWindow::onFunctionDoubleClicked(QTreeWidgetItem *item, int /*column*/)
//{
//  QString text = item->text(0);
//  ScriptTabs[CurrentTab]->TextEdit->insertPlainText(text);
//}

void AScriptWindow::closeEvent(QCloseEvent *e)
{
  if (ScriptManager->fEngineIsRunning)
    {
      e->ignore();
      return;
    }
  ScriptManager->deleteMsgDialog();
}

bool AScriptWindow::event(QEvent *e)
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
                break;
            case QEvent::Hide :
                emit WindowHidden("script");//MW->WindowNavigator->HideWindowTriggered("gain");
                break;
            case QEvent::Show :
                emit WindowShown("script");//MW->WindowNavigator->ShowWindowTriggered("gain");
                break;
            default:;
        };

    return QMainWindow::event(e) ;
}

void AScriptWindow::onDefaulFontSizeChanged(int size)
{
    GlobSet->DefaultFontSize_ScriptWindow = size;
    for (AScriptWindowTabItem* tab : ScriptTabs)
        tab->TextEdit->SetFontSize(size);
}

QStringList AScriptWindow::getCustomCommandsOfObject(QObject *obj, QString ObjName, bool fWithArguments)
{
  QStringList commands;
  int methods = obj->metaObject()->methodCount();
  for (int i=0; i<methods; i++)
    {
      const QMetaMethod &m = obj->metaObject()->method(i);
      bool fSlot   = (m.methodType() == QMetaMethod::Slot);
      bool fPublic = (m.access() == QMetaMethod::Public);
      QString commCandidate, extra;
      if (fSlot && fPublic)
        {
          if (m.name() == "deleteLater") continue;
          if (m.name() == "help") continue;
          if (ObjName.isEmpty()) commCandidate = m.name();
          else commCandidate = ObjName + "." + m.name();

          if (fWithArguments)
            {
              commCandidate += "(";
              extra = commCandidate;

              int args = m.parameterCount();
              for (int i=0; i<args; i++)
                {
                  QString typ = m.parameterTypes().at(i);
                  if (typ == "QString") typ = "string";
                  extra += " " + typ + " " + m.parameterNames().at(i);
                  commCandidate     += " " + m.parameterNames().at(i);
                  if (i != args-1)
                    {
                      commCandidate += ", ";
                      extra += ", ";
                    }
                }
              commCandidate += " )";
              extra += " )";
              extra = QString() + m.typeName() + " " + extra;

              commCandidate += "_:_" + extra;
            }
          if (commands.isEmpty() || commands.last() != commCandidate)
             commands << commCandidate;
        }
    }
  return commands;
}

AScriptWindowTabItem::AScriptWindowTabItem(QAbstractItemModel* model)
{
    TextEdit = new CompletingTextEditClass();
    TextEdit->setLineWrapMode(QTextEdit::NoWrap);

    completer = new QCompleter(this);
    completer->setModel(model);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    //completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setFilterMode(Qt::MatchContains);
    completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->setWrapAround(false);
    TextEdit->setCompleter(completer);

    highlighter = new AHighlighterScriptWindow(TextEdit->document());
}

AScriptWindowTabItem::~AScriptWindowTabItem()
{
    delete TextEdit;
}

void AScriptWindowTabItem::UpdateHighlight()
{
    highlighter->rehighlight();
}

void AScriptWindowTabItem::WriteToJson(QJsonObject &json)
{
    if (!TextEdit) return;
    json["FileName"] = FileName;
    json["Script"] = TextEdit->document()->toPlainText();
}

void AScriptWindowTabItem::ReadFromJson(QJsonObject &json)
{
    if (!TextEdit) return;
    QString Script = json["Script"].toString();
    TextEdit->clear();
    TextEdit->append(Script);

    FileName.clear();
    FileName = json["FileName"].toString();
}

void AScriptWindow::onCurrentTabChanged(int tab)
{
    //qDebug() << "Current changed!";
    CurrentTab = tab;
}

void AScriptWindow::onRequestTabWidgetContextMenu(QPoint pos)
{
    if (pos.isNull())return;

    QMenu menu;
    int tab = twScriptTabs->tabBar()->tabAt(pos);

    QAction* add = menu.addAction("Add new tab");
    menu.addSeparator();
    QAction* remove = (tab==-1) ? 0 : menu.addAction("Close tab");
    menu.addSeparator();
    QAction* removeAll = (ScriptTabs.isEmpty()) ? 0 : menu.addAction("Close all tabs");

    QAction* selectedItem = menu.exec(twScriptTabs->mapToGlobal(pos));
    if (!selectedItem) return; //nothing was selected

    if (selectedItem == add)
      {
        AddNewTab();
      }
    else if (selectedItem == remove)
      {
        QMessageBox m(this);
        //m.setText("Confirmation.");
        m.setIcon(QMessageBox::Question);
        m.setText("Close tab "+twScriptTabs->tabText(tab)+"?");  //setInformativeText
        m.setStandardButtons(QMessageBox::Yes| QMessageBox::Cancel);
        m.setDefaultButton(QMessageBox::Cancel);
        int ret = m.exec();
        if (ret == QMessageBox::Yes) removeTab(tab);
      }
    else if (selectedItem == removeAll)
      {
        QMessageBox m(this);
        //m.setText("Confirmation.");
        m.setIcon(QMessageBox::Warning);
        m.setText("Close ALL tabs?");
        m.setStandardButtons(QMessageBox::Yes| QMessageBox::Cancel);
        m.setDefaultButton(QMessageBox::Cancel);
        int ret = m.exec();
        if (ret == QMessageBox::Yes)
        {
            clearAllTabs();
            AddNewTab();
        }
    }
}

void AScriptWindow::onScriptTabMoved(int from, int to)
{
   qDebug() << "Form->to:"<<from<<to;
   ScriptTabs.swap(from, to);
}

void AScriptWindow::AddNewTab()
{
    AScriptWindowTabItem* tab = new AScriptWindowTabItem(completitionModel);
    tab->highlighter->setCustomCommands(functions);
    tab->TextEdit->SetFontSize(GlobSet->DefaultFontSize_ScriptWindow);
    QObject::connect(tab->TextEdit, &CompletingTextEditClass::fontSizeChanged, this, &AScriptWindow::onDefaulFontSizeChanged);
    ScriptTabs.append(tab);

    twScriptTabs->addTab(ScriptTabs.last()->TextEdit, createNewTabName());
    QObject::connect(ScriptTabs.last()->TextEdit, SIGNAL(requestHelp(QString)), this, SLOT(onF1pressed(QString)));
    CurrentTab = ScriptTabs.size()-1;
    twScriptTabs->setCurrentIndex(CurrentTab);
}

QString AScriptWindow::createNewTabName()
{
    int counter = 1;
    QString res;
    bool fFound;

    do
    {
        fFound = false;
        res = QString("new_%1").arg(counter);
        for (int i=0; i<twScriptTabs->count(); i++)
            if ( twScriptTabs->tabText(i) == res )
            {
                fFound = true;
                counter++;
                break;
            }
    }
    while (fFound);

    return res;
}

void AScriptWindow::removeTab(int tab)
{
    int numTabs = twScriptTabs->count();
    if (numTabs==0) return;
    if (tab<0 || tab>numTabs-1) return;

    twScriptTabs->removeTab(tab);
    delete ScriptTabs[tab];
    ScriptTabs.removeAt(tab);

    if (ScriptTabs.isEmpty()) AddNewTab();
}

void AScriptWindow::clearAllTabs()
{
    twScriptTabs->clear();
    for (int i=0; i<ScriptTabs.size(); i++) delete ScriptTabs[i];
    ScriptTabs.clear();
}

void AScriptWindow::on_pbConfig_toggled(bool checked)
{
    frJsonBrowser->setVisible(checked);
}

void AScriptWindow::on_pbHelp_toggled(bool checked)
{
    frHelper->setVisible(checked);
}

void AScriptWindow::on_actionIncrease_font_size_triggered()
{
    onDefaulFontSizeChanged(++GlobSet->DefaultFontSize_ScriptWindow);
}

void AScriptWindow::on_actionDecrease_font_size_triggered()
{
    if (GlobSet->DefaultFontSize_ScriptWindow<1) return;

    onDefaulFontSizeChanged(--GlobSet->DefaultFontSize_ScriptWindow);
    //qDebug() << "New font size:"<<GlobSet->DefaultFontSize_ScriptWindow;
}
