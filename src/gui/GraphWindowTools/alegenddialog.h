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

class ALegendModelRecord
{
public:
    ALegendModelRecord(const QString Label, TObject * TObj, const QString Options) : Label(Label), TObj(TObj), Options(Options) {}
    ALegendModelRecord() {}

    QString   Label;
    TObject * TObj = nullptr;  // nullptr -> plain text, no connected object
    QString   Options;

    //runtime
    bool _flag = false; //used for reorder
};

class ALegendDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ALegendDialog(TLegend & Legend, const QVector<ADrawObject> & DrawObjects, QWidget * parent);
    ~ALegendDialog();


private slots:
    void onCurrentEntryChanged(int currentRow);
    void onLabelTextChanged();

    void on_pbCancel_clicked();
    void on_pbAccept_clicked();

private:
    Ui::ALegendDialog *ui;
    ABasketListWidget * lwList;
    TLegend & Legend;
    TLegend OriginalCopy;
    const QVector<ADrawObject> & DrawObjects;

    QVector<ALegendModelRecord> Model;
    int NumColumns = 1;

    TObject * SelectedObject = nullptr;

private slots:
    void onReorderEntriesRequested(const QVector<int> &indexes, int toRow);
    void onListMenuRequested(const QPoint &pos);
    void onListMenuMultipleSelection(const QPoint &pos);

    void on_twTree_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_sbNumColumns_editingFinished();

    void on_pbConfigureFrame_clicked();

private:
    void updateModel(TLegend & legend);

    void updateList();
    void updateTree();

    void updateLegend();

    void removeAllSelectedEntries();
    void clearLegend();
    void deleteSelectedEntry();
    void addText();

signals:
    void requestCanvasUpdate();
};

#endif // ALEGENDDIALOG_H
