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
    explicit AMonitorDelegateForm(QStringList particles, QWidget *parent = 0);
    ~AMonitorDelegateForm();

    Ui::AMonitorDelegateForm *ui;

    bool updateGUI(const AGeoObject *obj);
    const QString getName() const;
    void updateObject(AGeoObject* obj) const;

public slots:
    void UpdateVisibility();

private slots:
    void on_cobShape_currentIndexChanged(int index);
    void on_cobMonitoring_currentIndexChanged(int index);
    void on_pbContentChanged_clicked();
    void on_pbShowSensitiveDirection_clicked();

signals:
    void contentChanged();
    void showSensDirection();
};

#endif // AMONITORDELEGATEFORM_H
