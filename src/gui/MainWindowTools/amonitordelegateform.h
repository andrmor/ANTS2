#ifndef AMONITORDELEGATEFORM_H
#define AMONITORDELEGATEFORM_H

#include <QWidget>

namespace Ui {
class AMonitorDelegateForm;
}

class AGeoObject;

class AMonitorDelegateForm : public QWidget
{
    Q_OBJECT

public:
    explicit AMonitorDelegateForm(QWidget *parent = 0);
    ~AMonitorDelegateForm();

    Ui::AMonitorDelegateForm *ui;

    bool updateGUI(const AGeoObject *obj);
    QString getName() const;
    void updateObject(AGeoObject* obj);

public slots:
    void UpdateVisibility();

private slots:
    void on_cobShape_currentIndexChanged(int index);
    void on_cobMonitoring_currentIndexChanged(int index);
    void on_pbContentChanged_clicked();

signals:
    void contentChanged();
};

#endif // AMONITORDELEGATEFORM_H
