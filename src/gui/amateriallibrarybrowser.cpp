#include "amateriallibrarybrowser.h"
#include "ui_amateriallibrarybrowser.h"
#include "aglobalsettings.h"
#include "ajsontools.h"
#include "amaterialparticlecolection.h"
#include "amessage.h"

#include <QDebug>
#include <QDir>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QListWidget>
#include <QListWidgetItem>

AMaterialLibraryRecord::AMaterialLibraryRecord(const QString & FileName, const QString & MaterialName) :
    FileName(FileName), MaterialName(MaterialName) {}

ATagRecord::ATagRecord(const QString & Tag) :
    Tag(Tag) {}

AMaterialLibraryBrowser::AMaterialLibraryBrowser(AMaterialParticleCollection & MpCollection, QWidget * parentWidget) :
    QDialog(parentWidget),
    MpCollection(MpCollection),
    ui(new Ui::AMaterialLibraryBrowser)
{
    ui->setupUi(this);
    setWindowTitle("Load material from library");

    ui->pbDummy->setDefault(true);
    ui->pbDummy->setVisible(false);

    QString dirName = AGlobalSettings::getInstance().ResourcesDir + "/MaterialLibrary";
    Dir = QDir(dirName, "*.mat");
    if (!Dir.exists()) out("DATA/MaterialLibrary directory not found", true);

    Dir.setFilter( QDir::AllEntries | QDir::NoDotAndDotDot );

    readFiles();
    updateGui();
}

AMaterialLibraryBrowser::~AMaterialLibraryBrowser()
{
    delete ui;
}

QString AMaterialLibraryBrowser::getFileName() const
{
    return ReturnFileName;
}

bool AMaterialLibraryBrowser::isAdvancedLoadRequested() const
{
    return bAdvancedLoadRequested;
}

void AMaterialLibraryBrowser::on_pbDummy_clicked()
{
    //do nothing - just intercepts "enter" pressed
}

void AMaterialLibraryBrowser::on_pbLoad_clicked()
{
    bAdvancedLoadRequested = false;
    load();
}

void AMaterialLibraryBrowser::on_pbAdvancedLoad_clicked()
{
    bAdvancedLoadRequested = true;
    load();
}

void AMaterialLibraryBrowser::load()
{
    int row = ui->lwMaterials->currentRow();
    if (row == -1)
    {
        message("Select material to load", this);
        return;
    }

    if (row < 0 || row >= ShownMaterials.size())
    {
        message("Error: model corrupted", this);
        return;
    }

    ReturnFileName = ShownMaterials.at(row).FileName;

    if (MpCollection.getListOfMaterialNames().contains(ShownMaterials.at(row).MaterialName))
        bAdvancedLoadRequested = true;

    accept();
}

void AMaterialLibraryBrowser::on_pbClose_clicked()
{
    reject();
}

void AMaterialLibraryBrowser::readFiles()
{
    QStringList Files = Dir.entryList();

    for (const QString & file : Files)
    {
        QString absolutePath = Dir.absoluteFilePath(file);
        QJsonObject json, js;
        LoadJsonFromFile(json, absolutePath);
        if (!parseJson(json, "Material", js))
        {
            qWarning() << "Bad json structure for:\n" << absolutePath;
            continue;
        }

        QString Name;
        parseJson(js, "*MaterialName", Name);
        AMaterialLibraryRecord rec(absolutePath, Name);
        QJsonArray ar;
        parseJson(js, "*Tags", ar);
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

void AMaterialLibraryBrowser::out(const QString &text, bool bBold)
{
    if (bBold) ui->pte->appendHtml( QString("<html><b>%1</b</html>").arg(text) );
    else       ui->pte->appendPlainText(text);
}

void AMaterialLibraryBrowser::updateGui()
{
    ui->lwTags->clear();
    ui->lwMaterials->clear();

    QSet<QString> CheckedTags;
    for (ATagRecord & tagRec : TagRecords)
        if (tagRec.bChecked) CheckedTags << tagRec.Tag;

    QSet<QString> NarrowingTags;
    ShownMaterials.clear();
    for (const AMaterialLibraryRecord & rec : MaterialRecords)
    {
        bool bComply = true;
        for (const QString & tag : CheckedTags)
        {
            if (!rec.Tags.contains(tag))
            {
                bComply = false;
                break;
            }
        }

        if (bComply)
        {
            ui->lwMaterials->addItem(rec.MaterialName);
            ShownMaterials << rec;
            for (const QString & tag : rec.Tags)
                NarrowingTags << tag;
        }
    }

    // checked first
    for (ATagRecord & tagRec : TagRecords)
    {
        if (!tagRec.bChecked) continue;

        QListWidgetItem * item = new QListWidgetItem(ui->lwTags);
        QCheckBox * cb = new QCheckBox(tagRec.Tag);
        cb->setChecked(true);
        ui->lwTags->setItemWidget(item, cb);
        QObject::connect(cb, &QCheckBox::clicked, [this, &tagRec](bool flag)
        {
            tagRec.bChecked = flag;
            updateGui();
        });
    }
    // next narrowing ones
    for (ATagRecord & tagRec : TagRecords)
    {
        if (tagRec.bChecked) continue;                      //already processed
        if (!NarrowingTags.contains(tagRec.Tag)) continue;

        QListWidgetItem * item = new QListWidgetItem(ui->lwTags);
        QCheckBox * cb = new QCheckBox(tagRec.Tag);
        cb->setChecked(false);
        ui->lwTags->setItemWidget(item, cb);
        QObject::connect(cb, &QCheckBox::clicked, [this, &tagRec](bool flag)
        {
            tagRec.bChecked = flag;
            updateGui();
        });
    }
    // everything else as disabled
    for (ATagRecord & tagRec : TagRecords)
    {
        if (tagRec.bChecked) continue;                      //already processed
        if (NarrowingTags.contains(tagRec.Tag)) continue;   //already processed

        QListWidgetItem * item = new QListWidgetItem(ui->lwTags);
        QCheckBox * cb = new QCheckBox(tagRec.Tag);
        cb->setChecked(false);
        cb->setEnabled(false);
        ui->lwTags->setItemWidget(item, cb);
    }
}

void AMaterialLibraryBrowser::on_pbClearTags_clicked()
{
    for (ATagRecord & tagRec : TagRecords) tagRec.bChecked = false;
    updateGui();
}

void AMaterialLibraryBrowser::on_lwMaterials_itemDoubleClicked(QListWidgetItem * /*item*/)
{
    on_pbLoad_clicked();
}

void AMaterialLibraryBrowser::on_lwMaterials_itemClicked(QListWidgetItem * )
{
    int row = ui->lwMaterials->currentRow();
    if (row < 0 || row >= ShownMaterials.size())
    {
        message("Error: model corrupted", this);
        return;
    }
    const AMaterialLibraryRecord & Record = ShownMaterials.at(row);

    AMaterial mat;
    AMaterialParticleCollection Dummy;
    if (Dummy.countParticles() == 1) Dummy.RemoveParticle(0);
    QJsonObject json;
    LoadJsonFromFile(json, Record.FileName);
    QJsonObject js = json["Material"].toObject();
    mat.readFromJson(js, &Dummy);

    ui->pte->clear();
    out("Composition:");
    out(mat.ChemicalComposition.getCompositionString(), true);
    out("");
    out("Tags:");
    QString txt;
    for (auto & s : Record.Tags) txt.append(" " + s + ",");
    txt.chop(1);
    out(txt, true);
    out("");
    out("Defined particles:");
    out(Dummy.getListOfParticleNames().join(", "), true);
    out("");
    out("Comments:");
    QStringList sl = mat.Comments.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
    txt = ">" + sl.join("\n>");
    out(txt);
}

void AMaterialLibraryBrowser::on_lwMaterials_currentRowChanged(int currentRow)
{
    if (currentRow == -1) ui->pte->clear(); //selection cleared
}
