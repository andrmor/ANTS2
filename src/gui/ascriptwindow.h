#ifndef ASCRIPTWINDOW_H
#define ASCRIPTWINDOW_H

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
class AScriptWindowTabItem;
class GlobalSettingsClass;

namespace Ui {
class AScriptWindow;
}

class AScriptWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AScriptWindow(GlobalSettingsClass* GlobSet, TRandom2* RandGen, QWidget *parent = 0);
    ~AScriptWindow();

    void SetInterfaceObject(QObject *interfaceObject, QString name = "");
    void SetScript(QString *text);
    void SetShowEvaluationResult(bool flag) {ShowEvalResult = flag;} //if false, window only reports "success", ptherwise eval result is shown

    void AddNewTab();

    void ReportError(QString error, int line = 0);   //0 - no line is highligted
    void HighlightErrorLine(int line);

    void WriteToJson(QJsonObject &json);
    void ReadFromJson(QJsonObject &json);

    void UpdateHighlight();

    void SetMainSplitterSizes(QList<int> values);

    AScriptManager* ScriptManager;
    QStringList functions;

public slots:
    void updateJsonTree();

    void ShowText(QString text); //shows text in the output box
    void ClearText(); //clears text in the output box
    void on_pbRunScript_clicked();
    //void abortEvaluation(QString message = "Aborted!");
    void onF1pressed(QString text);
    void onLoadRequested(QString NewScript);

private slots:
    void onCurrentTabChanged(int tab);
    void onRequestTabWidgetContextMenu(QPoint pos);
    void onScriptTabMoved(int from, int to);
    void onContextMenuRequestedByJsonTree(QPoint pos);
    void onContextMenuRequestedByHelp(QPoint pos);
    //void onFunctionDoubleClicked(QTreeWidgetItem* item, int column);

    void on_pbStop_clicked();
    void on_pbLoad_clicked();
    void on_pbSave_clicked();
    void on_pbSaveAs_clicked();
    void on_pbExample_clicked();
    void onRequestDraw(TObject* obj, QString options, bool fFocus) {emit RequestDraw(obj, options, fFocus);}    
    void onFunctionClicked(QTreeWidgetItem* item, int column);  //help has to be shown
    void onKeyDoubleClicked(QTreeWidgetItem* item, int column);
    void onKeyClicked(QTreeWidgetItem* item, int column);
    void onFindTextChanged(const QString &arg1);
    void onFindTextJsonChanged(const QString &arg1);

    void onJsonTWExpanded(QTreeWidgetItem* item);
    void onJsonTWCollapsed(QTreeWidgetItem* item);

    void on_pbConfig_toggled(bool checked);
    void on_pbHelp_toggled(bool checked);

    void on_actionIncrease_font_size_triggered();
    void on_actionDecrease_font_size_triggered();

private:
    Ui::AScriptWindow *ui;
    QStringListModel* completitionModel;
    GlobalSettingsClass* GlobSet;

    int CurrentTab;
    QList<AScriptWindowTabItem*> ScriptTabs;
    QTabWidget* twScriptTabs;

    QSplitter* splMain;
    QSplitter* splHelp;
    QPlainTextEdit* pteOut;
    QTreeWidget* trwHelp;
    QTreeWidget* trwJson;
    QFrame* frHelper;
    QFrame* frJsonBrowser;
    QPlainTextEdit* pteHelp;
    QLineEdit* leFind;
    QLineEdit* leFindJ;
    QIcon* RedIcon;

    QString* Script;     //pointer to external script
    QString LocalScript; //if external not provided, this is the default script

    bool tmpIgnore;
    bool ShowEvalResult;

    QSet<QString> ExpandedItemsInJsonTW;

    void fillSubObject(QTreeWidgetItem* parent, const QJsonObject& obj);
    void fillSubArray(QTreeWidgetItem* parent, const QJsonArray& arr);
    QString getDesc(const QJsonValue &ref);
    void fillHelper(QObject* obj, QString module, QString helpText = "");  //fill help TreeWidget according to the data in the obj
    QString getFunctionReturnType(QString UnitFunction);
    QString getKeyPath(QTreeWidgetItem *item);
    void showContextMenuForJsonTree(QTreeWidgetItem *item, QPoint pos);
    QStringList getCustomCommandsOfObject(QObject *obj, QString ObjName, bool fWithArguments = false);

    void removeTab(int tab);
    void clearAllTabs();
    QString createNewTabName();

protected:
  virtual void closeEvent(QCloseEvent *e);
  virtual bool event(QEvent * e);

signals:
    void WindowShown(QString);
    void WindowHidden(QString);
    void RequestDraw(TObject* obj, QString options, bool fFocus);
    //just retranslators:
    void onStart();
    void onAbort();
    void success(QString eval);

public slots:
    void receivedOnStart() {emit onStart();}
    void receivedOnAbort() {emit onAbort();}
    void receivedOnSuccess(QString eval) {emit success(eval);}
    void onDefaulFontSizeChanged(int size);
};

class AScriptWindowTabItem : QObject
{
    Q_OBJECT
public:
    AScriptWindowTabItem(QAbstractItemModel *model);
    ~AScriptWindowTabItem();

    CompletingTextEditClass* TextEdit;
    QString FileName;

    QCompleter* completer;
    AHighlighterScriptWindow* highlighter;

    void UpdateHighlight();

    void WriteToJson(QJsonObject &json);
    void ReadFromJson(QJsonObject &json);
};

#endif // ASCRIPTWINDOW_H
