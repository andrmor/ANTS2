#include "alegenddialog.h"
#include "ui_alegenddialog.h"
#include "adrawobject.h"
#include "abasketlistwidget.h"
#include "amessage.h"
#include "aroottextconfigurator.h"

#include <QDebug>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QKeyEvent>

#include "TLegendEntry.h"
#include "TList.h"

#include <QVBoxLayout>

ALegendDialog::ALegendDialog(TLegend & Legend, const QVector<ADrawObject> & DrawObjects, QWidget * parent) :
    QDialog(parent), ui(new Ui::ALegendDialog),
    Legend(Legend), DrawObjects(DrawObjects)
{
    ui->setupUi(this);

    ui->pbDummy->setDefault(true);
    ui->pbDummy->setVisible(false);

    lwList = new ABasketListWidget(this);
    lwList->setContextMenuPolicy(Qt::CustomContextMenu);
    lwList->setSpacing(4);
    lwList->setDropIndicatorShown(true);
    //lwList->setAutoFillBackground(false);
    //lwList->setStyleSheet("QListWidget{ background: lightGray; }" );
    ui->splitter->insertWidget(0, lwList);
    connect(lwList, &ABasketListWidget::requestReorder, this, &ALegendDialog::onReorderEntriesRequested);
    //connect(lwList, &ABasketListWidget::currentRowChanged, this, &ALegendDialog::onCurrentEntryChanged);
    connect(lwList, &ABasketListWidget::itemSelectionChanged, this, &ALegendDialog::onEntrySelectionChanged);
    connect(lwList, &ABasketListWidget::customContextMenuRequested, this, &ALegendDialog::onListMenuRequested);
    //connect(lwList->itemDelegate(), &QAbstractItemDelegate::commitData, this, &ALegendDialog::onLabelTextChanged);

    ui->splitter->setSizes( {500,200});

    updateModel(Legend);
    OriginalModel = CurrentModel;

    updateList();
    updateTree();

    connect(qApp, &QApplication::focusChanged, this, &ALegendDialog::onFocusChanged);
}

ALegendDialog::~ALegendDialog()
{
    delete ui;
}

void ALegendDialog::updateModel(TLegend & legend)
{
    CurrentModel.NumColumns = legend.GetNColumns();

    CurrentModel.DefaultTextColor = Legend.GetTextColor();
    CurrentModel.DefaultTextAlign = Legend.GetTextAlign();
    CurrentModel.DefaultTextFont  = Legend.GetTextFont();
    CurrentModel.DefaultTextSize  = Legend.GetTextSize();

    CurrentModel.Xfrom = Legend.GetX1NDC();
    CurrentModel.Xto   = Legend.GetX2NDC();
    CurrentModel.Yfrom = Legend.GetY1NDC();
    CurrentModel.Yto   = Legend.GetY2NDC();

    TList * elist = legend.GetListOfPrimitives();
    int num = elist->GetEntries();
    for (int ie = 0; ie < num; ie++)
    {
        TLegendEntry * en = static_cast<TLegendEntry*>( (*elist).At(ie));
        CurrentModel.Model << ALegendEntryRecord(en->GetLabel(), en->GetObject(), en->GetOption());
        if (en->GetTextColor()!=CurrentModel.DefaultTextColor ||
                en->GetTextAlign()!=CurrentModel.DefaultTextAlign ||
                en->GetTextFont()!=CurrentModel.DefaultTextFont ||
                en->GetTextSize()!=CurrentModel.DefaultTextSize)
        {
            ALegendEntryRecord & m = CurrentModel.Model.last();
            m.bAttributeOverride = true;
            m.TextColor = en->GetTextColor();
            m.TextAlign = en->GetTextAlign();
            m.TextFont  = en->GetTextFont();
            m.TextSize  = en->GetTextSize();
        }
    }

    //Legend.GetX1()

}

void ALegendDialog::updateList()
{
    lwList->clear();

    for (int iModel = 0; iModel < CurrentModel.Model.size(); iModel++)
    {
        const ALegendEntryRecord & rec = CurrentModel.Model.at(iModel);

        QListWidgetItem * item = new QListWidgetItem();
        item->setBackgroundColor(QColor(230,230,230));
        lwList->addItem(item);
        ALegendEntryDelegate * ed = new ALegendEntryDelegate(rec, iModel);
        connect(ed, &ALegendEntryDelegate::contentWasEdited, this, &ALegendDialog::onEntryWasEdited);
        //connect(ed, &ALegendEntryDelegate::activated, this, &ALegendDialog::onActive);
        lwList->setItemWidget(item, ed);
        item->setSizeHint(ed->sizeHint());
    }

    ui->sbNumColumns->setValue(CurrentModel.NumColumns);
    ui->ledXfrom->setText( QString::number(CurrentModel.Xfrom) );
    ui->ledXto->setText  ( QString::number(CurrentModel.Xto) );
    ui->ledYfrom->setText( QString::number(CurrentModel.Yfrom) );
    ui->ledYto->setText  ( QString::number(CurrentModel.Yto) );
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

    Legend.SetTextColor(CurrentModel.DefaultTextColor);
    Legend.SetTextAlign(CurrentModel.DefaultTextAlign);
    Legend.SetTextFont (CurrentModel.DefaultTextFont);
    Legend.SetTextSize (CurrentModel.DefaultTextSize);

    Legend.SetX1NDC(CurrentModel.Xfrom);
    Legend.SetX2NDC(CurrentModel.Xto);
    Legend.SetY1NDC(CurrentModel.Yfrom);
    Legend.SetY2NDC(CurrentModel.Yto);

    for (const ALegendEntryRecord & rec : CurrentModel.Model)
    {
        TLegendEntry *le = Legend.AddEntry(rec.TObj, rec.Label.toLatin1().data(), rec.Options.toLatin1().data());
        if (rec.bAttributeOverride)
        {
            le->SetTextColor(rec.TextColor);
            le->SetTextAlign(rec.TextAlign);
            le->SetTextFont (rec.TextFont);
            le->SetTextSize (rec.TextSize);
        }
    }

    Legend.SetNColumns(CurrentModel.NumColumns);

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
        CurrentModel.Model.remove(indexes.at(i));

    updateList();
    updateLegend();
}

void ALegendDialog::onEntrySelectionChanged()
{
    //qDebug() << currentRow;
    SelectedObject = nullptr;

    int selSize = lwList->selectedItems().size();
    if (selSize == 1)
        SelectedObject = CurrentModel.Model.at(lwList->currentRow()).TObj;

    ui->pbThisEntryTextAttributes->setEnabled(selSize != 0);

    updateTree();
}

void ALegendDialog::onFocusChanged(QWidget *, QWidget * newW)
{
    //qDebug() << old << now;
    if (newW && newW->parent())
    {
        for (int i=0; i<lwList->count(); i++)
        {
            QWidget * w = lwList->itemWidget(lwList->item(i));
            if (newW->parent() == w)
            {
                //onCurrentEntryChanged(i);
                lwList->setCurrentRow(i);
            }
        }
    }
}

void ALegendDialog::on_pbCancel_clicked()
{
    CurrentModel = OriginalModel;
    updateLegend();

    reject();
}

void ALegendDialog::on_pbAccept_clicked()
{
    updateLegend();

    accept();
}

void ALegendDialog::onEntryWasEdited(int index, const QString &label, bool line, bool mark, bool fill)
{
    if (index >= 0 && index < CurrentModel.Model.size())
    {
        ALegendEntryRecord & rec = CurrentModel.Model[index];
        rec.Label = label;
        rec.Options.clear();
        if (line) rec.Options.append('l');
        if (mark) rec.Options.append('p');
        if (fill) rec.Options.append('f');

        lwList->setCurrentRow(index);
    }
    updateLegend();
}

void ALegendDialog::onReorderEntriesRequested(const QVector<int> &indexes, int toRow)
{
    QVector< ALegendEntryRecord > ItemsToMove;
    for (int i = 0; i < indexes.size(); i++)
    {
        const int index = indexes.at(i);
        ItemsToMove << CurrentModel.Model.at(index);
        CurrentModel.Model[index]._flag = true;       // mark to be deleted
    }

    for (int i = 0; i < ItemsToMove.size(); i++)
    {
        CurrentModel.Model.insert(toRow, ItemsToMove.at(i));
        toRow++;
    }

    for (int i = CurrentModel.Model.size()-1; i >= 0; i--)
        if (CurrentModel.Model.at(i)._flag)
            CurrentModel.Model.remove(i);

    updateList();
    updateLegend();
}

void ALegendDialog::onListMenuRequested(const QPoint &pos)
{
    if (lwList->selectedItems().size() > 1)
    {
        QMenu Menu;
        QAction * removeAllSelected = Menu.addAction("Remove all selected entries");
        removeAllSelected->setShortcut(Qt::Key_Delete);

        QAction* selectedItem = Menu.exec(lwList->mapToGlobal(pos));
        if (!selectedItem) return;

        if (selectedItem == removeAllSelected)
            removeAllSelectedEntries();
    }
}

void ALegendDialog::clearLegend()
{
    bool bConfirm = confirm("Remove all entries?", this);
    if (!bConfirm) return;

    CurrentModel.Model.clear();
    updateList();
    updateLegend();
}

void ALegendDialog::deleteSelectedEntry()
{
    int row = lwList->currentRow();
    if (row == -1) return;

    CurrentModel.Model.remove(row);
    updateList();
}

void ALegendDialog::addText()
{
    CurrentModel.Model << ALegendEntryRecord("Text", nullptr, "");
    updateList();
    updateLegend();
}

void ALegendDialog::on_twTree_itemDoubleClicked(QTreeWidgetItem *item, int)
{
    int index = item->text(1).toInt();
    if (index < 0 || index >= DrawObjects.size())
    {
        message("Bad index - the tree widget is corrupted", this);
        return;
    }

    CurrentModel.Model << ALegendEntryRecord(DrawObjects[index].Name, DrawObjects[index].Pointer, "lpf");
    updateList();
    updateLegend();
}

void ALegendDialog::on_sbNumColumns_editingFinished()
{
    CurrentModel.NumColumns = ui->sbNumColumns->value();
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

#include <QLineEdit>
#include <QCheckBox>
ALegendEntryDelegate::ALegendEntryDelegate(const ALegendEntryRecord & record, int index) : QFrame(), Index(index)
{
    setFrameShape(QFrame::StyledPanel);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    QHBoxLayout * lh = new QHBoxLayout(this);
    lh->setContentsMargins(4, 1, 4, 1);

    QFrame * fr1 = new QFrame();
    fr1->setFrameShape(QFrame::NoFrame);
    QGridLayout * gl = new QGridLayout(fr1);
    gl->setContentsMargins(0,0,0,0);
    gl->setHorizontalSpacing(0);
    gl->setVerticalSpacing(0);
        cbLine = new QCheckBox("Line");
        cbLine->setChecked(record.Options.contains('l', Qt::CaseInsensitive));
        cbLine->setEnabled(record.TObj);
        if (record.TObj) cbLine->setChecked(record.Options.contains('l', Qt::CaseInsensitive));
        connect(cbLine, &QCheckBox::clicked, this, &ALegendEntryDelegate::onContentChanged);
    gl->addWidget(cbLine, 1, 1);
        cbMarker = new QCheckBox("Mark");
        cbMarker->setChecked(record.Options.contains('p', Qt::CaseInsensitive));
        cbMarker->setEnabled(record.TObj);
        if (record.TObj) cbMarker->setChecked(record.Options.contains('p', Qt::CaseInsensitive));
        connect(cbMarker, &QCheckBox::clicked, this, &ALegendEntryDelegate::onContentChanged);
    gl->addWidget(cbMarker, 2, 1);
        cbFill = new QCheckBox("Fill");
        cbFill->setChecked(record.Options.contains('f', Qt::CaseInsensitive));
        cbFill->setEnabled(record.TObj);
        if (record.TObj) cbFill->setChecked(record.Options.contains('f', Qt::CaseInsensitive));
        connect(cbFill, &QCheckBox::clicked, this, &ALegendEntryDelegate::onContentChanged);
    gl->addWidget(cbFill, 2, 2);
    lh->addWidget(fr1);

        QFrame * fr2 = new QFrame();
        fr2->setFrameShape(QFrame::VLine);
        fr2->setFrameShadow(QFrame::Raised);
    lh->addWidget(fr2);

    le = new QLineEdit(record.Label, this);
    le->setContextMenuPolicy(Qt::NoContextMenu);
    connect(le, &QLineEdit::editingFinished, this, &ALegendEntryDelegate::onContentChanged);
    lh->addWidget(le);
}

void ALegendEntryDelegate::onContentChanged()
{
    emit contentWasEdited(Index, le->text(), cbLine->isChecked(), cbMarker->isChecked(), cbFill->isChecked());
}

void ALegendEntryDelegate::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Enter)
    {
        onContentChanged();
        event->ignore();
    }
}

void ALegendDialog::on_pbAddEntry_clicked()
{
    addText();
}

void ALegendDialog::on_pbRemoveSelected_clicked()
{
    removeAllSelectedEntries();
}

void ALegendDialog::on_pbRemoveAll_clicked()
{
    clearLegend();
}

void ALegendDialog::on_pbDefaultTextProperties_clicked()
{
    int   color = CurrentModel.DefaultTextColor;
    int   align = CurrentModel.DefaultTextAlign;
    int   font  = CurrentModel.DefaultTextFont;
    float size  = CurrentModel.DefaultTextSize;

    ARootTextConfigurator D(color, align, font, size, this);
    int res = D.exec();
    if (res == QDialog::Accepted)
    {
        CurrentModel.DefaultTextColor = color;
        CurrentModel.DefaultTextAlign = align;
        CurrentModel.DefaultTextFont  = font;
        CurrentModel.DefaultTextSize  = size;
        updateLegend();
    }
}

void ALegendDialog::on_pbThisEntryTextAttributes_clicked()
{
    int selSize = lwList->selectedItems().size();
    if (selSize == 0) return;

    int index = lwList->row(lwList->selectedItems().first());
    ALegendEntryRecord & rec = CurrentModel.Model[index];
    int   color = ( rec.bAttributeOverride ? rec.TextColor : CurrentModel.DefaultTextColor );
    int   align = ( rec.bAttributeOverride ? rec.TextAlign : CurrentModel.DefaultTextAlign );
    int   font  = ( rec.bAttributeOverride ? rec.TextFont  : CurrentModel.DefaultTextFont );
    float size  = ( rec.bAttributeOverride ? rec.TextSize  : CurrentModel.DefaultTextSize );

    ARootTextConfigurator D(color, align, font, size, this);
    int res = D.exec();
    if (res == QDialog::Accepted)
    {
        for (QListWidgetItem * item : lwList->selectedItems())
        {
            int index = lwList->row(item);
            ALegendEntryRecord & rec = CurrentModel.Model[index];
            rec.TextColor = color;
            rec.TextAlign = align;
            rec.TextFont  = font;
            rec.TextSize  = size;
        }
        updateLegend();
    }
}

void ALegendDialog::on_ledXfrom_editingFinished()
{
    CurrentModel.Xfrom = ui->ledXfrom->text().toDouble();
    updateLegend();
}

void ALegendDialog::on_ledXto_editingFinished()
{
    CurrentModel.Xto = ui->ledXto->text().toDouble();
    updateLegend();
}

void ALegendDialog::on_ledYfrom_editingFinished()
{
    CurrentModel.Yfrom = ui->ledYfrom->text().toDouble();
    updateLegend();
}

void ALegendDialog::on_ledYto_editingFinished()
{
    CurrentModel.Yto = ui->ledYto->text().toDouble();
    updateLegend();
}
