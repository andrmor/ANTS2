#ifndef AMATERIALLIBRARYBROWSER_H
#define AMATERIALLIBRARYBROWSER_H

#include <QDialog>
#include <QSet>
#include <QString>
#include <QDir>

class AMaterialParticleCollection;
class QListWidgetItem;

namespace Ui {
class AMaterialLibraryBrowser;
}

struct AMaterialLibraryRecord
{
    QString       FileName;
    QString       MaterialName;
    QSet<QString> Tags;

    AMaterialLibraryRecord(const QString &FileName, const QString &MaterialName);
    AMaterialLibraryRecord(){}
};

struct ATagRecord
{
    QString Tag;
    bool    bChecked = false;

    ATagRecord(const QString & Tag);
    ATagRecord(){}
};

class AMaterialLibraryBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit AMaterialLibraryBrowser(AMaterialParticleCollection & MpCollection, QWidget * parentWidget);
    ~AMaterialLibraryBrowser();

    QString getFileName() const;
    bool    isAdvancedLoadRequested() const;

private slots:
    void on_pbDummy_clicked();
    void on_pbLoad_clicked();
    void on_pbClose_clicked();
    void on_pbClearTags_clicked();
    void on_lwMaterials_itemDoubleClicked(QListWidgetItem * item);
    void on_lwMaterials_itemClicked(QListWidgetItem *item);
    void on_lwMaterials_currentRowChanged(int currentRow);

    void on_pbAdvancedLoad_clicked();

private:
    AMaterialParticleCollection & MpCollection;
    Ui::AMaterialLibraryBrowser *ui;
    QDir Dir;
    QVector<AMaterialLibraryRecord> MaterialRecords;
    QSet<QString> Tags;
    QVector<ATagRecord> TagRecords;
    QVector<AMaterialLibraryRecord> ShownMaterials;

    QString ReturnFileName;
    bool    bAdvancedLoadRequested = false;

private:
    void updateGui();
    void readFiles();
    void out(const QString & text, bool bBold = false);
    void load();
};

#endif // AMATERIALLIBRARYBROWSER_H
