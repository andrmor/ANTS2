#include "alegenddialog.h"
#include "ui_alegenddialog.h"
#include "adrawobject.h"
#include "abasketlistwidget.h"

#include <QDebug>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "TLegendEntry.h"
#include "TList.h"

ALegendDialog::ALegendDialog(TLegend & Legend, const QVector<ADrawObject> & DrawObjects, QWidget * parent) :
    QDialog(parent), ui(new Ui::ALegendDialog),
    Legend(Legend), OriginalCopy(Legend), DrawObjects(DrawObjects)
{
    ui->setupUi(this);

    lwList = new ABasketListWidget(this);
    ui->splitter->insertWidget(0, lwList);
    connect(lwList, &ABasketListWidget::requestReorder, this, &ALegendDialog::onReorderEntriesRequested);
    connect(lwList, &ABasketListWidget::currentRowChanged, this, &ALegendDialog::onCurrentEntryChanged);

    connect(lwList->itemDelegate(), &QAbstractItemDelegate::commitData, this, &ALegendDialog::onLabelTextChanged);

    updateModel(Legend);

    updateList();
    updateTree();
}

ALegendDialog::~ALegendDialog()
{
    delete ui;
}

void ALegendDialog::onLabelTextChanged()
{
    int row = lwList->currentRow();
    //qDebug() << "label change:"<<row << lwList->item(row)->text();
    if (row >= 0) Model[row].Label = lwList->item(row)->text();

    updateLegend();
}

void ALegendDialog::updateModel(TLegend & legend)
{
    TList * elist = legend.GetListOfPrimitives();
    int num = elist->GetEntries();
    for (int ie = 0; ie < num; ie++)
    {
        TLegendEntry * en = static_cast<TLegendEntry*>( (*elist).At(ie));
        Model << ALegendModelRecord(en->GetLabel(), en->GetObject());
    }
}

void ALegendDialog::updateList()
{
    ui->leTitle->setText( Legend.GetHeader() );

    lwList->clear();

    TList * elist = Legend.GetListOfPrimitives();
    const int num = elist->GetEntries();
    for (int ie = 0; ie < num; ie++)
    {
        TLegendEntry * en = static_cast<TLegendEntry*>( (*elist).At(ie));

        //TObject * obj = en->GetObject();
        //QString opt   = en->GetOption();
        QString label = en->GetLabel();

        QListWidgetItem * item = new QListWidgetItem(label);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        lwList->addItem(item);
    }
}

void ALegendDialog::updateTree()
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
        if (drObj.Pointer == SelectedObject)
        {
            item->setBackground(0, QBrush(Qt::blue));
            item->setForeground(0, QBrush(Qt::white));
        }

        ui->twTree->addTopLevelItem(item);
    }
}

void ALegendDialog::updateLegend()
{
    Legend.Clear();

    QString title = ui->leTitle->text();
    if (!title.isEmpty()) Legend.SetHeader(title.toLatin1(), (ui->cbCentered->isChecked() ? "C" : "") );

    for (const ALegendModelRecord & rec : Model)
    {
        Legend.AddEntry(rec.TObj, rec.Label.toLatin1());
    }

    emit requestCanvasUpdate();
}

void ALegendDialog::onCurrentEntryChanged(int currentRow)
{
    //qDebug() << currentRow;
    SelectedObject = nullptr;
    if (currentRow >= 0) SelectedObject = Model.at(currentRow).TObj;
    updateTree();
}

void ALegendDialog::on_pbCancel_clicked()
{
    //Legend = OriginalCopy;
    reject();
}

void ALegendDialog::on_pbAccept_clicked()
{
    updateLegend();
    accept();
}

void ALegendDialog::onReorderEntriesRequested(const QVector<int> &indexes, int toRow)
{
    qDebug() << indexes << toRow;
}
