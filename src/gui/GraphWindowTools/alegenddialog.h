#ifndef ALEGENDDIALOG_H
#define ALEGENDDIALOG_H

#include <QDialog>
#include <QVector>

#include "TLegend.h"

namespace Ui {
class ALegendDialog;
}

class ABasketListWidget;
class ADrawObject;
class TObject;
class QListWidgetItem;
class QTreeWidgetItem;

class ALegendEntryRecord
{
public:
    ALegendEntryRecord(const QString Label, TObject * TObj, const QString Options) : Label(Label), TObj(TObj), Options(Options) {}
    ALegendEntryRecord() {}

    QString   Label;
    TObject * TObj = nullptr;  // nullptr -> plain text, no connected object
    QString   Options;

    bool      bAttributeOverride = false;
    int       TextColor = 1;
    int       TextAlign = 12;
    int       TextFont  = 42;
    float     TextSize  = 0;

    //runtime
    bool _flag = false; //used for reorder
};

class ALegendData
{
public:
    QVector<ALegendEntryRecord> Model;
    int NumColumns = 1;

    double    Xfrom, Xto, Yfrom, Yto;

    int       DefaultTextColor = 1;
    int       DefaultTextAlign = 12;
    int       DefaultTextFont  = 42;
    float     DefaultTextSize  = 0;
};

class ALegendDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ALegendDialog(TLegend & Legend, const QVector<ADrawObject> & DrawObjects, QWidget * parent);
    ~ALegendDialog();

private slots:
    void onListMenuRequested(const QPoint &pos);
    void onFocusChanged(QWidget * oldW, QWidget * newW );
    void onEntrySelectionChanged();
    void onEntryWasEdited(int index, const QString & label, bool line, bool mark, bool fill);
    void onReorderEntriesRequested(const QVector<int> &indexes, int toRow);
    void on_twTree_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void setNumberOfColumns();

private:
    Ui::ALegendDialog *ui;
    ABasketListWidget * lwList;
    TLegend & Legend;
    const QVector<ADrawObject> & DrawObjects;

    ALegendData CurrentModel;
    ALegendData OriginalModel;

    TObject * SelectedObject = nullptr;

private slots:
    void on_pbCancel_clicked();
    void on_pbAccept_clicked();
    void on_pbConfigureFrame_clicked();
    void on_pbAddEntry_clicked();
    void on_pbRemoveSelected_clicked();
    void on_pbRemoveAll_clicked();
    void on_pbDefaultTextProperties_clicked();
    void on_pbThisEntryTextAttributes_clicked();
    void on_ledXfrom_editingFinished();
    void on_ledXto_editingFinished();
    void on_ledYfrom_editingFinished();
    void on_ledYto_editingFinished();

private:
    void updateModel(TLegend & legend);

    void updateMainGui();
    void updateTree();

    void updateLegend();

    void removeAllSelectedEntries();
    void clearLegend();
    void deleteSelectedEntry();
    void addText();
    void configureMenu();

signals:
    void requestCanvasUpdate();
};

#include <QFrame>
class QCheckBox;
class QLineEdit;

class ALegendEntryDelegate : public QFrame
{
    Q_OBJECT
public:
    ALegendEntryDelegate(const ALegendEntryRecord & record, int index);

private:
    int Index;

    QLineEdit * le = nullptr;
    QCheckBox * cbLine   = nullptr;
    QCheckBox * cbMarker = nullptr;
    QCheckBox * cbFill   = nullptr;

private slots:
    void onContentChanged();

protected:
    void keyPressEvent(QKeyEvent *event);

signals:
    void contentWasEdited(int index, const QString & label, bool line, bool mark, bool fill);

};

#endif // ALEGENDDIALOG_H
