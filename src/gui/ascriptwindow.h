#ifndef ASCRIPTWINDOW_H
#define ASCRIPTWINDOW_H

#include "aguiwindow.h"
#include <QSet>
#include <QHash>
#include <QString>

class AScriptInterface;
class AHighlighterScriptWindow;
class QCompleter;
class QStringListModel;
class ATextEdit;
class QPlainTextEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QSplitter;
class QFrame;
class QLineEdit;
class TObject;
class AScriptManager;
class AScriptWindowTabItem;
class AGlobalSettings;

namespace Ui {
class AScriptWindow;
}

class AScriptWindow : public AGuiWindow
{
    Q_OBJECT

public:
    explicit AScriptWindow(AScriptManager *ScriptManager, bool LightMode, QWidget *parent);
    ~AScriptWindow();

    //void SetInterfaceObject(QObject *interfaceObject, QString name = ""); // if not lightMode, do not forget to call UpdateAllTabs() after all units were registered!
    void RegisterInterfaceAsGlobal(AScriptInterface* interface);
    void RegisterCoreInterfaces(bool bCore = true, bool bMath = true);
    void RegisterInterface(AScriptInterface* interface, const QString& name);
    void UpdateGui(); //highlighter, helper etc - call it to take into account all changes introduced by introduction of new interface units!

    void SetShowEvaluationResult(bool flag) {ShowEvalResult = flag;} //if false, window only reports "success", ptherwise eval result is shown

    void AddNewTab();  // new tab !

    void ReportError(QString error, int line = 0);   //0 - no line is highligted

    void WriteToJson();
    void ReadFromJson();

    void SetMainSplitterSizes(QList<int> values);

    void onBusyOn();
    void onBusyOff();

    void ConfigureForLightMode(QString* ScriptPtr, const QString &WindowTitle, const QString& Example);
    void EnableAcceptReject();
    bool isAccepted() const {return bAccepted;}

    AScriptManager* ScriptManager;
    QStringList functions;

private:
    void doRegister(AScriptInterface *interface, const QString& name);

public slots:
    void updateJsonTree();

    void HighlightErrorLine(int line);
    void ShowText(QString text); //shows html-formatted text in the output box
    void ShowPlainText(QString text); //shows plain text in the output box
    void ClearText(); //clears text in the output box
    void on_pbRunScript_clicked();
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

    void on_pbCloseFindReplaceFrame_clicked();
    void on_actionShow_Find_Replace_triggered();
    void on_pbFindNext_clicked();
    void on_pbFindPrevious_clicked();
    void on_leFind_textChanged(const QString &arg1);
    void on_pbReplaceOne_clicked();
    void on_pbReplaceAll_clicked();
    void on_actionReplace_widget_Ctr_r_triggered();

public:
    enum ScriptLanguageEnum {_JavaScript_ = 0, _PythonScript_ = 1};

private:
    Ui::AScriptWindow *ui;
    AGlobalSettings& GlobSet;
    ScriptLanguageEnum ScriptLanguage = _JavaScript_;

    int CurrentTab;
    QList<AScriptWindowTabItem*> ScriptTabs;
    QTabWidget* twScriptTabs;

    bool bLightMode = false;  // true -> to imitate former genericscriptwindow. Used for local scripts
    bool bAccepted = false;
    QString* LightModeScript = 0;
    QString  LightModeExample;

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

    bool ShowEvalResult;

    QSet<QString> ExpandedItemsInJsonTW;
    QStringList functionList; //functions to populate tooltip helper
    QHash<QString, QString> DeprecatedOrRemovedMethods;
    QStringList ListOfDeprecatedOrRemovedMethods;
    QStringList ListOfConstants;

    void fillSubObject(QTreeWidgetItem* parent, const QJsonObject& obj);
    void fillSubArray(QTreeWidgetItem* parent, const QJsonArray& arr);
    QString getDesc(const QJsonValue &ref);
    void fillHelper(QObject* obj, QString module);  //fill help TreeWidget according to the data in the obj
    QString getKeyPath(QTreeWidgetItem *item);
    void showContextMenuForJsonTree(QTreeWidgetItem *item, QPoint pos);
    QStringList getListOfMethods(QObject *obj, QString ObjName, bool fWithArguments = false);
    void appendDeprecatedOrRemovedMethods(const AScriptInterface *obj, const QString& name);

    void ReadFromJson(QJsonObject &json);
    void WriteToJson(QJsonObject &json);

    void askRemoveTab(int tab);
    void removeTab(int tab);
    void clearAllTabs();
    QString createNewTabName();
    void renameTab(int tab);

    void applyTextFindState();
    void findText(bool bForward);

    void UpdateTab(AScriptWindowTabItem *tab);
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
    void onFinish(bool bError);
    void success(QString eval);

public slots:
    void receivedOnStart() {emit onStart();}
    void receivedOnAbort();
    void receivedOnSuccess(QString eval);
    void onDefaulFontSizeChanged(int size);
    void onProgressChanged(int percent);
    void updateFileStatusIndication();

private slots:
    void onFindSelected();
    void onReplaceSelected();
    virtual void onFindFunction();
    virtual void onFindVariable();
    void onBack();
    void onForward();
    void on_pbAccept_clicked();
    void on_pbCancel_clicked();
};

class AScriptWindowTabItem : public QObject
{
    Q_OBJECT
public:
    AScriptWindowTabItem(const QStringList& functions, AScriptWindow::ScriptLanguageEnum language);
    ~AScriptWindowTabItem();

    ATextEdit* TextEdit;

    QString FileName;
    QString TabName;
    bool    bExplicitlyNamed = false;   //if true save will not auto-rename

    const QStringList& functions;

    QCompleter* completer = 0;
    QStringListModel* completitionModel;
    AHighlighterScriptWindow* highlighter = 0;

    QVector<int> VisitedLineNumber;
    int indexInVisitedLineNumber = 0;
    int maxLineNumbers = 20;

    void UpdateHighlight();

    void WriteToJson(QJsonObject &json);
    void ReadFromJson(QJsonObject &json);

    bool wasModified() const;
    void setModifiedStatus(bool flag);

    void goBack();
    void goForward();

private slots:
    void onCustomContextMenuRequested(const QPoint& pos);
    void onLineNumberChanged(int lineNumber);
    void onTextChanged();

signals:
    void requestFindText();
    void requestReplaceText();
    void requestFindFunction();
    void requestFindVariable();
};

#endif // ASCRIPTWINDOW_H
