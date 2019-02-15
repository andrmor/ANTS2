#include "ascriptexampleexplorer.h"
#include "ui_ascriptexampleexplorer.h"
#include "ascriptexampledatabase.h"
//#include "aglobalsettings.h"

#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QTimer>
#include <QClipboard>
#include <QListWidgetItem>

AScriptExampleExplorer::AScriptExampleExplorer(QString RecordsFileName, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AScriptExampleExplorer)
{
    ui->setupUi(this);
    setWindowTitle("Script example explorer");

    QFile file(RecordsFileName);
    if (!file.open(QIODevice::ReadOnly))
      {
        qWarning() << "Failed to open example list file:\n"+RecordsFileName;
        this->close();
        return;
      }

    QTextStream in(&file);
    QString Records = in.readAll();
    Examples = new AScriptExampleDatabase(Records);
    Path = QFileInfo(RecordsFileName).absolutePath() + "/";
      //qDebug() << Path;

    QObject::connect(ui->lwExamples, SIGNAL(currentRowChanged(int)), this, SLOT(onExampleSelected(int)));

    //showing tags - they will not be modified, only checked/unchecked
    fBulkUpdate = true;
    for (int i=0; i<Examples->Tags.size(); i++)
    {
        QListWidgetItem* item = new QListWidgetItem(Examples->Tags.at(i), ui->lwTags);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);        
    }
    fBulkUpdate = false;

    connect(ui->lwTags, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(onTagStateChanged(QListWidgetItem*)));
    connect(ui->leFind, &QLineEdit::textChanged, this, &AScriptExampleExplorer::onFindTextChanged);
    UpdateGui();

    QTimer::singleShot(100, this, SLOT(ClearSelection()));
}

AScriptExampleExplorer::~AScriptExampleExplorer()
{
     //qDebug() << "Script example explorer: destructor triggered";
    delete ui;
    delete Examples;
}

void AScriptExampleExplorer::UpdateGui()
{    
    QStringList SelectedTags;
    for (int i=0; i<ui->lwTags->count(); i++)
        if (ui->lwTags->item(i)->checkState() == Qt::Checked) SelectedTags.append(ui->lwTags->item(i)->text());
    Examples->Select(SelectedTags);

    ui->lwExamples->clear();

    for (int i=0; i<Examples->size(); i++)
    {
        if (Examples->Examples.at(i).fSelected)
          ui->lwExamples->addItem(Examples->Examples.at(i).FileName);
    }
}

void AScriptExampleExplorer::ClearSelection()
{
    ui->lwExamples->clearSelection();
    ui->lwExamples->setCurrentRow(-1);
    ui->leFileName->clear();
    ui->teDescription->clear();
    ui->tePreview->clear();
}

void AScriptExampleExplorer::onExampleSelected(int row)
{
    if (row<0)
    {
        ClearSelection();
        return;
    }

    QString fn = ui->lwExamples->currentItem()->text();
    int index = Examples->Find(fn);
    if (index < 0)
    {
        ClearSelection();
        qWarning() << "Example not found!";
        return;
    }

    const AScriptExample& ex = Examples->Examples.at(index);
    ui->leFileName->setText(ex.FileName);
    ui->teDescription->setText(ex.Description);
    QString script;
    QFile file(Path + ex.FileName);
    if (!file.open(QIODevice::ReadOnly))
        script = "Failed to open example file";
    else
    {
        QTextStream in(&file);
        script = in.readAll();
    }
    ui->tePreview->setText(script);
}

void AScriptExampleExplorer::on_pbLoad_clicked()
{
    QString fn = ui->leFileName->text();
    if (fn.isEmpty()) return;

    emit LoadRequested(ui->tePreview->document()->toPlainText());
    this->close();
}

void AScriptExampleExplorer::on_pbToClipboard_clicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->tePreview->document()->toPlainText());
}

void AScriptExampleExplorer::on_pbSelectAllTags_clicked()
{
    for (int i=0; i<ui->lwTags->count(); i++)
        ui->lwTags->item(i)->setCheckState(Qt::Checked);
}

void AScriptExampleExplorer::on_pbUnselectAllTags_clicked()
{
    for (int i=0; i<ui->lwTags->count(); i++)
        ui->lwTags->item(i)->setCheckState(Qt::Unchecked);
}

void AScriptExampleExplorer::onTagStateChanged(QListWidgetItem* /*item*/)
{
    if (fBulkUpdate) return;
    UpdateGui();
}

void AScriptExampleExplorer::onFindTextChanged(const QString &text)
{
    const bool bEmpty = text.isEmpty();

    for (int i=0; i<ui->lwTags->count(); i++)
    {
        bool bVisible = bEmpty || ui->lwTags->item(i)->text().contains(text, Qt::CaseInsensitive);

        ui->lwTags->item(i)->setHidden( !bVisible );
        ui->lwTags->item(i)->setCheckState( bVisible ? Qt::Checked : Qt::Unchecked );
    }
}

void AScriptExampleExplorer::on_lwTags_itemDoubleClicked(QListWidgetItem *item)
{
    for (int i=0; i<ui->lwTags->count(); i++)
        if (ui->lwTags->item(i) == item)
            ui->lwTags->item(i)->setCheckState(Qt::Checked);
        else
            ui->lwTags->item(i)->setCheckState(Qt::Unchecked);
}
