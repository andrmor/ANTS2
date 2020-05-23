#include "ascriptwindow.h"
#include "ui_ascriptwindow.h"
#include "ahighlighters.h"
#include "atextedit.h"
#include "ascriptinterface.h"
#include "localscriptinterfaces.h"
#include "acore_si.h"
#include "amath_si.h"
#include "aconfig_si.h"
#include "histgraphinterfaces.h"
#include "amessage.h"
#include "ascriptexampleexplorer.h"
#include "aconfiguration.h"
#include "ajsontools.h"

#include "ascriptmanager.h"
#include "ajavascriptmanager.h"

#ifdef __USE_ANTS_PYTHON__
#include "apythonscriptmanager.h"
#endif

#include "aglobalsettings.h"
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
//#include <QTextDocumentFragment>
#include <QHeaderView>

AScriptWindow::AScriptWindow(AScriptManager* ScriptManager, bool LightMode, QWidget *parent) :
    AGuiWindow( (dynamic_cast<AJavaScriptManager*>(ScriptManager) ? "script" : "python" ), parent),
    ScriptManager(ScriptManager), bLightMode(LightMode),
    ui(new Ui::AScriptWindow), GlobSet(AGlobalSettings::getInstance())
{
    ui->setupUi(this);

    if ( dynamic_cast<AJavaScriptManager*>(ScriptManager) )
    {
        ScriptLanguage = AScriptLanguageEnum::JavaScript;
        setWindowTitle("JavaScript");
    }
#ifdef __USE_ANTS_PYTHON__
    if ( dynamic_cast<APythonScriptManager*>(ScriptManager) )
    {
        ScriptLanguage = AScriptLanguageEnum::Python;
        setWindowTitle("Python script");
    }
#endif

    if (parent)
    {
        //not a standalone window
        Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
        windowFlags |= Qt::WindowCloseButtonHint;
        //windowFlags |= Qt::Tool;
        this->setWindowFlags( windowFlags );
    }

    QObject::connect(ScriptManager, &AScriptManager::showMessage, this, &AScriptWindow::showHtmlText);
    QObject::connect(ScriptManager, &AScriptManager::showPlainTextMessage, this, &AScriptWindow::showPlainText);
    QObject::connect(ScriptManager, &AScriptManager::requestHighlightErrorLine, this, &AScriptWindow::highlightErrorLine);
    QObject::connect(ScriptManager, &AScriptManager::clearText, this, &AScriptWindow::clearOutputText);
    //retranslators:
    QObject::connect(ScriptManager, &AScriptManager::onStart, this, &AScriptWindow::receivedOnStart);
    QObject::connect(ScriptManager, &AScriptManager::onAbort, this, &AScriptWindow::receivedOnAbort);
    QObject::connect(ScriptManager, &AScriptManager::onFinish, this, &AScriptWindow::receivedOnSuccess);

    ScriptManager->LibScripts  = &GlobSet.LibScripts;
    ScriptManager->LastOpenDir = &GlobSet.LastOpenDir;
    ScriptManager->ExamplesDir = &GlobSet.ExamplesDir;

    ui->pbStop->setVisible(false);
    ui->prbProgress->setValue(0);
    ui->prbProgress->setVisible(false);
    ui->cbActivateTextReplace->setChecked(false);
    ui->frFindReplace->setVisible(false);
    ui->frAccept->setVisible(false);

    QPixmap rm(16, 16);
    rm.fill(Qt::transparent);
    QPainter b(&rm);
    b.setBrush(QBrush(Qt::red));
    b.drawEllipse(0, 0, 14, 14);
    RedIcon = new QIcon(rm);

    if (bLightMode)
    {
        StandaloneTab = new ATabRecord(Functions, ScriptLanguage);
        formatTab(StandaloneTab);
        bShowResult = false;
    }
    else
    {
        twBooks = new QTabWidget();
        twBooks->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(twBooks, &QTabWidget::customContextMenuRequested, this, &AScriptWindow::twBooks_customContextMenuRequested);
        connect(twBooks, &QTabWidget::currentChanged, this, &AScriptWindow::twBooks_currentChanged);
        connect(twBooks->tabBar(), &QTabBar::tabMoved, this, &AScriptWindow::onBookTabMoved);
        twBooks->setMovable(true);
        twBooks->setTabShape(QTabWidget::Triangular);
        twBooks->setMinimumHeight(25);
        addNewBook();
    }

    createGuiElements();

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
    QShortcut* DoAlign = new QShortcut(QKeySequence("Ctrl+I"), this);
    connect(DoAlign, &QShortcut::activated, [&](){getTab()->TextEdit->align();});
    //QShortcut* DoPaste = new QShortcut(QKeySequence("Ctrl+V"), this);
    //connect(DoPaste, &QShortcut::activated, [&](){getTab()->TextEdit->paste();});

    if (bLightMode)
    {
        ui->pbConfig->setEnabled(false);
        //getTabWidget()->setStyleSheet("QTabWidget::tab-bar { width: 0; height: 0; margin: 0; padding: 0; border: none; }");
        ui->pbExample->setText("Example");
        ui->menuTabs->setEnabled(false);
        ui->menuView->setEnabled(false);
    }
    else ReadFromJson();

}

AScriptWindow::~AScriptWindow()
{
    bShutDown = true;

    delete ui;
    delete RedIcon;
    delete ScriptManager;

    if (bLightMode)
    {
        delete StandaloneTab;
        StandaloneTab = nullptr;
    }
    else
    {
        for (int i = (int)ScriptBooks.size() - 1; i > -1; i--)
        {
            for (ATabRecord * r : ScriptBooks[i].Tabs) delete r;
            ScriptBooks[i].Tabs.clear();
        }
    }
}

void AScriptWindow::createGuiElements()
{
    splMain = new QSplitter();  // upper + output with buttons
    splMain->setOrientation(Qt::Vertical);
    splMain->setChildrenCollapsible(false);

    QSplitter* hor = new QSplitter(); //all upper widgets are here
    hor->setContentsMargins(0,0,0,0);
      if (bLightMode) hor->addWidget(StandaloneTab->TextEdit);
      else            hor->addWidget(twBooks);

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
            QObject::connect(trwHelp, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onFunctionClicked(QTreeWidgetItem*,int)));
            QObject::connect(trwHelp, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onContextMenuRequestedByHelp(QPoint)));
            sh->addWidget(trwHelp);

            pteHelp = new QPlainTextEdit();
            pteHelp->setReadOnly(true);
            pteHelp->setMinimumHeight(20);
          sh->addWidget(pteHelp);
          QList<int> sizes;
          sizes << 800 << 175;
          sh->setSizes(sizes);

          vb1->addWidget(sh);

            leFind = new QLineEdit("Find");
            leFind->setMinimumHeight(20);
            leFind->setMaximumHeight(20);
            connect(leFind, &QLineEdit::textChanged, this, &AScriptWindow::onFindTextChanged);
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

        vbl->addWidget(trwJson);

          leFindJ = new QLineEdit("Find");
          leFindJ->setMinimumHeight(20);
          leFindJ->setMaximumHeight(20);
          QObject::connect(leFindJ, &QLineEdit::textChanged, this, &AScriptWindow::onFindTextJsonChanged);
        vbl->addWidget(leFindJ);
        frJsonBrowser->setLayout(vbl);

        splHelp->addWidget(frJsonBrowser);

    sizes.clear();
    sizes << 500 << 500 << 500;
    splHelp->setSizes(sizes);
    frJsonBrowser->setVisible(false);

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

    ui->centralwidget->layout()->removeWidget(ui->frAccept);
    ui->centralwidget->layout()->addWidget(ui->frAccept);

    trwJson->header()->resizeSection(0, 200);

    sizes.clear();
    sizes << 800 << 70;
    splMain->setSizes(sizes);
}

void AScriptWindow::RegisterInterfaceAsGlobal(AScriptInterface *interface)
{
    ScriptManager->RegisterInterfaceAsGlobal(interface);

    doRegister(interface, "");
}

void AScriptWindow::RegisterCoreInterfaces(bool bCore, bool bMath)
{
    ScriptManager->RegisterCoreInterfaces(bCore, bMath);

    if (bCore)
    {
        ACore_SI core(0); //dummy to extract method names
        doRegister(&core, "core");
    }
    if (bMath)
    {
        AMath_SI math(0); //dummy to extract method names
        QString mathName = (ScriptLanguage == AScriptLanguageEnum::JavaScript ? "math" : "MATH");
        doRegister(&math, mathName);
    }
}

void AScriptWindow::RegisterInterface(AScriptInterface *interface, const QString &name)
{
    ScriptManager->RegisterInterface(interface, name);

    doRegister(interface, name);

    //special "needs" of particular interface objects
    if ( dynamic_cast<AInterfaceToHist*>(interface) || dynamic_cast<AInterfaceToGraph*>(interface)) //"graph" or "hist"
       QObject::connect(interface, SIGNAL(RequestDraw(TObject*,QString,bool)), this, SLOT(onRequestDraw(TObject*,QString,bool)));
}

void AScriptWindow::doRegister(AScriptInterface *interface, const QString &name)
{
    // populating help
    fillHelper(interface, name);
    QStringList newFunctions;
    newFunctions << getListOfMethods(interface, name, false);
    appendDeprecatedOrRemovedMethods(interface, name);

    //filling autocompleter
    for (int i=0; i<newFunctions.size(); i++) newFunctions[i] += "()";
    Functions << newFunctions;
}

void AScriptWindow::UpdateGui()
{
    if (bLightMode) UpdateTab(StandaloneTab);
    else
    {
        QList<ATabRecord *> ScriptTabs = getScriptTabs();
        for (int i = 0; i < ScriptTabs.size(); i++)
            UpdateTab(ScriptTabs[i]);
    }

    if (bLightMode && trwHelp->topLevelItemCount() > 0)
        trwHelp->expandItem(trwHelp->itemAt(0,0));
    else trwHelp->collapseAll();
}

void AScriptWindow::ReportError(QString error, int line)
{
   error = "<font color=\"red\">Error:</font><br>" + error;
   pteOut->appendHtml( error );
   highlightErrorLine(line);
}

void AScriptWindow::highlightErrorLine(int line)
{
  if (line < 0) return;

  //highlight line with error
  if (line > 1) line--;
  QTextBlock block = getTab()->TextEdit->document()->findBlockByLineNumber(line);
  int loc = block.position();
  QTextCursor cur = getTab()->TextEdit->textCursor();
  cur.setPosition(loc);
  getTab()->TextEdit->setTextCursor(cur);
  getTab()->TextEdit->ensureCursorVisible();

  int length = block.text().split("\n").at(0).length();
  cur.movePosition(cur.Right, cur.KeepAnchor, length);

  QTextCharFormat tf = block.charFormat();
  tf.setBackground(QBrush(Qt::yellow));
  QTextEdit::ExtraSelection es;
  es.cursor = cur;
  es.format = tf;

  QList<QTextEdit::ExtraSelection> esList;
  esList << es;
  getTab()->TextEdit->setExtraSelections(esList);
}

void AScriptWindow::WriteToJson()
{
    /*
    if (bLightMode) return;

    QJsonObject* ScriptWindowJsonPtr = 0;
    if ( ScriptLanguage == _JavaScript_) ScriptWindowJsonPtr = &GlobSet.ScriptWindowJson;
    else if ( ScriptLanguage == _PythonScript_) ScriptWindowJsonPtr = &GlobSet.PythonScriptWindowJson;
    if (!ScriptWindowJsonPtr) return;

    QJsonObject& json = *ScriptWindowJsonPtr;
    WriteToJson(json);
    */
}

void AScriptWindow::WriteToJson(QJsonObject& json)
{
    /*
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
    */
}

void AScriptWindow::ReadFromJson()
{
    /*
    if (bLightMode) return;

    QJsonObject* ScriptWindowJsonPtr = 0;
    if ( ScriptLanguage == _JavaScript_) ScriptWindowJsonPtr = &GlobSet.ScriptWindowJson;
    else if ( ScriptLanguage == _PythonScript_) ScriptWindowJsonPtr = &GlobSet.PythonScriptWindowJson;
    if (!ScriptWindowJsonPtr) return;

    QJsonObject& json = *ScriptWindowJsonPtr;
    ReadFromJson(json);
    */
}

void AScriptWindow::ReadFromJson(QJsonObject& json)
{
    /*
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
    */
}

/*
void AScriptWindow::SetMainSplitterSizes(QList<int> values)
{
    splMain->setSizes(values);
}
*/

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
        getTab()->TextEdit->clear();
        getTab()->TextEdit->appendPlainText(*LightModeScript);
    }
}

void AScriptWindow::setAcceptRejectVisible()
{
    ui->frAccept->setVisible(true);

    ui->pbRunScript->setText("Test script");
    QFont f = ui->pbRunScript->font();
    f.setBold(false);
    ui->pbRunScript->setFont(f);
}

void AScriptWindow::showHtmlText(QString text)
{
    pteOut->appendHtml(text);
    qApp->processEvents();
}

void AScriptWindow::showPlainText(QString text)
{
    pteOut->appendPlainText(text);
    qApp->processEvents();
}

void AScriptWindow::clearOutputText()
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
       GlobSet.saveANTSconfiguration();
   }

   QString Script = getTab()->TextEdit->document()->toPlainText();
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
       ReportError("Script error: " + ScriptManager->getLastError(), ScriptManager->getLastErrorLineNumber());
   }
   else if (ScriptManager->isUncaughtException())
   {
       int lineNum = ScriptManager->getUncaughtExceptionLineNumber();
       const QString message = ScriptManager->getUncaughtExceptionString();
       //qDebug() << "Error message:" << message;
       //QString backtrace = engine.uncaughtExceptionBacktrace().join('\n');
       //qDebug() << "backtrace:" << backtrace;
       ReportError("Script error: " + message, lineNum);
   }
   else
   {
       //success
       //qDebug() << "Script returned:" << result;
       if (!ScriptManager->isEvalAborted())
       {
            if (bShowResult && result != "undefined" && !result.isEmpty())
                showPlainText("Script evaluation result:\n" + result);
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
      showPlainText("Sending stop signal...");
      ScriptManager->AbortEvaluation("Aborted by user!");
      qApp->processEvents();
    }
}

void AScriptWindow::on_pbLoad_clicked()
{
    /*
  QString starter = (GlobSet.LibScripts.isEmpty()) ? GlobSet.LastOpenDir : GlobSet.LibScripts;
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

  getTab()->FileName = fileName;

  getTab()->TabName = QFileInfo(fileName).baseName();
  twScriptTabs->setTabText(CurrentTab, getTab()->TabName);
  updateFileStatusIndication();
  */
}

void AScriptWindow::onLoadRequested(QString NewScript)
{
    ATabRecord * tab = getTab();
    if (!tab->TextEdit->document()->isEmpty()) addNewTab();

    tab->TextEdit->clear();
    tab->TextEdit->appendPlainText(NewScript);

    //for example load (triggered on signal from example explorer): do not register file name!
    tab->FileName.clear();
    tab->TabName = createNewTabName();
    getTabWidget()->setTabText(getCurrentTabIndex(), getTab()->TabName);
    updateFileStatusIndication();
}

void AScriptWindow::on_pbSave_clicked()
{
    /*
    if (ScriptTabs.isEmpty()) return;

    QString SavedName = getTab()->FileName;
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

    QString Script = getTab()->TextEdit->document()->toPlainText();
    QTextStream outStream(&outputFile);
    outStream << Script;
    outputFile.close();

    if (!getTab()->bExplicitlyNamed)
    {
        getTab()->TabName = QFileInfo(SavedName).baseName();
        twScriptTabs->setTabText(CurrentTab, getTab()->TabName);
    }

    getTab()->setModifiedStatus(false);
    updateFileStatusIndication();
    */
}

void AScriptWindow::on_pbSaveAs_clicked()
{
    /*
    if (ScriptTabs.isEmpty()) return;
    QString starter = (GlobSet.LibScripts.isEmpty()) ? GlobSet.LastOpenDir : GlobSet.LibScripts;
    if (!getTab()->FileName.isEmpty()) starter = getTab()->FileName;
    QString fileName = QFileDialog::getSaveFileName(this,"Save script", starter, "Script files (*.txt *.js);;All files (*.*)");
    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    if(fileInfo.suffix().isEmpty()) fileName += ".txt";

    getTab()->FileName = fileName;
    AScriptWindow::on_pbSave_clicked();
    */
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

        if (!getTab()->TextEdit->document()->isEmpty())
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

        getTab()->TextEdit->clear();
        getTab()->TextEdit->appendPlainText(*LightModeScript);
        return;
    }

    //reading example database
    QString target = (ScriptLanguage == AScriptLanguageEnum::JavaScript ? "ScriptExamples.cfg" : "PythonScriptExamples.cfg");
    QString RecordsFilename = GlobSet.ExamplesDir + "/" + target;
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
      if (ScriptLanguage == AScriptLanguageEnum::Python) UnitDescription.remove("Multithread-capable");
    }

  QStringList functions = getListOfMethods(obj, module, true);
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
      functionList << Flong;

      QTreeWidgetItem *fItem = new QTreeWidgetItem(objItem);
      fItem->setText(0, Fshort);
      fItem->setText(1, Flong);

      QString help = "Help not provided";
      QString methodName = Fshort.remove(QRegExp("\\((.*)\\)"));
      if (!module.isEmpty()) methodName.remove(0, module.length()+1); //remove module name and '.'
      AScriptInterface * io = dynamic_cast<AScriptInterface*>(obj);
      if (io)
      {
          const QString str = io->getMethodHelp(methodName);
          if (!str.isEmpty()) help = str;
      }
      fItem->setToolTip(0, help);
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
      AConfig_SI* inter = dynamic_cast<AConfig_SI*>(ScriptManager->interfaces[i]);
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

    QAction* plainKey = menu.addAction("Add Key path to clipboard");
    menu.addSeparator();
    QAction* keyQuatation = menu.addAction("Add Key path to clipboard: in quatations");
    QAction* keyGet = menu.addAction("Add Key path to clipboard: get key value command");
    QAction* keySet = menu.addAction("Add Key path to clipboard: replace key value command");
    //menu.addSeparator();

    QAction* sa = menu.exec(trwJson->mapToGlobal(pos));
    if (!sa) return;

    QClipboard *clipboard = QApplication::clipboard();
    QString text = getKeyPath(item);
    if (sa == plainKey) ;
    else if (sa == keyQuatation) text = "\"" + text + "\"";
    else if (sa == keyGet) text = "config.GetKeyValue(\"" + text + "\")";
    else if (sa == keySet) text = "config.Replace(\"" + text + "\",       )";
    clipboard->setText(text);
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
//  getTab()->TextEdit->insertPlainText(text);
//}

void AScriptWindow::closeEvent(QCloseEvent* e)
{
    QString Script = getTab()->TextEdit->document()->toPlainText();
    //in light mode save the script directly
    if (bLightMode && LightModeScript) *LightModeScript = Script;

    QMainWindow::closeEvent(e);
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
                break;
            case QEvent::Show :
                //qDebug() << "Script window: show event";
                ScriptManager->restoreMsgDialogs();
                break;
            default:;
        };

    return AGuiWindow::event(e);
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
    emit onFinish(ScriptManager->isUncaughtException());
}

void AScriptWindow::onDefaulFontSizeChanged(int size)
{
    GlobSet.DefaultFontSize_ScriptWindow = size;
    for (ATabRecord* tab : getScriptTabs())
        tab->TextEdit->SetFontSize(size);
}

void AScriptWindow::onProgressChanged(int percent)
{
    ui->prbProgress->setValue(percent);
    ui->prbProgress->setVisible(true);
    qApp->processEvents();
}

QStringList AScriptWindow::getListOfMethods(QObject *obj, QString ObjName, bool fWithArguments)
{
  QStringList commands;
  if (!obj) return commands;
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

void AScriptWindow::appendDeprecatedOrRemovedMethods(const AScriptInterface *obj, const QString &name)
{
    if (!obj) return;

    QHashIterator<QString, QString> iter(obj->getDeprecatedOrRemovedMethods());
    while (iter.hasNext())
    {
        iter.next();

        QString key = iter.key();
        if (!name.isEmpty()) key = name + "." + key;

        DeprecatedOrRemovedMethods[key] = iter.value();
        ListOfDeprecatedOrRemovedMethods << key;
    }
}

ATabRecord::ATabRecord(const QStringList & functions, AScriptLanguageEnum language) :
    Functions(functions)
{
    TextEdit = new ATextEdit();
    TextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    Completer = new QCompleter(this);
    CompletitionModel = new QStringListModel(functions, this);
    Completer->setModel(CompletitionModel);
    Completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    //completer->setCompletionMode(QCompleter::PopupCompletion);
    Completer->setFilterMode(Qt::MatchContains);
    //completer->setFilterMode(Qt::MatchStartsWith);
    Completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    Completer->setCaseSensitivity(Qt::CaseSensitive);
    Completer->setWrapAround(false);
    TextEdit->setCompleter(Completer);

    if (language == AScriptLanguageEnum::Python)
        Highlighter = new APythonHighlighter(TextEdit->document());
    else
        Highlighter = new AHighlighterScriptWindow(TextEdit->document());

    TextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(TextEdit, &ATextEdit::customContextMenuRequested, this, &ATabRecord::onCustomContextMenuRequested);
    connect(TextEdit, &ATextEdit::lineNumberChanged, this, &ATabRecord::onLineNumberChanged);
    connect(TextEdit, &ATextEdit::textChanged, this, &ATabRecord::onTextChanged);
}

ATabRecord::~ATabRecord()
{
    delete TextEdit;
}

void ATabRecord::UpdateHighlight()
{
    Highlighter->rehighlight();
}

void ATabRecord::WriteToJson(QJsonObject & json) const
{
    json["FileName"]         = FileName;
    json["TabName"]          = TabName;
    json["bExplicitlyNamed"] = bExplicitlyNamed;
    json["Script"]           = (TextEdit ? TextEdit->document()->toPlainText() : "");
}

void ATabRecord::ReadFromJson(const QJsonObject & json)
{
    if (!TextEdit) return;

    QString Script = json["Script"].toString();
    TextEdit->clear();
    TextEdit->appendPlainText(Script);
    TextEdit->document()->clearUndoRedoStacks();
    FileName.clear();
    FileName = json["FileName"].toString();

    bExplicitlyNamed = false;
    parseJson(json, "bExplicitlyNamed", bExplicitlyNamed);

    if (json.contains("TabName")) TabName = json["TabName"].toString();
    else //compatibility
    {
        if (FileName.isEmpty()) TabName.clear();
        else
        {
            QFileInfo fi(FileName);
            TabName = fi.baseName();
        }
    }
}

/*
QTextEdit holds a QTextDocument object which can be retrieved using the document() method.
You can also set your own document object using setDocument(). QTextDocument emits a textChanged() signal
 if the text changes and it also provides a isModified() function which will return true if the text has been modified
 since it was either loaded or since the last call to setModified with false as argument.
 In addition it provides methods for undo and redo.
*/

bool ATabRecord::wasModified() const
{
    return TextEdit->document()->isModified();
}

void ATabRecord::setModifiedStatus(bool flag)
{
    TextEdit->document()->setModified(flag);
}

void AScriptWindow::onCurrentTabChanged(int tab)
{
    if (bShutDown) return;

    //qDebug() << "Current changed!" << tab;
    if (tab < 0) return; //also called by window destructor
    setCurrentTabIndex(tab);

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
    ATabRecord * tab = getTab();
    if (!tab) return;

    QString fileName  = tab->FileName;
    bool bWasModified = tab->wasModified();

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
    QString s = getTab()->FileName;
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
    int tab = getTabWidget()->tabBar()->tabAt(pos);

    QAction* add = menu.addAction("Add new tab");
    menu.addSeparator();
    QAction* rename = (tab == -1 ? 0 : menu.addAction("Rename tab") );
    menu.addSeparator();
    QAction* remove = (tab == -1 ? 0 : menu.addAction("Close tab") );
    menu.addSeparator();
    QAction * markTabA = (tab == -1 ? 0 : menu.addAction("Mark this tab for copy/move") );
    //QAction* removeAll = (getScriptTabs().isEmpty()) ? 0 : menu.addAction("Close all tabs");

    QAction* selectedItem = menu.exec(getTabWidget()->mapToGlobal(pos));
    if (!selectedItem) return; //nothing was selected

    if (selectedItem == add)            addNewTab();
    else if (selectedItem == remove)    askRemoveTab(tab);
    //else if (selectedItem == removeAll) on_actionRemove_all_tabs_triggered();
    else if (selectedItem == rename)    renameTab(tab);
    else if (selectedItem == markTabA)  markTab(tab);
}

void AScriptWindow::onScriptTabMoved(int from, int to)
{
    //qDebug() << "Form->to:"<<from<<to;
    getScriptTabs().swap(from, to);

    iMarkedTab = -1;
}

void AScriptWindow::UpdateTab(ATabRecord* tab)
{
    tab->Highlighter->setHighlighterRules(Functions, ListOfDeprecatedOrRemovedMethods, ListOfConstants);
    tab->UpdateHighlight();
    tab->TextEdit->functionList = functionList;
    tab->TextEdit->DeprecatedOrRemovedMethods = &DeprecatedOrRemovedMethods;
}

ATabRecord & AScriptWindow::addNewTab(int iBook)
{
    ATabRecord * tab = new ATabRecord(Functions, ScriptLanguage);
    tab->TabName = createNewTabName();

    formatTab(tab);

    getScriptTabs(iBook).append(tab);
    getTabWidget(iBook)->addTab(tab->TextEdit, tab->TabName);

    int index = getScriptTabs(iBook).size() - 1;
    setCurrentTabIndex(index, iBook);
    getTabWidget(iBook)->setCurrentIndex(index);

    return *tab;
}

ATabRecord & AScriptWindow::addNewTab()
{
    return addNewTab(iCurrentBook);
}

void AScriptWindow::formatTab(ATabRecord * tab)
{
    UpdateTab(tab);

    if (GlobSet.DefaultFontFamily_ScriptWindow.isEmpty())
         tab->TextEdit->SetFontSize(GlobSet.DefaultFontSize_ScriptWindow);
    else
    {
        QFont font(GlobSet.DefaultFontFamily_ScriptWindow, GlobSet.DefaultFontSize_ScriptWindow, GlobSet.DefaultFontWeight_ScriptWindow, GlobSet.DefaultFontItalic_ScriptWindow);
        tab->TextEdit->setFont(font);
    }

    connect(tab->TextEdit, &ATextEdit::fontSizeChanged, this, &AScriptWindow::onDefaulFontSizeChanged);
    connect(tab->TextEdit, &ATextEdit::requestHelp, this, &AScriptWindow::onF1pressed);
    connect(tab->TextEdit->document(), &QTextDocument::modificationChanged, this, &AScriptWindow::updateFileStatusIndication);
    connect(tab, &ATabRecord::requestFindText, this, &AScriptWindow::onFindSelected);
    connect(tab, &ATabRecord::requestReplaceText, this, &AScriptWindow::onReplaceSelected);
    connect(tab, &ATabRecord::requestFindFunction, this, &AScriptWindow::onFindFunction);
    connect(tab, &ATabRecord::requestFindVariable, this, &AScriptWindow::onFindVariable);
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
        for (int i = 0; i < countTabs(); i++)
            if ( getTabWidget()->tabText(i) == res )
            {
                fFound = true;
                counter++;
                break;
            }
    }
    while (fFound);
    return res;
}

QString AScriptWindow::createNewBookName()
{
    int counter = 1;
    QString res;
    bool fFound;
    do
    {
        fFound = false;
        res = QString("Book%1").arg(counter);
        for (const AScriptBook & b : ScriptBooks)
            if (b.Name == res)
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
    ScriptBooks[iCurrentBook].removeTab(tab);

    if (countTabs() == 0) addNewTab();
    updateFileStatusIndication();

    iMarkedTab = -1;
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
    onDefaulFontSizeChanged(++GlobSet.DefaultFontSize_ScriptWindow);
}

void AScriptWindow::on_actionDecrease_font_size_triggered()
{
    if (GlobSet.DefaultFontSize_ScriptWindow<1) return;

    onDefaulFontSizeChanged(--GlobSet.DefaultFontSize_ScriptWindow);
    //qDebug() << "New font size:"<<GlobSet.DefaultFontSize_ScriptWindow;
}

#include <QFontDialog>
void AScriptWindow::on_actionSelect_font_triggered()
{
  bool ok;
  QFont font = QFontDialog::getFont(
                  &ok,
                  QFont(GlobSet.DefaultFontFamily_ScriptWindow,
                        GlobSet.DefaultFontSize_ScriptWindow,
                        GlobSet.DefaultFontWeight_ScriptWindow,
                        GlobSet.DefaultFontItalic_ScriptWindow),
                  this);
  if (!ok) return;

  GlobSet.DefaultFontFamily_ScriptWindow = font.family();
  GlobSet.DefaultFontSize_ScriptWindow   = font.pointSize();
  GlobSet.DefaultFontWeight_ScriptWindow = font.weight();
  GlobSet.DefaultFontItalic_ScriptWindow = font.italic();

  for (ATabRecord* tab : getScriptTabs())
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
    addNewTab();
}

void AScriptWindow::askRemoveTab(int tab)
{
    if (tab < 0 || tab >= countTabs()) return;

    QMessageBox m(this);
    m.setIcon(QMessageBox::Question);
    m.setText("Close tab " + getTabWidget()->tabText(tab) + "?");
    m.setStandardButtons(QMessageBox::Yes| QMessageBox::Cancel);
    m.setDefaultButton(QMessageBox::Cancel);
    int ret = m.exec();
    if (ret == QMessageBox::Yes) removeTab(tab);
}

void AScriptWindow::renameTab(int tab)
{
    ATabRecord * tr = getTab(tab);
    if (!tr) return;

    bool ok;
    QString text = QInputDialog::getText(this, "Input text",
                                             "New name for the tab:", QLineEdit::Normal,
                                             tr->TabName, &ok);
    if (ok && !text.isEmpty())
    {
       tr->TabName = text;
       tr->bExplicitlyNamed = true;
       getTabWidget()->setTabText(tab, text);
    }
}

void AScriptWindow::markTab(int tab)
{
    iMarkedBook = iCurrentBook;
    iMarkedTab = tab;
}

void AScriptWindow::copyTab(int iBook)
{
    qDebug() << "Copy to book" << iBook;
    if (iMarkedTab == -1) return;

    QString script = ScriptBooks[iMarkedBook].getTab(iMarkedTab)->TextEdit->document()->toPlainText();
    QString newName = "CopyOf_" + ScriptBooks[iMarkedBook].getTab(iMarkedTab)->TabName;

    ATabRecord & tab = addNewTab(iBook);

    tab.TextEdit->appendPlainText(script);
    tab.TabName = newName;
    getTabWidget(iBook)->setTabText(countTabs(iBook)-1, newName);
}

void AScriptWindow::moveTab(int iBook)
{
    qDebug() << "Move to book" << iBook;
    if (iMarkedTab == -1) return;

    ATabRecord * tab = getTab(iMarkedTab, iMarkedBook);
    ScriptBooks[iMarkedBook].removeTabNoCleanup(iMarkedTab);
    if (countTabs(iMarkedBook) == 0) addNewTab(iMarkedBook);

    ScriptBooks[iBook].Tabs.append(tab);
    ScriptBooks[iBook].TabWidget->addTab(tab->TextEdit, tab->TabName);

    iMarkedTab = -1;
}

void AScriptWindow::on_actionRemove_current_tab_triggered()
{
    askRemoveTab(getCurrentTabIndex());
}

void AScriptWindow::on_actionStore_all_tabs_triggered()
{
    if (countTabs() == 0) return;
    QString starter = GlobSet.LastOpenDir;
    QString fileName = QFileDialog::getSaveFileName(this,"Save session", starter, "Json files (*.json);;All files (*.*)");
    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    if(fileInfo.suffix().isEmpty()) fileName += ".json";

    QJsonObject json;
    WriteToJson(json);
    bool bOK = SaveJsonToFile(json, fileName);
    if (!bOK) message("Failed to save json to file: "+fileName, this);
}

void AScriptWindow::on_actionRestore_session_triggered()
{
    if (countTabs() == 1 && getScriptTabs().at(0)->TextEdit->document()->isEmpty())
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

    QString starter = GlobSet.LastOpenDir;
    QString fileName = QFileDialog::getOpenFileName(this, "Load script", starter, "Json files (*.json);;All files (*.*)");
    if (fileName.isEmpty()) return;

    QJsonObject json;
    bool bOK = LoadJsonFromFile(json, fileName);
    if (!bOK) message("Cannot open file: "+fileName, this);
    else ReadFromJson(json);
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
            if (getTab()->TextEdit->textCursor().selectedText() == ui->leFind->text())
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

    QTextCursor tc = getTab()->TextEdit->textCursor();
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
            if (getTab()->TextEdit->textCursor().selectedText() == ui->leFind->text())
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

    QTextCursor tc = getTab()->TextEdit->textCursor();
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
    ATextEdit* te = getTab()->TextEdit;
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
    ATextEdit* te = getTab()->TextEdit;
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
    getTab()->goBack();
}

void AScriptWindow::onForward()
{
    getTab()->goForward();
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
    ATextEdit* te = getTab()->TextEdit;
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
    bool bActive = ui->frFindReplace->isVisible();
    QString Text = (bActive ? ui->leFind->text() : "");

    ATextEdit * te = getTab()->TextEdit;
    te->FindString = Text;
    te->RefreshExtraHighlight();
}

void AScriptWindow::on_pbReplaceOne_clicked()
{
    ATextEdit* te = getTab()->TextEdit;
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
    ATextEdit* te = getTab()->TextEdit;
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

void ATabRecord::onCustomContextMenuRequested(const QPoint& pos)
{
    QMenu menu;

    QAction* paste = menu.addAction("Paste"); paste->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_V));
    QAction* copy  = menu.addAction("Copy");   copy->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    QAction* cut   = menu.addAction("Cut");     cut->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_X));
    menu.addSeparator();
    QAction* findSel =    menu.addAction("Find selected text");      findSel->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F));
    QAction* replaceSel = menu.addAction("Replace selected text");replaceSel->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    menu.addSeparator();
    QAction* findFunct = menu.addAction("Find function definition");      findFunct->setShortcut(QKeySequence(Qt::Key_F2));
    QAction* findVar =   menu.addAction("Find variable definition (F3)");   findVar->setShortcut(QKeySequence(Qt::Key_F3));
    menu.addSeparator();
    QAction* shiftBack = menu.addAction("Go back");          shiftBack->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Left));
    QAction* shiftForward = menu.addAction("Go forward"); shiftForward->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Right));
    menu.addSeparator();
    QAction* alignText = menu.addAction("Align selected text"); alignText->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));

    QAction* selectedItem = menu.exec(TextEdit->mapToGlobal(pos));
    if (!selectedItem) return; //nothing was selected

    if (selectedItem == findSel)         emit requestFindText();
    if (selectedItem == replaceSel)      emit requestReplaceText();
    else if (selectedItem == findFunct)  emit requestFindFunction();
    else if (selectedItem == findVar)    emit requestFindVariable();
    else if (selectedItem == replaceSel) emit requestReplaceText();

    else if (selectedItem == shiftBack) goBack();
    else if (selectedItem == shiftForward) goForward();

    else if (selectedItem == alignText) emit TextEdit->align();

    else if (selectedItem == cut) TextEdit->cut();
    else if (selectedItem == copy) TextEdit->copy();
    else if (selectedItem == paste) TextEdit->paste();
}

void ATabRecord::goBack()
{
    if (IndexVisitedLines >= 1 && IndexVisitedLines < VisitedLines.size())
    {
        IndexVisitedLines--;
        int goTo = VisitedLines.at(IndexVisitedLines);
        QTextCursor tc = TextEdit->textCursor();
        int nowAt = tc.blockNumber();
        if (nowAt == goTo) return;
        else if (nowAt < goTo) tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, goTo-nowAt );
        else if (nowAt > goTo) tc.movePosition(QTextCursor::Up,   QTextCursor::MoveAnchor, nowAt-goTo );
        TextEdit->setTextCursorSilently(tc);
        TextEdit->ensureCursorVisible();
    }
}

void ATabRecord::goForward()
{
    if (IndexVisitedLines >= 0 && IndexVisitedLines < VisitedLines.size()-1)
    {
        IndexVisitedLines++;
        int goTo = VisitedLines.at(IndexVisitedLines);
        QTextCursor tc = TextEdit->textCursor();
        int nowAt = tc.blockNumber();
        if (nowAt == goTo) return;
        else if (nowAt < goTo) tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, goTo-nowAt );
        else if (nowAt > goTo) tc.movePosition(QTextCursor::Up,   QTextCursor::MoveAnchor, nowAt-goTo );
        TextEdit->setTextCursorSilently(tc);
        TextEdit->ensureCursorVisible();
    }
}

void ATabRecord::onLineNumberChanged(int lineNumber)
{
    if (!VisitedLines.isEmpty())
        if (VisitedLines.last() == lineNumber) return;

    VisitedLines.append(lineNumber);
    if (VisitedLines.size() > MaxLineNumbers) VisitedLines.removeFirst();

    IndexVisitedLines = VisitedLines.size() - 1;
}

void ATabRecord::onTextChanged()
{
    //qDebug() << "Text changed!";
    QTextDocument* d = TextEdit->document();
    QRegularExpression re("(?<=var)\\s+\\w+\\b");

    QStringList Variables;
    QTextCursor tc = d->find(re, 0);//, flags);
    while (!tc.isNull())
    {
        Variables << tc.selectedText().trimmed();
        tc = d->find(re, tc);//, flags);
    }

    Variables.append(Functions);
    CompletitionModel->setStringList(Variables);
}

void AScriptWindow::on_pbAccept_clicked()
{
    bAccepted = true;
    close();
}

void AScriptWindow::on_pbCancel_clicked()
{
    bAccepted = false;
    close();
}

void AScriptWindow::on_actionShortcuts_triggered()
{
    QString s = "For the current line:\n"
                "Ctrl + Alt + Del\tDelete line\n"
                "Ctrl + Alt + Down\tDublicate line\n"
                "Ctrl + Shift + Up\tShift line up\n"
                "Ctrl + Shift + Down\tShift line down\n"
                "\n"
                "For selected text:\n"
                "Ctrl + i\t\tAuto-align JavaScript";

    message(s, this);
}

// -------------------

AScriptBook::AScriptBook()
{
    TabWidget = new QTabWidget();

    TabWidget->setMovable(true);
    TabWidget->setMinimumHeight(25);
    TabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
}

void AScriptBook::writeToJson(QJsonObject & json) const
{
    json["Name"] = Name;
    json["CurrentTab"] = iCurrentTab;

    QJsonArray ar;
    for (const ATabRecord * r : Tabs)
    {
        QJsonObject js;
        r->WriteToJson(js);
        ar.append(js);
    }
    json["ScriptTabs"] = ar;
}

ATabRecord * AScriptBook::getCurrentTab()
{
    if (iCurrentTab == -1) return nullptr;
    return Tabs[iCurrentTab];
}

ATabRecord *AScriptBook::getTab(int index)
{
    if (index < 0 || index >= Tabs.size()) return nullptr;
    return Tabs[index];
}

const ATabRecord *AScriptBook::getTab(int index) const
{
    if (index < 0 || index >= Tabs.size()) return nullptr;
    return Tabs[index];
}

QTabWidget *AScriptBook::getTabWidget()
{
    return TabWidget;
}

void AScriptBook::removeTabNoCleanup(int index)
{
    if (index < 0 || index >= Tabs.size()) return;

    if (TabWidget)
    {
        if (index < TabWidget->count()) TabWidget->removeTab(index);
        else qWarning() << "Bad TabWidget index";
    }
    Tabs.removeAt(index);
    if (iCurrentTab >= Tabs.size()) iCurrentTab = Tabs.size() - 1;
}

void AScriptBook::removeTab(int index)
{
    if (index < 0 || index >= Tabs.size()) return;

    if (index < TabWidget->count())
    {
        TabWidget->removeTab(index);
        delete Tabs[index];
        Tabs.removeAt(index);
    }
}

void AScriptBook::removeAllTabs()
{
    for (int i = Tabs.size() - 1; i > -1; i--)
        removeTab(i);
}

// ----------------------

void AScriptWindow::addNewBook()
{
    //qDebug() << "Add book triggered";
    size_t iNewBook = ScriptBooks.size();

    ScriptBooks.resize(iNewBook + 1);
    twBooks->addTab(getTabWidget(iNewBook), "");

    renameBook(iNewBook, createNewBookName());

    QTabWidget * twScriptTabs = getTabWidget(iNewBook);

    connect(twScriptTabs, &QTabWidget::currentChanged, this, &AScriptWindow::onCurrentTabChanged);
    connect(twScriptTabs, &QTabWidget::customContextMenuRequested, this, &AScriptWindow::onRequestTabWidgetContextMenu);
    connect(twScriptTabs->tabBar(), &QTabBar::tabMoved, this, &AScriptWindow::onScriptTabMoved);
}

void AScriptWindow::removeBook(int iBook, bool bConfirm)
{
    if (iBook < 0 || iBook >= (int)ScriptBooks.size()) return;
    if (ScriptBooks.size() == 1)
    {
        message("Cannot remove the last book", this);
        return;
    }

    if (bConfirm)
    {
        QString t = QString("Close book %1?\nUnsaved data will be lost").arg(ScriptBooks[iBook].Name);
        if ( !confirm(t, this) ) return;
    }

    twBooks->removeTab(iBook);
    ScriptBooks[iBook].removeAllTabs();
    delete ScriptBooks[iBook].TabWidget;
    ScriptBooks.erase(ScriptBooks.begin() + iBook);

    if (iCurrentBook >= (int)ScriptBooks.size()) iCurrentBook = (int)ScriptBooks.size() - 1;

    if (countTabs() == 0) addNewTab();
    updateFileStatusIndication();

    iMarkedTab = -1;
}

void AScriptWindow::saveBook(int iBook)
{
    if (iBook < 0 || iBook >= (int)ScriptBooks.size()) return;

    QString starter = GlobSet.LastOpenDir;
    QString fileName = QFileDialog::getSaveFileName(this,"Save book", starter, "json files (*.json);;All files (*.*)");
    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    if(fileInfo.suffix().isEmpty()) fileName += ".json";

    QJsonObject json;
    ScriptBooks[iCurrentBook].writeToJson(json);
    bool bOK = SaveJsonToFile(json, fileName);
    if (!bOK) message("Failed to save json to file: " + fileName, this);
}

void AScriptWindow::loadBook(int iBook, const QString & fileName)
{
    QJsonObject json;
    bool ok = LoadJsonFromFile(json, fileName);
    if (!ok)
    {
        message("Cannot open file: " + fileName, this);
        return;
    }

    if (json.isEmpty() || !json.contains("ScriptTabs"))
    {
        message("Json format error", this);
        return;
    }

    QJsonArray ar = json["ScriptTabs"].toArray();
    ScriptBooks[iBook].removeAllTabs();
    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar[i].toObject();
        ATabRecord & tab = addNewTab(iBook);
        tab.ReadFromJson(js);

        if (tab.TabName.isEmpty()) tab.TabName = createNewTabName();
        getTabWidget(iBook)->setTabText(countTabs(iBook) - 1, tab.TabName);

        if (!tab.FileName.isEmpty())
        {
            QString ScriptInFile;
            if ( LoadTextFromFile(tab.FileName, ScriptInFile) )
            {
                QPlainTextEdit te;
                te.appendPlainText(ScriptInFile);
                tab.setModifiedStatus( !(te.document()->toPlainText() == tab.TextEdit->document()->toPlainText()) );
            }
        }
    }
    if (countTabs(iBook) == 0) addNewTab(iBook);

    int iCur = -1;
    parseJson(json, "CurrentTab", iCur);
    if (iCur < 0 || iCur >= countTabs(iBook)) iCur = 0;
    ScriptBooks[iBook].iCurrentTab = iCur;

    QString Name = ScriptBooks[iBook].Name;
    parseJson(json, "Name", Name);
    renameBook(iBook, Name);

    updateFileStatusIndication();
}

void AScriptWindow::renameBook(int iBook, const QString & newName)
{
    if (iBook < 0 || iBook >= (int)ScriptBooks.size()) return;

    ScriptBooks[iBook].Name = newName;
    twBooks->setTabText(iBook, newName);
}

void AScriptWindow::renameBookRequested(int iBook)
{
    bool ok;
    QString text = QInputDialog::getText(this, "Input text",
                                             "New name for the tab:", QLineEdit::Normal,
                                             ScriptBooks[iBook].Name, &ok);
    if (ok && !text.isEmpty())
        renameBook(iBook, text);
}

bool AScriptWindow::isUntouchedBook(int iBook) const
{
    if (iBook < 0 || iBook >= (int)ScriptBooks.size()) return false;

    int numTabs = countTabs(iBook);
    if (numTabs == 0) return true;

    if (numTabs == 1) return getTab(0, iBook)->TextEdit->document()->isEmpty();
    else return false;
}

QList<ATabRecord *> & AScriptWindow::getScriptTabs()
{
    return ScriptBooks[iCurrentBook].Tabs;
}

QList<ATabRecord *> &AScriptWindow::getScriptTabs(int iBook)
{
    return ScriptBooks[iBook].Tabs;
}

ATabRecord * AScriptWindow::getTab()
{
    if (bLightMode) return StandaloneTab;
    else           return ScriptBooks[iCurrentBook].getCurrentTab();
}

ATabRecord *AScriptWindow::getTab(int index)
{
    return ScriptBooks[iCurrentBook].getTab(index);
}

ATabRecord *AScriptWindow::getTab(int index, int iBook)
{
    if (iBook < 0 || iBook >= (int)ScriptBooks.size()) return nullptr;
    return ScriptBooks[iBook].getTab(index);
}

const ATabRecord *AScriptWindow::getTab(int index, int iBook) const
{
    if (iBook < 0 || iBook >= (int)ScriptBooks.size()) return nullptr;
    return ScriptBooks[iBook].getTab(index);
}

QTabWidget *AScriptWindow::getTabWidget()
{
    return ScriptBooks[iCurrentBook].getTabWidget();
}

QTabWidget *AScriptWindow::getTabWidget(int iBook)
{
    if (iBook < 0 || iBook >= (int)ScriptBooks.size()) return nullptr;
    return ScriptBooks[iBook].getTabWidget();
}

int AScriptWindow::getCurrentTabIndex()
{
    return ScriptBooks[iCurrentBook].iCurrentTab;
}

void AScriptWindow::setCurrentTabIndex(int index)
{
    ScriptBooks[iCurrentBook].iCurrentTab = index;
}

void AScriptWindow::setCurrentTabIndex(int index, int iBook)
{
    ScriptBooks[iBook].iCurrentTab = index;
}

int AScriptWindow::countTabs(int iBook) const
{
    if (iBook < 0 || iBook >= (int)ScriptBooks.size()) return 0;
    return ScriptBooks[iBook].Tabs.size();
}

int AScriptWindow::countTabs() const
{
    return ScriptBooks[iCurrentBook].Tabs.size();
}

void AScriptWindow::twBooks_customContextMenuRequested(const QPoint & pos)
{
    QMenu menu;

    int iBook = twBooks->tabBar()->tabAt(pos);
    bool bBook = (iBook != -1);

    QAction * add = menu.addAction("Add new book");
    menu.addSeparator();
    QAction * rename = menu.addAction("Rename");             rename->setEnabled(bBook);
    menu.addSeparator();
    QAction * remove = menu.addAction("Close");              remove->setEnabled(bBook);
    menu.addSeparator();
    QAction * save = menu.addAction("Save");                 save->setEnabled(bBook);
    menu.addSeparator();
    QAction * copy = menu.addAction("Copy marked tab here"); copy->setEnabled(bBook && iMarkedTab != -1);
    QAction * move = menu.addAction("Move marked tab here"); move->setEnabled(bBook && iMarkedTab != -1);

    QAction* selectedItem = menu.exec(twBooks->mapToGlobal(pos));
    if (!selectedItem) return;

    if      (selectedItem == rename) renameBookRequested(iBook);
    else if (selectedItem == add)    addNewBook();
    else if (selectedItem == copy)   copyTab(iBook);
    else if (selectedItem == move)   moveTab(iBook);
    else if (selectedItem == remove) removeBook(iBook);
    else if (selectedItem == save)   saveBook(iBook);
}

void AScriptWindow::twBooks_currentChanged(int index)
{
    //qDebug() << "book index changed to" << index;
    iCurrentBook = index;

    if (ScriptBooks[index].Tabs.isEmpty())
        addNewTab();
}

void AScriptWindow::onBookTabMoved(int from, int to)
{
    //qDebug() << "Book from->to:"<<from<<to;
    std::swap(ScriptBooks[from], ScriptBooks[to]);
    iMarkedTab = -1;
}

void AScriptWindow::on_actionAdd_new_book_triggered()
{
    addNewBook();
}

void AScriptWindow::on_actionRename_book_triggered()
{
    renameBookRequested(iCurrentBook);
}

void AScriptWindow::on_actionLoad_book_triggered()
{
    if ( !isUntouchedBook(iCurrentBook) )
    {
        addNewBook();
        iCurrentBook = countTabs() - 1;
        twBooks->setCurrentIndex(iCurrentBook);
    }

    QString starter = GlobSet.LastOpenDir;
    QString fileName = QFileDialog::getOpenFileName(this, "Load script book", starter, "json files (*.json);;All files (*.*)");
    if (fileName.isEmpty()) return;

    loadBook(iCurrentBook, fileName);
}

void AScriptWindow::on_actionClose_book_triggered()
{
    removeBook(iCurrentBook);
}

void AScriptWindow::on_actionSave_book_triggered()
{
    saveBook(iCurrentBook);
}

void AScriptWindow::on_actionSave_all_triggered()
{

}

void AScriptWindow::on_actionload_session_triggered()
{

}

void AScriptWindow::on_actionClose_all_books_triggered()
{
    QString t = "Close all books?\nUnsaved data will be lost";
    if ( !confirm(t, this) ) return;

    for (int i = (int)ScriptBooks.size() - 1; i > 0; i--) removeBook(i, false);
    ScriptBooks[0].removeAllTabs();

    addNewTab();
}
