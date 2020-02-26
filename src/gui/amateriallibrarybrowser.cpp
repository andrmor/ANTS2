#include "amateriallibrarybrowser.h"
#include "ui_amateriallibrarybrowser.h"
#include "aglobalsettings.h"
#include "ajsontools.h"

#include <QDebug>
#include <QDir>
#include <QPlainTextEdit>

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
        if (!parseJson(json, "Material", js))
            return;

        QString Name;
        parseJson(js, "*MaterialName", Name);
        AMaterialLibraryRecord rec(index, absolutePath, Name);
        QJsonArray ar;
        parseJson(js, "Tags", ar);
        for (int i=0; i<ar.size(); i++)
        {
            QString tag = ar.at(i).toString();
            rec.addTag(tag);
            Tags << tag;
        }
        Records.append(rec);
    }

    qDebug() << Tags;
}

void AMaterialLibraryBrowser::updateGui()
{
    ui->lwTags->clear();

    QList<QString> TagsList = QList<QString>::fromSet(Tags);
    std::sort(TagsList.begin(), TagsList.end());
    for (const QString & s : TagsList)
    {
        qDebug() << s;
        ui->lwTags->addItem(s);
    }

    for (int i=0; i<Records.size(); i++)
    {
        qDebug() << Records.at(i).MaterialName;
        ui->lwMaterials->addItem(Records.at(i).MaterialName);
    }

}

AMaterialLibraryRecord::AMaterialLibraryRecord(int Index, const QString & FileName, const QString & MaterialName) :
    Index(Index), FileName(FileName), MaterialName(MaterialName) {}

void AMaterialLibraryRecord::addTag(const QString & tag)
{
    Tags << tag;
}
