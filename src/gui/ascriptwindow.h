#ifndef ASCRIPTWINDOW_H
#define ASCRIPTWINDOW_H

#include "aguiwindow.h"
#include <QSet>
#include <QHash>
#include <QString>
#include <QList>

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
class ATabRecord;
class AGlobalSettings;
class QTabWidget;

namespace Ui {
class AScriptWindow;
}

class AScriptBook
{
public:
    AScriptBook();

    QString             Name;
    QList<ATabRecord *> Tabs;
    QTabWidget *        TabWidget   = nullptr; // will be owned by the QTabItemWidget
    int                 iCurrentTab = 0;

    ATabRecord *        getCurrentTab();
    ATabRecord *        getTab(int index);
    QTabWidget *        getTabWidget();
    void                removeTab(int index);
};

class AScriptWindow : public AGuiWindow
{
    Q_OBJECT

public:
    explicit AScriptWindow(AScriptManager *ScriptManager, bool LightMode, QWidget *parent);
    ~AScriptWindow();

    enum AScriptEnum {JavaScript = 0, Python = 1};

    void RegisterInterfaceAsGlobal(AScriptInterface * interface);
    void RegisterCoreInterfaces(bool bCore = true, bool bMath = true);
    void RegisterInterface(AScriptInterface * interface, const QString & name);
    void UpdateGui(); //highlighter, helper etc - call it to take into account all changes introduced by introduction of new interface units!

    void ReportError(QString error, int line = 0);   //0 - no line is highligted

    void WriteToJson();
    void ReadFromJson();

    void onBusyOn();
    void onBusyOff();

    void ConfigureForLightMode(QString * ScriptPtr, const QString & WindowTitle, const QString & Example);
    void EnableAcceptReject();
    bool isAccepted() const {return bAccepted;}

public slots:
    void updateJsonTree();

    void HighlightErrorLine(int line);
    void ShowText(QString text); //shows html-formatted text in the output box
    void ShowPlainText(QString text); //shows plain text in the output box
    void ClearText(); //clears text in the output box
    void onF1pressed(QString text);
    void onLoadRequested(QString NewScript);

private slots:
    //context menus
    void twBooks_customContextMenuRequested(const QPoint & pos);
    void onRequestTabWidgetContextMenu(QPoint pos);
    void onContextMenuRequestedByJsonTree(QPoint pos);
    void onContextMenuRequestedByHelp(QPoint pos);

    //tabs
    void onCurrentTabChanged(int tab);   //also updates status (modified, filename)
    void onScriptTabMoved(int from, int to);

    //books
    void twBooks_currentChanged(int index);
    void onBookTabMoved(int from, int to);

    //gui buttons
    void on_pbRunScript_clicked();
    void on_pbStop_clicked();
    void on_pbLoad_clicked();
    void on_pbSave_clicked();
    void on_pbSaveAs_clicked();
    void on_pbExample_clicked();
    void on_pbConfig_toggled(bool checked);
    void on_pbHelp_toggled(bool checked);
    void on_pbFileName_clicked();
    void on_pbCloseFindReplaceFrame_clicked();
    void on_pbFindNext_clicked();
    void on_pbFindPrevious_clicked();
    void on_leFind_textChanged(const QString &arg1);
    void on_pbReplaceOne_clicked();
    void on_pbReplaceAll_clicked();
    void on_pbAccept_clicked();
    void on_pbCancel_clicked();

    // main menu actions
    void on_actionIncrease_font_size_triggered();
    void on_actionDecrease_font_size_triggered();
    void on_actionSelect_font_triggered();
    void on_actionShow_all_messenger_windows_triggered();
    void on_actionHide_all_messenger_windows_triggered();
    void on_actionClear_unused_messenger_windows_triggered();
    void on_actionClose_all_messenger_windows_triggered();
    void on_actionShow_Find_Replace_triggered();
    void on_actionReplace_widget_Ctr_r_triggered();
    void on_actionAdd_new_tab_triggered();
    void on_actionRemove_current_tab_triggered();
    void on_actionRemove_all_tabs_triggered();
    void on_actionStore_all_tabs_triggered();
    void on_actionRestore_session_triggered();
    void on_actionShortcuts_triggered();

    void onRequestDraw(TObject* obj, QString options, bool fFocus) {emit RequestDraw(obj, options, fFocus);}    
    void onFunctionClicked(QTreeWidgetItem* item, int column);  //help has to be shown
    void onKeyDoubleClicked(QTreeWidgetItem* item, int column);
    void onKeyClicked(QTreeWidgetItem* item, int column);
    void onFindTextChanged(const QString &arg1);
    void onFindTextJsonChanged(const QString &arg1);

    void onJsonTWExpanded(QTreeWidgetItem* item);
    void onJsonTWCollapsed(QTreeWidgetItem* item);

private:
    AScriptManager *    ScriptManager  = nullptr;
    AScriptEnum         ScriptLanguage = JavaScript;
    bool                bLightMode     = false;  // true -> to imitate former genericscriptwindow. Used for local scripts
    Ui::AScriptWindow * ui             = nullptr;
    AGlobalSettings &   GlobSet;
    QStringList         functions;

    std::vector<AScriptBook> ScriptBooks;       //vector does not require default constructor, while QVector does
    int                 iCurrentBook   = 0;
    QTabWidget *        twBooks        = nullptr;

    ATabRecord *        StandaloneTab  = nullptr;

    int                 iMarkedBook    = -1;
    int                 iMarkedTab     = -1;

    bool                bAccepted      = false;
    QString *           LightModeScript = nullptr;
    QString             LightModeExample;

    QSplitter *         splMain        = nullptr;
    QSplitter *         splHelp        = nullptr;
    QPlainTextEdit *    pteOut         = nullptr;
    QTreeWidget *       trwHelp        = nullptr;
    QTreeWidget *       trwJson        = nullptr;
    QFrame *            frHelper       = nullptr;
    QFrame *            frJsonBrowser  = nullptr;
    QPlainTextEdit *    pteHelp        = nullptr;
    QLineEdit *         leFind         = nullptr;
    QLineEdit *         leFindJ        = nullptr;
    QIcon *             RedIcon        = nullptr;

    bool                bShowResult    = true;   //if false, window only reports "success", otherwise eval result is shown  !*! need it?

    QSet<QString>       ExpandedItemsInJsonTW;
    QStringList         functionList; //functions to populate tooltip helper
    QHash<QString, QString> DeprecatedOrRemovedMethods;
    QStringList         ListOfDeprecatedOrRemovedMethods;
    QStringList         ListOfConstants;

    void                addNewBook();
    void                renameBook(int iBook, const QString & newName);
    void                renameBookRequested(int iBook);
    QList<ATabRecord *> & getScriptTabs();          // !*! to pointer
    QList<ATabRecord *> & getScriptTabs(int iBook); // !*! to pointer
    ATabRecord *        getTab();
    ATabRecord *        getTab(int index);
    ATabRecord *        getTab(int index, int iBook);
    QTabWidget *        getTabWidget();
    QTabWidget *        getTabWidget(int iBook);
    int                 getCurrentTabIndex();
    void                setCurrentTabIndex(int index);
    void                setCurrentTabIndex(int index, int iBook);
    int                 countTabs(int iBook) const;
    int                 countTabs() const;

    ATabRecord &        addNewTab(int iBook);
    ATabRecord &        addNewTab();

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
    void markTab(int tab);
    void copyTab(int iBook);
    void moveTab(int iBook);

    void applyTextFindState();
    void findText(bool bForward);

    void UpdateTab(ATabRecord *tab);
    void doRegister(AScriptInterface *interface, const QString& name);
    void formatTab(ATabRecord *tab);

protected:
  virtual void closeEvent(QCloseEvent * e);
  virtual bool event(QEvent * e);

signals:
    void WindowShown(QString);
    void WindowHidden(QString);
    void RequestDraw(TObject* obj, QString options, bool fFocus);
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
    virtual void onFindFunction();              // why virtual? is it the idea to make python / JS subclasses?
    virtual void onFindVariable();
    void onBack();
    void onForward();

};


class ATabRecord : public QObject
{
    Q_OBJECT
public:
    ATabRecord(const QStringList & functions, AScriptWindow::AScriptEnum language);
    ~ATabRecord();

    ATextEdit *     TextEdit            = nullptr;

    QString         FileName;
    QString         TabName;
    bool            bExplicitlyNamed    = false;   //if true save will not auto-rename

    const QStringList & Functions;

    QCompleter *    Completer           = nullptr;
    QStringListModel * CompletitionModel;
    AHighlighterScriptWindow * Highlighter = nullptr;

    QVector<int>    VisitedLines;
    int             IndexVisitedLines   = 0;
    int             MaxLineNumbers      = 20;

    void UpdateHighlight();

    void WriteToJson(QJsonObject & json);
    void ReadFromJson(QJsonObject & json);

    bool wasModified() const;
    void setModifiedStatus(bool flag);

    void goBack();
    void goForward();

private slots:
    void onCustomContextMenuRequested(const QPoint & pos);
    void onLineNumberChanged(int lineNumber);
    void onTextChanged();

signals:
    void requestFindText();
    void requestReplaceText();
    void requestFindFunction();
    void requestFindVariable();
};

#endif // ASCRIPTWINDOW_H
