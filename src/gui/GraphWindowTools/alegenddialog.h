#ifndef ALEGENDDIALOG_H
#define ALEGENDDIALOG_H

#include <QDialog>
#include <QVector>

namespace Ui {
class ALegendDialog;
}

class ADrawObject;
class TLegend;
class TObject;


class ALegendDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ALegendDialog(TLegend & Legend, const QVector<ADrawObject> & DrawObjects, QWidget * parent);
    ~ALegendDialog();

    void updateMain();

private slots:
    void on_lwList_currentRowChanged(int currentRow);

    void on_pbCancel_clicked();

private:
    Ui::ALegendDialog *ui;
    TLegend & Legend;
    const QVector<ADrawObject> & DrawObjects;

    QVector<TObject*> EntryObjects;

private:
    void updateTree(TObject *selectedObj);
    bool isValidObject(TObject *obj);
};

#endif // ALEGENDDIALOG_H
