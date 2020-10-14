#include "ageotreewidget.h"
#include "ageobasedelegate.h"
#include "ageoobjectdelegate.h"
#include "amonitordelegate.h"
#include "agridelementdelegate.h"
#include "ageoslabdelegate.h"
#include "ageoobject.h"
#include "ageoshape.h"
#include "atypegeoobject.h"
#include "ashapehelpdialog.h"
#include "arootlineconfigurator.h"
#include "aslablistwidget.h"
#include "slabdelegate.h"
#include "aslab.h"
#include "asandwich.h"
#include "agridelementdialog.h"
#include "amonitordelegateform.h"
#include "amessage.h"
#include "ageoconsts.h"

#include <QDebug>
#include <QDropEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QApplication>
#include <QPainter>
#include <QClipboard>
#include <QShortcut>
#include <QStringList>

#include "TMath.h"
#include "TGeoShape.h"

AGeoTreeWidget::AGeoTreeWidget(ASandwich *Sandwich)
    : Sandwich(Sandwich), World(Sandwich->World), Prototypes(Sandwich->Prototypes)
{
    loadImages();

    // main tree widget
    setHeaderHidden(true);
    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDropIndicatorShown(false);
    //setIndentation(20);
    setContentsMargins(0, 0, 0, 0);
    setFrameStyle(QFrame::NoFrame);
    setIconSize(QSize(20, 20));
    configureStyle(this);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &AGeoTreeWidget::customContextMenuRequested, this, &AGeoTreeWidget::customMenuRequested);
    connect(this, &AGeoTreeWidget::itemSelectionChanged,       this, &AGeoTreeWidget::onItemSelectionChanged);
    connect(this, &AGeoTreeWidget::itemExpanded,               this, &AGeoTreeWidget::onItemExpanded);
    connect(this, &AGeoTreeWidget::itemCollapsed,              this, &AGeoTreeWidget::onItemCollapsed);
    connect(this, &AGeoTreeWidget::itemClicked,                this, &AGeoTreeWidget::onItemClicked);

    // widget for delegates
    EditWidget = new AGeoWidget(Sandwich, this);
    connect(EditWidget, &AGeoWidget::showMonitor,                this, &AGeoTreeWidget::RequestShowMonitor);
    connect(EditWidget, &AGeoWidget::requestBuildScript,         this, &AGeoTreeWidget::objectToScript);
    connect(this,       &AGeoTreeWidget::ObjectSelectionChanged, EditWidget, &AGeoWidget::onObjectSelectionChanged);

    // tree for prototypes
    createPrototypeTreeWidget();
    configureStyle(twPrototypes);

    // shortcuts
    QShortcut * Del = new QShortcut(Qt::Key_Backspace, this);
    connect(Del, &QShortcut::activated, this, &AGeoTreeWidget::onRemoveTriggered);
    QShortcut * DelRec = new QShortcut(QKeySequence(QKeySequence::Delete), this);
    connect(DelRec, &QShortcut::activated, this, &AGeoTreeWidget::onRemoveRecursiveTriggered);
}

void AGeoTreeWidget::createPrototypeTreeWidget()
{
    twPrototypes = new QTreeWidget();

    twPrototypes->setHeaderHidden(true);
    //twPrototypes->setAcceptDrops(true);
    //twPrototypes->setDragEnabled(true);
    //twPrototypes->setDragDropMode(QAbstractItemView::InternalMove);
    twPrototypes->setSelectionMode(QAbstractItemView::ExtendedSelection);
    //twPrototypes->setDropIndicatorShown(false);
    twPrototypes->setContentsMargins(0,0,0,0);
    twPrototypes->setFrameStyle(QFrame::NoFrame);
    twPrototypes->setIconSize(QSize(20,20));
    twPrototypes->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(twPrototypes, &QTreeWidget::customContextMenuRequested,     this,       &AGeoTreeWidget::customProtoMenuRequested);
    connect(twPrototypes, &QTreeWidget::itemExpanded,                   this,       &AGeoTreeWidget::onPrototypeItemExpanded);
    connect(twPrototypes, &QTreeWidget::itemCollapsed,                  this,       &AGeoTreeWidget::onPrototypeItemCollapsed);
    connect(twPrototypes, &QTreeWidget::itemSelectionChanged,           this,       &AGeoTreeWidget::onProtoItemSelectionChanged);
    connect(twPrototypes, &QTreeWidget::itemClicked,                    this,       &AGeoTreeWidget::onProtoItemClicked);
    connect(this,         &AGeoTreeWidget::ProtoObjectSelectionChanged, EditWidget, &AGeoWidget::onObjectSelectionChanged);
}

void AGeoTreeWidget::loadImages()
{
    QString dir = ":/images/";

    Lock.load(dir+"lock.png");
    //GroupStart.load(dir+"TopGr.png");
    //GroupMid.load(dir+"MidGr.png");
    //GroupEnd.load(dir+"BotGr.png");
    StackStart.load(dir+"TopSt.png");
    StackMid.load(dir+"MidSt.png");
    StackEnd.load(dir+"BotSt.png");
}

void AGeoTreeWidget::SelectObjects(QStringList ObjectNames)
{
   clearSelection();
   //qDebug() << "Request select the following objects:"<<ObjectNames;

   for (int i=0; i<ObjectNames.size(); i++)
     {
       QList<QTreeWidgetItem*> list = findItems(ObjectNames.at(i), Qt::MatchExactly | Qt::MatchRecursive);
       if (!list.isEmpty())
         {
            //qDebug() << "Attempting to focus:"<<list.first()->text(0);
            list.first()->setSelected(true);
         }
     }
}

void AGeoTreeWidget::UpdateGui(QString ObjectName)
{
    if (!World) return;

    //qDebug() << "==> Update tree triggered, ObjectName = "<<ObjectName;

    EditWidget->ClearGui();

    blockSignals(true);
    clear();
    blockSignals(false);

    twPrototypes->blockSignals(true);
    twPrototypes->clear();
    twPrototypes->blockSignals(false);

    //World
    QTreeWidgetItem * topItem = new QTreeWidgetItem(this);
    topItem->setText(0, "World");
    QFont f = topItem->font(0);
    f.setBold(true);
    topItem->setFont(0, f);
    topItem->setSizeHint(0, QSize(50, 20));
    topItem->setFlags(topItem->flags() & ~Qt::ItemIsDragEnabled);// & ~Qt::ItemIsSelectable);
    //w->setBackgroundColor(0, BackgroundColor);

    populateTreeWidget(topItem, World);
    updateExpandState(topItem, false);
    updatePrototypeTreeGui();


    // restoring delegate for the last shown obeject if possible, otherwise showing delegate for the World
    if (ObjectName.isEmpty())
    {
        if (LastShownObjectName.isEmpty())
        {
            ObjectName = "World";
            LastShownObjectName = "World";
        }
        else ObjectName = LastShownObjectName;
    }
    QList<QTreeWidgetItem*> list = twPrototypes->findItems(ObjectName, Qt::MatchExactly | Qt::MatchRecursive);
    if (list.isEmpty())
    {
        bWorldTreeSelected = true;
        list = findItems(ObjectName, Qt::MatchExactly | Qt::MatchRecursive);
    }
    else bWorldTreeSelected = false;

    if (list.isEmpty())
    {
        if (topLevelItemCount() > 0) setCurrentItem(topLevelItem(0));
    }
    else
    {
        //list.first()->setSelected(true);
        setCurrentItem(list.first());
    }
    //qDebug() << "<==";
}

void AGeoTreeWidget::updatePrototypeTreeGui()
{
    if (!Prototypes) return;

    topItemPrototypes = new QTreeWidgetItem(twPrototypes);
    topItemPrototypes->setText(0, "Defined prototypes:");
    QFont f = topItemPrototypes->font(0); f.setBold(true); topItemPrototypes->setFont(0, f);
    topItemPrototypes->setSizeHint(0, QSize(50, 20));
    topItemPrototypes->setFlags(topItemPrototypes->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsSelectable);
    Prototypes->fExpanded = true;  // force-expand top!

    populateTreeWidget(topItemPrototypes, Prototypes);
    updateExpandState(topItemPrototypes, true);
}

void AGeoTreeWidget::onGridReshapeRequested(QString objName)
{
    AGeoObject* obj = World->findObjectByName(objName);
    if (!obj) return;
    if (!obj->ObjectType->isGrid()) return;    
    if (!obj->getGridElement()) return;
    ATypeGridElementObject* GE = static_cast<ATypeGridElementObject*>(obj->getGridElement()->ObjectType);

    AGridElementDialog* d = new AGridElementDialog(Sandwich->Materials, this);
    switch (GE->shape)
     {
      case 0: d->setValues(0, GE->size1, GE->size2, obj->getGridElement()->Shape->getHeight()-0.001); break;
      case 1: d->setValues(1, GE->size1, GE->size2, obj->getGridElement()->Shape->getHeight()-0.001); break;
      case 2:
      {
        AGeoPgon* pg = dynamic_cast<AGeoPgon*>(obj->getGridElement()->Shape);
        if (pg)
          d->setValues(2, GE->size1, GE->size2, obj->getGridElement()->Shape->getHeight()-0.001);
        break;
      }
    }

    //setting materials
    d->setBulkMaterial(obj->Material);
    if (!obj->HostedObjects.isEmpty())
        if (!obj->HostedObjects.first()->HostedObjects.isEmpty())
        {
            int wireMat = obj->HostedObjects.first()->HostedObjects.first()->Material;
            d->setWireMaterial(wireMat);
        }

    int res = d->exec();

    if (res != 0)
    {
        //qDebug() << "Accepted!";
        switch (d->shape())
        {
        case 0: Sandwich->shapeGrid(obj, 0, d->pitch(), d->length(), d->diameter(), d->wireMaterial()); break;
        case 1: Sandwich->shapeGrid(obj, 1, d->pitchX(), d->pitchY(), d->diameter(), d->wireMaterial()); break;
        case 2: Sandwich->shapeGrid(obj, 2, d->outer(), d->inner(), d->height(), d->wireMaterial()); break;
        default:
            qWarning() << "Unknown grid type!";
        }

        obj->Material = d->bulkMaterial();

        emit RequestRebuildDetector();
        UpdateGui(objName);
    }
    //else qDebug() << "Rejected!";
    delete d;
}

void AGeoTreeWidget::populateTreeWidget(QTreeWidgetItem * parent, AGeoObject * Container, bool fDisabled)
{  
    for (AGeoObject * obj : Container->HostedObjects)
    {
        if (obj->ObjectType->isPrototypes()) continue;

        QTreeWidgetItem *item = new QTreeWidgetItem(parent);

        bool fDisabledLocal = fDisabled || !obj->fActive;
        if (fDisabledLocal) item->setForeground(0, QBrush(Qt::red));

        item->setText(0, obj->Name);
        item->setSizeHint(0, QSize(50, 20));  // ?

        if (obj->ObjectType->isHandlingStatic())
        { //this is one of the slabs or World
            item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);// & ~Qt::ItemIsSelectable);
            QFont f = item->font(0); f.setBold(true); item->setFont(0, f);
        }
        else if (obj->ObjectType->isInstance())
        {
            item->setForeground(0, Qt::blue);
            updateIcon(item, obj);
        }
        else if (obj->ObjectType->isHandlingSet() || obj->ObjectType->isArray() || obj->ObjectType->isGridElement())
        { //group or stack or array or gridElement
            QFont f = item->font(0); f.setItalic(true); item->setFont(0, f);
            updateIcon(item, obj);
            //item->setBackgroundColor(0, BackgroundColor);
        }      
        else
        {
            updateIcon(item, obj);
            if (obj->isStackReference())
            {
                item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);
                QFont f = item->font(0); f.setBold(true); item->setFont(0, f);
            }
            //item->setBackgroundColor(0, BackgroundColor);
        }      

        populateTreeWidget(item, obj, fDisabledLocal);
    }
}

void AGeoTreeWidget::updateExpandState(QTreeWidgetItem * item, bool bPrototypes)
{
    QTreeWidget * treeWidget = nullptr;
    AGeoObject  * obj        = nullptr;

    if (bPrototypes)
    {
        treeWidget = twPrototypes;
        obj        = ( item == topItemPrototypes ? Prototypes : Prototypes->findObjectByName(item->text(0)) );
    }
    else
    {
        treeWidget = this;
        obj        = World->findObjectByName(item->text(0));
    }

    if (obj && obj->fExpanded)
    {
        treeWidget->expandItem(item);
        for (int i = 0; i < item->childCount(); i++)
            updateExpandState(item->child(i), bPrototypes);
    }
}

void AGeoTreeWidget::dropEvent(QDropEvent* event)
{
    if (previousHoverItem)
    {
        previousHoverItem->setBackgroundColor(0, Qt::white);
        previousHoverItem = nullptr;
    }

    QList<QTreeWidgetItem*> selected = selectedItems();

    QTreeWidgetItem * itemTo = this->itemAt(event->pos());
    if (!itemTo)
    {
        qDebug() << "No item on drop position - rejected!";
        event->ignore();
        return;
    }
    QString DraggedTo = itemTo->text(0);
    AGeoObject * objTo = World->findObjectByName(DraggedTo);
    if (!objTo)
    {
        qWarning() << "Error: objTo not found!";
        event->ignore();
        return;
    }

    if (objTo->fLocked || objTo->ObjectType->isInstance())
    {
        qWarning() << "Cannot drop to a locked object or a prototype instance";
        event->ignore();
        return;
    }

    QStringList selNames;

    AGeoObject * ContainerTo = nullptr;
    //qDebug() <<"start of drop";
    if (!showDropIndicator())
    {
        ContainerTo = objTo;
        //qDebug() <<"ON ITEM";
    }
    else
    {
        if (objTo->Container)
             ContainerTo = objTo->Container;
        else ContainerTo = objTo;
        //qDebug() <<"NOT ON ITEM";
    }
    QString containerErrorStr;
    bool containerOk = true;
    if (!ContainerTo->isWorld()) containerOk = ContainerTo->isContainerValidForDrop(containerErrorStr);

    for (int i=0; i<selected.size(); i++) //error catching
    {
        QTreeWidgetItem* DraggedItem = this->selectedItems().at(i);
        if (!DraggedItem)
        {
            //qDebug() << "Drag source item invalid, ignore";
            event->ignore();
            return;
        }

        QString DraggedName = DraggedItem->text(0);
        AGeoObject* obj = World->findObjectByName(DraggedName);
        if (!obj)
        {
            qWarning() << "Error: obj not found!";
            event->ignore();
            return;
        }

        if (ContainerTo != obj->Container)
        {
            if (!containerOk)
            {
                event->ignore();
                QMessageBox::information(this, "", containerErrorStr);
                return;
            }

//            if (ContainerTo->ObjectType->isArray() && obj->ObjectType->isArray())
//            {
//                event->ignore();
//                QMessageBox::information(this, "", "Cannot move array directly inside another array!");
//                return;
//            }
            if (obj->isInUseByComposite())
            {
                event->ignore();
                QMessageBox::information(this, "", "Cannot move objects: Contains object(s) used in a composite object generation");
                return;
            }
            if (ContainerTo->ObjectType->isCompositeContainer() && !obj->ObjectType->isSingle())
            {
                event->ignore();
                QMessageBox::information(this, "", "Can insert only elementary objects to the list of composite members!");
                return;
            }
            if (ContainerTo->ObjectType->isHandlingSet() && !obj->ObjectType->isSingle())
            {
                event->ignore();
                QMessageBox::information(this, "", "Can insert only elementary objects to sets!");
                return;
            }
        }
    }

    for (int i=selected.size()-1; i>-1; i--) //actual move and reorder
    {
        QTreeWidgetItem* DraggedItem = this->selectedItems().at(i);

        QString DraggedName = DraggedItem->text(0);
        AGeoObject* obj = World->findObjectByName(DraggedName);

        if (ContainerTo != obj->Container)
        {
            AGeoObject* objFormerContainer = obj->Container;

            bool ok = obj->migrateTo(ContainerTo);
            if (!ok)
            {
                qWarning() << "Object migration failed: cannot migrate down the chain (or to itself)!";
                event->ignore();
                return;
            }

            if (ContainerTo->ObjectType->isStack())
            {
                //qDebug() << "updating stack of this object";
                obj->updateStack();
            }
            if (objFormerContainer && objFormerContainer->ObjectType->isStack())
            {
                //qDebug() << "updating stack of the former container";
                if (objFormerContainer->HostedObjects.size()>0)
                    objFormerContainer->HostedObjects.first()->updateStack();
            }
        }

        if (ContainerTo != objTo)
        {
            bool fAfter = (dropIndicatorPosition() == QAbstractItemView::BelowItem);
            obj->repositionInHosted(objTo, fAfter);

            if (obj && obj->Container && obj->Container->ObjectType->isStack())
                obj->updateStack();

        }

    }
    //qDebug() << "Drag completed, updating gui";
    UpdateGui();
    emit RequestRebuildDetector();

    // Restore selection
    for (int i=0; i<selNames.size(); i++)
    {
        QList<QTreeWidgetItem*> list = findItems(selNames.at(i), Qt::MatchExactly | Qt::MatchRecursive);
        if (!list.isEmpty()) list.first()->setSelected(true);
    }

    event->ignore();
    return;
}

void AGeoTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    previousHoverItem = nullptr;
    //qDebug() << "Drag enter. Selection size:"<< selectedItems().size();
    //attempt to drag items contaning locked objects should be canceled!

    const int numItems = selectedItems().size();
    for (int iItem = 0; iItem < numItems; iItem++)
    {
        QTreeWidgetItem * DraggedItem = selectedItems().at(iItem);
        QString DraggedName = DraggedItem->text(0);
        //qDebug() << "Draggin item:"<< DraggedName;
        AGeoObject * obj = World->findObjectByName(DraggedName);
        //if (obj->fLocked || obj->isContainsLocked() || obj->ObjectType->isGridElement() || obj->ObjectType->isCompositeContainer())
        if (obj->fLocked || obj->ObjectType->isGridElement() || obj->ObjectType->isCompositeContainer())
        {
            qDebug() << "Drag forbidden for one of the items!";
            event->ignore();
            return;
        }
    }

    // Drop and mouseRelease are not fired if drop on the same item as started -> teal highlight is not removed
    // Clumsy fix - do not show teal highlight if the item is the same
    movingItem = ( numItems > 0 ? selectedItems().at(0)
                                : nullptr);

    event->accept();
}

void AGeoTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);

    const Qt::KeyboardModifiers mod = event->keyboardModifiers();
    bool bRearrange = (mod == Qt::ALT || mod == Qt::CTRL || mod == Qt::SHIFT);

    setDropIndicatorShown(bRearrange);

    if (previousHoverItem)
    {
        previousHoverItem->setBackgroundColor(0, Qt::white);
        previousHoverItem = nullptr;
    }
    if (!bRearrange)
    {
        QTreeWidgetItem * itemOver = this->itemAt(event->pos());
        if (itemOver && itemOver != movingItem)
        {
            itemOver->setBackgroundColor(0, Qt::cyan);
            previousHoverItem = itemOver;
        }
    }
}

void AGeoTreeWidget::configureStyle(QTreeWidget * wid)
{
    QString style;
    style = "QTreeView::branch:has-siblings:!adjoins-item {"
            "border-image: url(:/images/tw-vline.png) 0; }"
    "QTreeView::branch:has-siblings:adjoins-item {"
        "border-image: url(:/images/tw-branch-more.png) 0; }"
    "QTreeView::branch:!has-children:!has-siblings:adjoins-item {"
        "border-image: url(:/images/tw-branch-end.png) 0; }"
    "QTreeView::branch:has-children:!has-siblings:closed,"
    "QTreeView::branch:closed:has-children:has-siblings {"
            "border-image: none;"
            "image: url(:/images/tw-branch-closed.png);}"
    "QTreeView::branch:open:has-children:!has-siblings,"
    "QTreeView::branch:open:has-children:has-siblings  {"
            "border-image: none;"
            "image: url(:/images/tw-branch-open.png);}";
    wid->setStyleSheet(style);
}

void AGeoTreeWidget::onItemSelectionChanged()
{
  //  qDebug() << "---Widget selection cghanged";
  QList<QTreeWidgetItem*> sel = selectedItems();

  if      (sel.size() == 0) emit ObjectSelectionChanged("");
  else if (sel.size() == 1)
  {
      const QString name = sel.first()->text(0);
      emit ObjectSelectionChanged(name);
  }
  else emit ObjectSelectionChanged(""); //with multiple selected do not show EditWidget
}

void AGeoTreeWidget::onProtoItemSelectionChanged()
{
    QList<QTreeWidgetItem*> sel = twPrototypes->selectedItems();

    if      (sel.size() == 0) emit ProtoObjectSelectionChanged("");
    else if (sel.size() == 1)
    {
        const QString name = sel.first()->text(0);
        emit ProtoObjectSelectionChanged(name);
        return;
    }
    else
    {
        //allow to select only one prototype object
        //allow only selection of objects of the same container
        QTreeWidgetItem * FirstParent = sel.first()->parent();
        for (int i = 1; i < sel.size(); i++)
        {
            if (sel.at(i)->parent() != FirstParent)
            {
                qDebug() << "Cannot select together items from different containers!";
                sel.at(i)->setSelected(false);
                return; // will retrigger anyway
            }
            /*
            if (sel.at(i)->font(0).bold())
            {
                //qDebug() << "Cannot select together different slabs or    world and slab(s)";
                sel.at(i)->setSelected(false);
                return; // will retrigger anyway
            }
            */
        }
        emit ProtoObjectSelectionChanged(""); //with multiple selected do not show EditWidget
    }
}

QAction* Action(QMenu& Menu, QString Text)
{
  QAction* s = Menu.addAction(Text);
  s->setEnabled(false);
  return s;
}

void AGeoTreeWidget::customMenuRequested(const QPoint &pos)
{  
  QMenu menu;
  QList<QTreeWidgetItem*> selected = selectedItems();

  QAction* focusObjA = Action(menu, "Show - focus geometry view");
  QAction* showA     = Action(menu, "Show - highlight in geometry");
  QAction* showAdown = Action(menu, "Show - this object with content");
  QAction* showAonly = Action(menu, "Show - only this object");
  QAction* lineA     = Action(menu, "Change line color/width/style");

  menu.addSeparator();

  QAction* enableDisableA = Action(menu, "Enable/Disable");

  menu.addSeparator();

  QMenu * addObjMenu = menu.addMenu("Add object");
    QAction* newBox  = addObjMenu->addAction("Box");
    QMenu * addTubeMenu = addObjMenu->addMenu("Tube");
        QAction* newTube =        addTubeMenu->addAction("Tube");
        QAction* newTubeSegment = addTubeMenu->addAction("Tube segment");
        QAction* newTubeSegCut =  addTubeMenu->addAction("Tube segment cut");
        QAction* newTubeElli =    addTubeMenu->addAction("Elliptical tube");
    QMenu * addTrapMenu = addObjMenu->addMenu("Trapezoid");
        QAction* newTrapSim =     addTrapMenu->addAction("Trapezoid simplified");
        QAction* newTrap    =     addTrapMenu->addAction("Trapezoid");
    QAction* newPcon = addObjMenu->addAction("Polycone");
    QMenu * addPgonMenu = addObjMenu->addMenu("Polygon");
        QAction* newPgonSim =     addPgonMenu->addAction("Polygon simplified");
        QAction* newPgon    =     addPgonMenu->addAction("Polygon");
    QAction* newPara = addObjMenu->addAction("Parallelepiped");
    QAction* newSphere = addObjMenu->addAction("Sphere");
    QMenu * addConeMenu = addObjMenu->addMenu("Cone");
        QAction* newCone =        addConeMenu->addAction("Cone");
        QAction* newConeSeg =     addConeMenu->addAction("Cone segment");
    QAction* newTor = addObjMenu->addAction("Torus");
    QAction* newParabol = addObjMenu->addAction("Paraboloid");
    QAction* newArb8 = addObjMenu->addAction("Arb8");

  QAction* newArrayA  = Action(menu, "Add array");
  QAction* newCompositeA  = Action(menu, "Add composite object");
  QAction* newGridA = Action(menu, "Add optical grid");
  QAction* newMonitorA = Action(menu, "Add monitor");

  menu.addSeparator();

  QMenu * addInstanceMenu = menu.addMenu("Add instance of");
    QVector< QPair<QAction*, QString> > addInstanceA;
    if (Sandwich->Prototypes->HostedObjects.isEmpty())
        Action(*addInstanceMenu, "There are no defined prototypes");
    else
        for (AGeoObject * protoObj : Sandwich->Prototypes->HostedObjects)
            addInstanceA << QPair<QAction*, QString>(addInstanceMenu->addAction(protoObj->Name), protoObj->Name);

  menu.addSeparator();

  QAction* cloneA = Action(menu, "Clone this object");

  menu.addSeparator();

  QAction* removeWithContA = Action(menu, "Remove object AND hosted");
  removeWithContA->setShortcut(QKeySequence(QKeySequence::Delete));
  QAction* removeKeepContA = Action(menu, "Remove object, KEEP hosted");
  removeKeepContA->setShortcut(Qt::Key_Backspace);
  QAction* removeHostedA = Action(menu, "Remove all hosted objects");

  menu.addSeparator();

  QAction* stackA = Action(menu, "Form a stack");
  QAction* stackRefA = Action(menu, "Mark as the stack reference volume");

  menu.addSeparator();

  QAction* prototypeA = Action(menu, QString("Make prototype (move object%1)").arg(selected.size()>1 ? "s" : ""));


  // enable actions according to selection
  QString objName;
  AGeoObject * obj = nullptr;
  if      (selected.size() == 0)
  {
      objName = "World";
      obj = World;
  }
  else if (selected.size() == 1)
  {
      objName = selected.first()->text(0);
      obj = World->findObjectByName(objName);
      if (!obj) return;
      const ATypeGeoObject & ObjectType = *obj->ObjectType;

      bool fNotGridNotMonitor = !ObjectType.isGrid() && !ObjectType.isMonitor();

      addObjMenu->setEnabled(fNotGridNotMonitor);
      enableDisableA->setEnabled( !obj->isWorld() );
      enableDisableA->setText( (obj->isDisabled() ? "Enable object" : "Disable object" ) );
      if (obj->getSlabModel())
          if (obj->getSlabModel()->fCenter) enableDisableA->setEnabled(false);

      newCompositeA->setEnabled(fNotGridNotMonitor);
      newArrayA->setEnabled(fNotGridNotMonitor);
      newMonitorA->setEnabled(fNotGridNotMonitor);
      newGridA->setEnabled(fNotGridNotMonitor);
      cloneA->setEnabled(true);
      removeHostedA->setEnabled(fNotGridNotMonitor);
      removeWithContA->setEnabled(!ObjectType.isWorld());
      removeKeepContA->setEnabled(!ObjectType.isWorld());
      lineA->setEnabled(true);
      focusObjA->setEnabled(true);
      showA->setEnabled(true);
      showAonly->setEnabled(true);
      showAdown->setEnabled(true);
      stackRefA->setEnabled(obj->isStackMember());
      prototypeA->setEnabled(obj->isPossiblePrototype());
  }
  else
  {
      addObjMenu->setEnabled(false);
      addInstanceMenu->setEnabled(false);
      removeWithContA->setEnabled(true);
      stackA->setEnabled(true);
      prototypeA->setEnabled(true);
  }

  QAction* SelectedAction = menu.exec(mapToGlobal(pos));
  if (!SelectedAction) return;

  // -- EXECUTE SELECTED ACTION --
  if (SelectedAction == focusObjA)  // FOCUS OBJECT
  {
      emit RequestFocusObject(objName);
      UpdateGui(objName);
  }
  if (SelectedAction == showA)               ShowObject(obj);
  else if (SelectedAction == showAonly)      ShowObjectOnly(obj);
  else if (SelectedAction == showAdown)      ShowObjectRecursive(obj);
  else if (SelectedAction == lineA)          SetLineAttributes(obj);
  else if (SelectedAction == enableDisableA) menuActionEnableDisable(obj);
  // ADD NEW OBJECT
  else if (SelectedAction == newBox)         menuActionAddNewObject(obj, new AGeoBox());
  else if (SelectedAction == newTube)        menuActionAddNewObject(obj, new AGeoTube());
  else if (SelectedAction == newTubeSegment) menuActionAddNewObject(obj, new AGeoTubeSeg());
  else if (SelectedAction == newTubeSegCut)  menuActionAddNewObject(obj, new AGeoCtub());
  else if (SelectedAction == newTubeElli)    menuActionAddNewObject(obj, new AGeoEltu());
  else if (SelectedAction == newTrapSim)     menuActionAddNewObject(obj, new AGeoTrd1());
  else if (SelectedAction == newTrap)        menuActionAddNewObject(obj, new AGeoTrd2());
  else if (SelectedAction == newPcon)        menuActionAddNewObject(obj, new AGeoPcon());
  else if (SelectedAction == newPgonSim)     menuActionAddNewObject(obj, new AGeoPolygon());
  else if (SelectedAction == newPgon)        menuActionAddNewObject(obj, new AGeoPgon());
  else if (SelectedAction == newPara)        menuActionAddNewObject(obj, new AGeoPara());
  else if (SelectedAction == newSphere)      menuActionAddNewObject(obj, new AGeoSphere());
  else if (SelectedAction == newCone)        menuActionAddNewObject(obj, new AGeoCone());
  else if (SelectedAction == newConeSeg)     menuActionAddNewObject(obj, new AGeoConeSeg());
  else if (SelectedAction == newTor)         menuActionAddNewObject(obj, new AGeoTorus());
  else if (SelectedAction == newParabol)     menuActionAddNewObject(obj, new AGeoParaboloid());
  else if (SelectedAction == newArb8)        menuActionAddNewObject(obj, new AGeoArb8());
  else if (SelectedAction == newCompositeA)  menuActionAddNewComposite(obj);
  else if (SelectedAction == newArrayA)      menuActionAddNewArray(obj);
  else if (SelectedAction == newGridA)       menuActionAddNewGrid(obj);
  else if (SelectedAction == newMonitorA)    menuActionAddNewMonitor(obj);
  else if (SelectedAction == cloneA)         menuActionCloneObject(obj);
  else if (SelectedAction == stackA)         menuActionFormStack(selected);
  else if (SelectedAction == stackRefA)      markAsStackRefVolume(obj);
  else if (SelectedAction == removeKeepContA)menuActionRemoveKeepContent();
  else if (SelectedAction == removeWithContA)menuActionRemoveWithContent(this);
  else if (SelectedAction == removeHostedA)  menuActionRemoveHostedObjects(obj);

  else if (SelectedAction == prototypeA)     menuActionMakeItPrototype(selected);

  else
  {
      for (auto & pair : addInstanceA)
          if (SelectedAction == pair.first)  menuActionAddInstance(obj, pair.second);
  }
}

void AGeoTreeWidget::customProtoMenuRequested(const QPoint &pos)
{
    // top level (Prototypes) can have only single selection (see onProtoSelectionChanged())
    QList<QTreeWidgetItem*> selected = twPrototypes->selectedItems();
    if (selected.isEmpty()) return; // non-empty selection is assumed below!

    QMenu menu;

    QAction* showAllA  = Action(menu, "Show all instances");
    QAction* lineA     = Action(menu, "Change line color/width/style");

    menu.addSeparator();

    QAction* enableDisableA = Action(menu, "Enable/Disable");

    menu.addSeparator();

    QMenu * addObjMenu = menu.addMenu("Add object");
      QAction* newBox  = addObjMenu->addAction("Box");
      QMenu * addTubeMenu = addObjMenu->addMenu("Tube");
          QAction* newTube =        addTubeMenu->addAction("Tube");
          QAction* newTubeSegment = addTubeMenu->addAction("Tube segment");
          QAction* newTubeSegCut =  addTubeMenu->addAction("Tube segment cut");
          QAction* newTubeElli =    addTubeMenu->addAction("Elliptical tube");
      QMenu * addTrapMenu = addObjMenu->addMenu("Trapezoid");
          QAction* newTrapSim =     addTrapMenu->addAction("Trapezoid simplified");
          QAction* newTrap    =     addTrapMenu->addAction("Trapezoid");
      QAction* newPcon = addObjMenu->addAction("Polycone");
      QMenu * addPgonMenu = addObjMenu->addMenu("Polygon");
          QAction* newPgonSim =     addPgonMenu->addAction("Polygon simplified");
          QAction* newPgon    =     addPgonMenu->addAction("Polygon");
      QAction* newPara = addObjMenu->addAction("Parallelepiped");
      QAction* newSphere = addObjMenu->addAction("Sphere");
      QMenu * addConeMenu = addObjMenu->addMenu("Cone");
          QAction* newCone =        addConeMenu->addAction("Cone");
          QAction* newConeSeg =     addConeMenu->addAction("Cone segment");
      QAction* newTor = addObjMenu->addAction("Torus");
      QAction* newParabol = addObjMenu->addAction("Paraboloid");
      QAction* newArb8 = addObjMenu->addAction("Arb8");

    QAction* newArrayA  = Action(menu, "Add array");
    QAction* newCompositeA  = Action(menu, "Add composite object");
    //QAction* newGridA = Action(menu, "Add optical grid");
    QAction* newMonitorA = Action(menu, "Add monitor");

    menu.addSeparator();

    QAction* cloneA = Action(menu, "Clone this object");

    menu.addSeparator();

    QAction* removeWithContA = Action(menu, "Remove object and content");
    QAction* removeKeepContA = Action(menu, "Remove object, keep its content");
    QAction* removeHostedA = Action(menu, "Remove all objects inside");

    menu.addSeparator();

    QAction* stackA = Action(menu, "Form a stack");
    QAction* stackRefA = Action(menu, "Mark as the stack reference volume");

    menu.addSeparator();

    QAction* moveToWorldA = Action(menu, "Move to World");

    // selection is not empty!
    QString objName = selected.first()->text(0);
    AGeoObject * obj = Prototypes->findObjectByName(objName);
    if (!obj) return;
    const ATypeGeoObject & ObjectType = *obj->ObjectType;
    const bool bNotGridNotMonitor = !ObjectType.isGrid() && !ObjectType.isMonitor();
    const bool bIsPrototype = ObjectType.isPrototype();

    if (selected.size() == 1)
    {
        showAllA->setEnabled(bIsPrototype);
        lineA->setEnabled(!bIsPrototype);
        enableDisableA->setEnabled(!obj->isWorld() && !bIsPrototype);
        enableDisableA->setText( (obj->isDisabled() ? "Enable object" : "Disable object" ) );
        addObjMenu->setEnabled(bNotGridNotMonitor);
        newCompositeA->setEnabled(bNotGridNotMonitor);
        newArrayA->setEnabled(bNotGridNotMonitor);
        newMonitorA->setEnabled(bNotGridNotMonitor);
        cloneA->setEnabled(true);
        removeHostedA->setEnabled(bNotGridNotMonitor);
        removeWithContA->setEnabled(true);
        removeKeepContA->setEnabled(!bIsPrototype);
        stackRefA->setEnabled(obj->isStackMember());
        moveToWorldA->setEnabled(bIsPrototype);
    }
    else
    {
        addObjMenu->setEnabled(false);
        removeWithContA->setEnabled(true);
        stackA->setEnabled(!bIsPrototype);
    }

    QAction * SelectedAction = menu.exec(twPrototypes->mapToGlobal(pos));
    if (!SelectedAction) return; //nothing was selected

    // -- EXECUTE SELECTED ACTION --
    //if (SelectedAction == showA)               ShowObject(obj); else
    if (SelectedAction == showAllA)            ShowAllInstances(obj);
    else if (SelectedAction == lineA)          SetLineAttributes(obj);
    else if (SelectedAction == enableDisableA) menuActionEnableDisable(obj);
    // ADD NEW OBJECT
    else if (SelectedAction == newBox)         menuActionAddNewObject(obj, new AGeoBox());
    else if (SelectedAction == newTube)        menuActionAddNewObject(obj, new AGeoTube());
    else if (SelectedAction == newTubeSegment) menuActionAddNewObject(obj, new AGeoTubeSeg());
    else if (SelectedAction == newTubeSegCut)  menuActionAddNewObject(obj, new AGeoCtub());
    else if (SelectedAction == newTubeElli)    menuActionAddNewObject(obj, new AGeoEltu());
    else if (SelectedAction == newTrapSim)     menuActionAddNewObject(obj, new AGeoTrd1());
    else if (SelectedAction == newTrap)        menuActionAddNewObject(obj, new AGeoTrd2());
    else if (SelectedAction == newPcon)        menuActionAddNewObject(obj, new AGeoPcon());
    else if (SelectedAction == newPgonSim)     menuActionAddNewObject(obj, new AGeoPolygon());
    else if (SelectedAction == newPgon)        menuActionAddNewObject(obj, new AGeoPgon());
    else if (SelectedAction == newPara)        menuActionAddNewObject(obj, new AGeoPara());
    else if (SelectedAction == newSphere)      menuActionAddNewObject(obj, new AGeoSphere());
    else if (SelectedAction == newCone)        menuActionAddNewObject(obj, new AGeoCone());
    else if (SelectedAction == newConeSeg)     menuActionAddNewObject(obj, new AGeoConeSeg());
    else if (SelectedAction == newTor)         menuActionAddNewObject(obj, new AGeoTorus());
    else if (SelectedAction == newParabol)     menuActionAddNewObject(obj, new AGeoParaboloid());
    else if (SelectedAction == newArb8)        menuActionAddNewObject(obj, new AGeoArb8());
    else if (SelectedAction == newCompositeA)  menuActionAddNewComposite(obj);
    else if (SelectedAction == newArrayA)      menuActionAddNewArray(obj);
    //else if (SelectedAction == newGridA)       menuActionAddNewGrid(obj);
    else if (SelectedAction == newMonitorA)    menuActionAddNewMonitor(obj);

    else if (SelectedAction == cloneA)         menuActionCloneObject(obj);
    else if (SelectedAction == stackA)         menuActionFormStack(selected);
    else if (SelectedAction == stackRefA)      markAsStackRefVolume(obj);

    else if (SelectedAction == removeKeepContA)menuActionRemoveKeepContent();
    else if (SelectedAction == removeWithContA)menuActionRemoveWithContent(twPrototypes);
    else if (SelectedAction == removeHostedA)  menuActionRemoveHostedObjects(obj);

    else if (SelectedAction == moveToWorldA)   menuActionMoveProtoToWorld(obj);
}

void AGeoTreeWidget::onItemClicked()
{
    if (fSpecialGeoViewMode)
    {
          fSpecialGeoViewMode = false;
          emit RequestNormalDetectorDraw();
    }
    if (!bWorldTreeSelected) onItemSelectionChanged();
    bWorldTreeSelected = true;
}

void AGeoTreeWidget::onProtoItemClicked()
{
    if (fSpecialGeoViewMode)
    {
          fSpecialGeoViewMode = false;
          emit RequestNormalDetectorDraw();
    }
    if (bWorldTreeSelected) onProtoItemSelectionChanged();
    bWorldTreeSelected = false;
}

void AGeoTreeWidget::onItemExpanded(QTreeWidgetItem *item)
{
    AGeoObject * obj = World->findObjectByName(item->text(0));
    if (obj) obj->fExpanded = true;
}

void AGeoTreeWidget::onItemCollapsed(QTreeWidgetItem *item)
{
    AGeoObject * obj = World->findObjectByName(item->text(0));
    if (obj) obj->fExpanded = false;
}

void AGeoTreeWidget::onPrototypeItemExpanded(QTreeWidgetItem * item)
{
    AGeoObject * obj = ( item == twPrototypes->topLevelItem(0) ? Prototypes
                                                               : Prototypes->findObjectByName(item->text(0)) );
    if (obj) obj->fExpanded = true;
}

void AGeoTreeWidget::onPrototypeItemCollapsed(QTreeWidgetItem * item)
{
    AGeoObject * obj = ( item == twPrototypes->topLevelItem(0) ? Prototypes
                                                               : Prototypes->findObjectByName(item->text(0)) );
    if (obj) obj->fExpanded = false;
}

void AGeoTreeWidget::onRemoveTriggered()
{
    menuActionRemoveKeepContent();
}

void AGeoTreeWidget::onRemoveRecursiveTriggered()
{
    menuActionRemoveWithContent(this);
}

void AGeoTreeWidget::onRequestShowPrototype(QString ProtoName)
{
    emit RequestShowPrototypeList();

    QList<QTreeWidgetItem*> list = twPrototypes->findItems(ProtoName, Qt::MatchExactly | Qt::MatchRecursive);
    if (!list.isEmpty())
    {
        list.first()->setSelected(true);
        twPrototypes->setCurrentItem(list.first());
    }
}

void AGeoTreeWidget::onRequestIsValidPrototypeName(const QString &ProtoName, bool &bResult) const
{
    bResult = Sandwich->isValidPrototypeName(ProtoName);
}

void AGeoTreeWidget::menuActionRemoveKeepContent()
{
  QList<QTreeWidgetItem*> selected = selectedItems();
  if (selected.isEmpty()) return;

  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle("");
  QString str = ( selected.size() == 1 ? "Remove "+selected.first()->text(0)+"?"
                                       : "Remove selected objects?" );
  //str += "                                             ";
  msgBox.setText(str);
  QPushButton *remove = msgBox.addButton(QMessageBox::Yes);
  QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
  msgBox.setDefaultButton(cancel);

  msgBox.exec();

  QString failedDeletes;
  if (msgBox.clickedButton() == remove)
  {
      for (int i=0; i<selected.size(); i++)
      {
          QString ObjectName = selected.at(i)->text(0);
          AGeoObject* obj = World->findObjectByName(ObjectName);
          if (obj)
          {
              if (  obj->isInUseByComposite() ||
                   (obj->ObjectType->isPrototype() && Sandwich->World->isPrototypeInUseRecursive(obj->Name, nullptr)) )
              {
                  failedDeletes += "  " + obj->Name;
                  continue;
              }
              obj->suicide(true);
          }
      }

      if (!failedDeletes.isEmpty())
          message("The following objects are in use and could not be deleted:\n" + failedDeletes);

      emit RequestRebuildDetector();
  }
}

void AGeoTreeWidget::menuActionRemoveWithContent(QTreeWidget * treeWidget)
{
    QList<QTreeWidgetItem*> selected = treeWidget->selectedItems();
    if (selected.isEmpty()) return;

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle("");
    QString str = ( selected.size() == 1 ? "Remove " + selected.first()->text(0) + "?"
                                         : "Remove selected objects?" );
    msgBox.setText(str);
    QPushButton *remove = msgBox.addButton(QMessageBox::Yes);
    QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(cancel);

    msgBox.exec();

    QString failedDeletes;
    if (msgBox.clickedButton() == remove)
    {
        for (QTreeWidgetItem * item : selected)
        {
            AGeoObject * obj = World->findObjectByName(item->text(0));
            if (obj)
            {
                if (Sandwich->canBeDeleted(obj)) obj->recursiveSuicide();
                else failedDeletes += obj->Name + "\n";
            }
        }
    }

    emit RequestRebuildDetector();

    if (!failedDeletes.isEmpty())
        message("The following objects are in use and could not be deleted:\n\n" + failedDeletes, treeWidget);
}

void AGeoTreeWidget::menuActionRemoveHostedObjects(AGeoObject * obj)
{
    if (!obj) return;

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle("");
    msgBox.setText("Delete objects hosted inside " + obj->Name + "?");
    msgBox.addButton(QMessageBox::Yes);
    QPushButton * cancel = msgBox.addButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(cancel);

    msgBox.exec();
    if (msgBox.clickedButton() == cancel) return;

    for (int i = obj->HostedObjects.size()-1; i > -1; i--)
        obj->HostedObjects[i]->recursiveSuicide();

    const QString name = obj->Name;
    emit RequestRebuildDetector();
    UpdateGui(name);
}

void AGeoTreeWidget::menuActionCloneObject(AGeoObject * obj)
{
    if (!obj) return;
    if (obj->ObjectType->isWorld()) return;
    if (obj->ObjectType->isSlab())
    {
        message("Cannot clone slabs in this way, use widget at the main window", this);
        return;
    }

    AGeoObject * clone = obj->makeClone(World);
    if (!clone)
    {
        message("Failed to clone object " + obj->Name);
        return;
    }

    if (clone->PositionStr[2].isEmpty()) clone->Position[2] += 10.0;
    else clone->PositionStr[2] += " + 10";

    AGeoObject * container = obj->Container;
    if (!container) container = World;
    container->addObjectFirst(clone);  //inserts to the first position in the list of HostedObjects!
    clone->repositionInHosted(obj, true);

    const QString name = clone->Name;
    emit RequestRebuildDetector();
    emit RequestHighlightObject(name);
}

void AGeoTreeWidget::menuActionAddNewObject(AGeoObject * ContObj, AGeoShape * shape)
{
    if (!ContObj) return;

    AGeoObject * newObj = new AGeoObject();
    while (World->isNameExists(newObj->Name))
        newObj->Name = AGeoObject::GenerateRandomObjectName();

    delete newObj->Shape;
    newObj->Shape = shape;

    newObj->color = 1;
    ContObj->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!

    const QString name = newObj->Name;
    emit RequestRebuildDetector();
    UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewArray(AGeoObject * ContObj)
{
  if (!ContObj) return;

  AGeoObject* newObj = new AGeoObject();
  do newObj->Name = AGeoObject::GenerateRandomArrayName();
  while (World->isNameExists(newObj->Name));

  delete newObj->ObjectType;
  newObj->ObjectType = new ATypeArrayObject();

  newObj->color = 1;
  ContObj->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!

  //element inside
  AGeoObject* elObj = new AGeoObject();
  while (World->isNameExists(elObj->Name))
    elObj->Name = AGeoObject::GenerateRandomObjectName();
  elObj->color = 1;
  newObj->addObjectFirst(elObj);

  const QString name = newObj->Name;
  emit RequestRebuildDetector();
  UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewGrid(AGeoObject * ContObj)
{
  if (!ContObj) return;

  AGeoObject* newObj = new AGeoObject();
  do newObj->Name = AGeoObject::GenerateRandomGridName();
  while (World->isNameExists(newObj->Name));
  if (newObj->Shape) delete newObj->Shape;
  newObj->Shape = new AGeoBox(50, 50, 0.501);
  newObj->Material = ContObj->Material;

  newObj->color = 1;
  ContObj->addObjectFirst(newObj);
  Sandwich->convertObjToGrid(newObj);

  const QString name = newObj->Name;
  emit RequestRebuildDetector();
  UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewMonitor(AGeoObject * ContObj)
{
    if (!ContObj) return;

    AGeoObject* newObj = new AGeoObject();
    do newObj->Name = AGeoObject::GenerateRandomMonitorName();
    while (World->isNameExists(newObj->Name));

    newObj->Material = ContObj->Material;

    delete newObj->ObjectType;
    newObj->ObjectType = new ATypeMonitorObject();

    newObj->updateMonitorShape();

    newObj->color = 1;
    ContObj->addObjectFirst(newObj);

    const QString name = newObj->Name;
    emit RequestRebuildDetector();
    UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddInstance(AGeoObject * ContObj, const QString & PrototypeName)
{
    if (!ContObj) return;

    AGeoObject * newObj = new AGeoObject();
    do newObj->Name = "Instance_" + AGeoObject::GenerateRandomName();
    while (World->isNameExists(newObj->Name));

    delete newObj->ObjectType;
    newObj->ObjectType = new ATypeInstanceObject(PrototypeName);

    AGeoObject * protoObj = Prototypes->findObjectByName(PrototypeName);
    if (!protoObj)
    {
        message("Something went very wrong: prototype not found", this);
        return;
    }
    for (int i = 0; i < 3; i++)
    {
        newObj->Position[i]       = protoObj->Position[i];
        newObj->PositionStr[i]    = protoObj->PositionStr[i];
        newObj->Orientation[i]    = protoObj->Orientation[i];
        newObj->OrientationStr[i] = protoObj->OrientationStr[i];
    }

    ContObj->addObjectFirst(newObj);

    const QString name = newObj->Name;
    emit RequestRebuildDetector();
    UpdateGui(name);
}

void AGeoTreeWidget::menuActionMakeItPrototype(const QList<QTreeWidgetItem*> & selected)
{
    QVector<AGeoObject*> vec;
    for (const QTreeWidgetItem * item : selected)
    {
        AGeoObject * obj = World->findObjectByName(item->text(0));
        if (!obj)
        {
            message("Something went wrong: object not found", this);
            return;
        }
        vec << obj;
    }
    QString err = Sandwich->convertToNewPrototype(vec);
    if (!err.isEmpty())
    {
        message(err, this);
        return;
    }

    const QString name = "";//obj->Name;
    emit RequestRebuildDetector();
    UpdateGui(name);
    emit RequestShowPrototypeList();
}

void AGeoTreeWidget::menuActionMoveProtoToWorld(AGeoObject * obj)
{
    if (!obj || !obj->ObjectType->isPrototype()) return;

    QStringList users;
    bool bIsUsed = Sandwich->World->isPrototypeInUseRecursive(obj->Name, &users);
    if (bIsUsed)
    {
        message("This prototype is in used by these instances(s):\n   " + users.join("\n   "), this);
        return;
    }

    for (AGeoObject * hosted : obj->HostedObjects)
        hosted->migrateTo(World, true);
    Sandwich->Prototypes->HostedObjects.removeOne(obj);
    delete obj;

    emit RequestRebuildDetector();
}

void AGeoTreeWidget::menuActionAddNewComposite(AGeoObject * ContObj)
{
  if (!ContObj) return;

  AGeoObject* newObj = new AGeoObject();
  do newObj->Name = AGeoObject::GenerateRandomCompositeName();
  while (World->isNameExists(newObj->Name));

  newObj->color = 1;
  ContObj->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!

  Sandwich->convertObjToComposite(newObj);

  const QString name = newObj->Name;
  emit RequestRebuildDetector();
  UpdateGui(name);
}

void AGeoTreeWidget::SetLineAttributes(AGeoObject * obj)
{
    if (!obj) return;

    ARootLineConfigurator* rlc = new ARootLineConfigurator(&obj->color, &obj->width, &obj->style, this);
    int res = rlc->exec();
    if (res != 0)
    {
        if (obj->ObjectType->isSlab())
        {
            obj->getSlabModel()->color = obj->color;
            obj->getSlabModel()->width = obj->width;
            obj->getSlabModel()->style = obj->style;
        }
        if (obj->ObjectType->isArray() || obj->ObjectType->isHandlingSet())
        {
            QVector<AGeoObject*> vec;
            obj->collectContainingObjects(vec);
            for (AGeoObject * co : vec)
            {
                co->color = obj->color;
                co->width = obj->width;
                co->style = obj->style;
            }
        }
        const QString name = obj->Name;
        emit RequestRebuildDetector();
        UpdateGui(name);
    }
}

void AGeoTreeWidget::ShowObject(AGeoObject * obj)
{
    if (obj)
    {
        fSpecialGeoViewMode = true;
        emit RequestHighlightObject(obj->Name);
        UpdateGui(obj->Name);
    }
}

void AGeoTreeWidget::ShowObjectRecursive(AGeoObject * obj)
{
    if (obj)
    {
        fSpecialGeoViewMode = true;
        emit RequestShowObjectRecursive(obj->Name);
        UpdateGui(obj->Name);
    }
}

void AGeoTreeWidget::ShowObjectOnly(AGeoObject * obj)
{
    if (obj)
    {
        fSpecialGeoViewMode = true;
        TGeoShape * sh = obj->Shape->createGeoShape();  // make window member?
        sh->Draw();
    }
}

void AGeoTreeWidget::ShowAllInstances(AGeoObject * obj)
{
    if (obj)
    {
        fSpecialGeoViewMode = true;
        emit RequestShowAllInstances(obj->Name);
        UpdateGui(obj->Name);
    }
}

void AGeoTreeWidget::menuActionEnableDisable(AGeoObject * obj)
{
    if (!obj) return;

    if (obj->isDisabled()) obj->enableUp();
    else
    {
        obj->fActive = false;
        if (obj->ObjectType->isSlab()) obj->getSlabModel()->fActive = false;
    }

    obj->fExpanded = obj->fActive;

    const QString name = obj->Name;
    emit RequestRebuildDetector();
    UpdateGui(name);
}

void AGeoTreeWidget::menuActionFormStack(QList<QTreeWidgetItem*> selected)
{
    if (selected.isEmpty()) return;

    QVector<AGeoObject*> objs;
    AGeoObject * ContObj = nullptr;
    for (QTreeWidgetItem * item : selected)
    {
        AGeoObject * obj  = World->findObjectByName(item->text(0));
        if (!obj)
        {
            message("Something went wrong: object with name " + item->text(0) + " not found!", this);
            return;
        }
        if (obj->ObjectType->isWorld() || obj->ObjectType->isSlab())
        {
            message("World or slabs/lightguides cannot be a member of a stack", this);
            return;
        }
        if (obj->ObjectType->isArray())
        {
            message("Array cannot be a member of a stack", this);
            return;
        }
        if (obj->ObjectType->isComposite() || obj->ObjectType->isGrid())
        {
            message("Composite objects (and optical grids) cannot be a member of a stack", this);
            return;
        }
        if (obj->ObjectType->isHandlingSet() || obj->ObjectType->isLogical())
        {
            message("Stacks/groups cannot be a member of a stack", this);
            return;
        }
        if (obj->ObjectType->isPrototype() || obj->ObjectType->isInstance())
        {
            message("Prototypes and instances cannot be a member of a stack", this);
            return;
        }
        if (!ContObj) ContObj = obj->Container;
        if (ContObj != obj->Container)
        {
            message("To form a stack all objects have to have the same container", this);
            return;
        }

        objs << obj;
    }

    AGeoObject * stackObj = new AGeoObject();
    delete stackObj->ObjectType; stackObj->ObjectType = new ATypeStackContainerObject();
    static_cast<ATypeStackContainerObject*>(stackObj->ObjectType)->ReferenceVolume = objs.first()->Name;

    do stackObj->Name = AGeoObject::GenerateRandomStackName();
    while (World->isNameExists(stackObj->Name));

    AGeoObject * contObj = objs.first()->Container; // All selected objects always have the same container!
    stackObj->Container = contObj;

    for (AGeoObject * obj : objs)
    {
        contObj->HostedObjects.removeOne(obj);
        obj->Container = stackObj;
        stackObj->HostedObjects.append(obj);
    }
    contObj->HostedObjects.insert(0, stackObj);

    const QString name = stackObj->Name;
    emit RequestRebuildDetector();  // automatically calculates stack positions there
    UpdateGui(name);
}

void AGeoTreeWidget::markAsStackRefVolume(AGeoObject * obj)
{
    if (!obj->Container) return;
    if (!obj->Container->ObjectType) return;
    ATypeStackContainerObject * sc = dynamic_cast<ATypeStackContainerObject*>(obj->Container->ObjectType);
    if (!sc) return;
    const QString name = obj->Name;
    sc->ReferenceVolume = name;

    emit RequestRebuildDetector();
    UpdateGui(name);
}

void AGeoTreeWidget::AddLightguide(bool bUpper)
{
    if (Sandwich->GetMaterials().isEmpty())
    {
        message("Cannot create lightguide: there are no materials defined");
        return;
    }

    AGeoObject * obj = Sandwich->addLightguide(bUpper);
    if (!obj) return;

    const QString name = obj->Name;
    emit RequestRebuildDetector();
    UpdateGui(name);
}

QImage createImageWithOverlay(const QImage& base, const QImage& overlay)
{
    QImage imageWithOverlay = QImage(overlay.size(), QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&imageWithOverlay);

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(imageWithOverlay.rect(), Qt::transparent);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, base);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, overlay);

    painter.end();

    return imageWithOverlay;
}

void AGeoTreeWidget::updateIcon(QTreeWidgetItem* item, AGeoObject *obj)
{  
  if (!obj || !item) return;

  QImage image;

  AGeoObject* cont = obj->Container;
  if (cont && !cont->HostedObjects.isEmpty())
  {
      /*
      if (cont->ObjectType->isGroup())
      {
          if (obj == cont->HostedObjects.first())
              image = GroupStart;
          else if (obj == cont->HostedObjects.last())
              image = GroupEnd;
          else
              image = GroupMid;
      }
      else
      */
      if (cont->ObjectType->isStack())
      {
          if (obj == cont->HostedObjects.first())
              image = StackStart;
          else if (obj == cont->HostedObjects.last())
              image = StackEnd;
          else
              image = StackMid;
      }
  }

  if (obj->fLocked)
    {
      if (image.isNull())
        image = Lock;
      else
        image = createImageWithOverlay(Lock, image);
    }

  QIcon icon = QIcon(QPixmap::fromImage(image));
  item->setIcon(0, icon); 
}

void AGeoTreeWidget::objectMembersToScript(AGeoObject* Master, QString &script, int ident, bool bExpandMaterial, bool bRecursive, bool usePython)
{
    for (AGeoObject* obj : Master->HostedObjects)
        objectToScript(obj, script, ident, bExpandMaterial, bRecursive, usePython);
}

void AGeoTreeWidget::objectToScript(AGeoObject *obj, QString &script, int ident, bool bExpandMaterial, bool bRecursive, bool usePython)
{
    int bigIdent = ident + 4;
    int medIdent = ident + 2;
    QString CommentStr;
    if (!usePython)
    {
        CommentStr = "//";
    }
    else
    {
        bigIdent = medIdent = 0;
        CommentStr = "#";
    }

    const QString Starter = "\n" + QString(" ").repeated(ident);

    if (obj->ObjectType->isLogical())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_basicObject(obj, bExpandMaterial, usePython);
    }
    else if (obj->ObjectType->isCompositeContainer())
    {
         //nothing to do
    }
    else if (obj->ObjectType->isSlab())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_slab(obj, bExpandMaterial, ident);
        script += "\n" + QString(" ").repeated(ident)+ makeLinePropertiesString(obj);

        if (obj->ObjectType->isLightguide())
        {
            script += "\n";
            script += "\n" + QString(" ").repeated(ident)+ CommentStr + "=== Lightguide object is not supported! ===";
            script += "\n";
        }
        if (bRecursive) objectMembersToScript(obj, script, medIdent, bExpandMaterial, bRecursive, usePython);
    }
    else if (obj->ObjectType->isSingle() )
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_basicObject(obj, bExpandMaterial, usePython);
        script += "\n" + QString(" ").repeated(ident)+ makeLinePropertiesString(obj);
        if (bRecursive) objectMembersToScript(obj, script, medIdent, bExpandMaterial, bRecursive, usePython);
    }
    else if (obj->ObjectType->isComposite())
    {
        script += "\n" + QString(" ").repeated(ident) + CommentStr + "-->-- logical volumes for " + obj->Name;
        objectMembersToScript(obj->getContainerWithLogical(), script, bigIdent, bExpandMaterial, bRecursive, usePython);
        script += "\n" + QString(" ").repeated(ident) + CommentStr + "--<-- logical volumes end for " + obj->Name;

        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_basicObject(obj, bExpandMaterial, usePython);
        script += "\n" + QString(" ").repeated(ident)+ makeLinePropertiesString(obj);
        if (bRecursive) objectMembersToScript(obj, script, medIdent, bExpandMaterial, bRecursive, usePython);
    }
    else if (obj->ObjectType->isArray())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_arrayObject(obj);
        script += "\n" + QString(" ").repeated(ident)+ CommentStr + "-->-- array elements for " + obj->Name;
        objectMembersToScript(obj, script, medIdent, bExpandMaterial, bRecursive, usePython);
        script += "\n" + QString(" ").repeated(ident)+ CommentStr + "--<-- array elements end for " + obj->Name;
    }
    else if (obj->ObjectType->isMonitor())
    {
        script += Starter + makeScriptString_monitorBaseObject(obj);
        script += Starter + makeScriptString_monitorConfig(obj);
        script += "\n" + QString(" ").repeated(ident)+ makeLinePropertiesString(obj);
    }
    else if (obj->ObjectType->isStack())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_stackObjectStart(obj);
        script += "\n" + QString(" ").repeated(ident)+ CommentStr + "-->-- stack elements for " + obj->Name;
        script += "\n" + QString(" ").repeated(ident)+ CommentStr + " Values of x, y, z only matter for the stack element, refered to at InitializeStack below";
        script += "\n" + QString(" ").repeated(ident)+ CommentStr + " For the rest of elements they are calculated automatically";
        objectMembersToScript(obj, script, medIdent, bExpandMaterial, bRecursive, usePython);
        script += "\n" + QString(" ").repeated(ident)+ CommentStr + "--<-- stack elements end for " + obj->Name;
        if (!obj->HostedObjects.isEmpty())
            script += "\n" + QString(" ").repeated(ident)+ makeScriptString_stackObjectEnd(obj);
    }
    else if (obj->ObjectType->isGroup())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_groupObjectStart(obj);
        script += "\n" + QString(" ").repeated(ident)+ CommentStr + "-->-- group elements for " + obj->Name;
        objectMembersToScript(obj, script, medIdent, bExpandMaterial, bRecursive, usePython);
        script += "\n" + QString(" ").repeated(ident)+ CommentStr + "--<-- group elements end for " + obj->Name;
    }
    else if (obj->ObjectType->isGrid())
    {
        script += "\n";
        script += "\n" + QString(" ").repeated(ident)+ CommentStr + "=== Optical grid object is not supported! Make a request to the developers ===";
        script += "\n";
    }
    else if (obj->ObjectType->isInstance())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_instanceObject(obj, usePython);
    }
    else if (obj->ObjectType->isPrototype())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_prototypeObject(obj);
        if (bRecursive) objectMembersToScript(obj, script, medIdent, bExpandMaterial, bRecursive, usePython);
    }

    if (obj->isDisabled())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_DisabledObject(obj);
    }
}

void AGeoTreeWidget::commonSlabToScript(QString &script, const QString &identStr)
{
    script += identStr + QString ("geo.SetCommonSlabMode(") +
                      QString::number(Sandwich->SandwichState) + ")\n";

    ASlabXYModel* xy =static_cast<ASlabXYModel*>(Sandwich->DefaultXY);
    script += identStr + QString("geo.SetCommonSlabProperties(") +
            QString::number(xy->shape) + ", " +
            QString::number(xy->size1) + ", " +
            QString::number(xy->size2) + ", " +
            QString::number(xy->angle) + ", " +
            QString::number(xy->sides) + ")\n";
}

void AGeoTreeWidget::rebuildDetectorAndRestoreCurrentDelegate()
{
    const QString CurrentObjName = EditWidget->getCurrentObjectName();
    emit RequestRebuildDetector();
    UpdateGui(CurrentObjName);
}

QString AGeoTreeWidget::makeScriptString_basicObject(AGeoObject* obj, bool bExpandMaterials, bool usePython) const
{
    QVector<QString> posStrs; posStrs.reserve(3);
    QVector<QString> oriStrs; oriStrs.reserve(3);

    QString GenerationString = obj->Shape->getGenerationString(true);
    if (usePython) GenerationString = obj->Shape->getPythonGenerationString(GenerationString);

    for (int i = 0; i < 3; i++)
    {
        posStrs << ( obj->PositionStr[i].isEmpty() ? QString::number(obj->Position[i]) : obj->PositionStr[i] );
        oriStrs << ( obj->OrientationStr[i].isEmpty() ? QString::number(obj->Orientation[i]) : obj->OrientationStr[i] );
    }

    QString str = QString("geo.TGeo( ") +
            "'" + obj->Name + "', " +
            "'" + GenerationString + "', " +
            (bExpandMaterials && obj->Material < Sandwich->GetMaterials().size() ?
                 Sandwich->GetMaterials().at(obj->Material) + "_mat" : QString::number(obj->Material)) + ", "
            "'" + obj->Container->Name + "',   "+
            posStrs[0] + ", " +
            posStrs[1] + ", " +
            posStrs[2] + ",   " +
            oriStrs[0] + ", " +
            oriStrs[1] + ", " +
            oriStrs[2] + " )";

    AGeoConsts::getConstInstance().formulaToScript(str, usePython);
    return str;
}

QString AGeoTreeWidget::makeScriptString_slab(AGeoObject *obj, bool bExpandMaterials, int ident) const
{
    ATypeSlabObject *slab = static_cast<ATypeSlabObject*>(obj->ObjectType);
    ASlabModel *m = static_cast<ASlabModel*>(slab->SlabModel);
    QString str;
    if (m->fCenter)
    {
        str += makeScriptString_setCenterSlab(obj) + "\n" + QString(" ").repeated(ident);
    }

    QString matStr = (bExpandMaterials && obj->Material < Sandwich->GetMaterials().size() ?
         Sandwich->GetMaterials().at(obj->Material) + "_mat" : QString::number(obj->Material));
    str += m->makeSlabScriptString(matStr);

    return str;

}

QString AGeoTreeWidget::makeScriptString_setCenterSlab(AGeoObject *obj) const
{
    ATypeSlabObject *slab = static_cast<ATypeSlabObject*>(obj->ObjectType);
    ASlabModel *m = static_cast<ASlabModel*>(slab->SlabModel);

    if (m->fCenter)
    {
        return QString("geo.SetCenterSlab(") +
                "'" + m->name + "', " +
                QString::number(Sandwich->ZOriginType) + ")     //setting center slab" ;
    }
    return "";
}

QString AGeoTreeWidget::makeScriptString_arrayObject(AGeoObject *obj) const
{
    ATypeArrayObject* a = dynamic_cast<ATypeArrayObject*>(obj->ObjectType);
    if (!a)
    {
        qWarning() << "It is not an array!";
        return "Error accessing object as array!";
    }

    QString snumX  = (a  ->strNumX          .isEmpty() ? QString::number(a  ->numX)              : a->strNumX);
    QString snumY  = (a  ->strNumY          .isEmpty() ? QString::number(a  ->numY)              : a->strNumY);
    QString snumZ  = (a  ->strNumZ          .isEmpty() ? QString::number(a  ->numZ)              : a->strNumZ);
    QString sstepX = (a  ->strStepX         .isEmpty() ? QString::number(a  ->stepX)             : a->strStepX);
    QString sstepY = (a  ->strStepY         .isEmpty() ? QString::number(a  ->stepY)             : a->strStepY);
    QString sstepZ = (a  ->strStepZ         .isEmpty() ? QString::number(a  ->stepZ)             : a->strStepZ);
    QString sPos0  = (obj->PositionStr[0]   .isEmpty() ? QString::number(obj->Position[0])       : obj->PositionStr[0]);
    QString sPos1  = (obj->PositionStr[1]   .isEmpty() ? QString::number(obj->Position[1])       : obj->PositionStr[1]);
    QString sPos2  = (obj->PositionStr[2]   .isEmpty() ? QString::number(obj->Position[2])       : obj->PositionStr[2]);
    QString sOri2  = (obj->OrientationStr[2].isEmpty() ? QString::number(obj->Orientation[2])    : obj->OrientationStr[2]);

    QString str =  QString("geo.Array( ") +
            "'" + obj->Name + "', " +
            snumX + ", " +
            snumY + ", " +
            snumZ + ",   " +
            sstepX + ", " +
            sstepY + ", " +
            sstepZ + ", " +
            "'" + obj->Container->Name + "',   " +
            sPos0 + ", " +
            sPos1 + ", " +
            sPos2 + ",   " +
            sOri2 + " )";
    //qDebug() <<"strrr" << str;
    return str;
}

QString AGeoTreeWidget::makeScriptString_instanceObject(AGeoObject *obj, bool usePython) const
{
    ATypeInstanceObject * ins = dynamic_cast<ATypeInstanceObject*>(obj->ObjectType);
    if (!ins)
    {
        qWarning() << "It is not an instance!";
        return "Error accessing object as instance!";
    }

    QVector<QString> posStrs(3);
    QVector<QString> oriStrs(3);
    for (int i = 0; i < 3; i++)
    {
        posStrs[i] = ( obj->PositionStr[i].isEmpty()    ? QString::number(obj->Position[i])    : obj->PositionStr[i] );
        oriStrs[i] = ( obj->OrientationStr[i].isEmpty() ? QString::number(obj->Orientation[i]) : obj->OrientationStr[i] );
    }

    QString str =  QString("geo.Instance( ") +
            "'" + obj->Name +            "', " +
            "'" + ins->PrototypeName +   "', " +
            "'" + obj->Container->Name + "',   " +
            posStrs[0] + ", " +
            posStrs[1] + ", " +
            posStrs[2] + ",   " +
            oriStrs[0] + ", " +
            oriStrs[1] + ", " +
            oriStrs[2] + " )";

    AGeoConsts::getConstInstance().formulaToScript(str, usePython);
    return str;
}

QString AGeoTreeWidget::makeScriptString_prototypeObject(AGeoObject * obj) const
{
    ATypePrototypeObject * pro = dynamic_cast<ATypePrototypeObject*>(obj->ObjectType);
    if (!pro)
    {
        qWarning() << "It is not a prototype!";
        return "Error accessing object as prototype!";
    }

    QString str =  QString("geo.Prototype( ") +
            "'" + obj->Name +            "' )";
    return str;
}

QString AGeoTreeWidget::makeScriptString_monitorBaseObject(const AGeoObject * obj) const
{
    ATypeMonitorObject * m = dynamic_cast<ATypeMonitorObject*>(obj->ObjectType);
    if (!m)
    {
        qWarning() << "It is not a monitor!";
        return "Error accessing monitor!";
    }
    const AMonitorConfig & c = m->config;

    // geo.Monitor( name,  shape,  size1,  size2,  container,  x,  y,  z,  phi,  theta,  psi,  SensitiveTop,  SensitiveBottom,  StopsTraking )
    return QString("geo.Monitor( %1, %2,  %3, %4,  %5,   %6, %7, %8,   %9, %10, %11,   %12, %13,   %14 )")
            .arg("'" + obj->Name + "'")
            .arg(c.shape)
            .arg(c.str2size1.isEmpty() ? QString::number(2.0 * c.size1) : c.str2size1)
            .arg(c.str2size2.isEmpty() ? QString::number(2.0 * c.size2) : c.str2size2)
            .arg("'" + obj->Container->Name + "'")
            .arg(obj->PositionStr[0].isEmpty() ? QString::number(obj->Position[0]) : obj->PositionStr[0])
            .arg(obj->PositionStr[1].isEmpty() ? QString::number(obj->Position[1]) : obj->PositionStr[1])
            .arg(obj->PositionStr[2].isEmpty() ? QString::number(obj->Position[2]) : obj->PositionStr[2])
            .arg(obj->OrientationStr[0].isEmpty() ? QString::number(obj->Orientation[0]) : obj->OrientationStr[0])
            .arg(obj->OrientationStr[1].isEmpty() ? QString::number(obj->Orientation[1]) : obj->OrientationStr[1])
            .arg(obj->OrientationStr[2].isEmpty() ? QString::number(obj->Orientation[2]) : obj->OrientationStr[2])
            .arg(c.bUpper ? "true" : "false")
            .arg(c.bLower ? "true" : "false")
            .arg(c.bStopTracking ? "true" : "false");
}

QString AGeoTreeWidget::makeScriptString_monitorConfig(const AGeoObject *obj) const
{
    ATypeMonitorObject * m = dynamic_cast<ATypeMonitorObject*>(obj->ObjectType);
    if (!m)
    {
        qWarning() << "It is not a monitor!";
        return "Error accessing monitor!";
    }
    const AMonitorConfig & c = m->config;

    if (c.PhotonOrParticle == 0)
    {
        //geo.Monitor_ConfigureForPhotons( MonitorName,  Position,  Time,  Angle,  Wave )
        return QString("geo.Monitor_ConfigureForPhotons( %1,  [%2, %3],  [%4, %5, %6],  [%7, %8, %9],  [%10, %11, %12] )")
                .arg("'" + obj->Name + "'")
                .arg(c.xbins)
                .arg(c.ybins)
                .arg(c.timeBins)
                .arg(c.timeFrom)
                .arg(c.timeTo)
                .arg(c.angleBins)
                .arg(c.angleFrom)
                .arg(c.angleTo)
                .arg(c.waveBins)
                .arg(c.waveFrom)
                .arg(c.waveTo);
    }
    else
    {
        //geo.Monitor_ConfigureForParticles( MonitorName,  ParticleIndex,  Both_Primary_Secondary,  Both_Direct_Indirect,  Position,  Time,  Angle,  Energy )
        return QString("geo.Monitor_ConfigureForParticles( %1,  %2,  %3,  %4,   [%5, %6],  [%7, %8, %9],  [%10, %11, %12],  [%13, %14, %15, %16] )")
                .arg("'" + obj->Name + "'")
                .arg(c.ParticleIndex)
                .arg(c.bPrimary && c.bSecondary ? 0 : (c.bPrimary ? 1 : 2))
                .arg(c.bDirect  && c.bIndirect  ? 0 : (c.bDirect  ? 1 : 2))
                .arg(c.xbins)
                .arg(c.ybins)
                .arg(c.timeBins)
                .arg(c.timeFrom)
                .arg(c.timeTo)
                .arg(c.angleBins)
                .arg(c.angleFrom)
                .arg(c.angleTo)
                .arg(c.energyBins)
                .arg(c.energyFrom)
                .arg(c.energyTo)
                .arg(c.energyUnitsInHist);
    }
}

QString AGeoTreeWidget::makeScriptString_stackObjectStart(AGeoObject *obj) const
{
    return  QString("geo.Stack(") +
            "'" + obj->Name + "', " +
            "'" + obj->Container->Name + "' )";
}

QString AGeoTreeWidget::makeScriptString_groupObjectStart(AGeoObject *obj) const
{
    return  QString("geo.MakeGroup(") +
            "'" + obj->Name + "', " +
            "'" + obj->Container->Name + "' )";
}

QString AGeoTreeWidget::makeScriptString_stackObjectEnd(AGeoObject *obj) const
{
    return QString("geo.InitializeStack( ") +
           "'" + obj->Name + "',  " +
           "'" + obj->getOrMakeStackReferenceVolume()->Name + "' )";  //obj->HostedObjects.first()->Name
}

QString AGeoTreeWidget::makeLinePropertiesString(AGeoObject *obj) const
{
    return "geo.SetLine( '" +
            obj->Name +
            "',  " +
            QString::number(obj->color) + ",  " +
            QString::number(obj->width) + ",  " +
            QString::number(obj->style) + " )";
}

QString AGeoTreeWidget::makeScriptString_DisabledObject(AGeoObject *obj) const
{
    return QString("geo.DisableObject( '%1')").arg(obj->Name);
}

// ================== EDIT WIDGET ===================

AGeoWidget::AGeoWidget(ASandwich * Sandwich, AGeoTreeWidget * tw) :
  Sandwich(Sandwich), tw(tw)
{
  lMain = new QVBoxLayout(this);
  lMain->setContentsMargins(2,2,2,5);
  this->setLayout(lMain);

  //Scroll area in middle
  QScrollArea* sa = new QScrollArea(this);
  sa->setFrameShape(QFrame::Box);//NoFrame);
  sa->setContentsMargins(2,2,2,2);
  sa->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  sa->setWidgetResizable(true);
  sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QWidget* scrollAreaWidgetContents = new QWidget();
  scrollAreaWidgetContents->setGeometry(QRect(0, 0, 350, 200));

  ObjectLayout = new QVBoxLayout(scrollAreaWidgetContents);
  ObjectLayout->setContentsMargins(0,0,0,0);

  sa->setWidget(scrollAreaWidgetContents);
  lMain->addWidget(sa);

  frBottom = new QFrame();
  frBottom->setFrameShape(QFrame::StyledPanel);
  frBottom->setMinimumHeight(38);
  frBottom->setMaximumHeight(38);
  QPalette palette = frBottom->palette();
  palette.setColor( backgroundRole(), QColor( 255, 255, 255 ) );
  frBottom->setPalette( palette );
  frBottom->setAutoFillBackground( true );
  QHBoxLayout* lb = new QHBoxLayout();
  lb->setContentsMargins(0,0,0,0);
  frBottom->setLayout(lb);
    pbConfirm = new QPushButton("Confirm changes");    
    pbConfirm->setMinimumHeight(25);
    connect(pbConfirm, SIGNAL(clicked()), this, SLOT(onConfirmPressed()));
    pbConfirm->setMaximumWidth(150);
    lb->addWidget(pbConfirm);
    pbCancel = new QPushButton("Cancel changes");
    connect(pbCancel, SIGNAL(clicked()), this, SLOT(onCancelPressed()));
    pbCancel->setMaximumWidth(150);
    pbCancel->setMinimumHeight(25);
    lb->addWidget(pbCancel);
  lMain->addWidget(frBottom);

  pbConfirm->setEnabled(false);
  pbCancel->setEnabled(false);

  fIgnoreSignals = false;
}

void AGeoWidget::ClearGui()
{
    //qDebug() << "AGeoWidget clear triggered (Delegate will be deleted)";
    fIgnoreSignals = true;

    while (ObjectLayout->count() > 0)
    {
        QLayoutItem * item = ObjectLayout->takeAt(0);
        if (item->widget())
            delete item->widget();
        delete item;
    }

    delete GeoDelegate; GeoDelegate = nullptr;

    fIgnoreSignals = false;

    //if update triggered during editing
    exitEditingMode();
}

void AGeoWidget::UpdateGui()
{  
    //qDebug() << "UpdateGui triggered for AGeoWidget--->->->->";
    ClearGui(); //deletes Delegate!

    if (!CurrentObject) return;

    pbConfirm->setEnabled(true);
    pbCancel->setEnabled(true);

    if (CurrentObject->ObjectType->isWorld())
        GeoDelegate = new AWorldDelegate(Sandwich->Materials, this);
    else if (CurrentObject->ObjectType->isSlab())        // SLAB or LIGHTGUIDE
        GeoDelegate = createAndAddSlabDelegate();
    else if (CurrentObject->ObjectType->isGridElement())
        GeoDelegate = createAndAddGridElementDelegate();
    else if (CurrentObject->ObjectType->isMonitor())
        GeoDelegate = createAndAddMonitorDelegate();
    else
        GeoDelegate = createAndAddGeoObjectDelegate();

    GeoDelegate->Update(CurrentObject);

    GeoDelegate->Widget->setEnabled(!CurrentObject->fLocked);
    connect(GeoDelegate, &AGeoBaseDelegate::ContentChanged,             this, &AGeoWidget::onStartEditing);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestChangeVisAttributes, this, &AGeoWidget::onRequestSetVisAttributes);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestShow,                this, &AGeoWidget::onRequestShowCurrentObject);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestScriptToClipboard,   this, &AGeoWidget::onRequestScriptLineToClipboard);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestScriptRecursiveToClipboard,   this, &AGeoWidget::onRequestScriptRecursiveToClipboard);

    ObjectLayout->addStretch();
    ObjectLayout->addWidget(GeoDelegate->Widget);
    ObjectLayout->addStretch();

    tw->LastShownObjectName = CurrentObject->Name;
}

AGeoBaseDelegate * AGeoWidget::createAndAddGeoObjectDelegate()
{
    AGeoObjectDelegate * Del;

    AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(CurrentObject->Shape);
    const QString shape = (scaled ? scaled->getBaseShapeType() : CurrentObject->Shape->getShapeType());

    if (CurrentObject->ObjectType->isArray())
        Del = new AGeoArrayDelegate(Sandwich->Materials, this);
    else if (CurrentObject->ObjectType->isInstance())
    {
        Del = new AGeoInstanceDelegate(Sandwich->Materials, this);
        connect((AGeoInstanceDelegate*)Del, &AGeoInstanceDelegate::RequestShowPrototype,        tw, &AGeoTreeWidget::onRequestShowPrototype);
        connect((AGeoInstanceDelegate*)Del, &AGeoInstanceDelegate::RequestIsValidPrototypeName, tw, &AGeoTreeWidget::onRequestIsValidPrototypeName);
    }
    else if (CurrentObject->ObjectType->isPrototype())
        Del = new AGeoPrototypeDelegate(Sandwich->Materials, this);
    else if (CurrentObject->ObjectType->isHandlingSet())
        Del = new AGeoSetDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoBBox")
        Del = new AGeoBoxDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoTube")
        Del = new AGeoTubeDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoTubeSeg")
        Del = new AGeoTubeSegDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoCtub")
        Del = new AGeoTubeSegCutDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoEltu")
        Del = new AGeoElTubeDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoPara")
        Del = new AGeoParaDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoSphere")
        Del = new AGeoSphereDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoTrd1")
        Del = new AGeoTrapXDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoTrd2")
        Del = new AGeoTrapXYDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoCone")
        Del = new AGeoConeDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoConeSeg")
        Del = new AGeoConeSegDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoParaboloid")
        Del = new AGeoParaboloidDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoTorus")
        Del = new AGeoTorusDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoPolygon")
        Del = new AGeoPolygonDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoPcon")
        Del = new AGeoPconDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoPgon")
        Del = new AGeoPgonDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoCompositeShape")
        Del = new AGeoCompositeDelegate(Sandwich->Materials, this);
    else if (shape == "TGeoArb8")
        Del = new AGeoArb8Delegate(Sandwich->Materials, this);
    else
        Del = new AGeoObjectDelegate(Sandwich->Materials, this);

    connect(Del, &AGeoObjectDelegate::RequestChangeShape,   this, &AGeoWidget::onRequestChangeShape);

    return Del;
}

AGeoBaseDelegate * AGeoWidget::createAndAddSlabDelegate()
{
    AGeoObjectDelegate * Del;
    ASlabModel * SlabModel = (static_cast<ATypeSlabObject*>(CurrentObject->ObjectType))->SlabModel;
    switch (SlabModel->XYrecord.shape)
    {
    default: qWarning() << "Unknown slab shape, assuming rectangular";
    case 0:
        Del = new AGeoSlabDelegate_Box(Sandwich->Materials, static_cast<int>(Sandwich->SandwichState), this); break;
    case 1:
        Del = new AGeoSlabDelegate_Tube(Sandwich->Materials, static_cast<int>(Sandwich->SandwichState), this); break;
    case 2:
        Del = new AGeoSlabDelegate_Poly(Sandwich->Materials, static_cast<int>(Sandwich->SandwichState), this); break;
    }
    connect(Del, &AGeoObjectDelegate::RequestChangeSlabShape, this, &AGeoWidget::onRequestChangeSlabShape);

    //Del = new AGeoSlabDelegate(tw->Sandwich->Materials, static_cast<int>(tw->Sandwich->SandwichState), this);
    return Del;
}

AGeoBaseDelegate * AGeoWidget::createAndAddGridElementDelegate()
{
    AGridElementDelegate * Del = new AGridElementDelegate(this);
    connect(Del, &AGridElementDelegate::RequestReshapeGrid, tw, &AGeoTreeWidget::onGridReshapeRequested);
    return Del;
}

AGeoBaseDelegate *AGeoWidget::createAndAddMonitorDelegate()
{
    QStringList particles;
    emit tw->RequestListOfParticles(particles);
    AMonitorDelegate* Del = new AMonitorDelegate(particles, this);
    connect(Del, &AMonitorDelegate::requestShowSensitiveFaces, this, &AGeoWidget::onMonitorRequestsShowSensitiveDirection);
    return Del;
}

void AGeoWidget::onObjectSelectionChanged(QString SelectedObject)
{  
    if (fIgnoreSignals) return;

    //qDebug() << "Object selection changed! ->" << SelectedObject;

    CurrentObject = nullptr;
    ClearGui();
    if (SelectedObject.isEmpty()) return;

    AGeoObject * obj = Sandwich->World->findObjectByName(SelectedObject);
    //qDebug() << "Object for this name:" << obj;
    if (!obj) return;

    CurrentObject = obj;
    //qDebug() << "New current object:"<<CurrentObject->Name;
    UpdateGui();
    fEditingMode = false;
    //qDebug() << "OnObjectSelection procedure completed";
}

void AGeoWidget::onStartEditing()
{
  //qDebug() << "Start editing";
  if (fIgnoreSignals) return;
  if (!CurrentObject) return;

  if (!fEditingMode)
  {
      fEditingMode = true;
      tw->setEnabled(false);
      tw->twPrototypes->setEnabled(false);
      QFont f = pbConfirm->font(); f.setBold(true); pbConfirm->setFont(f);
      pbConfirm->setStyleSheet("QPushButton {color: red;}");
      emit requestEnableGeoConstWidget(false);
  }
}

void AGeoWidget::onRequestChangeShape(AGeoShape * NewShape)
{
    if (!GeoDelegate) return;
    if (!CurrentObject) return;
    if (!NewShape) return;

    delete CurrentObject->Shape;
    CurrentObject->Shape = NewShape;
    if (!CurrentObject->ObjectType->isGrid()) CurrentObject->removeCompositeStructure();
    UpdateGui();
    onConfirmPressed();
}

void AGeoWidget::updateInstancesOnProtoNameChange(QString oldName, QString newName)
{
    QVector<AGeoObject*> vec;
    Sandwich->World->findAllInstancesRecursive(vec);

    for (AGeoObject * inst : vec)
    {
        ATypeInstanceObject * insType = static_cast<ATypeInstanceObject*>(inst->ObjectType);
        if (insType->PrototypeName == oldName)
            insType->PrototypeName = newName;
    }
}

void AGeoWidget::onRequestChangeSlabShape(int NewShape)
{
    if (!GeoDelegate) return;
    if (!CurrentObject) return;
    if (NewShape < 0 || NewShape > 2) return;
    if (!CurrentObject->ObjectType->isSlab()) return;

    ASlabModel * SlabModel = CurrentObject->getSlabModel();
    SlabModel->XYrecord.shape = NewShape;
    CurrentObject->UpdateFromSlabModel(SlabModel);

    exitEditingMode();
    QString name = CurrentObject->Name;
    emit tw->RequestRebuildDetector();
    tw->UpdateGui(name);
}

void AGeoWidget::onRequestShowCurrentObject()
{
    if (!CurrentObject) return;

    QString name = CurrentObject->Name;
    emit tw->RequestHighlightObject(name);
    tw->UpdateGui(name);
}

void AGeoWidget::onRequestScriptLineToClipboard()
{
    if (!CurrentObject) return;

    QString script;
    bool bNotRecursive = (CurrentObject->ObjectType->isSlab() || CurrentObject->ObjectType->isSingle() || CurrentObject->ObjectType->isComposite());
    emit requestBuildScript(CurrentObject, script, 0, false, !bNotRecursive, false);     // !*! the false may be temporary

    qDebug() << script;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(script);
}

void AGeoWidget::onRequestScriptRecursiveToClipboard()
{
    if (!CurrentObject) return;

    QString script;
    emit requestBuildScript(CurrentObject, script, 0, false, true, false);            // !*! the false may be temporary


    qDebug() << script;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(script);
}

void AGeoWidget::onRequestSetVisAttributes()
{
    if (!CurrentObject) return;
    tw->SetLineAttributes(CurrentObject);
}

QString AGeoWidget::getCurrentObjectName() const
{
    if (CurrentObject) return CurrentObject->Name;
    else return "";
}

void AGeoWidget::onMonitorRequestsShowSensitiveDirection()
{
    emit showMonitor(CurrentObject);
}

void AGeoWidget::exitEditingMode()
{
    fEditingMode = false;
    tw->setEnabled(true);
    tw->twPrototypes->setEnabled(true);
    QFont f = pbConfirm->font(); f.setBold(false); pbConfirm->setFont(f);
    pbConfirm->setStyleSheet("QPushButton {color: black;}");
    pbConfirm->setEnabled(false);
    pbCancel->setEnabled(false);
    emit requestEnableGeoConstWidget(true);
}

void AGeoWidget::onConfirmPressed()
{
    if (!GeoDelegate)
    {
        qWarning() << "|||---Confirm triggered without active Delegate!";
        exitEditingMode();
        tw->UpdateGui();
        return;
    }

    const QString newName = GeoDelegate->getName();
    QString errorStr;
    if (newName != CurrentObject->Name && Sandwich->World->isNameExists(newName)) errorStr = QString("%1 name already exists").arg(newName);
    else if (newName.isEmpty()) errorStr = "Name cannot be empty";
    else if (newName.contains(QRegExp("\\s"))) errorStr = "Name cannot contain spaces";
    if (!errorStr.isEmpty())
    {
        QMessageBox::warning(this, "", errorStr);
        return;
    }

    const QString oldName = CurrentObject->Name;
    bool ok = GeoDelegate->updateObject(CurrentObject);
    if (!ok) return;

    if (CurrentObject->ObjectType->isPrototype() && oldName != newName)
        updateInstancesOnProtoNameChange(oldName, newName);

    exitEditingMode();
    QString name = CurrentObject->Name;
    emit tw->RequestRebuildDetector();
    tw->UpdateGui(name);
}

void AGeoWidget::onCancelPressed()
{
    exitEditingMode();
    tw->UpdateGui( (CurrentObject) ? CurrentObject->Name : "" );
}

