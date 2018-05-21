#ifndef ASCRIPTEXAMPLEEXPLORER_H
#define ASCRIPTEXAMPLEEXPLORER_H

#include <QMainWindow>

namespace Ui {
class AScriptExampleExplorer;
}

class AScriptExampleDatabase;
class QListWidgetItem;

class AScriptExampleExplorer : public QMainWindow
{
    Q_OBJECT

public:
    explicit AScriptExampleExplorer(QString RecordsFileName, QWidget *parent);
    ~AScriptExampleExplorer();

    void UpdateGui();

private:
    Ui::AScriptExampleExplorer *ui;

    AScriptExampleDatabase* Examples;
    QString Path;
    bool fBulkUpdate;

private slots:

    void ClearSelection();
    void onExampleSelected(int row);

    void on_pbLoad_clicked();
    void on_pbToClipboard_clicked();
    void on_pbSelectAllTags_clicked();
    void on_pbUnselectAllTags_clicked();

    void onTagStateChanged(QListWidgetItem* item);
    void onFindTextChanged(const QString& text);
    void on_lwTags_itemDoubleClicked(QListWidgetItem *item);

signals:
    void LoadRequested(QString);
};

#endif // ASCRIPTEXAMPLEEXPLORER_H
