#ifndef AWEBSOCKETSERVERDIALOG_H
#define AWEBSOCKETSERVERDIALOG_H

#include <QDialog>

namespace Ui {
class AWebSocketServerDialog;
}

class MainWindow;

class AWebSocketServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AWebSocketServerDialog(MainWindow* MW);
    ~AWebSocketServerDialog();

public slots:
    void updateNetGui();
    void addText(const QString text);

private slots:
    void on_pbStartWS_clicked();
    void on_pbStopWS_clicked();
    void on_pbStartJSR_clicked();
    void on_pbStopJSR_clicked();
    void on_pbSettings_clicked();

private:
    MainWindow* MW;
    Ui::AWebSocketServerDialog *ui;
};

#endif // AWEBSOCKETSERVERDIALOG_H
