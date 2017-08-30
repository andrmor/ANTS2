#include "aneutronreactionsconfigurator.h"
#include "ui_aneutronreactionsconfigurator.h"
#include "aneutronreactionwidget.h"
#include "amessage.h"

#include <QKeyEvent>
#include <QDebug>

ANeutronReactionsConfigurator::ANeutronReactionsConfigurator(ANeutronInteractionElement *element, QStringList DefinedParticles, QWidget *parent) :
    QDialog(parent), ui(new Ui::ANeutronReactionsConfigurator), Element(element), DefinedParticles(DefinedParticles)
{
    ui->setupUi(this);
    ui->labTitle->setText( element->Name + "-" + QString::number(element->Mass) );
    ui->lwReactions->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->lwReactions->setSpacing(5);

    ui->pbDummy->setDefault(true);
    ui->pbDummy->setVisible(false);

    Element_LocalCopy = *Element;

    updateDecayScenarios();
}

ANeutronReactionsConfigurator::~ANeutronReactionsConfigurator()
{
    delete ui;
}

void ANeutronReactionsConfigurator::on_pbAdd_clicked()
{
    Element_LocalCopy.DecayScenarios << ( Element_LocalCopy.DecayScenarios.isEmpty() ? ADecayScenario(1) : ADecayScenario(0) );
    updateDecayScenarios();
}

void ANeutronReactionsConfigurator::on_pbRemove_clicked()
{
    int numReact = Element_LocalCopy.DecayScenarios.size();
    if (numReact<1) return;
    if (numReact==1)
    {
        Element_LocalCopy.DecayScenarios.clear();
        updateDecayScenarios();
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
                Element_LocalCopy.DecayScenarios.removeAt(i);
                updateDecayScenarios();
                return;
            }
        }
        qWarning() << "This item was not found!";
    }
}

void ANeutronReactionsConfigurator::on_pbConfirm_clicked()
{
    if (!Element_LocalCopy.DecayScenarios.isEmpty())
    {
        double totBranching = 0;
        for (int i=0; i<Element_LocalCopy.DecayScenarios.size(); i++)
            totBranching += Element_LocalCopy.DecayScenarios.at(i).Branching;
        if (totBranching != 1.0)
        {
            message("Branching sum of all reactions should be 100%", this);
            return;
        }
    }

    *Element = Element_LocalCopy;
    accept();
}

void ANeutronReactionsConfigurator::onResizeRequest()
{
    updateDecayScenarios();
}

void ANeutronReactionsConfigurator::updateDecayScenarios()
{
   ui->lwReactions->clear();

   for (int i=0; i<Element_LocalCopy.DecayScenarios.size(); i++)
   {
       ANeutronReactionWidget* del = new ANeutronReactionWidget(&Element_LocalCopy.DecayScenarios[i], DefinedParticles);
       QObject::connect(del, &ANeutronReactionWidget::RequestParentResize, this, &ANeutronReactionsConfigurator::onResizeRequest);
       QListWidgetItem* lwi = new QListWidgetItem;
       lwi->setSizeHint( del->sizeHint() );
       ui->lwReactions->addItem(lwi);
       ui->lwReactions->setItemWidget(lwi, del);
   }

   if (Element_LocalCopy.DecayScenarios.isEmpty())
   {
       QListWidgetItem* lwi = new QListWidgetItem("No decay scenarios are defined");
       lwi->setTextAlignment(Qt::AlignCenter);
       lwi->setTextColor(Qt::green);
       ui->lwReactions->addItem(lwi);
   }
}
