#include "alegenddialog.h"
#include "ui_alegenddialog.h"
#include "adrawobject.h"
#include "abasketlistwidget.h"
#include "amessage.h"

#include <QDebug>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMenu>

#include "TLegendEntry.h"
#include "TList.h"

ALegendDialog::ALegendDialog(TLegend & Legend, const QVector<ADrawObject> & DrawObjects, QWidget * parent) :
    QDialog(parent), ui(new Ui::ALegendDialog),
    Legend(Legend), OriginalCopy(Legend), DrawObjects(DrawObjects)
{
    ui->setupUi(this);

    lwList = new ABasketListWidget(this);
    lwList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->splitter->insertWidget(0, lwList);
    connect(lwList, &ABasketListWidget::requestReorder, this, &ALegendDialog::onReorderEntriesRequested);
    connect(lwList, &ABasketListWidget::currentRowChanged, this, &ALegendDialog::onCurrentEntryChanged);
    connect(lwList, &ABasketListWidget::customContextMenuRequested, this, &ALegendDialog::onListMenuRequested);
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
        qDebug() << ie << en->GetOption();
        Model << ALegendModelRecord(en->GetLabel(), en->GetObject(), en->GetOption());
    }

    NumColumns = legend.GetNColumns();
}

void ALegendDialog::updateList()
{
    lwList->clear();

    for (const ALegendModelRecord & rec : Model)
    {
        QListWidgetItem * item = new QListWidgetItem(rec.Label);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        lwList->addItem(item);
    }

    ui->sbNumColumns->setValue(NumColumns);
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

    for (const ALegendModelRecord & rec : Model)
    {
        Legend.AddEntry(rec.TObj, rec.Label.toLatin1().data(), rec.Options.toLatin1().data());
    }

    Legend.SetNColumns(NumColumns);

    emit requestCanvasUpdate();
}

void ALegendDialog::removeAllSelectedEntries()
{
    QList<QListWidgetItem*> selection = lwList->selectedItems();
    const int size = selection.size();
    if (size == 0) return;

    bool bConfirm = true;
    if (size > 1)
         bConfirm = confirm(QString("Remove %1 selected entr%2?").arg(size).arg(size == 1 ? "y" : "is"), this);
    if (!bConfirm) return;

    QVector<int> indexes;
    for (QListWidgetItem * item : selection)
        indexes << lwList->row(item);
    std::sort(indexes.begin(), indexes.end());
    for (int i = indexes.size() - 1; i >= 0; i--)
        Model.remove(indexes.at(i));

    updateList();
    updateLegend();
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
    QVector< ALegendModelRecord > ItemsToMove;
    for (int i = 0; i < indexes.size(); i++)
    {
        const int index = indexes.at(i);
        ItemsToMove << Model.at(index);
        Model[index]._flag = true;       // mark to be deleted
    }

    for (int i = 0; i < ItemsToMove.size(); i++)
    {
        Model.insert(toRow, ItemsToMove.at(i));
        toRow++;
    }

    for (int i = Model.size()-1; i >= 0; i--)
        if (Model.at(i)._flag)
            Model.remove(i);

    updateList();
    updateLegend();
}

void ALegendDialog::onListMenuRequested(const QPoint &pos)
{
    if (lwList->selectedItems().size() > 1)
    {
        onListMenuMultipleSelection(pos);
        return;
    }

    QMenu Menu;

    //int row = -1; if (temp) row = lwBasket->row(temp);
    QListWidgetItem* temp = lwList->itemAt(pos);

    QAction* addTextA  = Menu.addAction("Add text line");
    Menu.addSeparator();
    QAction* delA      = Menu.addAction("Delete"); delA->setEnabled(temp); delA->setShortcut(Qt::Key_Delete);
    Menu.addSeparator();
    QAction* clearA = Menu.addAction("Clear legend");

    //------

    QAction* selectedItem = Menu.exec(lwList->mapToGlobal(pos));
    if (!selectedItem) return; //nothing was selected

    if      (selectedItem == addTextA)  addText();
    else if (selectedItem == delA)      deleteSelectedEntry();
    else if (selectedItem == clearA)    clearLegend();
}

void ALegendDialog::clearLegend()
{
    bool bConfirm = confirm("Clear legend entries?", this);
    if (!bConfirm) return;

    Model.clear();
    updateList();
    updateLegend();
}

void ALegendDialog::deleteSelectedEntry()
{
    int row = lwList->currentRow();
    if (row == -1) return;

    Model.remove(row);
    updateList();
}

void ALegendDialog::addText()
{
    Model << ALegendModelRecord("Text", nullptr, "");
    updateList();
    updateLegend();
}

void ALegendDialog::onListMenuMultipleSelection(const QPoint &pos)
{
    QMenu Menu;
    QAction * removeAllSelected = Menu.addAction("Remove all selected entries");
    removeAllSelected->setShortcut(Qt::Key_Delete);

    QAction* selectedItem = Menu.exec(lwList->mapToGlobal(pos));
    if (!selectedItem) return;

    if (selectedItem == removeAllSelected)
        removeAllSelectedEntries();
}

void ALegendDialog::on_twTree_itemDoubleClicked(QTreeWidgetItem *item, int)
{
    int index = item->text(1).toInt();
    if (index < 0 || index >= DrawObjects.size())
    {
        message("Bad index - the tree widget is corrupted", this);
        return;
    }

    Model << ALegendModelRecord(DrawObjects[index].Name, DrawObjects[index].Pointer, "lpf");
    updateList();
    updateLegend();
}

void ALegendDialog::on_sbNumColumns_editingFinished()
{
    NumColumns = ui->sbNumColumns->value();
    updateLegend();
}

#include "arootlineconfigurator.h"
void ALegendDialog::on_pbConfigureFrame_clicked()
{
    int color = Legend.GetLineColor();
    int width = Legend.GetLineWidth();
    int style = Legend.GetLineStyle();
    ARootLineConfigurator RC(&color, &width, &style, this);
    int res = RC.exec();
    if (res == QDialog::Accepted)
    {
        Legend.SetLineColor(color);
        Legend.SetLineWidth(width);
        Legend.SetLineStyle(style);
        updateLegend();
    }
}
