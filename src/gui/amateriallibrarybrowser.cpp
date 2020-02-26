#include "amateriallibrarybrowser.h"
#include "ui_amateriallibrarybrowser.h"
#include "aglobalsettings.h"
#include "ajsontools.h"

#include <QDebug>
#include <QDir>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QListWidget>
#include <QListWidgetItem>

AMaterialLibraryRecord::AMaterialLibraryRecord(int Index, const QString & FileName, const QString & MaterialName) :
    Index(Index), FileName(FileName), MaterialName(MaterialName) {}

ATagRecord::ATagRecord(const QString & Tag) :
    Tag(Tag) {}

AMaterialLibraryBrowser::AMaterialLibraryBrowser(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AMaterialLibraryBrowser)
{
    ui->setupUi(this);
    ui->pbDummy->setVisible(false);

    ui->pte->clear();
    QString dirName = AGlobalSettings::getInstance().ResourcesDir + "/MaterialLibrary";
    Dir = QDir(dirName, "*.mat");
    if (!Dir.exists())
        ui->pte->appendPlainText("DATA/MaterialLibrary directory not found");

    Dir.setFilter( QDir::AllEntries | QDir::NoDotAndDotDot );

    readFiles();
    updateGui();
}

AMaterialLibraryBrowser::~AMaterialLibraryBrowser()
{
    delete ui;
}

void AMaterialLibraryBrowser::on_pbDummy_clicked()
{
    //do nothing
}

void AMaterialLibraryBrowser::on_pbLoad_clicked()
{

}

void AMaterialLibraryBrowser::on_pbClose_clicked()
{
    reject();
}

void AMaterialLibraryBrowser::readFiles()
{
    QStringList Files = Dir.entryList();

    int index = 0;
    for (const QString & file : Files)
    {
        QString absolutePath = Dir.absoluteFilePath(file);
        QJsonObject json, js;
        LoadJsonFromFile(json, absolutePath);
        if (!parseJson(json, "Material", js)) return;

        QString Name;
        parseJson(js, "*MaterialName", Name);
        AMaterialLibraryRecord rec(index, absolutePath, Name);
        QJsonArray ar;
        parseJson(js, "Tags", ar);
        for (int i=0; i<ar.size(); i++)
        {
            QString tag = ar.at(i).toString();
            rec.Tags << tag;
            Tags << tag;
        }
        MaterialRecords.append(rec);
    }

    QList<QString> TagsList = QList<QString>::fromSet(Tags);
    std::sort(TagsList.begin(), TagsList.end());
    for (const QString & s : TagsList)
    {
        ATagRecord rec(s);
        TagRecords << rec;
    }
}

void AMaterialLibraryBrowser::updateGui()
{
    ui->lwTags->clear();

    QSet<QString> CheckedTags;
    for (ATagRecord & tagRec : TagRecords)
    {
        QListWidgetItem * item = new QListWidgetItem(ui->lwTags);
        QCheckBox * cb = new QCheckBox(tagRec.Tag);
        cb->setChecked(tagRec.bChecked);
        ui->lwTags->setItemWidget(item, cb);
        QObject::connect(cb, &QCheckBox::clicked, [this, &tagRec](bool flag)
        {
            tagRec.bChecked = flag;
            updateGui();
        });
        if (tagRec.bChecked) CheckedTags << tagRec.Tag;
    }

    ui->lwMaterials->clear();
    qDebug() << "Checked tags:"<<CheckedTags;
    for (const AMaterialLibraryRecord & rec : MaterialRecords)
    {
        qDebug() << rec.MaterialName << rec.Tags;
        bool bComply = true;
        for (const QString & tag : CheckedTags)
        {
            if (!rec.Tags.contains(tag))
            {
                bComply = false;
                break;
            }
        }
        if (bComply) ui->lwMaterials->addItem(rec.MaterialName);
    }

}


void AMaterialLibraryBrowser::on_pbClearTags_clicked()
{
    for (ATagRecord & tagRec : TagRecords) tagRec.bChecked = false;
    updateGui();
}
