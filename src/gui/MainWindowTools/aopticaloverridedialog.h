#ifndef AOPTICALOVERRIDEDIALOG_H
#define AOPTICALOVERRIDEDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QSet>

namespace Ui {
class AOpticalOverrideDialog;
}

class AOpticalOverride;
class MainWindow;
class AOpticalOverrideTester;

class AOpticalOverrideDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AOpticalOverrideDialog(MainWindow* MW, int matFrom, int matTo);
    ~AOpticalOverrideDialog();

private slots:
    void on_pbAccept_clicked();
    void on_pbCancel_clicked();
    void on_cobType_activated(int index);
    void on_pbTestOverride_clicked();

protected:
    void closeEvent(QCloseEvent* e);

private:
    Ui::AOpticalOverrideDialog * ui;
    MainWindow* MW;
    AOpticalOverride * ovLocal = 0;
    int matFrom;
    int matTo;
    AOpticalOverrideTester* TesterWindow;

    int customWidgetPositionInLayout = 5;
    QWidget* customWidget = 0;

    QSet<AOpticalOverride*> openedOVs;

    void updateGui();
    AOpticalOverride* findInOpended(const QString& ovType);
    void clearOpenedExcept(AOpticalOverride* keepOV);
};

#endif // AOPTICALOVERRIDEDIALOG_H
