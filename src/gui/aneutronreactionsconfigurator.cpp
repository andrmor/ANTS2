#include "aneutronreactionsconfigurator.h"
#include "ui_aneutronreactionsconfigurator.h"
#include "aneutronreactionwidget.h"
#include "amessage.h"

#include <QKeyEvent>
#include <QDebug>

ANeutronReactionsConfigurator::ANeutronReactionsConfigurator(AAbsorptionElement *element, QStringList DefinedParticles, QWidget *parent) :
    QDialog(parent), ui(new Ui::ANeutronReactionsConfigurator), Element(element), DefinedParticles(DefinedParticles)
{
    ui->setupUi(this);
    ui->labTitle->setText( element->Name + "-" + QString::number(element->Mass) );
    ui->lwReactions->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->lwReactions->setSpacing(5);

    ui->pbDummy->setDefault(true);
    ui->pbDummy->setVisible(false);

    Element_LocalCopy = *Element;

    updateReactions();
}

ANeutronReactionsConfigurator::~ANeutronReactionsConfigurator()
{
    delete ui;
}

void ANeutronReactionsConfigurator::on_pbAdd_clicked()
{
    Element_LocalCopy.Reactions << ACaptureReaction(0);
    updateReactions();
}

void ANeutronReactionsConfigurator::on_pbRemove_clicked()
{
    int numReact = Element_LocalCopy.Reactions.size();
    if (numReact<1) return;
    if (numReact==1)
    {
        Element_LocalCopy.Reactions.clear();
        updateReactions();
        return;
    }
    else
    {
        QList<QListWidgetItem*> list = ui->lwReactions->selectedItems();
        if (list.isEmpty())
        {
            message("Select reaction to remove", this);
            return;
        }
        QListWidgetItem* thisItem = list.first();
        for (int i = 0; i<ui->lwReactions->count(); i++)
        {
            if (ui->lwReactions->item(i) == thisItem)
            {
                Element_LocalCopy.Reactions.removeAt(i);
                updateReactions();
                return;
            }
        }
        qWarning() << "This item was not found!";
    }
}

void ANeutronReactionsConfigurator::on_pbConfirm_clicked()
{
    *Element = Element_LocalCopy;
    accept();
}

void ANeutronReactionsConfigurator::onResizeRequest()
{
    updateReactions();
}

void ANeutronReactionsConfigurator::updateReactions()
{
   ui->lwReactions->clear();

   for (int i=0; i<Element_LocalCopy.Reactions.size(); i++)
   {
       ANeutronReactionWidget* del = new ANeutronReactionWidget(&Element_LocalCopy.Reactions[i], DefinedParticles);
       QObject::connect(del, &ANeutronReactionWidget::RequestParentResize, this, &ANeutronReactionsConfigurator::onResizeRequest);
       QListWidgetItem* lwi = new QListWidgetItem;
       lwi->setSizeHint( del->sizeHint() );
       ui->lwReactions->addItem(lwi);
       ui->lwReactions->setItemWidget(lwi, del);
   }
}
