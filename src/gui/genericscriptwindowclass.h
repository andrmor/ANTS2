#ifndef GENERICSCRIPTWINDOWCLASS_H
#define GENERICSCRIPTWINDOWCLASS_H

#include <QMainWindow>
#include <QSet>

class AHighlighterScriptWindow;
class QAbstractItemModel;
class QCompleter;
class QStringListModel;
class CompletingTextEditClass;
class QPlainTextEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QSplitter;
class QFrame;
class QLineEdit;
class TObject;
class QThread;
class TRandom2;
class AScriptManager;

namespace Ui {
class GenericScriptWindowClass;
}

class GenericScriptWindowClass : public QMainWindow
{
    Q_OBJECT

public:
    explicit GenericScriptWindowClass(TRandom2* RandGen, QWidget *parent = 0);
    ~GenericScriptWindowClass();

    void SetInterfaceObject(QObject *interfaceObject, QString name = "");
    void SetScript(QString *text);
    void SetLastOpenDir(QString lastOpenDir) {LastOpenDir = lastOpenDir;}
    void SetShowEvaluationResult(bool flag) {ShowEvalResult = flag;} //if false, window only reports "success", ptherwise eval result is shown
    void SetTitle(QString title);
    void SetExample(QString example);
    void SetJsonTreeAlwaysVisible(bool flag);

    void SetStarterDir(QString starter);
    void SetStarterDir(QString ScriptDir, QString WorkingDir, QString ExamplesDir); //more advanced version, used by core in global script

      //can be used from outside to access output text edit and highlight a specific line
    void ReportError(QString error, int line = 0);   //0 - no line is highligted   
    void HighlightErrorLine(int line);    

    AScriptManager* ScriptManager;
    AHighlighterScriptWindow* highlighter;

    QStringList ScriptHistory;
    int HistoryPosition;

    // specific directories
    QString LastOpenDir;
    QString LibScripts;
    QString ExamplesDir;

public slots:
    void ShowText(QString text); //shows text in the output box
    void ClearText(); //clears text in the output box
    void on_pbRunScript_clicked();    
    void abortEvaluation(QString message = "Aborted!");
    void onF1pressed(QString text);
    void onRequestHistoryBefore();
    void onRequestHistoryAfter();
    void updateJsonTree();

    void onLoadRequested(QString NewScript);
private slots:
    void on_pbStop_clicked();
    void on_pbHelp_clicked();
    void on_pbLoad_clicked();
    void on_pbSave_clicked();
    void on_pbExample_clicked();
    void cteScript_textChanged();        
    void onRequestDraw(TObject* obj, QString options, bool fFocus) {emit RequestDraw(obj, options, fFocus);}
    void onFunctionDoubleClicked(QTreeWidgetItem* item, int column);
    void onFunctionClicked(QTreeWidgetItem* item, int column);
    void onKeyDoubleClicked(QTreeWidgetItem* item, int column);
    void onKeyClicked(QTreeWidgetItem* item, int column);
    void onFindTextChanged(const QString &arg1);
    void onFindTextJsonChanged(const QString &arg1);
    void onContextMenuRequestedByJsonTree(QPoint pos);
    void onHelpTWExpanded(QTreeWidgetItem* item);
    void onHelpTWCollapsed(QTreeWidgetItem* item);
    void onJsonTWExpanded(QTreeWidgetItem* item);
    void onJsonTWCollapsed(QTreeWidgetItem* item);
    void on_pbSaveAs_clicked();
    void on_pbAppendScript_clicked();

private:
    Ui::GenericScriptWindowClass *ui;    
    QCompleter* completer;
    QStringListModel* completitionModel;

    QSplitter* splHelp;
    CompletingTextEditClass* cteScript;
    QPlainTextEdit* pteOut;
    QTreeWidget* trwHelp;
    QTreeWidget* trwJson;
    QFrame* frJsonBrowser;
    QPlainTextEdit* pteHelp;
    QLineEdit* leFind;
    QLineEdit* leFindJ;
    QIcon* RedIcon;

    QString* Script;     //pointer to external script
    QString LocalScript; //if external not provided, this is the default script
    QString Example;

    QString SavedName;
    bool fJsonTreeAlwayVisible;

    bool tmpIgnore;
    bool ShowEvalResult;

    QSet<QString> ExpandedItemsInJsonTW;

    QAbstractItemModel* createCompletitionModel(QStringList words);    
    void fillSubObject(QTreeWidgetItem* parent, const QJsonObject& obj);
    void fillSubArray(QTreeWidgetItem* parent, const QJsonArray& arr);
    QString getDesc(const QJsonValue &ref);
    void saveScriptHistory();
    void fillHelper(QObject* obj, QString module, QString helpText = "");  //fill help TreeWidget according to the data in the obj
    //void fillHelper(QScriptValue &val, QString module, QString helpText);
    QString getKeyPath(QTreeWidgetItem *item);    
    void showContextMenuForJsonTree(QTreeWidgetItem *item, QPoint pos);

protected:
  virtual void closeEvent(QCloseEvent *e);
  virtual bool event(QEvent * e);

signals:      
    void RequestDraw(TObject* obj, QString options, bool fFocus);
    //just retranslators:
    void onStart();
    void onAbort();
    void success(QString eval);
public slots:
    void receivedOnStart() {emit onStart();}
    void receivedOnAbort() {emit onAbort();}
    void receivedOnSuccess(QString eval) {emit success(eval);}
};

#endif // GENERICSCRIPTWINDOWCLASS_H
