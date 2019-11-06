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

class ALegendModelRecord
{
public:
    ALegendModelRecord(QString Label, TObject * TObj) : Label(Label), TObj(TObj) {}
    ALegendModelRecord() {}

    QString   Label;
    TObject * TObj = nullptr;  // nullptr -> plain text, no connected object
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
    TObject * SelectedObject = nullptr;

private slots:
    void onReorderEntriesRequested(const QVector<int> &indexes, int toRow);

private:
    void updateModel(TLegend & legend);

    void updateList();
    void updateTree();

    void updateLegend();

signals:
    void requestCanvasUpdate();
};

#endif // ALEGENDDIALOG_H
