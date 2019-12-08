#ifndef ATEMPLATESELECTIONDIALOG_H
#define ATEMPLATESELECTIONDIALOG_H

#include <QDialog>

namespace Ui {
class ATemplateSelectionDialog;
}

class ATemplateSelectionRecord;
class QCheckBox;

class ATemplateSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ATemplateSelectionDialog(ATemplateSelectionRecord & Selection, QWidget *parent = 0);
    ~ATemplateSelectionDialog();

private slots:
    void on_pbAccept_clicked();
    void on_pbCancel_clicked();

private:
    ATemplateSelectionRecord & Selection;
    Ui::ATemplateSelectionDialog *ui;

private:
    void addItem(ATemplateSelectionRecord * rec, ATemplateSelectionRecord * parent);
    void updateCheckStateIndication(ATemplateSelectionRecord * rec);
    QCheckBox * getCheckBox(ATemplateSelectionRecord *rec);
    void setCheckedStatusRecursive(ATemplateSelectionRecord * rec, bool flag);
    void setCheckedForParentsRecursive(ATemplateSelectionRecord * rec);
    bool containsCheckedChild(ATemplateSelectionRecord * rec) const;
    bool containsUncheckedChild(ATemplateSelectionRecord * rec) const;

};

#endif // ATEMPLATESELECTIONDIALOG_H
