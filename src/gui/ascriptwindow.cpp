#include "ascriptwindow.h"
#include "ui_ascriptwindow.h"
#include "ahighlighters.h"
#include "atextedit.h"
#include "localscriptinterfaces.h"
#include "coreinterfaces.h"
#include "histgraphinterfaces.h"
#include "interfacetoglobscript.h"
#include "amessage.h"
#include "ascriptexampleexplorer.h"
#include "aconfiguration.h"
#include "ajsontools.h"

#include "ascriptmanager.h"
#include "ajavascriptmanager.h"

#ifdef __USE_ANTS_PYTHON__
#include "apythonscriptmanager.h"
#endif

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
#include <QDesktopServices>
#include <QInputDialog>

AScriptWindow::AScriptWindow(AScriptManager* ScriptManager, GlobalSettingsClass *GlobSet, bool LightMode, QWidget *parent) :
    QMainWindow(parent), ScriptManager(ScriptManager), bLightMode(LightMode),
    ui(new Ui::AScriptWindow)
{
    if ( dynamic_cast<AJavaScriptManager*>(ScriptManager) )
    {
        ScriptLanguage = _JavaScript_;
    }
#ifdef __USE_ANTS_PYTHON__
    if ( dynamic_cast<APythonScriptManager*>(ScriptManager) )
    {
        ScriptLanguage = _PythonScript_;
    }
#endif

    if (parent)
    {
        //not a standalone window
        Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
        windowFlags |= Qt::WindowCloseButtonHint;
        this->setWindowFlags( windowFlags );
    }

    QObject::connect(ScriptManager, &AScriptManager::showMessage, this, &AScriptWindow::ShowText);
    QObject::connect(ScriptManager, &AScriptManager::requestHighlightErrorLine, this, &AScriptWindow::HighlightErrorLine);
    QObject::connect(ScriptManager, &AScriptManager::clearText, this, &AScriptWindow::ClearText);
    //retranslators:
    QObject::connect(ScriptManager, &AScriptManager::onStart, this, &AScriptWindow::receivedOnStart);
    QObject::connect(ScriptManager, &AScriptManager::onAbort, this, &AScriptWindow::receivedOnAbort);
    QObject::connect(ScriptManager, &AScriptManager::onFinish, this, &AScriptWindow::receivedOnSuccess);

    this->GlobSet = GlobSet;
    //SetStarterDir(GlobSet->LibScripts);
    ScriptManager->LibScripts = GlobSet->LibScripts;
    ScriptManager->LastOpenDir = GlobSet->LastOpenDir;
    ScriptManager->ExamplesDir = GlobSet->ExamplesDir;

    ShowEvalResult = true;
    ui->setupUi(this);
    ui->pbStop->setVisible(false);
    this->setWindowTitle("ANTS2 script");
    ui->prbProgress->setValue(0);
    ui->prbProgress->setVisible(false);
    //ui->labFileName->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->cbActivateTextReplace->setChecked(false);

    QPixmap rm(16, 16);
    rm.fill(Qt::transparent);
    QPainter b(&rm);
    b.setBrush(QBrush(Qt::red));
    b.drawEllipse(0, 0, 14, 14);
    RedIcon = new QIcon(rm);

    completitionModel = new QStringListModel(QStringList());

    //more GUI
    splMain = new QSplitter();  // upper + output with buttons
    splMain->setOrientation(Qt::Vertical);
    splMain->setChildrenCollapsible(false);
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
    hor->setMinimumHeight(60);
    splMain->addWidget(hor);
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

    splMain->addWidget(pteOut);
    ui->centralwidget->layout()->removeItem(ui->horizontalLayout);
    ui->centralwidget->layout()->addWidget(splMain);
    ui->centralwidget->layout()->addItem(ui->horizontalLayout);

    trwJson->header()->resizeSection(0, 200);

    sizes.clear();
    sizes << 800 << 70;
    splMain->setSizes(sizes);

    ui->frFindReplace->setVisible(false);

    //shortcuts
    QShortcut* Run = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(Run, &QShortcut::activated, this, &AScriptWindow::on_pbRunScript_clicked);
    QShortcut* Find = new QShortcut(QKeySequence("Ctrl+f"), this);
    connect(Find, &QShortcut::activated, this, &AScriptWindow::on_actionShow_Find_Replace_triggered);
    QShortcut* Replace = new QShortcut(QKeySequence("Ctrl+r"), this);
    connect(Replace, &QShortcut::activated, this, &AScriptWindow::on_actionReplace_widget_Ctr_r_triggered);
    QShortcut* FindFunction = new QShortcut(QKeySequence("F2"), this);
    connect(FindFunction, &QShortcut::activated, this, &AScriptWindow::onFindFunction);
    QShortcut* FindVariable = new QShortcut(QKeySequence("F3"), this);
    connect(FindVariable, &QShortcut::activated, this, &AScriptWindow::onFindVariable);
    QShortcut* GoBack = new QShortcut(QKeySequence("Alt+Left"), this);
    connect(GoBack, &QShortcut::activated, this, &AScriptWindow::onBack);
    QShortcut* GoForward = new QShortcut(QKeySequence("Alt+Right"), this);
    connect(GoForward, &QShortcut::activated, this, &AScriptWindow::onForward);

    if (!bLightMode)
        ReadFromJson();
    else
    {
        ui->pbConfig->setEnabled(false);
        twScriptTabs->setStyleSheet("QTabWidget::tab-bar { width: 0; height: 0; margin: 0; padding: 0; border: none; }");
        ui->pbExample->setText("Example");
        ui->menuTabs->setEnabled(false);
        ui->menuView->setEnabled(false);
    }

}

AScriptWindow::~AScriptWindow()
{
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
        if (bLightMode && interfaceObject) fillHelper(interfaceObject, "");
        AInterfaceToCore core(0); //dummy to extract methods
        fillHelper(&core, "core");
        newFunctions << getCustomCommandsOfObject(&core, "core", false);
        AInterfaceToMath math(0); //dummy to extract methods
        QString mathName = (ScriptLanguage == _JavaScript_ ? "math" : "MATH");
        fillHelper(&math, mathName);
        newFunctions << getCustomCommandsOfObject(&math, mathName, false);
        trwHelp->expandItem(trwHelp->itemAt(0,0));
    }
    else
    {
        fillHelper(interfaceObject, name);
        newFunctions << getCustomCommandsOfObject(interfaceObject, name, false);
    }

    // auto-read list of public slots for highlighter    
    for (int i=0; i<ScriptTabs.size(); i++)
    {
        ScriptTabs[i]->highlighter->setCustomCommands(newFunctions);
        ScriptTabs[i]->TextEdit->functionList = functionList;
    }

    //filling autocompleter
    for (int i=0; i<newFunctions.size(); i++)
        newFunctions[i] += "()";
    functions << newFunctions;
    completitionModel->setStringList(functions);

    //special "needs" of particular interface objects
    if ( dynamic_cast<AInterfaceToHist*>(interfaceObject) || dynamic_cast<AInterfaceToGraph*>(interfaceObject)) //"graph" or "hist"
       QObject::connect(interfaceObject, SIGNAL(RequestDraw(TObject*,QString,bool)), this, SLOT(onRequestDraw(TObject*,QString,bool)));

    if (bLightMode && interfaceObject && trwHelp->topLevelItemCount() > 0) trwHelp->expandItem(trwHelp->itemAt(0,0));
    else trwHelp->collapseAll();
}

void AScriptWindow::ReportError(QString error, int line)
{
   error = "<font color=\"red\">Error:</font><br>" + error;
   pteOut->appendHtml( error );
   HighlightErrorLine(line);
}

void AScriptWindow::HighlightErrorLine(int line)
{
  if (line < 0) return;

  //highlight line with error
  if (line > 1) line--;
  QTextBlock block = ScriptTabs[CurrentTab]->TextEdit->document()->findBlockByLineNumber(line);
  int loc = block.position();
  QTextCursor cur = ScriptTabs[CurrentTab]->TextEdit->textCursor();
  cur.setPosition(loc);
  ScriptTabs[CurrentTab]->TextEdit->setTextCursor(cur);
  ScriptTabs[CurrentTab]->TextEdit->ensureCursorVisible();

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

void AScriptWindow::WriteToJson()
{
    QJsonObject* ScriptWindowJsonPtr = 0;
    if ( ScriptLanguage == _JavaScript_) ScriptWindowJsonPtr = &GlobSet->ScriptWindowJson;
    else if ( ScriptLanguage == _PythonScript_) ScriptWindowJsonPtr = &GlobSet->PythonScriptWindowJson;
    if (!ScriptWindowJsonPtr) return;

    QJsonObject& json = *ScriptWindowJsonPtr;
    WriteToJson(json);
}

void AScriptWindow::WriteToJson(QJsonObject& json)
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

    QJsonArray sar;
    for (int& i : splMain->sizes()) sar << i;
    json["Sizes"] = sar;
}

void AScriptWindow::ReadFromJson()
{
    QJsonObject* ScriptWindowJsonPtr = 0;
    if ( ScriptLanguage == _JavaScript_) ScriptWindowJsonPtr = &GlobSet->ScriptWindowJson;
    else if ( ScriptLanguage == _PythonScript_) ScriptWindowJsonPtr = &GlobSet->PythonScriptWindowJson;
    if (!ScriptWindowJsonPtr) return;

    QJsonObject& json = *ScriptWindowJsonPtr;
    ReadFromJson(json);
}

void AScriptWindow::ReadFromJson(QJsonObject& json)
{
    if (json.isEmpty()) return;
    if (!json.contains("ScriptTabs")) return;

    clearAllTabs();
    QJsonArray ar = json["ScriptTabs"].toArray();
    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar[i].toObject();
        AddNewTab();
        AScriptWindowTabItem* st = ScriptTabs.last();
        st->ReadFromJson(js);
        if (st->TabName.isEmpty()) st->TabName = createNewTabName();
        if (!st->FileName.isEmpty())
        {
           QString ScriptInFile;
           if ( LoadTextFromFile(st->FileName, ScriptInFile) )
           {
               QPlainTextEdit te;
               te.appendPlainText(ScriptInFile);
               st->setModifiedStatus( !(te.document()->toPlainText() == st->TextEdit->document()->toPlainText()) );
           }
        }
        twScriptTabs->setTabText(twScriptTabs->count()-1, st->TabName);

    }
    if (ScriptTabs.isEmpty()) AddNewTab();

    CurrentTab = json["CurrentTab"].toInt();
    if (CurrentTab<0 || CurrentTab>ScriptTabs.size()-1) CurrentTab = 0;
    twScriptTabs->setCurrentIndex(CurrentTab);
    updateFileStatusIndication();

    if (json.contains("Sizes"))
    {
        QJsonArray sar = json["Sizes"].toArray();
        if (sar.size() == 2)
        {
            QList<int> sizes;
            sizes << sar.at(0).toInt(50) << sar.at(1).toInt(50);
            splMain->setSizes(sizes);
        }
    }
}

void AScriptWindow::UpdateHighlight()
{
   for (int i=0; i<ScriptTabs.size(); i++)
       ScriptTabs[i]->UpdateHighlight();
}

void AScriptWindow::SetMainSplitterSizes(QList<int> values)
{
    splMain->setSizes(values);
}

void AScriptWindow::onBusyOn()
{
    ui->pbRunScript->setEnabled(false);
}

void AScriptWindow::onBusyOff()
{
    ui->pbRunScript->setEnabled(true);
}

void AScriptWindow::ConfigureForLightMode(QString *ScriptPtr, const QString& WindowTitle, const QString &Example)
{
    LightModeScript = ScriptPtr;
    LightModeExample = Example;
    setWindowTitle(WindowTitle);

    if (LightModeScript)
    {
        ScriptTabs[CurrentTab]->TextEdit->clear();
        ScriptTabs[CurrentTab]->TextEdit->appendPlainText(*LightModeScript);
    }
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
   // if not light mode, save all tabs -> GlobSet
   if (!bLightMode)
   {
       WriteToJson();
       GlobSet->SaveANTSconfiguration();
   }

   QString Script = ScriptTabs[CurrentTab]->TextEdit->document()->toPlainText();
   //in light mode save the script directly
   if (bLightMode && LightModeScript) *LightModeScript = Script;

   //qDebug() << "Init on Start done";
   pteOut->clear();
   //AScriptWindow::ShowText("Processing script");

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

   if (!ScriptManager->getLastError().isEmpty())
   {
       AScriptWindow::ReportError("Script error: "+ScriptManager->getLastError(), -1);
   }
   else if (ScriptManager->isUncaughtException())
   {   //Script has uncaught exception
       int lineNum = ScriptManager->getUncaughtExceptionLineNumber();
       QString message = ScriptManager->getUncaughtExceptionString();
       //qDebug() << "Error message:" << message;
       //QString backtrace = engine.uncaughtExceptionBacktrace().join('\n');
       //qDebug() << "backtrace:" << backtrace;
       AScriptWindow::ReportError("Script error: "+message, lineNum);
   }
   else
   {   //success
       //qDebug() << "Script returned:" << result;
       if (!ScriptManager->isEvalAborted())
         {
            if (ShowEvalResult && result!="undefined" && !result.isEmpty()) ShowText("Script evaluation result:\n"+result);
            //else ShowText("Script evaluation: success");
         }
       else
         {
           //ShowText("Aborted!");
         }
       ui->pbRunScript->setIcon(QIcon()); //clear red icon
     }

   ScriptManager->collectGarbage();
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
  if (ScriptManager->isEngineRunning())
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
  if (bLightMode && LightModeScript) *LightModeScript = Script;
  file.close();

  onLoadRequested(Script);

  ScriptTabs[CurrentTab]->FileName = fileName;

  ScriptTabs[CurrentTab]->TabName = QFileInfo(fileName).baseName();
  twScriptTabs->setTabText(CurrentTab, ScriptTabs[CurrentTab]->TabName);
  updateFileStatusIndication();
}

void AScriptWindow::onLoadRequested(QString NewScript)
{
    if (!ScriptTabs[CurrentTab]->TextEdit->document()->isEmpty()) AddNewTab();
    //twScriptTabs->setTabText(CurrentTab, "__123456789");
    //twScriptTabs->setTabText(CurrentTab, createNewTabName());

    ScriptTabs[CurrentTab]->TextEdit->clear();
    ScriptTabs[CurrentTab]->TextEdit->appendPlainText(NewScript);

    //for examples (triggered on signal from example explorer -> do not register file name!)
    ScriptTabs[CurrentTab]->FileName.clear();
    ScriptTabs[CurrentTab]->TabName = createNewTabName();
    twScriptTabs->setTabText(CurrentTab, ScriptTabs[CurrentTab]->TabName);
    updateFileStatusIndication();
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

    if (!ScriptTabs[CurrentTab]->bExplicitlyNamed)
    {
        ScriptTabs[CurrentTab]->TabName = QFileInfo(SavedName).baseName();
        twScriptTabs->setTabText(CurrentTab, ScriptTabs[CurrentTab]->TabName);
    }

    ScriptTabs[CurrentTab]->setModifiedStatus(false);
    updateFileStatusIndication();
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
    if (bLightMode)
    {
        if (!LightModeScript)
        {
            message("Error: script pointer is not set", this);
            return;
        }

        if (!ScriptTabs[CurrentTab]->TextEdit->document()->isEmpty())
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
                *LightModeScript += "\n" + LightModeExample;
            else
                *LightModeScript = LightModeExample;
        }
        else *LightModeScript = LightModeExample;

        ScriptTabs[CurrentTab]->TextEdit->clear();
        ScriptTabs[CurrentTab]->TextEdit->appendPlainText(*LightModeScript);
        return;
    }

    //reading example database
    QString target = (ScriptLanguage == _JavaScript_ ? "ScriptExamples.cfg" : "PythonScriptExamples.cfg");
    QString RecordsFilename = GlobSet->ExamplesDir + "/" + target;
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

void AScriptWindow::fillHelper(QObject* obj, QString module)
{
  QString UnitDescription;
  AScriptInterface* io = dynamic_cast<AScriptInterface*>(obj);
  if (io)
    {
      UnitDescription = io->getDescription();
      if (ScriptLanguage == _PythonScript_) UnitDescription.remove("Multithread-capable");
    }

  QStringList functions = getCustomCommandsOfObject(obj, module, true);
  functions.sort();

  QTreeWidgetItem *objItem = new QTreeWidgetItem(trwHelp);
  objItem->setText(0, module);
  QFont f = objItem->font(0);
  f.setBold(true);
  objItem->setFont(0, f);
  //objItem->setBackgroundColor(QColor(0, 0, 255, 80));
  objItem->setToolTip(0, UnitDescription);
  for (int i=0; i<functions.size(); i++)
  {
      QStringList sl = functions.at(i).split("_:_");
      QString Fshort = sl.first();
      QString Flong  = sl.last();
      //functionList << Fshort;
      functionList << Flong;

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
      AInterfaceToConfig* inter = dynamic_cast<AInterfaceToConfig*>(ScriptManager->interfaces[i]);
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

void AScriptWindow::fillSubObject(QTreeWidgetItem *parent, const QJsonObject &obj)
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

void AScriptWindow::closeEvent(QCloseEvent* /*e*/)
{
//  qDebug() << "Script window: Close event";
//  if (ScriptManager->fEngineIsRunning)
//    {
//      e->ignore();
//      return;
//    }
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
                //qDebug() << "Script window: hide event";
                ScriptManager->hideMsgDialogs();
                emit WindowHidden( ScriptLanguage == _JavaScript_ ? "script" : "python" );
                break;
            case QEvent::Show :
                //qDebug() << "Script window: show event";
                ScriptManager->restoreMsgDialogs();
                emit WindowShown( ScriptLanguage == _JavaScript_ ? "script" : "python" );
                break;
            default:;
        };

    return QMainWindow::event(e) ;
}

void AScriptWindow::receivedOnAbort()
{
    ui->prbProgress->setValue(0);
    ui->prbProgress->setVisible(false);
    emit onAbort();
}

void AScriptWindow::receivedOnSuccess(QString eval)
{
    ui->prbProgress->setValue(0);
    ui->prbProgress->setVisible(false);
    emit success(eval);
}

void AScriptWindow::onDefaulFontSizeChanged(int size)
{
    GlobSet->DefaultFontSize_ScriptWindow = size;
    for (AScriptWindowTabItem* tab : ScriptTabs)
        tab->TextEdit->SetFontSize(size);
}

void AScriptWindow::onProgressChanged(int percent)
{
    ui->prbProgress->setValue(percent);
    ui->prbProgress->setVisible(true);
    qApp->processEvents();
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

AScriptWindowTabItem::AScriptWindowTabItem(QAbstractItemModel* model, AScriptWindow::ScriptLanguageEnum language)
{
    TextEdit = new ATextEdit();
    TextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    completer = new QCompleter(this);
    completer->setModel(model);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    //completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setFilterMode(Qt::MatchContains);
    completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->setWrapAround(false);
    TextEdit->setCompleter(completer);

    if (language == AScriptWindow::_PythonScript_)
        highlighter = new AHighlighterPythonScriptWindow(TextEdit->document());
    else
        highlighter = new AHighlighterScriptWindow(TextEdit->document());

    TextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(TextEdit, &ATextEdit::customContextMenuRequested, this, &AScriptWindowTabItem::onCustomContextMenuRequested);
    connect(TextEdit, &ATextEdit::lineNumberChanged, this, &AScriptWindowTabItem::onLineNumberChanged);
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
    json["TabName"] = TabName;
    json["bExplicitlyNamed"] = bExplicitlyNamed;
    json["Script"] = TextEdit->document()->toPlainText();
}

void AScriptWindowTabItem::ReadFromJson(QJsonObject &json)
{
    if (!TextEdit) return;
    QString Script = json["Script"].toString();
    TextEdit->clear();
    TextEdit->appendPlainText(Script);
    FileName.clear();
    FileName = json["FileName"].toString();

    bExplicitlyNamed = false;
    parseJson(json, "bExplicitlyNamed", bExplicitlyNamed);

    if (json.contains("TabName")) TabName = json["TabName"].toString();
    else
    {
        //compatibility
        if (!FileName.isEmpty())
        {
            QFileInfo fi(FileName);
            TabName = fi.baseName();
        }
        else TabName.clear();
    }
}

/*
QTextEdit holds a QTextDocument object which can be retrieved using the document() method.
You can also set your own document object using setDocument(). QTextDocument emits a textChanged() signal
 if the text changes and it also provides a isModified() function which will return true if the text has been modified
 since it was either loaded or since the last call to setModified with false as argument.
 In addition it provides methods for undo and redo.
*/

bool AScriptWindowTabItem::wasModified() const
{
    return TextEdit->document()->isModified();
}

void AScriptWindowTabItem::setModifiedStatus(bool flag)
{
    TextEdit->document()->setModified(flag);
}

void AScriptWindow::onCurrentTabChanged(int tab)
{
    //qDebug() << "Current changed!" << tab << ScriptTabs.size();
    CurrentTab = tab;

    updateFileStatusIndication();
    applyTextFindState();
}

QIcon makeIcon(int h)
{
    QPixmap pm(h-2, h-2);
    pm.fill(Qt::transparent);
    QPainter b(&pm);
    b.setBrush(QBrush(Qt::yellow));
    b.drawEllipse(0, 2, h-5, h-5);
    return QIcon(pm);
}


void AScriptWindow::updateFileStatusIndication()
{
    if (CurrentTab < 0 || CurrentTab >= ScriptTabs.size()) return;

    QString fileName = ScriptTabs.at(CurrentTab)->FileName;
    const bool bWasModified = ScriptTabs.at(CurrentTab)->wasModified();

    ui->labNotSaved->setVisible(fileName.isEmpty());

    QString s;
    if (fileName.isEmpty())
        ui->labWasModified->setVisible(false);
    else
    {
        ui->labWasModified->setVisible(bWasModified);

#ifdef Q_OS_WIN32
        fileName.replace("/", "\\");
#endif

        s = fileName;
    }
    ui->pbFileName->setText(s);
}

void AScriptWindow::on_pbFileName_clicked()
{
    QString s = ScriptTabs.at(CurrentTab)->FileName;
    QFileInfo fi(s);
    QString path = fi.path();
    pteOut->appendPlainText(path);
    QDesktopServices::openUrl(QUrl("file:///"+path, QUrl::TolerantMode));
}

void AScriptWindow::onRequestTabWidgetContextMenu(QPoint pos)
{
    if (bLightMode) return;

    if (pos.isNull()) return;

    QMenu menu;
    int tab = twScriptTabs->tabBar()->tabAt(pos);

    QAction* add = menu.addAction("Add new tab");
    menu.addSeparator();
    QAction* rename = (tab == -1 ? 0 : menu.addAction("Rename tab") );
    menu.addSeparator();
    QAction* remove = (tab == -1 ? 0 : menu.addAction("Close tab") );
    menu.addSeparator();
    QAction* removeAll = (ScriptTabs.isEmpty()) ? 0 : menu.addAction("Close all tabs");

    QAction* selectedItem = menu.exec(twScriptTabs->mapToGlobal(pos));
    if (!selectedItem) return; //nothing was selected

    if (selectedItem == add)            AddNewTab();
    else if (selectedItem == remove)    askRemoveTab(tab);
    else if (selectedItem == removeAll) on_actionRemove_all_tabs_triggered();
    else if (selectedItem == rename)    renameTab(tab);
}

void AScriptWindow::onScriptTabMoved(int from, int to)
{
   //qDebug() << "Form->to:"<<from<<to;
   ScriptTabs.swap(from, to);
}

void AScriptWindow::AddNewTab()
{
    AScriptWindowTabItem* tab = new AScriptWindowTabItem(completitionModel, ScriptLanguage);
    tab->highlighter->setCustomCommands(functions);
    tab->TextEdit->functionList = functionList;

    if (GlobSet->DefaultFontFamily_ScriptWindow.isEmpty())
      {
         tab->TextEdit->SetFontSize(GlobSet->DefaultFontSize_ScriptWindow);
      }
    else
      {
        QFont font(GlobSet->DefaultFontFamily_ScriptWindow, GlobSet->DefaultFontSize_ScriptWindow, GlobSet->DefaultFontWeight_ScriptWindow, GlobSet->DefaultFontItalic_ScriptWindow);
        tab->TextEdit->setFont(font);
      }

    connect(tab->TextEdit, &ATextEdit::fontSizeChanged, this, &AScriptWindow::onDefaulFontSizeChanged);
    connect(tab->TextEdit, &ATextEdit::requestHelp, this, &AScriptWindow::onF1pressed);

    ScriptTabs.append(tab);
    twScriptTabs->addTab(tab->TextEdit, createNewTabName());

    CurrentTab = ScriptTabs.size()-1;
    twScriptTabs->setCurrentIndex(CurrentTab);

    connect(tab->TextEdit->document(), &QTextDocument::modificationChanged, this, &AScriptWindow::updateFileStatusIndication);
    connect(tab, &AScriptWindowTabItem::requestFindText, this, &AScriptWindow::onFindSelected);
    connect(tab, &AScriptWindowTabItem::requestReplaceText, this, &AScriptWindow::onReplaceSelected);
    connect(tab, &AScriptWindowTabItem::requestFindFunction, this, &AScriptWindow::onFindFunction);
    connect(tab, &AScriptWindowTabItem::requestFindVariable, this, &AScriptWindow::onFindVariable);
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

#include <QFontDialog>
void AScriptWindow::on_actionSelect_font_triggered()
{
  bool ok;
  QFont font = QFontDialog::getFont(
                  &ok,
                  QFont(GlobSet->DefaultFontFamily_ScriptWindow,
                        GlobSet->DefaultFontSize_ScriptWindow,
                        GlobSet->DefaultFontWeight_ScriptWindow,
                        GlobSet->DefaultFontItalic_ScriptWindow),
                  this);
  if (!ok) return;

  GlobSet->DefaultFontFamily_ScriptWindow = font.family();
  GlobSet->DefaultFontSize_ScriptWindow = font.pointSize();
  GlobSet->DefaultFontWeight_ScriptWindow = font.weight();
  GlobSet->DefaultFontItalic_ScriptWindow = font.italic();

  for (AScriptWindowTabItem* tab : ScriptTabs)
      tab->TextEdit->setFont(font);
}

void AScriptWindow::on_actionShow_all_messenger_windows_triggered()
{
    ScriptManager->restoreMsgDialogs();
}

void AScriptWindow::on_actionHide_all_messenger_windows_triggered()
{
    ScriptManager->hideMsgDialogs();
}

void AScriptWindow::on_actionClear_unused_messenger_windows_triggered()
{
    AJavaScriptManager* JSM = dynamic_cast<AJavaScriptManager*>(ScriptManager);
    if (JSM) JSM->clearUnusedMsgDialogs();
}

void AScriptWindow::on_actionClose_all_messenger_windows_triggered()
{
    AJavaScriptManager* JSM = dynamic_cast<AJavaScriptManager*>(ScriptManager);
    if (JSM) JSM->closeAllMsgDialogs();
}

void AScriptWindow::on_actionAdd_new_tab_triggered()
{
    AddNewTab();
}

void AScriptWindow::askRemoveTab(int tab)
{
    if (tab < 0 || tab >= ScriptTabs.size()) return;

    QMessageBox m(this);
    //m.setText("Confirmation.");
    m.setIcon(QMessageBox::Question);
    m.setText("Close tab "+twScriptTabs->tabText(tab)+"?");  //setInformativeText
    m.setStandardButtons(QMessageBox::Yes| QMessageBox::Cancel);
    m.setDefaultButton(QMessageBox::Cancel);
    int ret = m.exec();
    if (ret == QMessageBox::Yes) removeTab(tab);
}

void AScriptWindow::renameTab(int tab)
{
    if (tab < 0 || tab >= ScriptTabs.size()) return;

    bool ok;
    QString text = QInputDialog::getText(this, "Input text",
                                             "New name for the tab:", QLineEdit::Normal,
                                             ScriptTabs.at(tab)->TabName, &ok);
    if (ok && !text.isEmpty())
    {

       ScriptTabs[tab]->TabName = text;
       ScriptTabs[tab]->bExplicitlyNamed = true;
       twScriptTabs->setTabText(tab, text);
    }
}

void AScriptWindow::on_actionRemove_current_tab_triggered()
{
    askRemoveTab(CurrentTab);
}

void AScriptWindow::on_actionRemove_all_tabs_triggered()
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

void AScriptWindow::on_actionStore_all_tabs_triggered()
{
    if (ScriptTabs.isEmpty()) return;
    QString starter = GlobSet->LastOpenDir;
    QString fileName = QFileDialog::getSaveFileName(this,"Save session", starter, "Json files (*.json);;All files (*.*)");
    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    if(fileInfo.suffix().isEmpty()) fileName += ".json";

    QJsonObject json;
    WriteToJson(json);
    SaveJsonToFile(json, fileName);
}

void AScriptWindow::on_actionRestore_session_triggered()
{
    if (ScriptTabs.size() == 1 && ScriptTabs.at(0)->TextEdit->document()->isEmpty())
    {
        //empty - do not ask confirmation
    }
    else
    {
        QMessageBox m(this);
        m.setText("Confirmation.");
        m.setIcon(QMessageBox::Warning);
        m.setText("This will close all tabs and unsaved data will be lost.\nContinue?");
        m.setStandardButtons(QMessageBox::Yes| QMessageBox::Cancel);
        m.setDefaultButton(QMessageBox::Cancel);
        int ret = m.exec();
        if (ret != QMessageBox::Yes) return;
    }

    QString starter = GlobSet->LastOpenDir;
    QString fileName = QFileDialog::getOpenFileName(this, "Load script", starter, "Json files (*.json);;All files (*.*)");
    if (fileName.isEmpty()) return;

    QJsonObject json;
    LoadJsonFromFile(json, fileName);
    ReadFromJson(json);
}

void AScriptWindow::on_pbCloseFindReplaceFrame_clicked()
{
    ui->frFindReplace->hide();
    applyTextFindState();
}

void AScriptWindow::on_actionShow_Find_Replace_triggered()
{
    if (ui->frFindReplace->isVisible())
    {
        if (ui->cbActivateTextReplace->isChecked())
            ui->cbActivateTextReplace->setChecked(false);
        else
        {
            if (ScriptTabs[CurrentTab]->TextEdit->textCursor().selectedText() == ui->leFind->text())
            {
                ui->frFindReplace->setVisible(false);
                return;
            }
        }
    }
    else ui->frFindReplace->setVisible(true);

    onFindSelected();
}

void AScriptWindow::onFindSelected()
{
    ui->frFindReplace->setVisible(true);
    ui->cbActivateTextReplace->setChecked(false);

    QTextCursor tc = ScriptTabs[CurrentTab]->TextEdit->textCursor();
    QString sel = tc.selectedText();
    //if (!sel.isEmpty())
    //    ui->leFind->setText(sel);

    if (sel.isEmpty())
    {
        tc.select(QTextCursor::WordUnderCursor);
        sel = tc.selectedText();
    }

    ui->leFind->setText(sel);

    ui->leFind->setFocus();
    ui->leFind->selectAll();

    applyTextFindState();
}

void AScriptWindow::on_actionReplace_widget_Ctr_r_triggered()
{
    if (ui->frFindReplace->isVisible())
    {
        if (!ui->cbActivateTextReplace->isChecked())
            ui->cbActivateTextReplace->setChecked(true);
        else
        {
            if (ScriptTabs[CurrentTab]->TextEdit->textCursor().selectedText() == ui->leFind->text())
            {
                ui->frFindReplace->setVisible(false);
                return;
            }
        }
    }
    else ui->frFindReplace->setVisible(true);

    onReplaceSelected();
}

void AScriptWindow::onReplaceSelected()
{
    ui->frFindReplace->setVisible(true);
    ui->cbActivateTextReplace->setChecked(true);

    QTextCursor tc = ScriptTabs[CurrentTab]->TextEdit->textCursor();
    QString sel = tc.selectedText();
    //if (sel.isEmpty())
    //{
    //    ui->leFind->setFocus();
    //    ui->leFind->selectAll();
    //}
    //else
    //{

    if (sel.isEmpty())
    {
        tc.select(QTextCursor::WordUnderCursor);
        sel = tc.selectedText();
    }

        ui->leFind->setText(sel);
        ui->leReplace->setFocus();
        ui->leReplace->selectAll();
    //}

    applyTextFindState();
}

void AScriptWindow::onFindFunction()
{
    ATextEdit* te = ScriptTabs[CurrentTab]->TextEdit;
    QTextDocument* d = te->document();
    QTextCursor tc = te->textCursor();
    QString name = tc.selectedText();
    if (name.isEmpty())
    {
        tc.select(QTextCursor::WordUnderCursor);
        name = tc.selectedText();
    }

    QStringList sl = name.split("(");
    if (sl.size() > 0) name = sl.first();
    QRegExp sp("\\bfunction\\s+" + name + "\\s*" + "\\(");
    //qDebug() << "Looking for:"<<sp;

    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;

    tc = d->find(sp, 0, flags);

    if (tc.isNull())
    {
        message("Function definition for " + name + " not found", this);
        return;
    }

    QTextCursor tc_copy = QTextCursor(tc);
    tc_copy.setPosition(tc_copy.anchor(), QTextCursor::MoveAnchor); //position
    te->setTextCursor(tc_copy);
    te->ensureCursorVisible();

    QTextCharFormat tf;
    tf.setBackground(Qt::blue);
    tf.setForeground(Qt::white);
    tf.setUnderlineStyle(QTextCharFormat::SingleUnderline);

    QTextEdit::ExtraSelection es;
    es.cursor = tc;
    es.format = tf;

    QList<QTextEdit::ExtraSelection> esList = te->extraSelections();
    esList << es;
    te->setExtraSelections(esList);
}

void AScriptWindow::onFindVariable()
{
    ATextEdit* te = ScriptTabs[CurrentTab]->TextEdit;
    QTextDocument* d = te->document();
    QTextCursor tc = te->textCursor();
    QString name = tc.selectedText();
    if (name.isEmpty())
    {
        tc.select(QTextCursor::WordUnderCursor);
        name = tc.selectedText();
    }

    QStringList sl = name.split("(");
    if (sl.size() > 0) name = sl.first();
    QRegExp sp("\\bvar\\s+" + name + "\\b");
    //qDebug() << "Looking for:"<<sp;

    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;

    tc = d->find(sp, 0, flags);

    if (tc.isNull())
    {
        message("Variable definition for " + name + " not found", this);
        return;
    }

    QTextCursor tc_copy = QTextCursor(tc);
    tc_copy.setPosition(tc_copy.anchor(), QTextCursor::MoveAnchor); //position
    te->setTextCursor(tc_copy);
    te->ensureCursorVisible();

    QTextCharFormat tf;
    tf.setBackground(Qt::blue);
    tf.setForeground(Qt::white);
    tf.setUnderlineStyle(QTextCharFormat::SingleUnderline);

    QTextEdit::ExtraSelection es;
    es.cursor = tc;
    es.format = tf;

    QList<QTextEdit::ExtraSelection> esList = te->extraSelections();
    esList << es;
    te->setExtraSelections(esList);
}

void AScriptWindow::onBack()
{
    ScriptTabs[CurrentTab]->goBack();
}

void AScriptWindow::onForward()
{
    ScriptTabs[CurrentTab]->goForward();
}

void AScriptWindow::on_pbFindNext_clicked()
{
    findText(true);
}

void AScriptWindow::on_pbFindPrevious_clicked()
{
    findText(false);
}

void AScriptWindow::findText(bool bForward)
{
    ATextEdit* te = ScriptTabs[CurrentTab]->TextEdit;
    QTextDocument* d = te->document();

    QString textToFind = ui->leFind->text();
    const int oldPos = te->textCursor().anchor();
    QTextDocument::FindFlags flags;
    if (!bForward)
        flags = flags | QTextDocument::FindBackward;
    if (ui->cbFindTextCaseSensitive->isChecked())
        flags = flags | QTextDocument::FindCaseSensitively;
    if (ui->cbFindTextWholeWords->isChecked())
        flags = flags | QTextDocument::FindWholeWords;

    QTextCursor tc = d->find(textToFind, te->textCursor(), flags);

    if (!tc.isNull())
        if (oldPos == tc.anchor())
        {
            //just because the cursor was already on the start of the searched string
            tc = d->find(textToFind, tc, flags);
        }

    if (tc.isNull())
    {
        if (bForward)
            tc = d->find(textToFind, 0, flags);
        else
            tc = d->find(textToFind, d->characterCount()-1, flags);

        if (tc.isNull())
        {
            message("No matches found", this);
            return;
        }
    }

    QTextCursor tc_copy = QTextCursor(tc);
    tc_copy.setPosition(tc_copy.anchor(), QTextCursor::MoveAnchor); //position
    te->setTextCursor(tc_copy);
    te->ensureCursorVisible();

    QTextCharFormat tf;
    tf.setBackground(Qt::blue);
    tf.setForeground(Qt::white);
    tf.setUnderlineStyle(QTextCharFormat::SingleUnderline);

    QTextEdit::ExtraSelection es;
    es.cursor = tc;
    es.format = tf;

    QList<QTextEdit::ExtraSelection> esList = te->extraSelections();
    esList << es;
    te->setExtraSelections(esList);
}

void AScriptWindow::on_leFind_textChanged(const QString & /*arg1*/)
{
    applyTextFindState();
}

void AScriptWindow::applyTextFindState()
{
    if (CurrentTab < 0 || CurrentTab >= ScriptTabs.size()) return;

    bool bActive = ui->frFindReplace->isVisible();
    QString Text = (bActive ? ui->leFind->text() : "");

    ATextEdit* te = ScriptTabs[CurrentTab]->TextEdit;
    te->FindString = Text;
    te->RefreshExtraHighlight();
}

void AScriptWindow::on_pbReplaceOne_clicked()
{
    ATextEdit* te = ScriptTabs[CurrentTab]->TextEdit;
    QTextDocument* d = te->document();

    QString textToFind = ui->leFind->text();
    QString textReplacement = ui->leReplace->text();
    const int oldPos = te->textCursor().anchor();
    QTextDocument::FindFlags flags;
    if (ui->cbFindTextCaseSensitive->isChecked())
        flags = flags | QTextDocument::FindCaseSensitively;
    if (ui->cbFindTextWholeWords->isChecked())
        flags = flags | QTextDocument::FindWholeWords;

    QTextCursor tc = d->find(textToFind, te->textCursor(), flags);
    if (tc.isNull() || oldPos != tc.anchor())
    {
        message("Not found or cursor is not in front of the match pattern. Use find buttons above", this);
        return;
    }

    tc.insertText(textReplacement);
    te->setTextCursor(tc);
}

void AScriptWindow::on_pbReplaceAll_clicked()
{
    ATextEdit* te = ScriptTabs[CurrentTab]->TextEdit;
    QTextDocument* d = te->document();

    QString textToFind = ui->leFind->text();
    QString textReplacement = ui->leReplace->text();

    QTextDocument::FindFlags flags;
    if (ui->cbFindTextCaseSensitive->isChecked())
        flags = flags | QTextDocument::FindCaseSensitively;
    if (ui->cbFindTextWholeWords->isChecked())
        flags = flags | QTextDocument::FindWholeWords;

    int numReplacements = 0;
    QTextCursor tc = d->find(textToFind, 0, flags);
    while (!tc.isNull())
    {
        tc.insertText(textReplacement);
        numReplacements++;
        tc = d->find(textToFind, tc, flags);
    }
    message("Replacements performed: " + QString::number(numReplacements), this);
}

void AScriptWindowTabItem::onCustomContextMenuRequested(const QPoint& pos)
{
    QMenu menu;

    QAction* findSel = menu.addAction("Find selected text (Ctrl + F)");
    QAction* findFunct = menu.addAction("Find function definition (F2)");
    QAction* findVar = menu.addAction("Find variable definition (F3)");
    menu.addSeparator();
    QAction* replaceSel = menu.addAction("Replace selected text (Ctrl + R)");
    menu.addSeparator();
    QAction* shiftBack = menu.addAction("Go back (Alt + Left)");
    QAction* shiftForward = menu.addAction("Go forward (Alt + Right)");

    QAction* selectedItem = menu.exec(TextEdit->mapToGlobal(pos));
    if (!selectedItem) return; //nothing was selected

    if (selectedItem == findSel)         emit requestFindText();
    if (selectedItem == replaceSel)      emit requestReplaceText();
    else if (selectedItem == findFunct)  emit requestFindFunction();
    else if (selectedItem == findVar)    emit requestFindVariable();
    else if (selectedItem == replaceSel) emit requestReplaceText();

    else if (selectedItem == shiftBack) goBack();
    else if (selectedItem == shiftForward) goForward();
}

void AScriptWindowTabItem::goBack()
{
    if (indexInVisitedLineNumber >= 1 && indexInVisitedLineNumber < VisitedLineNumber.size())
    {
        indexInVisitedLineNumber--;
        int goTo = VisitedLineNumber.at(indexInVisitedLineNumber);
        QTextCursor tc = TextEdit->textCursor();
        int nowAt = tc.blockNumber();
        if (nowAt == goTo) return;
        else if (nowAt < goTo) tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, goTo-nowAt );
        else if (nowAt > goTo) tc.movePosition(QTextCursor::Up,   QTextCursor::MoveAnchor, nowAt-goTo );
        TextEdit->setTextCursorSilently(tc);
        TextEdit->ensureCursorVisible();
    }
}

void AScriptWindowTabItem::goForward()
{
    if (indexInVisitedLineNumber >= 0 && indexInVisitedLineNumber < VisitedLineNumber.size()-1)
    {
        indexInVisitedLineNumber++;
        int goTo = VisitedLineNumber.at(indexInVisitedLineNumber);
        QTextCursor tc = TextEdit->textCursor();
        int nowAt = tc.blockNumber();
        if (nowAt == goTo) return;
        else if (nowAt < goTo) tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, goTo-nowAt );
        else if (nowAt > goTo) tc.movePosition(QTextCursor::Up,   QTextCursor::MoveAnchor, nowAt-goTo );
        TextEdit->setTextCursorSilently(tc);
        TextEdit->ensureCursorVisible();
    }
}

void AScriptWindowTabItem::onLineNumberChanged(int lineNumber)
{
    if (!VisitedLineNumber.isEmpty())
        if (VisitedLineNumber.last() == lineNumber) return;

    VisitedLineNumber.append(lineNumber);
    if (VisitedLineNumber.size() > maxLineNumbers) VisitedLineNumber.removeFirst();

    indexInVisitedLineNumber = VisitedLineNumber.size() - 1;
}
