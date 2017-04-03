#ifndef CHECKUPWINDOWCLASS_H
#define CHECKUPWINDOWCLASS_H

#include <QMainWindow>
#include <QTableWidgetItem>

class MainWindow;
class QTableWidget;
class DetectorClass;

enum TriState { TriStateOk = 0, TriStateWarning = 1, TriStateError = 2 };
TriState operator &(const TriState a, const TriState b);
TriState operator &=(TriState &a, const TriState b);
TriState operator |(const TriState a, const TriState b);
TriState operator |=(TriState &a, const TriState b);


namespace Ui {
  class CheckUpWindowClass;
}

class CheckUpWindowClass : public QMainWindow
{
  Q_OBJECT

public:
  explicit CheckUpWindowClass(MainWindow *parent, DetectorClass* detector);
  ~CheckUpWindowClass();
  void CheckAll() {
      CheckGeoOverlaps();
      CheckOptics();
      CheckInteractions();
      CheckPMs();
  }
  TriState CheckGeoOverlaps();
  TriState CheckOptics();
  TriState CheckPMs();
  TriState CheckInteractions();
  TriState Refresh();
  static QIcon createColorCircleIcon(QSize size, Qt::GlobalColor color);

protected:
    QIcon& getIcon(TriState state);
    TriState setTabState(int tab, TriState state);
    void setupTable(QTableWidget *table, const QStringList &columns, bool columnsHaveIcons);
    TriState checkTable(QTableWidget *table, bool setColumnIcons);
    TriState refreshTab(int index);
    void adjustTable(QTableWidget *table);
    void adjustTable(QTableWidget *table, int maxw, int maxh);

    void showEvent(QShowEvent *ev);
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *ev);

private slots:
    void on_overlaplist_clicked(const QModelIndex &index);
    void on_tabWidget_currentChanged(int index);
    void onopticsTable_rowSelected(int row);
    void on_opticsTable_customContextMenuRequested(const QPoint &pos);
    void on_pmtsTable_customContextMenuRequested(const QPoint &pos);
    void onpmtsTable_rowSelected(int row);
    void on_buttonRefreshAll_clicked() { CheckAll(); }
    void on_buttonRefreshCurrent_clicked() { Refresh(); }



    void on_pbSaveOverlaps_clicked();

private:
    Ui::CheckUpWindowClass *ui;
    MainWindow* MW;
    DetectorClass* Detector;
    QIcon iconRed;
    QIcon iconYellow;
    QIcon iconGreen;
};

////////////////////////////////////////////////////////////////////////////////////////

class CheckUpItem
{
public:
    CheckUpItem() { this->enabled = true; }
    virtual TriState checkUp();
    virtual bool toggleEnabled();
    bool isEnabled() const { return enabled; }
    virtual TriState getState() const;

    static TriState rangeCheck(const QVector<double> &vec, double from, double to);

protected:
    virtual TriState doCheckUp() = 0;
    bool enabled;
    TriState state;

    static const QString wavelenToolTips[];
    static const QColor Color[];
    static const QColor ColorDisabled;
};

class CheckUpTableItem : public QTableWidgetItem, public CheckUpItem
{
public:
    CheckUpTableItem();
    virtual TriState checkUp();
    virtual TriState getState() const;
    bool toggleEnabled();

protected:
    virtual TriState setState(TriState state, QString text = "");
    virtual const QString getToolTip() const = 0;
};

class RefractionCheckUpItem : public CheckUpTableItem
{
public:
    RefractionCheckUpItem(const MainWindow *MW) : MW(MW), usingDefault(false) {}
    TriState doCheckUp();
protected:
    const QString getToolTip() const;
    const MainWindow *MW;
    bool usingDefault;
};

class AbsorptionCheckUpItem : public CheckUpTableItem
{
public:
    AbsorptionCheckUpItem(const MainWindow *MW) : MW(MW), usingDefault(false) {}
    TriState doCheckUp();
protected:
    const QString getToolTip() const;
    const MainWindow *MW;
    bool usingDefault;
};

class PrimaryScintCheckUpItem : public CheckUpTableItem
{
public:
    PrimaryScintCheckUpItem(const MainWindow *MW) : MW(MW) {}
    TriState doCheckUp();
protected:
    const QString getToolTip() const;
    const MainWindow *MW;
};

class SecondaryScintCheckUpItem : public CheckUpTableItem
{
public:
    SecondaryScintCheckUpItem(const MainWindow *MW) : MW(MW) {}
    TriState doCheckUp();
protected:
    const QString getToolTip() const;
    const MainWindow *MW;
};

class DESCheckUpItem : public CheckUpTableItem
{
public:
    DESCheckUpItem(const MainWindow *MW) : MW(MW) {}
    TriState doCheckUp();
protected:
    const QString getToolTip() const;
    const MainWindow *MW;
};

class DEWCheckUpItem : public CheckUpTableItem
{
public:
    DEWCheckUpItem(const MainWindow *MW) : MW(MW), inherited(TriStateOk), overriden(TriStateOk) {}
    TriState doCheckUp();
protected:
    const QString getToolTip() const;

    const MainWindow *MW;
    TriState inherited, overriden;
};

class AngularResponseCheckUpItem : public CheckUpTableItem
{
public:
    AngularResponseCheckUpItem(const MainWindow *MW) : MW(MW) {}
    TriState doCheckUp();
protected:
    const QString getToolTip() const;
    const MainWindow *MW;
};

class AreaResponseCheckUpItem : public CheckUpTableItem
{
public:
    AreaResponseCheckUpItem(const MainWindow *MW) : MW(MW) {}
    TriState doCheckUp();
protected:
    const QString getToolTip() const;
    const MainWindow *MW;
};

#endif // CHECKUPWINDOWCLASS_H
