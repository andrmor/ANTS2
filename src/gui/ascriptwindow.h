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
    explicit AScriptWindow(AScriptManager *ScriptManager, GlobalSettingsClass* GlobSet, QWidget *parent = 0);
    ~AScriptWindow();

    void SetInterfaceObject(QObject *interfaceObject, QString name = "");
    void SetScript(QString *text);
    void SetShowEvaluationResult(bool flag) {ShowEvalResult = flag;} //if false, window only reports "success", ptherwise eval result is shown

    void AddNewTab();

    void ReportError(QString error, int line = 0);   //0 - no line is highligted

    void WriteToJson();
    void ReadFromJson();

    void UpdateHighlight();

    void SetMainSplitterSizes(QList<int> values);

    void onBusyOn();
    void onBusyOff();

    AScriptManager* ScriptManager;
    QStringList functions;

public slots:
    void updateJsonTree();

    void HighlightErrorLine(int line);
    void ShowText(QString text); //shows text in the output box
    void ClearText(); //clears text in the output box
    void on_pbRunScript_clicked();
    //void abortEvaluation(QString message = "Aborted!");
    void onF1pressed(QString text);
    void onLoadRequested(QString NewScript);

private slots:
    void onCurrentTabChanged(int tab);   //also updates status (modified, filename)
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
    void on_actionSelect_font_triggered();

    void on_actionShow_all_messenger_windows_triggered();
    void on_actionHide_all_messenger_windows_triggered();
    void on_actionClear_unused_messenger_windows_triggered();
    void on_actionClose_all_messenger_windows_triggered();

    void on_pbFileName_clicked();
    void on_actionAdd_new_tab_triggered();
    void on_actionRemove_current_tab_triggered();
    void on_actionRemove_all_tabs_triggered();
    void on_actionStore_all_tabs_triggered();
    void on_actionRestore_session_triggered();

public:
    enum ScriptLanguageEnum {_JavaScript_ = 0, _PythonScript_ = 1};

private:
    Ui::AScriptWindow *ui;
    QStringListModel* completitionModel;
    GlobalSettingsClass* GlobSet;

    ScriptLanguageEnum ScriptLanguage = _JavaScript_;

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
    void fillHelper(QObject* obj, QString module);  //fill help TreeWidget according to the data in the obj
    QString getKeyPath(QTreeWidgetItem *item);
    void showContextMenuForJsonTree(QTreeWidgetItem *item, QPoint pos);
    QStringList getCustomCommandsOfObject(QObject *obj, QString ObjName, bool fWithArguments = false);

    void askRemoveTab(int tab);
    void removeTab(int tab);
    void clearAllTabs();
    QString createNewTabName();

    void ReadFromJson(QJsonObject &json);
    void WriteToJson(QJsonObject &json);

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
    void receivedOnAbort();
    void receivedOnSuccess(QString eval);
    void onDefaulFontSizeChanged(int size);
    void onProgressChanged(int percent);
    void updateFileStatusIndication();
};

class AScriptWindowTabItem : QObject
{
    Q_OBJECT
public:
    AScriptWindowTabItem(QAbstractItemModel *model, AScriptWindow::ScriptLanguageEnum language);
    ~AScriptWindowTabItem();

    CompletingTextEditClass* TextEdit;

    QString FileName;
    QString TabName;

    QCompleter* completer;
    AHighlighterScriptWindow* highlighter;

    void UpdateHighlight();

    void WriteToJson(QJsonObject &json);
    void ReadFromJson(QJsonObject &json);

    bool wasModified() const;
    void setModifiedStatus(bool flag);
};

#endif // ASCRIPTWINDOW_H
