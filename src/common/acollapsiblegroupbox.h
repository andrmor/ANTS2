#ifndef ACOLLAPSIBLEGROUPBOX_H
#define ACOLLAPSIBLEGROUPBOX_H

#include <QFrame>
#include <QCheckBox>

class ACollapsibleGroupBox : public QWidget
{
    Q_OBJECT
public:
    ACollapsibleGroupBox(QString title = "");
    QWidget* getClientAreaWidget() {return ClientArea;}

    void setChecked(bool flag);
    bool isChecked();

private:
    QWidget* ClientArea;
    QFrame* line1;
    QFrame* line2;
    QCheckBox* cb;
    void updateLineVisibility();

signals:
    void toggled(bool);
    void clicked();

private slots:
    void onToggle(bool flag);
    void onClicked();
};

#endif // ACOLLAPSIBLEGROUPBOX_H
