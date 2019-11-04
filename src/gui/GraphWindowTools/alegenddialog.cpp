#include "alegenddialog.h"
#include "ui_alegenddialog.h"
#include "adrawobject.h"

#include <QDebug>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "TLegend.h"
#include "TLegendEntry.h"
#include "TList.h"

ALegendDialog::ALegendDialog(TLegend & Legend, const QVector<ADrawObject> & DrawObjects, QWidget * parent) :
    QDialog(parent), ui(new Ui::ALegendDialog),
    Legend(Legend), DrawObjects(DrawObjects)
{
    ui->setupUi(this);

    updateMain();
    updateTree(nullptr);
}

ALegendDialog::~ALegendDialog()
{
    delete ui;
}

void ALegendDialog::updateMain()
{
    ui->leTitle->setText( Legend.GetHeader() );

    ui->lwList->clear();
    EntryObjects.clear();
    TList * elist = Legend.GetListOfPrimitives();
    int num = elist->GetEntries();
    for (int ie = 0; ie < num; ie++)
    {
        TLegendEntry * en = static_cast<TLegendEntry*>( (*elist).At(ie));
        TObject * obj = en->GetObject();
        QString opt   = en->GetOption();
        QString label = en->GetLabel();

        QString line;
        if (obj && isValidObject(obj))
        {
            line = QString("L:%2   %3").arg(opt).arg(label);
            EntryObjects << obj;
        }
        else
        {
            line = label;
            EntryObjects << nullptr;
        }

        ui->lwList->addItem(line);
    }
}

void ALegendDialog::updateTree(TObject * selectedObj)
{
    ui->twTree->clear();

    for (int i = 0; i < DrawObjects.size(); i++)
    {
        const ADrawObject & drObj = DrawObjects.at(i);
        TObject * tObj = drObj.Pointer;

        QTreeWidgetItem * item = new QTreeWidgetItem();
        QString name = drObj.Name;
        if (name.isEmpty()) name = "--";

        QString nameShort = name;
        if (nameShort.size() > 15)
        {
            nameShort.truncate(15);
            nameShort += "..";
        }

        QString className = tObj->ClassName();
        QString opt = drObj.Options;

        item->setText(0, QString("%1 %2").arg(nameShort).arg(className));
        item->setToolTip(0, QString("Name: %1\nClassName: %2\nDraw options: %3").arg(name).arg(className).arg(opt));
        item->setText(1, QString::number(i));

        if (!drObj.bEnabled) item->setDisabled(true);
        if (drObj.Pointer == selectedObj)
        {
            item->setBackground(0, QBrush(Qt::blue));
            item->setForeground(0, QBrush(Qt::white));
        }

        ui->twTree->addTopLevelItem(item);
    }
}

bool ALegendDialog::isValidObject(TObject *obj)
{
    for (int i=0; i<DrawObjects.size(); i++)
        if (DrawObjects.at(i).Pointer == obj) return true;
    return false;
}

void ALegendDialog::on_lwList_currentRowChanged(int currentRow)
{
    qDebug() << currentRow;
    TObject * selection = EntryObjects.at(currentRow);
    updateTree(selection);
}

void ALegendDialog::on_pbCancel_clicked()
{
    reject();
}
