#include "ageotreewidget.h"
#include "ageoobject.h"
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

#include <QDropEvent>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSet>
#include <QScrollArea>
#include <QSpacerItem>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QApplication>
#include <QPainter>
#include <QClipboard>
#include <QToolTip>

#include "TRotation.h"
#include "TVector3.h"
#include "TMath.h"
#include "TGeoShape.h"

#include <QShortcut>

AGeoTreeWidget::AGeoTreeWidget(ASandwich *Sandwich) : Sandwich(Sandwich)
{
  World = Sandwich->World;

  connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
  //setHeaderLabels(QStringList() << "Tree of geometry objects: use context menu and drag-and-drop");
  setHeaderHidden(true);

  setAcceptDrops(true);
  setDragEnabled(true);
  setDragDropMode(QAbstractItemView::InternalMove);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setDropIndicatorShown(false);
  //setIndentation(20);
  setContentsMargins(0,0,0,0);
  setFrameStyle(QFrame::NoFrame);
  setIconSize(QSize(20,20));

  QString dir = ":/images/";
  Lock.load(dir+"lock.png");
  GroupStart.load(dir+"TopGr.png");
  GroupMid.load(dir+"MidGr.png");
  GroupEnd.load(dir+"BotGr.png");
  StackStart.load(dir+"TopSt.png");
  StackMid.load(dir+"MidSt.png");
  StackEnd.load(dir+"BotSt.png");

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(customMenuRequested(const QPoint &)));

  BackgroundColor = QColor(240,240,240);
  fSpecialGeoViewMode = false;

  EditWidget = new AGeoWidget(Sandwich->World, this);
  connect(this, SIGNAL(ObjectSelectionChanged(QString)), EditWidget, SLOT(onObjectSelectionChanged(QString)));
  connect(this, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onItemClicked()));
  connect(EditWidget, &AGeoWidget::showMonitor, this, &AGeoTreeWidget::RequestShowMonitor);
  connect(EditWidget, &AGeoWidget::requestBuildScript, this, &AGeoTreeWidget::objectToScript);

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
  setStyleSheet(style);

  QShortcut* Del = new QShortcut(Qt::Key_Backspace, this);
  connect(Del, &QShortcut::activated, this, &AGeoTreeWidget::onRemoveTriggered);

  QShortcut* DelRec = new QShortcut(QKeySequence(QKeySequence::Delete), this);
  connect(DelRec, &QShortcut::activated, this, &AGeoTreeWidget::onRemoveRecursiveTriggered);
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

void AGeoTreeWidget::UpdateGui(QString selected)
{
  if (!World) return;

  //qDebug() << "Update, selected = "<<selected;
  if (selected.isEmpty() && currentItem())
  {
      //qDebug() << currentItem()->text(0);
      selected = currentItem()->text(0);
  }
  clear();

  //World
  QTreeWidgetItem *w = new QTreeWidgetItem(this);
  w->setText(0, "World");
  QFont f = w->font(0);
  f.setBold(true);
  w->setFont(0, f);
  w->setSizeHint(0, QSize(50, 20));
  w->setFlags(w->flags() & ~Qt::ItemIsDragEnabled);// & ~Qt::ItemIsSelectable);
  //w->setBackgroundColor(0, BackgroundColor);
  //qDebug() << "New world WidgetItem created";

  populateTreeWidget(w, World);
  //expandAll(); // less blunt later - e.g. on exapand remember the status in AGeoObject
  if (topLevelItemCount()>0)
    updateExpandState(this->topLevelItem(0));

  if (!selected.isEmpty())
  {
      //qDebug() << "Selection:"<<selected;
      QList<QTreeWidgetItem*> list = findItems(selected, Qt::MatchExactly | Qt::MatchRecursive);
      //qDebug() << list.size();
        if (!list.isEmpty())
        {
           //qDebug() << "Attempting to focus:"<<list.first()->text(0);
           list.first()->setSelected(true);
           setCurrentItem(list.first());
        }
  }
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

void AGeoTreeWidget::populateTreeWidget(QTreeWidgetItem* parent, AGeoObject *Container, bool fDisabled)
{  
  for (int i=0; i<Container->HostedObjects.size(); i++)
    {
      AGeoObject *obj = Container->HostedObjects[i];
      QString SubName = obj->Name;      
      QTreeWidgetItem *item = new QTreeWidgetItem(parent);

      bool fDisabledLocal = fDisabled || !obj->fActive;
      if (fDisabledLocal) item->setForeground(0, QBrush(Qt::red));

      item->setText(0, SubName);
      item->setSizeHint(0, QSize(50, 20));

      if (obj->ObjectType->isHandlingStatic())
        { //this is one of the slabs or World
          item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);// & ~Qt::ItemIsSelectable);
          QFont f = item->font(0);
          f.setBold(true);
          item->setFont(0, f);
        }
      else if (obj->ObjectType->isHandlingSet() || obj->ObjectType->isArray() || obj->ObjectType->isGridElement())
        { //group or stack or array or gridElement
          QFont f = item->font(0);
          f.setItalic(true);
          item->setFont(0, f);
          updateIcon(item, obj);
          item->setBackgroundColor(0, BackgroundColor);
        }      
      else
        {
          updateIcon(item, obj);
          item->setBackgroundColor(0, BackgroundColor);
        }      

      populateTreeWidget(item, obj, fDisabledLocal);
  }
}

void AGeoTreeWidget::updateExpandState(QTreeWidgetItem *item)
{
    AGeoObject* obj = World->findObjectByName(item->text(0));
    if (obj)
    {
        if (obj->fExpanded)
        {
            expandItem(item);
            for (int i=0; i<item->childCount(); i++)
                updateExpandState(item->child(i));
        }
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

    QStringList selNames;

    //if keyboard modifier is on, rearrange objects instead of change to new container
    const Qt::KeyboardModifiers mod = event->keyboardModifiers();
    if (mod == Qt::ALT || mod == Qt::CTRL || mod == Qt::SHIFT)
    {
        //qDebug() << "Rearrange order event triggered";
        if (selected.size() != 1)
        {
            //qDebug() << "Only one item should be selected to use rearrangment!";
            event->ignore();
            return;
        }
        QTreeWidgetItem * DraggedItem = this->selectedItems().first();
        if (!DraggedItem)
        {
            //qDebug() << "Drag source item invalid, ignore";
            event->ignore();
            return;
        }
        const QString DraggedName = DraggedItem->text(0);
        selNames << DraggedName;

        AGeoObject* obj = World->findObjectByName(DraggedName);
        if (obj)
        {
            //bool fAfter = (event->pos().y() > visualItemRect(itemTo).center().y());
            bool fAfter = (dropIndicatorPosition() == QAbstractItemView::BelowItem);
            obj->repositionInHosted(objTo, fAfter);
        }

        if (obj && obj->Container && obj->Container->ObjectType->isStack())
            obj->updateStack();
    }
    else
    {
      // Normal drag n drop
      if (objTo->ObjectType->isGrid())
      {
          event->ignore();
          QMessageBox::information(this, "", "Grid cannot contain anything but the grid element!");
          return;
      }
      if (objTo->isCompositeMemeber())
      {
          event->ignore();
          QMessageBox::information(this, "", "Cannot move objects to composite logical objects!");
          return;
      }
      if (objTo->ObjectType->isMonitor())
      {
          event->ignore();
          QMessageBox::information(this, "", "Monitors cannot host objects!");
          return;
      }

      //check if any of the items are composite members and are in use
      // possible optimisation: make search only once, -> container with AGeoObjects?
      for (int i=0; i<selected.size(); i++)
      {
          QTreeWidgetItem* DraggedItem = this->selectedItems().at(i);
          if (!DraggedItem)
            {
              qDebug() << "Drag source item invalid, ignore";
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
          if (objTo->ObjectType->isArray() && obj->ObjectType->isArray())
          {
              event->ignore();
              QMessageBox::information(this, "", "Cannot move array directly inside another array!");
              return;
          }
          if (obj->isInUseByComposite())
          {
              event->ignore();
              QMessageBox::information(this, "", "Cannot move objects: Contains object(s) used in a composite object generation");
              return;
          }
          if (objTo->ObjectType->isCompositeContainer() && !obj->ObjectType->isSingle())
          {
              event->ignore();
              QMessageBox::information(this, "", "Can insert only elementary objects to the list of composite members!");
              return;
          }
          if (objTo->ObjectType->isHandlingSet() && !obj->ObjectType->isSingle())
          {
              event->ignore();
              QMessageBox::information(this, "", "Can insert only elementary objects to sets!");
              return;
          }
      }

      for (int i=selected.size()-1; i>-1; i--)
        {
          QTreeWidgetItem* DraggedItem = this->selectedItems().at(i);
          QString DraggedName = DraggedItem->text(0);
          selNames << DraggedName;
          //qDebug() << "Draggin item:"<< DraggedName;

          AGeoObject* obj = World->findObjectByName(DraggedName);
          AGeoObject* objFormerContainer = obj->Container;
          //qDebug() << "Dragging"<<obj->Name<<"to"<<objTo->Name<<"from"<<objFormerContainer->Name;
          bool ok = obj->migrateTo(objTo);
          if (!ok)
          {
              qWarning() << "Object migration failed: cannot migrate down the chain (or to itself)!";
              event->ignore();
              return;
          }

          if (objTo->ObjectType->isStack())
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
        if (obj->fLocked || obj->isContainsLocked() || obj->ObjectType->isGridElement() || obj->ObjectType->isCompositeContainer())
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

void AGeoTreeWidget::onItemSelectionChanged()
{
  QList<QTreeWidgetItem*> sel = selectedItems();

  if (sel.size() == 0)
    {
      emit ObjectSelectionChanged("");
      return;
    }
  if (sel.size() == 1)
    {
      QString name = sel.first()->text(0);
      emit ObjectSelectionChanged(name);
      return;
    }

  //multiple selected

  //allow only selection of objects of the same container!
  //static objects cannot be mixed with others
  QTreeWidgetItem* FirstParent = sel.first()->parent();
  for (int i=1; i<sel.size(); i++)
    {
      if (sel.at(i)->parent() != FirstParent)
        {
          //qDebug() << "Cannot select together items from different containers!";
          sel.at(i)->setSelected(false);
          return; // will retrigger anyway
        }
      if (sel.at(i)->font(0).bold())
        {
          //qDebug() << "Cannot select together different slabs or    world and slab(s)";
          sel.at(i)->setSelected(false);
          return; // will retrigger anyway
        }
    }

  //with multiple selected do not show EditWidget
  emit ObjectSelectionChanged("");
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

  QAction* showA = Action(menu, "Show - highlight in geometry");
  QAction* showAdown = Action(menu, "Show - this object with content");
  QAction* showAonly = Action(menu, "Show - only this object");
  QAction* lineA = Action(menu, "Change line color/width/style");

  menu.addSeparator();

  QAction* enableDisableA = Action(menu, "Enable/Disable");

  menu.addSeparator();

  QMenu * addObjMenu = menu.addMenu("Add object");
  //QFont f = addObjMenu->font();
  //f.setBold(true);
  //addObjMenu->setFont(f);
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

  QAction* copyA = Action(menu, "Duplicate this object");

  menu.addSeparator();

  QAction* removeThisAndHostedA = Action(menu, "Remove object and content");
  removeThisAndHostedA->setShortcut(QKeySequence(QKeySequence::Delete));
  QAction* removeA = Action(menu, "Remove object, keep its content");
  removeA->setShortcut(Qt::Key_Backspace);
  QAction* removeHostedA = Action(menu, "Remove all objects inside");

  menu.addSeparator();

  QAction* lockA = Action(menu, "Lock");
  QAction* lockallA = Action(menu, "Lock objects inside");

  menu.addSeparator();

  QAction* unlockA = Action(menu, "Unlock");
  QAction* unlockallA = Action(menu, "Unlock objects inside");

  menu.addSeparator();

  QAction* groupA = Action(menu, "Group");
  QAction* stackA = Action(menu, "Form a stack");

  menu.addSeparator();

  QAction* addUpperLGA = Action(menu, "Define compound lightguide (top)");
  QAction* addLoweLGA = Action(menu, "Define compound lightguide (bottom)");


  // enable actions according to selection
  QList<QTreeWidgetItem*> selected = selectedItems();

  addUpperLGA->setEnabled(!World->containsUpperLightGuide());
  addLoweLGA->setEnabled(!World->containsLowerLightGuide());

  if (selected.size() == 0)
    { //menu triggered without selected items            
    }
  else if (selected.size() == 1)
    { //menu triggered with only one selected item
      AGeoObject* obj = World->findObjectByName(selected.first()->text(0));
      if (!obj) return;
      const ATypeObject& ObjectType = *obj->ObjectType;

      bool fNotGridNotMonitor = !ObjectType.isGrid() && !ObjectType.isMonitor();

      //newA->setEnabled(fNotGridNotMonitor);
      addObjMenu->setEnabled(fNotGridNotMonitor);
      enableDisableA->setEnabled(true);      
      enableDisableA->setText( (obj->isDisabled() ? "Enable object" : "Disable object" ) );
      if (obj->getSlabModel())
          if (obj->getSlabModel()->fCenter) enableDisableA->setEnabled(false);

      newCompositeA->setEnabled(fNotGridNotMonitor);
      newArrayA->setEnabled(fNotGridNotMonitor && !ObjectType.isArray());
      newMonitorA->setEnabled(fNotGridNotMonitor && !ObjectType.isArray());
      newGridA->setEnabled(fNotGridNotMonitor);
      copyA->setEnabled( ObjectType.isSingle() || ObjectType.isSlab() || ObjectType.isMonitor());  //supported so far only Single, Slab and Monitor
      removeHostedA->setEnabled(fNotGridNotMonitor);
      //removeThisAndHostedA->setEnabled(fNotGridNotMonitor);
      removeThisAndHostedA->setEnabled(!ObjectType.isWorld());
      removeA->setEnabled(!ObjectType.isWorld());
      lockA->setEnabled(!ObjectType.isHandlingStatic() || ObjectType.isLightguide());
      unlockA->setEnabled(true);
      lockallA->setEnabled(true);
      unlockallA->setEnabled(true);
      lineA->setEnabled(true);
      showA->setEnabled(true);
      showAonly->setEnabled(true);
      showAdown->setEnabled(true);
    }
  else if (!selected.first()->font(0).bold())
    { //menu triggered with several items selected, and they are not slabs
      removeA->setEnabled(true); //world cannot be in selection with anything else anyway
      lockA->setEnabled(true);
      unlockA->setEnabled(true);
      groupA->setEnabled(true);
      stackA->setEnabled(true);
    }

  QAction* SelectedAction = menu.exec(mapToGlobal(pos));
  if (!SelectedAction) return; //nothing was selected

  // -- EXECUTE SELECTED ACTION --
  if (SelectedAction == showA)  // SHOW OBJECT
     ShowObject(selected.first()->text(0));
  else if (SelectedAction == showAonly)
      ShowObjectOnly(selected.first()->text(0));
  else if (SelectedAction == showAdown)
      ShowObjectRecursive(selected.first()->text(0));
  else if (SelectedAction == lineA) // SET LINE ATTRIBUTES
     SetLineAttributes(selected.first()->text(0));
  else if (SelectedAction == enableDisableA)
     menuActionEnableDisable(selected.first()->text(0));
  // ADD NEW OBJECT
  else if (SelectedAction == newBox)         menuActionAddNewObject(selected.first()->text(0), new AGeoBox());
  else if (SelectedAction == newTube)        menuActionAddNewObject(selected.first()->text(0), new AGeoTube());
  else if (SelectedAction == newTubeSegment) menuActionAddNewObject(selected.first()->text(0), new AGeoTubeSeg());
  else if (SelectedAction == newTubeSegCut)  menuActionAddNewObject(selected.first()->text(0), new AGeoCtub());
  else if (SelectedAction == newTubeElli)    menuActionAddNewObject(selected.first()->text(0), new AGeoEltu());
  else if (SelectedAction == newTrapSim)     menuActionAddNewObject(selected.first()->text(0), new AGeoTrd1());
  else if (SelectedAction == newTrap)        menuActionAddNewObject(selected.first()->text(0), new AGeoTrd2());
  else if (SelectedAction == newPcon)        menuActionAddNewObject(selected.first()->text(0), new AGeoPcon());
  else if (SelectedAction == newPgonSim)     menuActionAddNewObject(selected.first()->text(0), new AGeoPolygon());
  else if (SelectedAction == newPgon)        menuActionAddNewObject(selected.first()->text(0), new AGeoPgon());
  else if (SelectedAction == newPara)        menuActionAddNewObject(selected.first()->text(0), new AGeoPara());
  else if (SelectedAction == newSphere)      menuActionAddNewObject(selected.first()->text(0), new AGeoSphere());
  else if (SelectedAction == newCone)        menuActionAddNewObject(selected.first()->text(0), new AGeoCone());
  else if (SelectedAction == newConeSeg)     menuActionAddNewObject(selected.first()->text(0), new AGeoConeSeg());
  else if (SelectedAction == newTor)         menuActionAddNewObject(selected.first()->text(0), new AGeoTorus());
  else if (SelectedAction == newParabol)     menuActionAddNewObject(selected.first()->text(0), new AGeoParaboloid());
  else if (SelectedAction == newArb8)        menuActionAddNewObject(selected.first()->text(0), new AGeoArb8());
  //ADD NEW COMPOSITE
  else if (SelectedAction == newCompositeA)
     menuActionAddNewComposite(selected.first()->text(0));
  else if (SelectedAction == newArrayA) //ADD NEW COMPOSITE
     menuActionAddNewArray(selected.first()->text(0));
  else if (SelectedAction == newGridA) //ADD NEW GRID
     menuActionAddNewGrid(selected.first()->text(0));
  else if (SelectedAction == newMonitorA) //ADD NEW MONITOR
     menuActionAddNewMonitor(selected.first()->text(0));
  else if (SelectedAction == addUpperLGA || SelectedAction == addLoweLGA) // ADD LIGHTGUIDE
     addLightguide(SelectedAction == addUpperLGA);
  else if (SelectedAction == copyA) // COPY OBJECT
     menuActionCopyObject(selected.first()->text(0));
  else if (SelectedAction == groupA || SelectedAction == stackA) //GROUP & STACK
    {
      int option = (SelectedAction == groupA) ? 0 : 1;
      formSet(selected, option);
    }
  else if (SelectedAction == lockA) // LOCK
     menuActionLock();
  else if (SelectedAction == unlockA) // UNLOCK
     menuActionUnlock();
  else if (SelectedAction == lockallA) // LOCK OBJECTS INSIDE
     menuActionLockAllInside(selected.first()->text(0));
  else if (SelectedAction == unlockallA)
     menuActionUnlockAllInside(selected.first()->text(0));
  else if (SelectedAction == removeA) // REMOVE
     menuActionRemove();
  else if (SelectedAction == removeThisAndHostedA) // REMOVE RECURSIVLY
     menuActionRemoveRecursively(selected.first()->text(0));
  else if (SelectedAction == removeHostedA) // REMOVE HOSTED
      menuActionRemoveHostedObjects(selected.first()->text(0));
}

void AGeoTreeWidget::onItemClicked()
{
    if (fSpecialGeoViewMode)
      {
          fSpecialGeoViewMode = false;
          emit RequestNormalDetectorDraw();
      }
}

void AGeoTreeWidget::onItemExpanded(QTreeWidgetItem *item)
{
    AGeoObject* obj = World->findObjectByName(item->text(0));
    if (obj) obj->fExpanded = true;
}

void AGeoTreeWidget::onItemCollapsed(QTreeWidgetItem *item)
{
    AGeoObject* obj = World->findObjectByName(item->text(0));
    if (obj) obj->fExpanded = false;
}

void AGeoTreeWidget::onRemoveTriggered()
{
    menuActionRemove();
}

void AGeoTreeWidget::onRemoveRecursiveTriggered()
{
    QList<QTreeWidgetItem*> selected = selectedItems();
    if (selected.isEmpty()) return;

    if (selected.size() > 1)
    {
        message("This action can be used when only ONE volume is selected", this);
        return;
    }

    menuActionRemoveRecursively(selected.first()->text(0));
}

void AGeoTreeWidget::menuActionRemove()
{
  QList<QTreeWidgetItem*> selected = selectedItems();
  if (selected.isEmpty()) return;

  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle("Locked objects are NOT removed!");
  QString str;// = "\nThis command, executed on a slab or lightguide removes it too!";
  if (selected.size() == 1)  str = "Remove "+selected.first()->text(0)+"?";
  else str = "Remove selected objects?";
  str += "                                             ";
  msgBox.setText(str);
  QPushButton *remove = msgBox.addButton(QMessageBox::Yes);
  QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
  msgBox.setDefaultButton(cancel);

  msgBox.exec();

  if (msgBox.clickedButton() == remove)
    {
      //emit ObjectSelectionChanged("");
      for (int i=0; i<selected.size(); i++)
        {
          QString ObjectName = selected.at(i)->text(0);
          AGeoObject* obj = World->findObjectByName(ObjectName);
          if (obj)
          {
              if (obj->isInUseByComposite()) continue;
              obj->suicide(true);
          }
        }
      UpdateGui();
      emit RequestRebuildDetector();
    }
}

void AGeoTreeWidget::menuActionRemoveRecursively(QString ObjectName)
{
    AGeoObject * obj = World->findObjectByName(ObjectName);
    if (!obj)
    {
        qWarning() << "Error: object" << ObjectName << "not found in the geometry!";
        return;
    }

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle("Locked objects are NOT removed!");

    QString str;
    if (obj->ObjectType->isSlab())
        str = "Remove all objects hosted inside " + ObjectName + " slab?";
    else if (obj->ObjectType->isWorld())
        str = "Remove all non-slab objects from the geometry?";
    else
        str = "Remove " + ObjectName + " and all objects hosted inside?";

    msgBox.setText(str);
    QPushButton *remove = msgBox.addButton(QMessageBox::Yes);
    QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(cancel);

    msgBox.exec();

    if (msgBox.clickedButton() == remove)
    {
        //emit ObjectSelectionChanged("");
        obj->recursiveSuicide();
        //UpdateGui();
        emit RequestRebuildDetector();
    }
}

void AGeoTreeWidget::menuActionRemoveHostedObjects(QString ObjectName)
{
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle("Locked objects will NOT be deleted!");
  msgBox.setText("Delete objects hosted inside " + ObjectName + "?\nSlabs and lightguides are NOT removed.");
  QPushButton *remove = msgBox.addButton(QMessageBox::Yes);
  QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
  msgBox.setDefaultButton(cancel);

  msgBox.exec();

  if (msgBox.clickedButton() == remove)
    {
      //emit ObjectSelectionChanged("");
      AGeoObject* obj = World->findObjectByName(ObjectName);
      if (obj)
        for (int i=obj->HostedObjects.size()-1; i>-1; i--)
          obj->HostedObjects[i]->recursiveSuicide();
      UpdateGui();
      emit RequestRebuildDetector();
    }
}

void AGeoTreeWidget::menuActionUnlockAllInside(QString ObjectName)
{
  int ret = QMessageBox::question(this, "",
                                 "Unlock all objects inside "+ObjectName+"?",
                                 QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
  if (ret == QMessageBox::Yes)
    {
      AGeoObject* obj = World->findObjectByName(ObjectName);
      if (obj)
        {
          for (int i=0; i<obj->HostedObjects.size(); i++)
            obj->HostedObjects[i]->unlockAllInside();
          UpdateGui();
        }
    }
}

void AGeoTreeWidget::menuActionLockAllInside(QString ObjectName)
{
  int ret = QMessageBox::question(this, "",
                                 "Lock all objects inside "+ObjectName+"?",
                                 QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
  if (ret == QMessageBox::Yes)
    {
      AGeoObject* obj = World->findObjectByName(ObjectName);
      if (obj)
        {
          obj->lockRecursively();
          obj->lockUpTheChain();
          UpdateGui();
        }
    }
}

void AGeoTreeWidget::menuActionUnlock()
{
  bool fContainsLocked = false;
  QList<QTreeWidgetItem*> selected = selectedItems();
  for (int i=0; i<selected.size(); i++)
    {
      QString Object = selected.at(i)->text(0);
      AGeoObject* obj = World->findObjectByName(Object);
      fContainsLocked = obj->isContainsLocked();
      if (fContainsLocked) break;
    }

  if (fContainsLocked) QMessageBox::information(this, "", "Cannot unlock selected object(s): some of the objects inside are locked!");
  else
    {
      for (int i=0; i<selected.size(); i++)
        {
          QString Object = selected.at(i)->text(0);
          AGeoObject* obj = World->findObjectByName(Object);
          if (obj) obj->fLocked = false;
        }
      if (selected.size() == 1)
          UpdateGui(selected.first()->text(0));
      else UpdateGui();
    }
}

void AGeoTreeWidget::menuActionLock()
{
  QList<QTreeWidgetItem*> selected = selectedItems();
  for (int i=0; i<selected.size(); i++)
    {
      QString Object = selected.at(i)->text(0);
      AGeoObject* obj = World->findObjectByName(Object);
      if (obj) obj->lockUpTheChain();
    }
  if (selected.size() == 1) UpdateGui(selected.first()->text(0));
  else UpdateGui();
}

void AGeoTreeWidget::menuActionCopyObject(QString ObjToCopyName)
{
  AGeoObject* ObjToCopy = World->findObjectByName(ObjToCopyName);
  if (!ObjToCopy || ObjToCopy->ObjectType->isWorld()) return;

  if ( !(ObjToCopy->ObjectType->isSingle() || ObjToCopy->ObjectType->isSlab() || ObjToCopy->ObjectType->isMonitor()) ) return; //supported so far only Single and Slab

  if (ObjToCopy->ObjectType->isSlab())
  {
    ATypeSlabObject* slab = static_cast<ATypeSlabObject*>(ObjToCopy->ObjectType);
    ObjToCopy->UpdateFromSlabModel(slab->SlabModel);
  }

  AGeoObject* newObj = new AGeoObject(ObjToCopy);
  if (ObjToCopy->ObjectType->isMonitor())
  {
      while (World->isNameExists(newObj->Name))
        newObj->Name = AGeoObject::GenerateRandomMonitorName();
      delete newObj->ObjectType;
      ATypeMonitorObject* mt = new ATypeMonitorObject();
      mt->config = static_cast<ATypeMonitorObject*>(ObjToCopy->ObjectType)->config;
      newObj->ObjectType = mt;
  }
  else
  {
      while (World->isNameExists(newObj->Name))
        newObj->Name = AGeoObject::GenerateRandomObjectName();
  }

  AGeoObject* container = ObjToCopy->Container;
  if (!container) container = World;
  container->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!

  QString name = newObj->Name;
  UpdateGui(name);
  emit RequestRebuildDetector();
  emit RequestHighlightObject(name);
  UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewObject(QString ContainerName, AGeoShape * shape)
{
  AGeoObject* ContObj = World->findObjectByName(ContainerName);
  if (!ContObj) return;

  AGeoObject* newObj = new AGeoObject();
  while (World->isNameExists(newObj->Name))
    newObj->Name = AGeoObject::GenerateRandomObjectName();

  delete newObj->Shape;
  newObj->Shape = shape;

  newObj->color = 1;
  ContObj->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!
  QString name = newObj->Name;
  UpdateGui(name);
  emit RequestRebuildDetector();
  UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewArray(QString ContainerName)
{
  AGeoObject* ContObj = World->findObjectByName(ContainerName);
  if (!ContObj) return;

  AGeoObject* newObj = new AGeoObject();
  do
    {
      newObj->Name = AGeoObject::GenerateRandomArrayName();
    }
  while (World->isNameExists(newObj->Name));

  delete newObj->ObjectType;
  newObj->ObjectType = new ATypeArrayObject();

  newObj->color = 1;
  ContObj->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!
  QString name = newObj->Name;

  //element inside
  AGeoObject* elObj = new AGeoObject();
  while (World->isNameExists(elObj->Name))
    elObj->Name = AGeoObject::GenerateRandomObjectName();
  elObj->color = 1;
  newObj->addObjectFirst(elObj);

  UpdateGui(name);
  emit RequestRebuildDetector();
  UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewGrid(QString ContainerName)
{
  AGeoObject* ContObj = World->findObjectByName(ContainerName);
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

  QString name = newObj->Name;
  UpdateGui(name);
  emit RequestRebuildDetector();
  UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewMonitor(QString ContainerName)
{
    AGeoObject* ContObj = World->findObjectByName(ContainerName);
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

    QString name = newObj->Name;
    //UpdateGui(name);
    emit RequestRebuildDetector();
    UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewComposite(QString ContainerName)
{
  AGeoObject* ContObj = World->findObjectByName(ContainerName);
  if (!ContObj) return;

  AGeoObject* newObj = new AGeoObject();
  do newObj->Name = AGeoObject::GenerateRandomCompositeName();
  while (World->isNameExists(newObj->Name));

  newObj->color = 1;
  ContObj->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!

  Sandwich->convertObjToComposite(newObj);

  QString name = newObj->Name;
  UpdateGui(name);
  emit RequestRebuildDetector();
  UpdateGui(name);
}

void AGeoTreeWidget::SetLineAttributes(QString ObjectName)
{
    AGeoObject* obj = World->findObjectByName(ObjectName);
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
        emit RequestRebuildDetector();
        //UpdateGui(ObjectName);
    }
}

void AGeoTreeWidget::ShowObject(QString ObjectName)
{
  AGeoObject* obj = World->findObjectByName(ObjectName);
  if (obj)
  {
      fSpecialGeoViewMode = true;
      emit RequestHighlightObject(ObjectName);
      UpdateGui(ObjectName);
  }
}

void AGeoTreeWidget::ShowObjectRecursive(QString ObjectName)
{
    AGeoObject* obj = World->findObjectByName(ObjectName);
    if (obj)
    {
        fSpecialGeoViewMode = true;
        emit RequestShowObjectRecursive(ObjectName);
        UpdateGui(ObjectName);
    }
}

void AGeoTreeWidget::ShowObjectOnly(QString ObjectName)
{
    fSpecialGeoViewMode = true;
    AGeoObject* obj = World->findObjectByName(ObjectName);
    TGeoShape* sh = obj->Shape->createGeoShape();
    sh->Draw();
}

void AGeoTreeWidget::menuActionEnableDisable(QString ObjectName)
{
    AGeoObject* obj = World->findObjectByName(ObjectName);
    if (obj)
    {
        if (obj->isDisabled()) obj->enableUp();
        else
        {
            obj->fActive = false;
            if (obj->ObjectType->isSlab()) obj->getSlabModel()->fActive = false;
        }

        obj->fExpanded = obj->fActive;

        QString name = obj->Name;
        emit RequestRebuildDetector();
        UpdateGui(name);
    }
}

void AGeoTreeWidget::formSet(QList<QTreeWidgetItem*> selected, int option) //option 0->group, option 1->stack
{
  //creating a set
  AGeoObject* grObj = new AGeoObject();

  delete grObj->ObjectType;
  if (option == 0)
      grObj->ObjectType = new ATypeGroupContainerObject();
  else
      grObj->ObjectType = new ATypeStackContainerObject();

  do
    grObj->Name = (option==0) ? AGeoObject::GenerateRandomGroupName() : AGeoObject::GenerateRandomStackName();
  while (World->isNameExists(grObj->Name));
  //qDebug() << "--Created new set object:"<<grObj->Name;

  QString FirstName = selected.first()->text(0); //selected objects always have the same container
  AGeoObject* firstObj = World->findObjectByName(FirstName);
  if (!firstObj) return;
  AGeoObject* contObj = firstObj->Container;
  grObj->Container = contObj;
  //qDebug() << "--Group will be hosted by:"<<grObj->Container->Name;

  for (int i=0; i<selected.size(); i++)
    {
      QString name = selected.at(i)->text(0);
      AGeoObject* obj = World->findObjectByName(name);
      if (!obj) continue;
      contObj->HostedObjects.removeOne(obj);
      obj->Container = grObj;
      grObj->HostedObjects.append(obj);
    }
  contObj->HostedObjects.insert(0, grObj);

  QString name = grObj->Name;
  if (option == 1)
  {
    firstObj->updateStack();
    emit RequestRebuildDetector();
  }
  //qDebug() << "--Done! updating gui";
  UpdateGui(name);
}

void AGeoTreeWidget::addLightguide(bool upper)
{
    AGeoObject* obj = Sandwich->addLightguide(upper);

    //qDebug() << "Done, rebuidling detector and refreshing gui";
    QString name = obj->Name;
    UpdateGui(name);
    qApp->processEvents();
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
      if (cont->ObjectType->isGroup())
      {
          if (obj == cont->HostedObjects.first())
              image = GroupStart;
          else if (obj == cont->HostedObjects.last())
              image = GroupEnd;
          else
              image = GroupMid;
      }
      else if (cont->ObjectType->isStack())
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


// ================== EDIT WIDGET ===================

AGeoWidget::AGeoWidget(AGeoObject *World, AGeoTreeWidget *tw) :
  World(World), tw(tw)
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
    //qDebug() << "Object widget clear triggered!";
    fIgnoreSignals = true;

    while(ObjectLayout->count() > 0)
    {
        QLayoutItem* item = ObjectLayout->takeAt(0);
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
    //qDebug() << "UpdateGui triggered";
    ClearGui(); //deletes Delegate!

    if (!CurrentObject) return;

    pbConfirm->setEnabled(true);
    pbCancel->setEnabled(true);

    //AGeoObject* contObj = CurrentObject->Container;
    //if (!contObj) return; //true only for World

    if (CurrentObject->ObjectType->isWorld())
        GeoDelegate = new AWorldDelegate(tw->Sandwich->Materials, this);
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
}

AGeoBaseDelegate * AGeoWidget::createAndAddGeoObjectDelegate()
{
    AGeoObjectDelegate * Del;

    AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(CurrentObject->Shape);
    const QString shape = (scaled ? scaled->getBaseShapeType() : CurrentObject->Shape->getShapeType());

    if (CurrentObject->ObjectType->isArray())
        Del = new AGeoArrayDelegate(tw->Sandwich->Materials, this);
    else if (CurrentObject->ObjectType->isHandlingSet())
        Del = new AGeoSetDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoBBox")
        Del = new AGeoBoxDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoTube")
        Del = new AGeoTubeDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoTubeSeg")
        Del = new AGeoTubeSegDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoCtub")
        Del = new AGeoTubeSegCutDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoEltu")
        Del = new AGeoElTubeDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoPara")
        Del = new AGeoParaDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoSphere")
        Del = new AGeoSphereDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoTrd1")
        Del = new AGeoTrapXDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoTrd2")
        Del = new AGeoTrapXYDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoCone")
        Del = new AGeoConeDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoConeSeg")
        Del = new AGeoConeSegDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoParaboloid")
        Del = new AGeoParaboloidDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoTorus")
        Del = new AGeoTorusDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoPolygon")
        Del = new AGeoPolygonDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoPcon")
        Del = new AGeoPconDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoPgon")
        Del = new AGeoPgonDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoCompositeShape")
        Del = new AGeoCompositeDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoArb8")
        Del = new AGeoArb8Delegate(tw->Sandwich->Materials, this);
    else
        Del = new AGeoObjectDelegate(tw->Sandwich->Materials, this);

    connect(Del, &AGeoObjectDelegate::RequestChangeShape, this, &AGeoWidget::onRequestChangeShape);

    return Del;
}

AGeoBaseDelegate * AGeoWidget::createAndAddSlabDelegate()
{
    AGeoSlabDelegate * Del = new AGeoSlabDelegate(tw->Sandwich->Materials, static_cast<int>(tw->Sandwich->SandwichState), this);
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

void AGeoWidget::onObjectSelectionChanged(const QString SelectedObject)
{  
    CurrentObject = nullptr;
    //qDebug() << "Object selection changed! ->" << SelectedObject;
    ClearGui();

    AGeoObject* obj = World->findObjectByName(SelectedObject);
    if (!obj) return;

    //if (obj->ObjectType->isWorld()) return;

    CurrentObject = obj;
    //qDebug() << "New current object:"<<CurrentObject->Name;
    UpdateGui();
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
      QFont f = pbConfirm->font();
      f.setBold(true);
      pbConfirm->setFont(f);
      pbConfirm->setStyleSheet("QPushButton {color: red;}");
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

/*
void AGeoWidget::OnCustomContextMenuTriggered_forMainObject(QPoint pos)
{
  if (!CurrentObject) return;

  QMenu Menu;

  QAction* convertToCompositeA = 0;
  QAction* convertToUpperLGA = 0;
  QAction* convertToLowerLGA = 0;

  Menu.addSeparator();
  QAction* showA = Menu.addAction("Show object");
  QAction* setLineA = Menu.addAction("Change line color/width/style");
  Menu.addSeparator();
  QAction* clipA = Menu.addAction("Object generation command -> clipboard");
  Menu.addSeparator();
  if (CurrentObject->ObjectType->isSingle())
    {
      //menu triggered on individual object
      convertToCompositeA = Menu.addAction("Convert to composite object");
      convertToCompositeA->setEnabled(!CurrentObject->fLocked && CurrentObject->ObjectType->isSingle() );
    }
  if (CurrentObject->ObjectType->isSlab() && !CurrentObject->ObjectType->isLightguide())
    {
      convertToUpperLGA = Menu.addAction("Convert to upper lightguide");
      convertToUpperLGA->setEnabled(CurrentObject->isFirstSlab() && !CurrentObject->fLocked);
    }
  if (CurrentObject->ObjectType->isSlab() && !CurrentObject->ObjectType->isLightguide() )
    {
      convertToLowerLGA = Menu.addAction("Convert to lower lightguide");
      convertToLowerLGA->setEnabled(CurrentObject->isLastSlab() && !CurrentObject->fLocked );
    }
  Menu.addSeparator();


  QAction* SelectedAction = Menu.exec(mapToGlobal(pos));
  if (!SelectedAction) return; //nothing was selected

  // Convert to composite
  if (SelectedAction == convertToCompositeA)
    {
      tw->Sandwich->convertObjToComposite(CurrentObject);
      QString name = CurrentObject->Name;
      UpdateGui();
      emit tw->RequestRebuildDetector();
      tw->UpdateGui(name);
    }
  else if (SelectedAction == convertToUpperLGA)
  {
      tw->Sandwich->convertObjToLightguide(CurrentObject, true);
      QString name = CurrentObject->Name;
      emit tw->RequestRebuildDetector();
      tw->UpdateGui(name);
  }
  else if (SelectedAction == convertToLowerLGA)
  {
      tw->Sandwich->convertObjToLightguide(CurrentObject, false);
      QString name = CurrentObject->Name;
      emit tw->RequestRebuildDetector();
      tw->UpdateGui(name);
  }
  else if (SelectedAction == showA) onRequestShowCurrentObject();
  else if (SelectedAction == clipA) onRequestScriptLineToClipboard();
  else if (SelectedAction == setLineA) onRequestSetVisAttributes();
}
*/

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
    emit requestBuildScript(CurrentObject, script, 0, false, !bNotRecursive);

    qDebug() << script;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(script);
}

void AGeoWidget::onRequestScriptRecursiveToClipboard()
{
    if (!CurrentObject) return;

    QString script;
    emit requestBuildScript(CurrentObject, script, 0, false, true);

    qDebug() << script;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(script);
}

void AGeoWidget::onRequestSetVisAttributes()
{
    if (!CurrentObject) return;

    tw->SetLineAttributes(CurrentObject->Name);
}

void AGeoWidget::onMonitorRequestsShowSensitiveDirection()
{
    emit showMonitor(CurrentObject);
}

void AGeoWidget::exitEditingMode()
{
    fEditingMode = false;
    tw->setEnabled(true);
    QFont f = pbConfirm->font();
    f.setBold(false);
    pbConfirm->setFont(f);
    pbConfirm->setStyleSheet("QPushButton {color: black;}");
    pbConfirm->setEnabled(false);
    pbCancel->setEnabled(false);
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

    //    qDebug() << "Validating update data for object" << CurrentObject->Name;
    bool ok = checkDelegateValidity();
    if (!ok) return;

    GeoDelegate->updateObject(CurrentObject);

    AWorldDelegate * del = dynamic_cast<AWorldDelegate*>(GeoDelegate);
    if (del)
    {
        AGeoBox * box = static_cast<AGeoBox*>(World->Shape);
        double WorldSizeXY = box->dx;
        double WorldSizeZ  = box->dz;
        ATypeWorldObject * typeWorld = static_cast<ATypeWorldObject *>(World->ObjectType);
        bool fWorldSizeFixed = typeWorld->bFixedSize;
        emit tw->RequestUpdateWorldSize(WorldSizeXY, WorldSizeZ, fWorldSizeFixed);
    }

    exitEditingMode();
    QString name = CurrentObject->Name;
    //tw->UpdateGui(name);
    emit tw->RequestRebuildDetector();
    tw->UpdateGui(name);
}

bool AGeoWidget::checkDelegateValidity()
{
    const QString newName = GeoDelegate->getName();
    if (newName != CurrentObject->Name && World->isNameExists(newName))
    {
        QMessageBox::warning(this, "", QString("%1 name already exists").arg(newName));
        return false;
    }

    return GeoDelegate->isValid(CurrentObject);
}

void AGeoWidget::onCancelPressed()
{
  exitEditingMode();
  tw->UpdateGui( (CurrentObject) ? CurrentObject->Name : "" );
}

// ---- base delegate ----

AGeoBaseDelegate::AGeoBaseDelegate(QWidget * ParentWidget) :
    ParentWidget(ParentWidget) {}

QHBoxLayout * AGeoBaseDelegate::createBottomButtons()
{
    QHBoxLayout * abl = new QHBoxLayout();

    pbShow = new QPushButton("Show");
    QObject::connect(pbShow, &QPushButton::clicked, this, &AGeoObjectDelegate::RequestShow);
    abl->addWidget(pbShow);
    pbChangeAtt = new QPushButton("Color/line");
    QObject::connect(pbChangeAtt, &QPushButton::clicked, this, &AGeoObjectDelegate::RequestChangeVisAttributes);
    abl->addWidget(pbChangeAtt);
    pbScriptLine = new QPushButton("Script to clipboard");
    pbScriptLine->setToolTip("Click right mouse button to generate script recursively");
    QObject::connect(pbScriptLine, &QPushButton::clicked, this, &AGeoObjectDelegate::RequestScriptToClipboard);
    pbScriptLine->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(pbScriptLine, &QPushButton::customContextMenuRequested, this, &AGeoObjectDelegate::RequestScriptRecursiveToClipboard);
    abl->addWidget(pbScriptLine);

    return abl;
}

// ---- object delegate ----

AGeoObjectDelegate::AGeoObjectDelegate(const QStringList & materials, QWidget * ParentWidget) :
    AGeoBaseDelegate(ParentWidget)
{
  QFrame * frMainFrame = new QFrame();
  frMainFrame->setFrameShape(QFrame::Box);

  Widget = frMainFrame;
  Widget->setContextMenuPolicy(Qt::CustomContextMenu);

  QPalette palette = frMainFrame->palette();
  palette.setColor( Widget->backgroundRole(), QColor( 255, 255, 255 ) );
  frMainFrame->setPalette( palette );
  frMainFrame->setAutoFillBackground( true );
  lMF = new QVBoxLayout();
    lMF->setContentsMargins(5,5,5,2);

    //object type
    labType = new QLabel("");
    labType->setAlignment(Qt::AlignCenter);
    QFont font = labType->font();
    font.setBold(true);
    labType->setFont(font);
    lMF->addWidget(labType);

    //name and material line
    QHBoxLayout* hl = new QHBoxLayout();
      hl->setContentsMargins(2,0,2,0);
      QLabel* lname = new QLabel();
      lname->setText("Name:");
      lname->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      lname->setMaximumWidth(50);
      hl->addWidget(lname);
      leName = new QLineEdit();
      connect(leName, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onContentChanged);
      leName->setMaximumWidth(200);
      leName->setContextMenuPolicy(Qt::NoContextMenu);
      hl->addWidget(leName);
      lMat = new QLabel();
      lMat->setText("Material:");
      lMat->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      lname->setMaximumWidth(60);
      hl->addWidget(lMat);
      cobMat = new QComboBox();
      cobMat->setContextMenuPolicy(Qt::NoContextMenu);
      cobMat->addItems(materials);
      connect(cobMat, SIGNAL(activated(int)), this, SLOT(onContentChanged()));
      //cobMat->setMaximumWidth(150);
      cobMat->setMinimumWidth(120);
      hl->addWidget(cobMat);
    lMF->addLayout(hl);

    //Shape box
    QHBoxLayout* h2 = new QHBoxLayout();
      h2->setContentsMargins(2,0,2,0);

      pteShape = new QPlainTextEdit();
      pteShape->setContextMenuPolicy(Qt::NoContextMenu);
      connect(pteShape, SIGNAL(textChanged()), this, SLOT(onContentChanged()));
      pteShape->setMaximumHeight(50);
      new AShapeHighlighter(pteShape->document());
      h2->addWidget(pteShape);
    lMF->addLayout(h2);

    //scale widget
    QHBoxLayout * hbs = new QHBoxLayout();
    hbs->setContentsMargins(2,0,2,0);
        hbs->addStretch();
        cbScale = new QCheckBox("Apply scaling factors");
        cbScale->setToolTip("Use scaling only if it is the only choice, e.g. to make ellipsoid from a sphere");
        connect(cbScale, &QCheckBox::clicked, this, &AGeoObjectDelegate::onLocalShapeParameterChange);
        connect(cbScale, &QCheckBox::clicked, this, &AGeoObjectDelegate::onContentChanged);
        hbs->addWidget(cbScale);
    scaleWidget = new QWidget();
        QHBoxLayout * hbsw = new QHBoxLayout(scaleWidget);
        hbsw->setContentsMargins(2,0,2,0);
        hbsw->addWidget(new QLabel("in X:"));
        ledScaleX = new QLineEdit("1.0"); hbsw->addWidget(ledScaleX);
        connect(ledScaleX, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onLocalShapeParameterChange);
        connect(ledScaleX, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onContentChanged);
        hbsw->addWidget(new QLabel("in Y:"));
        ledScaleY = new QLineEdit("1.0"); hbsw->addWidget(ledScaleY);
        connect(ledScaleY, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onLocalShapeParameterChange);
        connect(ledScaleY, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onContentChanged);
        hbsw->addWidget(new QLabel("in Z:"));
        ledScaleZ = new QLineEdit("1.0"); hbsw->addWidget(ledScaleZ);
        connect(ledScaleZ, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onLocalShapeParameterChange);
        connect(ledScaleZ, &QLineEdit::textChanged, this, &AGeoObjectDelegate::onContentChanged);
    hbs->addWidget(scaleWidget);
    hbs->addStretch();
    lMF->addLayout(hbs);
    scaleWidget->setVisible(false);
    QObject::connect(cbScale, &QCheckBox::toggled, scaleWidget, &QWidget::setVisible);

    // Transform block
    QHBoxLayout * lht = new QHBoxLayout();
        pbTransform = new QPushButton("Transform to ...");
        lht->addWidget(pbTransform);
        connect(pbTransform, &QPushButton::pressed, this, &AGeoObjectDelegate::onChangeShapePressed);
        pbShapeInfo = new QPushButton("Info on this shape");
        connect(pbShapeInfo, &QPushButton::pressed, this, &AGeoObjectDelegate::onHelpRequested);
        lht->addWidget(pbShapeInfo);
    lMF->addLayout(lht);

    // Position and orientation block
    PosOrient = new QWidget();
    PosOrient->setContentsMargins(0,0,0,0);
    PosOrient->setMaximumHeight(80);
    QGridLayout *gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);
      ledX = new QLineEdit();
      ledX->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledX, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledX, 0, 1);
      ledY = new QLineEdit();
      ledY->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledY, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledY, 1, 1);
      ledZ = new QLineEdit();
      ledZ->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledZ, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledZ, 2, 1);

      ledPhi = new QLineEdit();
      ledPhi->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledPhi, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledPhi, 0, 3);
      ledTheta = new QLineEdit();
      ledTheta->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledTheta, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledTheta, 1, 3);
      ledPsi = new QLineEdit();
      ledPsi->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledPsi, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      gr->addWidget(ledPsi, 2, 3);

      QLabel *l = new QLabel("X:");
      gr->addWidget(l, 0, 0);
      l = new QLabel("Y:");
      gr->addWidget(l, 1, 0);
      l = new QLabel("Z:");
      gr->addWidget(l, 2, 0);

      l = new QLabel("mm    Phi:");
      gr->addWidget(l, 0, 2);
      l = new QLabel("mm  Theta:");
      gr->addWidget(l, 1, 2);
      l = new QLabel("mm    Psi:");
      gr->addWidget(l, 2, 2);

      l = new QLabel("°");
      gr->addWidget(l, 0, 4);
      l = new QLabel("°");
      gr->addWidget(l, 1, 4);
      l = new QLabel("°");
      gr->addWidget(l, 2, 4);

    PosOrient->setLayout(gr);
    lMF->addWidget(PosOrient);

    // bottom line buttons
    QHBoxLayout * abl = createBottomButtons();
    lMF->addLayout(abl);

  frMainFrame->setLayout(lMF);

  //installing double validators for edit boxes
  QDoubleValidator* dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);
  ledX->setValidator(dv);
  ledY->setValidator(dv);
  ledZ->setValidator(dv);
  ledPhi->setValidator(dv);
  ledTheta->setValidator(dv);
  ledPsi->setValidator(dv);
}

const QString AGeoObjectDelegate::getName() const
{
    return leName->text();
}

bool AGeoObjectDelegate::isValid(AGeoObject * obj)
{
    if ( obj->ObjectType->isHandlingSet())
    {
        //for Set object there is no shape to check
    }
    else
    {
        // this is normal or composite object then
        //if composite, first check all members
        QString newShape = pteShape->document()->toPlainText();
        //qDebug() << "--> attempt to set shape using string:"<< newShape;

        bool fValid = obj->readShapeFromString(newShape, true); //only checks, no change!
        if (!fValid)
        {
            message(newShape.simplified().startsWith("TGeoArb8") ?
                        "Error parsing shape!\nIt could be non-clockwise order of defined nodes!" :
                        "Error parsing shape!", ParentWidget);
            return false;
        }
    }
    return true;
}

void AGeoObjectDelegate::updateObject(AGeoObject * obj) const
{
    const QString oldName = obj->Name;
    const QString newName = leName->text();
    obj->Name = newName;

    if ( obj->ObjectType->isHandlingSet() )
    {
        //set container object does not have material and shape
    }
    else
    {
        obj->Material = cobMat->currentIndex();
        if (obj->Material == -1) obj->Material = 0; //protection

        //inherit materials for composite members
        if (obj->isCompositeMemeber())
        {
            if (obj->Container && obj->Container->Container)
                obj->Material = obj->Container->Container->Material;
        }

        QString newShape = pteShape->document()->toPlainText();
        //qDebug() << "Geting shape values from the delegate"<<newShape;
        obj->readShapeFromString(newShape);

        //if it is a set member, need old values of position and angle
        QVector<double> old;
        old << obj->Position[0]    << obj->Position[1]    << obj->Position[2]
                << obj->Orientation[0] << obj->Orientation[1] << obj->Orientation[2];

        /*
        const AGeoConsts& gConsts = AGeoConsts::getConstInstance();
        qDebug() << obj->Position[0];

        gConsts.evaluateFormula(ledX->text(), obj->Position[0]);
        qDebug() << obj->Position[0];
        */

        obj->Position[0] = ledX->text().toDouble();
        obj->Position[1] = ledY->text().toDouble();
        obj->Position[2] = ledZ->text().toDouble();
        obj->Orientation[0] = ledPhi->text().toDouble();
        obj->Orientation[1] = ledTheta->text().toDouble();
        obj->Orientation[2] = ledPsi->text().toDouble();


        // checking was there a rotation of the main object
        bool fWasRotated = false;
        for (int i=0; i<3; i++)
            if (obj->Orientation[i] != old[3+i])
            {
                fWasRotated =true;
                break;
            }
        //qDebug() << "--Was rotated?"<< fWasRotated;

        //for grouped object, taking into accound the shift
        if (obj->Container && obj->Container->ObjectType->isGroup())
        {
            for (int iObj = 0; iObj < obj->Container->HostedObjects.size(); iObj++)
            {
                AGeoObject* hostedObj = obj->Container->HostedObjects[iObj];
                if (hostedObj == obj) continue;

                //center vector for rotation
                //in TGeoRotation, first rotation iz around Z, then new X(manual is wrong!) and finally around new Z
                TVector3 v(hostedObj->Position[0]-old[0], hostedObj->Position[1]-old[1], hostedObj->Position[2]-old[2]);

                //first rotate back to origin in rotation
                rotate(v, -old[3+0], 0, 0);
                rotate(v, 0, -old[3+1], 0);
                rotate(v, 0, 0, -old[3+2]);
                rotate(v, obj->Orientation[0], obj->Orientation[1], obj->Orientation[2]);

                for (int i=0; i<3; i++)
                {
                    double delta = obj->Position[i] - old[i]; //shift in position

                    if (fWasRotated)
                    {
                        //shift due to rotation  +  global shift
                        hostedObj->Position[i] = old[i]+v[i] + delta;
                        //rotation of the object
                        double deltaAng = obj->Orientation[i] - old[3+i];
                        hostedObj->Orientation[i] += deltaAng;
                    }
                    else
                        hostedObj->Position[i] += delta;
                }
            }
        }
        //for stack:
        if (obj->Container && obj->Container->ObjectType->isStack())
            obj->updateStack();
    }

    //additional post-processing
    if ( obj->ObjectType->isArray() )
    {
        //additional properties for array
        ATypeArrayObject* array = static_cast<ATypeArrayObject*>(obj->ObjectType);
        array->numX = sbNumX->value();
        array->numY = sbNumY->value();
        array->numZ = sbNumZ->value();
        array->stepX = ledStepX->text().toDouble();
        array->stepY = ledStepY->text().toDouble();
        array->stepZ = ledStepZ->text().toDouble();
    }
    else if (obj->ObjectType->isComposite())
    {
        AGeoObject* logicals = obj->getContainerWithLogical();
        if (logicals)
            logicals->Name = "CompositeSet_"+obj->Name;
    }
    else if (obj->ObjectType->isGrid())
    {
        AGeoObject* GE = obj->getGridElement();
        if (GE)
        {
            GE->Name = "GridElement_"+obj->Name;
            GE->Material = obj->Material;
        }
    }
    else if (obj->isCompositeMemeber())
    {
        AGeoObject* cont = obj->Container;
        if (cont)
        {
            if (cont->ObjectType->isCompositeContainer())
            {
                AGeoObject* Composite = cont->Container;
                if (Composite)
                { //updating Shape
                    AGeoComposite* cs = dynamic_cast<AGeoComposite*>(Composite->Shape);
                    if (cs)
                    {
                        cs->members.replaceInStrings(oldName, newName);
                        cs->GenerationString.replace(oldName, newName);
                    }
                }
            }
        }
    }
}

void AGeoObjectDelegate::onChangeShapePressed()
{
    QDialog * d = new QDialog(ParentWidget);
    d->setWindowTitle("Select new shape");

    QStringList list;
    list << "Box"
         << "Tube" << "Tube segment" << "Tube segment cut" << "Tube elliptical"
         << "Trapezoid simplified" << "Trapezoid"
         << "Polycone"
         << "Polygon simplified" << "Polygon"
         << "Parallelepiped"
         << "Sphere"
         << "Cone" << "Cone segment"
         << "Torus"
         << "Paraboloid"
         << "Arb8";

    QVBoxLayout * l = new QVBoxLayout(d);
        QListWidget * w = new QListWidget();
        w->addItems(list);
    l->addWidget(w);
    d->resize(d->width(), 400);

        QHBoxLayout * h = new QHBoxLayout();
            QPushButton * pbAccept = new QPushButton("Change shape");
            pbAccept->setEnabled(false);
            QObject::connect(pbAccept, &QPushButton::clicked, [this, d, w](){this->onShapeDialogActivated(d, w);});
            QObject::connect(w, &QListWidget::activated, [this, d, w](){this->onShapeDialogActivated(d, w);});
            h->addWidget(pbAccept);
            QPushButton * pbCancel = new QPushButton("Cancel");
            QObject::connect(pbCancel, &QPushButton::clicked, d, &QDialog::reject);
            h->addWidget(pbCancel);
    l->addLayout(h);

    l->addWidget(new QLabel("Warning! There is no undo after change!"));

    QObject::connect(w, &QListWidget::itemSelectionChanged, [pbAccept, w]()
    {
        pbAccept->setEnabled(w->currentRow() != -1);
    });

    d->exec();
    delete d;
}

void AGeoObjectDelegate::addLocalLayout(QLayout * lay)
{
    lMF->insertLayout(3, lay);
}

void AGeoObjectDelegate::updatePteShape(const QString & text)
{
    pteShape->clear();

    QString str = text;
    if (cbScale->isChecked())
        str = QString("TGeoScaledShape( %1, %2, %3, %4 )").arg(text).arg(ledScaleX->text()).arg(ledScaleY->text()).arg(ledScaleZ->text());

    pteShape->appendPlainText(str);
    pbShapeInfo->setToolTip(pteShape->document()->toPlainText());
}

const AGeoShape * AGeoObjectDelegate::getBaseShapeOfObject(const AGeoObject * obj)
{
    if (!obj && !obj->Shape) return nullptr;
    AGeoScaledShape * scaledShape = dynamic_cast<AGeoScaledShape*>(obj->Shape);
    if (!scaledShape) return nullptr;

    AGeoShape * baseShape = AGeoObject::GeoShapeFactory(scaledShape->getBaseShapeType());
    bool bOK = baseShape->readFromString( scaledShape->BaseShapeGenerationString );
    if (!bOK) qDebug() << "Failed to read shape properties:" << scaledShape->BaseShapeGenerationString;
    return baseShape;
}

void AGeoObjectDelegate::updateTypeLabel()
{
    if (CurrentObject->ObjectType->isGrid())
        DelegateTypeName = "Grid bulk, " + DelegateTypeName;

    if (CurrentObject->isCompositeMemeber())
        DelegateTypeName += " (logical)";
    else if (CurrentObject->Container)
    {
        if (CurrentObject->Container->ObjectType->isHandlingSet())
        {
            if (CurrentObject->Container->ObjectType->isGroup())
                DelegateTypeName += ",   groupped";
            else
                DelegateTypeName += ",   stacked";
        }
    }

    labType->setText(DelegateTypeName);
}

void AGeoObjectDelegate::updateControlUI()
{
    pteShape->setVisible(false);

    if (CurrentObject->ObjectType->isHandlingSet())
    {
        pteShape->setVisible(false);
        lMat->setVisible(false);
        cobMat->setVisible(false);
        PosOrient->setVisible(false);
    }

    if (CurrentObject->Container && CurrentObject->Container->ObjectType->isStack())
    {
        ledPhi->setEnabled(false);
        ledTheta->setEnabled(false);
        ledPhi->setText("0");
        ledTheta->setText("0");
    }

    if (CurrentObject->isCompositeMemeber())
    {
        cobMat->setEnabled(false);
        cobMat->setCurrentIndex(-1);
    }

    if (CurrentObject->isCompositeMemeber())
    {
        pbShow->setVisible(false);
        pbChangeAtt->setVisible(false);
        pbScriptLine->setVisible(false);
    }
}

void AGeoObjectDelegate::rotate(TVector3 & v, double dPhi, double dTheta, double dPsi) const
{
    v.RotateZ( TMath::Pi()/180.0* dPhi );
    TVector3 X(1,0,0);
    X.RotateZ( TMath::Pi()/180.0* dPhi );
    //v.RotateX( TMath::Pi()/180.0* Theta);
    v.Rotate( TMath::Pi()/180.0* dTheta, X);
    TVector3 Z(0,0,1);
    Z.Rotate( TMath::Pi()/180.0* dTheta, X);
    // v.RotateZ( TMath::Pi()/180.0* Psi );
    v.Rotate( TMath::Pi()/180.0* dPsi, Z );
}

void AGeoObjectDelegate::onShapeDialogActivated(QDialog * d, QListWidget * w)
{
    const QString sel = w->currentItem()->text();
    if      (sel == "Box")                  emit RequestChangeShape(new AGeoBox());
    else if (sel == "Parallelepiped")       emit RequestChangeShape(new AGeoPara());
    else if (sel == "Tube")                 emit RequestChangeShape(new AGeoTube());
    else if (sel == "Tube segment")         emit RequestChangeShape(new AGeoTubeSeg());
    else if (sel == "Tube segment cut")     emit RequestChangeShape(new AGeoCtub());
    else if (sel == "Tube elliptical")      emit RequestChangeShape(new AGeoEltu());
    else if (sel == "Sphere")               emit RequestChangeShape(new AGeoSphere());
    else if (sel == "Trapezoid simplified") emit RequestChangeShape(new AGeoTrd1());
    else if (sel == "Trapezoid")            emit RequestChangeShape(new AGeoTrd2());
    else if (sel == "Cone")                 emit RequestChangeShape(new AGeoCone());
    else if (sel == "Cone segment")         emit RequestChangeShape(new AGeoConeSeg());
    else if (sel == "Paraboloid")           emit RequestChangeShape(new AGeoParaboloid());
    else if (sel == "Torus")                emit RequestChangeShape(new AGeoTorus());
    else if (sel == "Polycone")             emit RequestChangeShape(new AGeoPcon());
    else if (sel == "Polygon simplified")   emit RequestChangeShape(new AGeoPolygon());
    else if (sel == "Polygon")              emit RequestChangeShape(new AGeoPgon());
    else if (sel == "Arb8")                 emit RequestChangeShape(new AGeoArb8());
    else
    {
        //nothing selected or unknown shape
        return;
    }
    d->accept();
}

void AGeoObjectDelegate::onHelpRequested()
{
    message(ShapeHelp, Widget);

    /*
    QList<AGeoShape*> AvailableShapes = AGeoObject::GetAvailableShapes();

    AShapeHelpDialog* d = new AShapeHelpDialog(CurrentObject->Shape, AvailableShapes, this);

    int res = d->exec();
    if (res == 1)
    {
        QString templ = d->SelectedTemplate;
        if (templ.startsWith("TGeoComposite") && CurrentObject->Shape->getShapeType()!="AGeoComposite")
        {
            QMessageBox::information(Widget, "", "Cannot convert to composite in this way:\nUse context menu of the left panel.");
            return;
        }

        pteShape->clear();
        pteShape->appendPlainText(templ);
    }

    for (int i=0; i<AvailableShapes.size(); i++) delete AvailableShapes[i];
    delete d;
    */
}

void AGeoObjectDelegate::Update(const AGeoObject *obj)
{
    CurrentObject = obj;
    leName->setText(obj->Name);

    int imat = obj->Material;
    if (imat < 0 || imat >= cobMat->count())
    {
        qWarning() << "Material index out of bounds!";
        imat = -1;
    }
    cobMat->setCurrentIndex(imat);

    pteShape->setPlainText(obj->Shape->getGenerationString());

    ledX->setText(QString::number(obj->Position[0]));
    ledY->setText(QString::number(obj->Position[1]));
    ledZ->setText(QString::number(obj->Position[2]));

    ledPhi->setText(QString::number(obj->Orientation[0]));
    ledTheta->setText(QString::number(obj->Orientation[1]));
    ledPsi->setText(QString::number(obj->Orientation[2]));

    updateTypeLabel();
    updateControlUI();

    AGeoScaledShape * scaledShape = dynamic_cast<AGeoScaledShape*>(CurrentObject->Shape);
    cbScale->setChecked(scaledShape);
    if (scaledShape)
    {
        ledScaleX->setText(QString::number(scaledShape->scaleX));
        ledScaleY->setText(QString::number(scaledShape->scaleY));
        ledScaleZ->setText(QString::number(scaledShape->scaleZ));
    }
}

void AGeoObjectDelegate::onContentChanged()
{
    pbShapeInfo->setToolTip(pteShape->document()->toPlainText());
    emit ContentChanged();
    //qDebug() <<pteShape->document()->toPlainText();
}

AShapeHighlighter::AShapeHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
  HighlightingRule rule;
  ShapeFormat.setForeground(Qt::blue);
  ShapeFormat.setFontWeight(QFont::Bold);

  QList<AGeoShape*> AvailableShapes = AGeoObject::GetAvailableShapes();
  QStringList ShapePatterns;
  while (!AvailableShapes.isEmpty())
  {
      ShapePatterns << AvailableShapes.first()->getShapeType();
      delete AvailableShapes.first();
      AvailableShapes.removeFirst();
  }
  //ShapePatterns << "\\bTGeoBBox\\b" << "\\bTGeoPara\\b" << "\\bTGeoSphere\\b" << "\\bTGeoTube\\b"
  //              << "\\bTGeoCompositeShape\\b" << "\\bTGeoTrd1\\b";// << "\\bfor\\b"
  foreach (const QString &pattern, ShapePatterns)
    {
      rule.pattern = QRegExp(pattern);
      rule.format = ShapeFormat;
      highlightingRules.append(rule);
    }
}

void AShapeHighlighter::highlightBlock(const QString &text)
{
  foreach (const HighlightingRule &rule, highlightingRules)
    {
      QRegExp expression(rule.pattern);
      int index = expression.indexIn(text);
      while (index >= 0) {
          int length = expression.matchedLength();
          setFormat(index, length, rule.format);
          index = expression.indexIn(text, index + length);
        }
    }
}

AGridElementDelegate::AGridElementDelegate(QWidget * ParentWidget)
    : AGeoBaseDelegate(ParentWidget)
{
    QFrame * frMainFrame = new QFrame();
    frMainFrame->setFrameShape(QFrame::Box);

    Widget = frMainFrame;

    QPalette palette = frMainFrame->palette();
    palette.setColor( frMainFrame->backgroundRole(), QColor( 255, 255, 255 ) );
    frMainFrame->setPalette( palette );
    frMainFrame->setAutoFillBackground( true );
    //frMainFrame->setMinimumHeight(150);
    //frMainFrame->setMaximumHeight(150);

    QVBoxLayout* vl = new QVBoxLayout();

    //object type
    QLabel * labType = new QLabel("Elementary block of the grid");
    labType->setAlignment(Qt::AlignCenter);
    QFont font = labType->font();
    font.setBold(true);
    labType->setFont(font);
    vl->addWidget(labType);

    QGridLayout *lay = new QGridLayout();
    lay->setContentsMargins(20, 5, 20, 5);
    lay->setVerticalSpacing(3);

      QLabel *la = new QLabel("Shape:");
      lay->addWidget(la, 0, 0);
      lSize1 = new QLabel("dX, mm:");
      lay->addWidget(lSize1, 1, 0);
      la = new QLabel("    dZ, mm:");
      la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      lay->addWidget(la, 0, 2);
      lSize2 = new QLabel("    dY, mm:");
      lSize2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      lay->addWidget(lSize2, 1, 2);

      cobShape = new QComboBox();
      cobShape->addItem("Rectangular");
      cobShape->addItem("Hexagonal");
      connect(cobShape, SIGNAL(activated(int)), this, SLOT(onContentChanged()));
      lay->addWidget(cobShape, 0, 1);

      ledDZ = new QLineEdit();
      connect(ledDZ, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      lay->addWidget(ledDZ, 0, 3);
      ledDX = new QLineEdit();
      connect(ledDX, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      lay->addWidget(ledDX, 1, 1);
      ledDY = new QLineEdit();
      connect(ledDY, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      lay->addWidget(ledDY, 1, 3);

    vl->addLayout(lay);    
    QPushButton* pbInstructions = new QPushButton();
    connect(pbInstructions, SIGNAL(clicked(bool)), this, SLOT(onInstructionsForGridRequested()));
    pbInstructions->setText("Read me");
    pbInstructions->setMinimumWidth(200);
    pbInstructions->setMaximumWidth(200);
    vl->addWidget(pbInstructions);
    vl->setAlignment(pbInstructions, Qt::AlignHCenter);

    QPushButton* pbAuto = new QPushButton();
    connect(pbAuto, SIGNAL(clicked(bool)), this, SLOT(StartDialog()));
    pbAuto->setText("Open generation/edit dialog");
    pbAuto->setMinimumWidth(200);
    pbAuto->setMaximumWidth(200);
    vl->addWidget(pbAuto);
    vl->setAlignment(pbAuto, Qt::AlignHCenter);

    frMainFrame->setLayout(vl);

    //installing double validators for edit boxes
    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    ledDX->setValidator(dv);
    ledDY->setValidator(dv);
    ledDZ->setValidator(dv);
}

const QString AGridElementDelegate::getName() const
{
    if (!CurrentObject) return "Undefined";
    return CurrentObject->Name;
}

bool AGridElementDelegate::isValid(AGeoObject * /*obj*/)
{
    return true;
}

void AGridElementDelegate::updateObject(AGeoObject * obj) const
{
    if (!obj) return;
    if (!obj->ObjectType->isGridElement()) return;

    ATypeGridElementObject* GE = static_cast<ATypeGridElementObject*>(obj->ObjectType);
    int shape = cobShape->currentIndex();
    if (shape == 0)
    {
        if (GE->shape == 2) GE->shape = 1;
    }
    else
    {
        if (GE->shape != 2) GE->shape = 2;
    }
    GE->dz =    ledDZ->text().toDouble();
    GE->size1 = ledDX->text().toDouble();
    GE->size2 = ledDY->text().toDouble();

    obj->updateGridElementShape();
}

void AGridElementDelegate::updateVisibility()
{
    if (cobShape->currentIndex() == 0)
    {  //rectangular
       lSize1->setText("dX, mm:");
       lSize2->setVisible(true);
       ledDY->setVisible(true);
    }
    else
    {
        lSize1->setText("dR, mm:");
        lSize2->setVisible(false);
        ledDY->setVisible(false);
    }
}

void AGridElementDelegate::Update(const AGeoObject *obj)
{
    ATypeGridElementObject* GE = dynamic_cast<ATypeGridElementObject*>(obj->ObjectType);
    if (!GE)
    {
        qWarning() << "Attempt to use non-grid element object to update grid element delegate!";
        return;
    }

    CurrentObject = obj;

    if (GE->shape==0 || GE->shape==1) cobShape->setCurrentIndex(0);
    else cobShape->setCurrentIndex(1);
    updateVisibility();

    ledDZ->setText( QString::number(GE->dz));
    ledDX->setText( QString::number(GE->size1));
    ledDY->setText( QString::number(GE->size2));
}

void AGridElementDelegate::onContentChanged()
{
    updateVisibility();
    emit ContentChanged();
}

void AGridElementDelegate::StartDialog()
{
    if (!CurrentObject->Container) return;
    emit RequestReshapeGrid(CurrentObject->Container->Name);
}

void AGridElementDelegate::onInstructionsForGridRequested()
{
    QString s = "There are two requirements which MUST be fullfiled!\n\n"
                "1. Photons are not allowed to enter objects (\"wires\")\n"
                "   placed within the grid element:\n"
                "   the user has to properly define the optical overrides\n"
                "   for the bulk -> wires interface.\n\n"
                "2. Photons are allowed to leave the grid element only\n"
                "   when they exit the grid object:\n"
                "   the user has to properly position the wires inside the\n"
                "   grid element, so photons cannot cross the border \"sideways\"\n"
                "   \n"
                "Grid generation dialog generates a correct grid element automatically\n"
                "   \n "
                "For modification of all properties of the grid except\n"
                "  - the position/orientation/XYsize of the grid bulk\n"
                "  - materials of the bulk and the wires\n"
                "it is _strongly_ recommended to use the auto-generation dialog.\n"
                "\n"
                "Press the \"Open generation/edit dialog\" button";


    QMessageBox::information(Widget, "", s);
}

AMonitorDelegate::AMonitorDelegate(const QStringList & definedParticles, QWidget * ParentWidget) :
    AGeoBaseDelegate(ParentWidget)
{
    QFrame * frMainFrame = new QFrame();
    frMainFrame->setFrameShape(QFrame::Box);

    Widget = frMainFrame;

    QPalette palette = frMainFrame->palette();
    palette.setColor( frMainFrame->backgroundRole(), QColor( 255, 255, 255 ) );
    frMainFrame->setPalette( palette );
    frMainFrame->setAutoFillBackground( true );
    //frMainFrame->setMinimumHeight(380);
    //frMainFrame->setMaximumHeight(380);
    //frMainFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    QVBoxLayout* vl = new QVBoxLayout();
    vl->setContentsMargins(5,5,5,5);

    //object type
    QLabel * labType = new QLabel("Monitor");
    labType->setAlignment(Qt::AlignCenter);
    QFont font = labType->font();
    font.setBold(true);
    labType->setFont(font);
    vl->addWidget(labType);

    del = new AMonitorDelegateForm(definedParticles, Widget);
    del->UpdateVisibility();
    connect(del, &AMonitorDelegateForm::contentChanged, this, &AMonitorDelegate::onContentChanged);
    connect(del, &AMonitorDelegateForm::showSensDirection, this, &AMonitorDelegate::requestShowSensitiveFaces);
    vl->addWidget(del);

    // bottom line buttons
    QHBoxLayout * abl = createBottomButtons();
    vl->addLayout(abl);

    frMainFrame->setLayout(vl);
}

const QString AMonitorDelegate::getName() const
{
    return del->getName();
}

bool AMonitorDelegate::isValid(AGeoObject * /*obj*/)
{
    return true;
}

void AMonitorDelegate::updateObject(AGeoObject *obj) const
{
    del->updateObject(obj);
}

void AMonitorDelegate::Update(const AGeoObject *obj)
{
    bool bOK = del->updateGUI(obj);
    if (!bOK) return;
}

void AMonitorDelegate::onContentChanged()
{
    emit ContentChanged();
    Widget->layout()->activate();
}

AGeoBoxDelegate::AGeoBoxDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Box";

    ShapeHelp = "A box shape\n"
            "\n"
            "Parameters are the full sizes in X, Y and Z direction\n"
            "\n"
            "The XYZ position is given for the center point\n"
            "\n"
            "Implemented using TGeoBBox(0.5*X_size, 0.5*Y_size, 0.5*Z_size)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("X full size:"), 0, 0);
    gr->addWidget(new QLabel("Y full size:"), 1, 0);
    gr->addWidget(new QLabel("Z full size:"), 2, 0);

    ex = new QLineEdit(); gr->addWidget(ex, 0, 1);
    ey = new QLineEdit(); gr->addWidget(ey, 1, 1);
    ez = new QLineEdit(); gr->addWidget(ez, 2, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {ex, ey, ez};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoBoxDelegate::onLocalShapeParameterChange);
}

void AGeoBoxDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!

    const AGeoBox * box = dynamic_cast<const AGeoBox *>(tmpShape ? tmpShape : obj->Shape);
    if (box)
    {
        ex->setText(QString::number(box->dx*2.0));
        ey->setText(QString::number(box->dy*2.0));
        ez->setText(QString::number(box->dz*2.0));
    }

    delete tmpShape;
}

void AGeoBoxDelegate::onLocalShapeParameterChange()
{

    updatePteShape(QString("TGeoBBox( %1, %2, %3 )").arg(0.5*ex->text().toDouble()).arg(0.5*ey->text().toDouble()).arg(0.5*ez->text().toDouble()));
}

AGeoTubeDelegate::AGeoTubeDelegate(const QStringList & materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Tube";

    ShapeHelp = "A cylinderical shape\n"
            "\n"
            "Parameters are the outer and inner diameters and the full height\n"
            "\n"
            "The XYZ position is given for the center point\n"
            "\n"
            "Implemented using TGeoTube(0.5*Inner_diameter, 0.5*Outer_diameter, 0.5*Height)";

    gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Outer diameter:"), 0, 0);
    gr->addWidget(new QLabel("Inner diameter:"), 1, 0);
    gr->addWidget(new QLabel("Height:"), 2,0);

    eo = new QLineEdit(); gr->addWidget(eo, 0, 1);
    ei = new QLineEdit(); gr->addWidget(ei, 1, 1);
    ez = new QLineEdit(); gr->addWidget(ez, 2, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {eo, ei, ez};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTubeDelegate::onLocalShapeParameterChange);
}

void AGeoTubeDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoTube * tube = dynamic_cast<const AGeoTube*>(tmpShape ? tmpShape : obj->Shape);
    if (tube)
    {
        eo->setText(QString::number(tube->rmax*2.0));
        ei->setText(QString::number(tube->rmin*2.0));
        ez->setText(QString::number(tube->dz*2.0));
    }
    delete tmpShape;
}

void AGeoTubeDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoTube( %1, %2, %3 )").arg(0.5*ei->text().toDouble()).arg(0.5*eo->text().toDouble()).arg(0.5*ez->text().toDouble()));
}

AGeoTubeSegDelegate::AGeoTubeSegDelegate(const QStringList & materials, QWidget * parent) :
    AGeoTubeDelegate(materials, parent)
{
    DelegateTypeName = "Tube segment";

    ShapeHelp = "A cylinderical segment shape\n"
            "\n"
            "Parameters are the outer and inner diameters, the full height\n"
            "and the segment angles from and to\n"
            "\n"
            "The XYZ position is given for the center point of the cylinder\n"
            "\n"
            "Implemented using TGeoTubeSeg(0.5*Inner_diameter, 0.5*Outer_diameter, 0.5*Height, phi_from, phi_to)";

    gr->addWidget(new QLabel("Phi from:"), 3, 0);
    gr->addWidget(new QLabel("Phi to:"), 4, 0);

    ep1 = new QLineEdit(); gr->addWidget(ep1, 3, 1);
    ep2 = new QLineEdit(); gr->addWidget(ep2, 4, 1);

    gr->addWidget(new QLabel("°"), 3, 2);
    gr->addWidget(new QLabel("°"), 4, 2);

    QVector<QLineEdit*> l = {ep1, ep2};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTubeSegDelegate::onLocalShapeParameterChange);
}

void AGeoTubeSegDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoTubeSeg * seg = dynamic_cast<const AGeoTubeSeg*>(tmpShape ? tmpShape : obj->Shape);
    if (seg)
    {
        eo->setText (QString::number(seg->rmax*2.0));
        ei->setText (QString::number(seg->rmin*2.0));
        ez->setText (QString::number(seg->dz*2.0));
        ep1->setText(QString::number(seg->phi1));
        ep2->setText(QString::number(seg->phi2));
    }
    delete tmpShape;
}

void AGeoTubeSegDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoTubeSeg( %1, %2, %3, %4, %5 )").arg(0.5*ei->text().toDouble()).arg(0.5*eo->text().toDouble()).arg(0.5*ez->text().toDouble())
                                                               .arg(ep1->text()).arg(ep2->text()) );
}

AGeoTubeSegCutDelegate::AGeoTubeSegCutDelegate(const QStringList &materials, QWidget *parent) :
    AGeoTubeSegDelegate(materials, parent)
{
    DelegateTypeName = "Tube segment cut";

    ShapeHelp = "A cylinderical segment cut shape\n"
            "\n"
            "Parameters are the outer and inner diameters,\n"
            "the full height in Z direction (from -dz to +dz in local coordinates),\n"
            "the segment angles from and to,\n"
            "and the unit vectors (Nx, Ny, Ny) of the normals for the lower\n"
            "and the upper faces at (0,0,-dz) and (0,0,+dz) local coordinates\n"
            "\n"
            "The XYZ position is given for the point with (0,0,0) local coordinates\n"
            "\n"
            "Implemented using TGeoCtub(0.5*Inner_diameter, 0.5*Outer_diameter, 0.5*Height, phi_from, phi_to, LNx, LNy, LNz, UNx, UNy, UNz)";

    gr->addWidget(new QLabel("Low Nx:"), 5, 0);
    gr->addWidget(new QLabel("Low Ny:"), 6, 0);
    gr->addWidget(new QLabel("Low Nz:"), 7, 0);
    gr->addWidget(new QLabel("Up  Nx:"), 8, 0);
    gr->addWidget(new QLabel("Up  Ny:"), 9, 0);
    gr->addWidget(new QLabel("Up  Nz:"), 10, 0);

    elnx = new QLineEdit(); gr->addWidget(elnx, 5, 1);
    elny = new QLineEdit(); gr->addWidget(elny, 6, 1);
    elnz = new QLineEdit(); gr->addWidget(elnz, 7, 1);
    eunx = new QLineEdit(); gr->addWidget(eunx, 8, 1);
    euny = new QLineEdit(); gr->addWidget(euny, 9, 1);
    eunz = new QLineEdit(); gr->addWidget(eunz, 10, 1);

    QVector<QLineEdit*> l = {elnx, elny, elnz, eunx, euny, eunz};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTubeSegCutDelegate::onLocalShapeParameterChange);
}

void AGeoTubeSegCutDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoCtub * seg = dynamic_cast<const AGeoCtub*>(tmpShape ? tmpShape : obj->Shape);
    if (seg)
    {
        eo->setText (QString::number(seg->rmax*2.0));
        ei->setText (QString::number(seg->rmin*2.0));
        ez->setText (QString::number(seg->dz*2.0));
        ep1->setText(QString::number(seg->phi1));
        ep2->setText(QString::number(seg->phi2));
        elnx->setText(QString::number(seg->nxlow));
        elny->setText(QString::number(seg->nylow));
        elnz->setText(QString::number(seg->nzlow));
        eunx->setText(QString::number(seg->nxhi));
        euny->setText(QString::number(seg->nyhi));
        eunz->setText(QString::number(seg->nzhi));
    }
    delete tmpShape;
}

void AGeoTubeSegCutDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoCtub( %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11 )")
                              .arg(0.5*ei->text().toDouble()).arg(0.5*eo->text().toDouble()).arg(0.5*ez->text().toDouble())
                              .arg(ep1->text()).arg(ep2->text())
                              .arg(elnx->text()).arg(elny->text()).arg(elnz->text())
                              .arg(eunx->text()).arg(euny->text()).arg(eunz->text()) );
}

AGeoParaDelegate::AGeoParaDelegate(const QStringList & materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Parallelepiped";

    ShapeHelp = "A parallelepiped is a shape having 3 pairs of parallel faces\n"
                "  out of which one is parallel with the XY plane (Z face).\n"
                "All faces are parallelograms. The Z faces have 2 edges\n"
                "  parallel to the X-axis.\n"
                "\n"
                "The shape has the center in the origin and is defined by\n"
                "  the full lengths of the projections of the edges on X, Y and Z.\n"
                "The lower Z face is positioned at -0.5*Zsize,\n"
                "  while the upper at +0.5*Zsize.\n"
                "Alpha: angle between the segment defined by the centers of the\n"
                "  X-parallel edges and Y axis [-90,90] in degrees\n"
                "Theta: theta angle of the segment defined by the centers of the Z faces\n"
                "Phi: phi angle of the same segment\n"
                "\n"
                "Implemented using TGeoPara(0.5*X_size, 0.5*Y_size, 0.5*Z_size, Alpha, Theta, Phi)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("X full size:"), 0, 0);
    gr->addWidget(new QLabel("Y full size:"), 1, 0);
    gr->addWidget(new QLabel("Z full size:"), 2, 0);
    gr->addWidget(new QLabel("Alpha:"),     3, 0);
    gr->addWidget(new QLabel("Theta:"),     4, 0);
    gr->addWidget(new QLabel("Phi:"),       5, 0);

    ex = new QLineEdit(); gr->addWidget(ex, 0, 1);
    ey = new QLineEdit(); gr->addWidget(ey, 1, 1);
    ez = new QLineEdit(); gr->addWidget(ez, 2, 1);
    ea = new QLineEdit(); gr->addWidget(ea, 3, 1);
    et = new QLineEdit(); gr->addWidget(et, 4, 1);
    ep = new QLineEdit(); gr->addWidget(ep, 5, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("°"),  3, 2);
    gr->addWidget(new QLabel("°"),  4, 2);
    gr->addWidget(new QLabel("°"),  5, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {ex, ey, ez, ea, et, ep};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoParaDelegate::onLocalShapeParameterChange);
}

void AGeoParaDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoPara * para = dynamic_cast<const AGeoPara*>(tmpShape ? tmpShape : obj->Shape);
    if (para)
    {
        ex->setText(QString::number(para->dx*2.0));
        ey->setText(QString::number(para->dy*2.0));
        ez->setText(QString::number(para->dz*2.0));
        ea->setText(QString::number(para->alpha));
        et->setText(QString::number(para->theta));
        ep->setText(QString::number(para->phi));
    }
    delete tmpShape;
}

void AGeoParaDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoPara( %1, %2, %3, %4, %5, %6 )").arg(0.5*ex->text().toDouble()).arg(0.5*ey->text().toDouble()).arg(0.5*ez->text().toDouble())
                                                                .arg(ea->text()).arg(et->text()).arg(ep->text())  );
}

AGeoSphereDelegate::AGeoSphereDelegate(const QStringList & materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Sphere";

    ShapeHelp = "A spherical sector, defined by\n"
                "the external and internal diameters,\n"
                "and two pairs of polar angles (in degrees):\n"
                "\n"
                "Theta from: [0, 180)\n"
                "Theta to:   (0, 180] with a condition theta_from < theta_to\n"
                "\n"
                "Phi from: [0, 360)\n"
                "Phi to:   (0, 360] with a condition phi_from < phi_to\n"
                "\n"
                "Implemented using TGeoSphere(0.5*Inner_Diameter, 0.5*Outer_Diameter, ThetaFrom, ThetaTo, PhiFrom, PhiTo)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Outer diameter:"), 0, 0);
    gr->addWidget(new QLabel("Inner diameter:"), 1, 0);
    gr->addWidget(new QLabel("Theta from:"), 2, 0);
    gr->addWidget(new QLabel("Theta to:"),     3, 0);
    gr->addWidget(new QLabel("Phi from:"),     4, 0);
    gr->addWidget(new QLabel("Phi to:"),       5, 0);

    eod = new QLineEdit(); gr->addWidget(eod, 0, 1);
    eid = new QLineEdit(); gr->addWidget(eid, 1, 1);
    et1 = new QLineEdit(); gr->addWidget(et1, 2, 1);
    et2 = new QLineEdit(); gr->addWidget(et2, 3, 1);
    ep1 = new QLineEdit(); gr->addWidget(ep1, 4, 1);
    ep2 = new QLineEdit(); gr->addWidget(ep2, 5, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("°"),  2, 2);
    gr->addWidget(new QLabel("°"),  3, 2);
    gr->addWidget(new QLabel("°"),  4, 2);
    gr->addWidget(new QLabel("°"),  5, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {eod, eid, et1, et2, ep1, ep2};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoSphereDelegate::onLocalShapeParameterChange);
}

void AGeoSphereDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoSphere * sph = dynamic_cast<const AGeoSphere*>(tmpShape ? tmpShape : obj->Shape);
    if (sph)
    {
        eid->setText(QString::number(sph->rmin*2.0));
        eod->setText(QString::number(sph->rmax*2.0));
        et1->setText(QString::number(sph->theta1));
        et2->setText(QString::number(sph->theta2));
        ep1->setText(QString::number(sph->phi1));
        ep2->setText(QString::number(sph->phi2));
    }
    delete tmpShape;
}

void AGeoSphereDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoSphere( %1, %2, %3, %4, %5, %6 )").arg(0.5*eid->text().toDouble()).arg(0.5*eod->text().toDouble())
                                                                  .arg(et1->text()).arg(et2->text())
                                                                  .arg(ep1->text()).arg(ep2->text())  );
}

AGeoConeDelegate::AGeoConeDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Cone";

    ShapeHelp = "A conical shape with the axis parallel to Z,\n"
                "  limited by two XY planes,\n"
                "  one at Z = -0.5*height, and\n"
                "  the other at Z = +0.5*height.\n"
                "\n"
                "The shape is also defined by two pairs\n"
                " of external/internal diameters,\n"
                " one pair at the lower plane and the other at the upper one.\n"
                "\n"
                "Implemented using TGeoCone(0.5*Height, 0.5*LowerInDiam, 0.5*LowerOutDiam, 0.5*UpperInDiam, 0.5*UpperOutDiam)";

    gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Height:"),               0, 0);
    gr->addWidget(new QLabel("Lower outer diameter:"), 1, 0);
    gr->addWidget(new QLabel("Lower inner diameter:"), 2, 0);
    gr->addWidget(new QLabel("Upper outer diameter:"), 3, 0);
    gr->addWidget(new QLabel("Upper inner diameter:"), 4, 0);

    ez  = new QLineEdit(); gr->addWidget(ez, 0, 1);
    elo = new QLineEdit(); gr->addWidget(elo, 1, 1);
    eli = new QLineEdit(); gr->addWidget(eli, 2, 1);
    euo = new QLineEdit(); gr->addWidget(euo, 3, 1);
    eui = new QLineEdit(); gr->addWidget(eui, 4, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("mm"), 3, 2);
    gr->addWidget(new QLabel("mm"), 4, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {ez, eli, elo, eui, euo};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoConeDelegate::onLocalShapeParameterChange);
}

void AGeoConeDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoCone * cone = dynamic_cast<const AGeoCone*>(tmpShape ? tmpShape : obj->Shape);
    if (cone)
    {
        ez ->setText(QString::number(cone->dz*2.0));
        eli->setText(QString::number(cone->rminL*2.0));
        elo->setText(QString::number(cone->rmaxL*2.0));
        eui->setText(QString::number(cone->rminU*2.0));
        euo->setText(QString::number(cone->rmaxU*2.0));
    }
    delete tmpShape;
}

void AGeoConeDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoCone( %1, %2, %3, %4, %5 )")
                   .arg(0.5*ez->text().toDouble())
                   .arg(0.5*eli->text().toDouble()).arg(0.5*elo->text().toDouble())
                   .arg(0.5*eui->text().toDouble()).arg(0.5*euo->text().toDouble()) );
}

AGeoConeSegDelegate::AGeoConeSegDelegate(const QStringList &materials, QWidget *parent)
    : AGeoConeDelegate(materials, parent)
{
    DelegateTypeName = "Cone segment";

    ShapeHelp = "A conical segment shape with the axis parallel to Z,\n"
                "  limited by two XY planes,\n"
                "  one at Z = -0.5*height, and\n"
                "  the other at Z = +0.5*height.\n"
                "\n"
                "The shape is also defined by two pairs\n"
                " of external/internal diameters,\n"
                " one pair at the lower plane and the other at the upper one.\n"
                "\n"
                "The segment angles are in degrees,\n"
                "  Phi from:  in the range [0, 360)\n"
                "  Phi to:    in the ramge (0, 360], should be smaller than Phi_from.\n"
                "\n"
                "Implemented using TGeoCone(0.5*Height, 0.5*LowerInDiam, 0.5*LowerOutDiam, 0.5*UpperInDiam, 0.5*UpperoutDiam, PhiFrom, PhiTo)";

    gr->addWidget(new QLabel("Phi from:"), 5, 0);
    gr->addWidget(new QLabel("Phi to:"),   6, 0);

    ep1 = new QLineEdit(); gr->addWidget(ep1, 5, 1);
    ep2 = new QLineEdit(); gr->addWidget(ep2, 6, 1);

    gr->addWidget(new QLabel("°"), 5, 2);
    gr->addWidget(new QLabel("°"), 6, 2);

    QVector<QLineEdit*> l = {ep1, ep2};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoConeSegDelegate::onLocalShapeParameterChange);
}

void AGeoConeSegDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoConeSeg * cone = dynamic_cast<const AGeoConeSeg*>(tmpShape ? tmpShape : obj->Shape);
    if (cone)
    {
        ez ->setText(QString::number(cone->dz*2.0));
        eli->setText(QString::number(cone->rminL*2.0));
        elo->setText(QString::number(cone->rmaxL*2.0));
        eui->setText(QString::number(cone->rminU*2.0));
        euo->setText(QString::number(cone->rmaxU*2.0));
        ep1->setText(QString::number(cone->phi1));
        ep2->setText(QString::number(cone->phi2));
    }
    delete tmpShape;
}

void AGeoConeSegDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoConeSeg( %1, %2, %3, %4, %5, %6, %7 )")
                   .arg(0.5*ez->text().toDouble())
                   .arg(0.5*eli->text().toDouble()).arg(0.5*elo->text().toDouble())
                   .arg(0.5*eui->text().toDouble()).arg(0.5*euo->text().toDouble())
                   .arg(ep1->text()).arg(ep2->text()) );
}

AGeoElTubeDelegate::AGeoElTubeDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Elliptical tube";

    ShapeHelp = "An elliptical tube\n"
            "\n"
            "Parameters are the diameters in X and Y directions and the full height\n"
            "\n"
            "The XYZ position is given for the center point\n"
            "\n"
            "Implemented using TGeoEltu(0.5*X_full_size, 0.5*Y_full_size, 0.5*Height)";

    gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("X full size:"), 0, 0);
    gr->addWidget(new QLabel("Y full size:"), 1, 0);
    gr->addWidget(new QLabel("Height:"),    2, 0);

    ex = new QLineEdit(); gr->addWidget(ex, 0, 1);
    ey = new QLineEdit(); gr->addWidget(ey, 1, 1);
    ez = new QLineEdit(); gr->addWidget(ez, 2, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {ex, ey, ez};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoElTubeDelegate::onLocalShapeParameterChange);
}

void AGeoElTubeDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoEltu * tube = dynamic_cast<const AGeoEltu*>(tmpShape ? tmpShape : obj->Shape);
    if (tube)
    {
        ex->setText(QString::number(tube->a*2.0));
        ey->setText(QString::number(tube->b*2.0));
        ez->setText(QString::number(tube->dz*2.0));
    }
    delete tmpShape;
}

void AGeoElTubeDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoEltu( %1, %2, %3 )").arg(0.5*ex->text().toDouble()).arg(0.5*ey->text().toDouble()).arg(0.5*ez->text().toDouble()) );
}

AGeoTrapXDelegate::AGeoTrapXDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Trapezoid simplified";

    ShapeHelp = "A trapezoid shape\n"
            "\n"
            "The two of the opposite faces are parallel to XY plane\n"
            "  and are positioned in Z at ± 0.5*Height.\n"
            "Full X size is given for the lower and upper planes.\n"
            "\n"
            "Implemented using TGeoTrd1(0.5*X_lower_size, 0.5*X_upper_size, 0.5*Y_size, 0.5*Height)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("X lower size:"), 0, 0);
    gr->addWidget(new QLabel("X upper size:"), 1, 0);
    gr->addWidget(new QLabel("Y size:"),       2, 0);
    gr->addWidget(new QLabel("Height:"),       3, 0);

    exl = new QLineEdit(); gr->addWidget(exl, 0, 1);
    exu = new QLineEdit(); gr->addWidget(exu, 1, 1);
    ey  = new QLineEdit(); gr->addWidget(ey,  2, 1);
    ez  = new QLineEdit(); gr->addWidget(ez,  3, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("mm"), 3, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {exl, exu, ey, ez};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTrapXDelegate::onLocalShapeParameterChange);
}

void AGeoTrapXDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoTrd1 * trap = dynamic_cast<const AGeoTrd1*>(tmpShape ? tmpShape : obj->Shape);
    if (trap)
    {
        exl->setText(QString::number(trap->dx1 * 2.0));
        exu->setText(QString::number(trap->dx2 * 2.0));
        ey-> setText(QString::number(trap->dy  * 2.0));
        ez-> setText(QString::number(trap->dz  * 2.0));
    }
    delete tmpShape;
}

void AGeoTrapXDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoTrd1( %1, %2, %3, %4 )")
                   .arg(0.5*exl->text().toDouble()).arg(0.5*exu->text().toDouble())
                   .arg(0.5* ey->text().toDouble()).arg(0.5* ez->text().toDouble()) );
}

AGeoTrapXYDelegate::AGeoTrapXYDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Trapezoid";

    ShapeHelp = "A trapezoid shape\n"
            "\n"
            "The two of the opposite faces are parallel to XY plane\n"
            "  and are positioned in Z at ± 0.5*Height.\n"
            "Full X and Y sizes are given for the lower and upper planes.\n"
            "\n"
            "Implemented using TGeoTrd2(0.5*X_lower_size, 0.5*X_upper_size, 0.5*Y_lower_size, 0.5*Y_upper_size, 0.5*Height)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("X lower size:"), 0, 0);
    gr->addWidget(new QLabel("X upper size:"), 1, 0);
    gr->addWidget(new QLabel("Y lower size:"), 2, 0);
    gr->addWidget(new QLabel("Y upper size:"), 3, 0);
    gr->addWidget(new QLabel("Height:"),       4, 0);

    exl = new QLineEdit(); gr->addWidget(exl, 0, 1);
    exu = new QLineEdit(); gr->addWidget(exu, 1, 1);
    eyl = new QLineEdit(); gr->addWidget(eyl, 2, 1);
    eyu = new QLineEdit(); gr->addWidget(eyu, 3, 1);
    ez  = new QLineEdit(); gr->addWidget(ez,  4, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("mm"), 3, 2);
    gr->addWidget(new QLabel("mm"), 4, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {exl, exu, eyl, eyu, ez};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTrapXYDelegate::onLocalShapeParameterChange);
}

void AGeoTrapXYDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoTrd2 * trap = dynamic_cast<const AGeoTrd2*>(tmpShape ? tmpShape : obj->Shape);
    if (trap)
    {
        exl->setText(QString::number(trap->dx1 * 2.0));
        exu->setText(QString::number(trap->dx2 * 2.0));
        eyl->setText(QString::number(trap->dy1 * 2.0));
        eyu->setText(QString::number(trap->dy2 * 2.0));
        ez-> setText(QString::number(trap->dz  * 2.0));
    }
    delete tmpShape;
}

void AGeoTrapXYDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoTrd2( %1, %2, %3, %4, %5)")
                   .arg(0.5*exl->text().toDouble()).arg(0.5*exu->text().toDouble())
                   .arg(0.5*eyl->text().toDouble()).arg(0.5*eyu->text().toDouble()).arg(0.5*ez->text().toDouble()) );
}

AGeoParaboloidDelegate::AGeoParaboloidDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Paraboloid";

    ShapeHelp = "A paraboloid shape\n"
            "\n"
            "Defined by the revolution surface generated by a parabola\n"
            "  and is bound by two planes perpendicular to Z axis.\n"
            "The parabola equation is taken in the form: z = a*r^2 + b,\n"
            "  where r^2 = x^2 + y^2.\n"
            "The coefficients a and b are computed from the input values\n"
            "  which are the diameters of the circular sections cut by\n"
            "  two planes, lower at -0.5*Height, and the upper at +0.5*Height:\n"
            "  a*(0.5*Lower_diameter)^2 + b   for the lower plane and\n"
            "  a*(0.5*Upper_diameter)^2 + b   for the upper one.\n"
            "\n"
            "Implemented using TGeoParaboloid(0.5*Lower_diameter, 0.5*Upper_diameter, 0.5*Height)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Lower diameter:"), 0, 0);
    gr->addWidget(new QLabel("Upper diameter:"), 1, 0);
    gr->addWidget(new QLabel("Height:"),         2, 0);

    el = new QLineEdit(); gr->addWidget(el, 0, 1);
    eu = new QLineEdit(); gr->addWidget(eu, 1, 1);
    ez = new QLineEdit(); gr->addWidget(ez, 2, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {el, eu, ez};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoParaboloidDelegate::onLocalShapeParameterChange);
}

void AGeoParaboloidDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoParaboloid * para = dynamic_cast<const AGeoParaboloid*>(tmpShape ? tmpShape : obj->Shape);
    if (para)
    {
        el->setText(QString::number(para->rlo * 2.0));
        eu->setText(QString::number(para->rhi * 2.0));
        ez->setText(QString::number(para->dz  * 2.0));
    }
    delete tmpShape;
}

void AGeoParaboloidDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoParaboloid( %1, %2, %3)")
                   .arg(0.5*el->text().toDouble())
                   .arg(0.5*eu->text().toDouble())
                   .arg(0.5*ez->text().toDouble()) );
}

AGeoTorusDelegate::AGeoTorusDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Torus";

    ShapeHelp = "A torus segment\n"
            "\n"
            "Defined by the axial, inner and outer diameters\n"
            "  and the segment angles (in degrees)\n"
            "Phi from: in the range [0, 360),\n"
            "Phi to:   in the range (0, 360], Phi_to > Phi_from\n"
            "\n"
            "Implemented using TGeoTorus(0.5*Axial_diameter, 0.5*Inner_diameter, 0.5*Outer_diameter, Phi_from, Phi_to)";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Axial diameter:"), 0, 0);
    gr->addWidget(new QLabel("Outer diameter:"), 1, 0);
    gr->addWidget(new QLabel("Inner diameter:"), 2, 0);
    gr->addWidget(new QLabel("Phi from:"),       3, 0);
    gr->addWidget(new QLabel("Phi to:"),         4, 0);

    ead = new QLineEdit(); gr->addWidget(ead, 0, 1);
    edo = new QLineEdit(); gr->addWidget(edo, 1, 1);
    edi = new QLineEdit(); gr->addWidget(edi, 2, 1);
    ep0 = new QLineEdit(); gr->addWidget(ep0, 3, 1);
    epe= new QLineEdit();  gr->addWidget(epe, 4, 1);

    gr->addWidget(new QLabel("mm"), 0, 2);
    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("°"),  3, 2);
    gr->addWidget(new QLabel("°"),  4, 2);

    addLocalLayout(gr);

    QVector<QLineEdit*> l = {ead, edi, edo, ep0, epe};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoTorusDelegate::onLocalShapeParameterChange);
}

void AGeoTorusDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoTorus * tor = dynamic_cast<const AGeoTorus*>(tmpShape ? tmpShape : obj->Shape);
    if (tor)
    {
        ead->setText(QString::number(tor->R    * 2.0));
        edi->setText(QString::number(tor->Rmin * 2.0));
        edo->setText(QString::number(tor->Rmax * 2.0));
        ep0->setText(QString::number(tor->Phi1));
        epe->setText(QString::number(tor->Dphi));
    }
    delete tmpShape;
}

void AGeoTorusDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoTorus( %1, %2, %3, %4, %5)")
                   .arg(0.5*ead->text().toDouble())
                   .arg(0.5*edi->text().toDouble())
                   .arg(0.5*edo->text().toDouble())
                   .arg(ep0->text())
                   .arg(epe->text()) );
}

AGeoPolygonDelegate::AGeoPolygonDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Polygon (simplified)";

    ShapeHelp = "A polygon section\n"
            "\n"
            "The shape is limited by the upper and lower planes,\n"
            "  positioned in Z at +0.5*Height and -0.5*Height\n"
            "For each plane are defined\n"
            "  the outer and inner diameters of the circles,\n"
            "  inscribed in the corresponding polygon.\n"
            "\n"
            "Section angle: in the range (0, 360] degrees\n"
            "\n"
            "Implemented using TGeoPgon";

    QGridLayout * gr = new QGridLayout();
    gr->setContentsMargins(50, 0, 50, 3);
    gr->setVerticalSpacing(1);

    gr->addWidget(new QLabel("Number of edges:"),      0, 0);
    gr->addWidget(new QLabel("Height:"),               1, 0);
    gr->addWidget(new QLabel("Lower outer diameter:"), 2, 0);
    gr->addWidget(new QLabel("Lower inner diameter:"), 3, 0);
    gr->addWidget(new QLabel("Upper outer diameter:"), 4, 0);
    gr->addWidget(new QLabel("Upper inner diameter:"), 5, 0);
    gr->addWidget(new QLabel("Angle:"),                6, 0);

    sbn = new QSpinBox();  gr->addWidget(sbn, 0, 1); sbn->setMinimum(3);
    ez  = new QLineEdit(); gr->addWidget(ez,  1, 1);
    elo = new QLineEdit(); gr->addWidget(elo, 2, 1);
    eli = new QLineEdit(); gr->addWidget(eli, 3, 1);
    euo = new QLineEdit(); gr->addWidget(euo, 4, 1);
    eui = new QLineEdit(); gr->addWidget(eui, 5, 1);
    edp = new QLineEdit(); gr->addWidget(edp, 6, 1);

    gr->addWidget(new QLabel("mm"), 1, 2);
    gr->addWidget(new QLabel("mm"), 2, 2);
    gr->addWidget(new QLabel("mm"), 3, 2);
    gr->addWidget(new QLabel("mm"), 4, 2);
    gr->addWidget(new QLabel("mm"), 5, 2);
    gr->addWidget(new QLabel("°"),  5, 2);

    addLocalLayout(gr);

    QObject::connect(sbn, SIGNAL(valueChanged(int)), this, SLOT(onLocalShapeParameterChange()));
    QVector<QLineEdit*> l = {edp, ez, eli, elo, eui, euo};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoPolygonDelegate::onLocalShapeParameterChange);
}

void AGeoPolygonDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoPolygon * pgon = dynamic_cast<const AGeoPolygon*>(tmpShape ? tmpShape : obj->Shape);
    if (pgon)
    {
        sbn->setValue(pgon->nedges);
        edp->setText(QString::number(pgon->dphi));
        ez-> setText(QString::number(pgon->dz    * 2.0));
        eli->setText(QString::number(pgon->rminL * 2.0));
        elo->setText(QString::number(pgon->rmaxL * 2.0));
        eui->setText(QString::number(pgon->rminU * 2.0));
        euo->setText(QString::number(pgon->rmaxU * 2.0));
    }
    delete tmpShape;
}

void AGeoPolygonDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoPolygon( %1, %2, %3, %4, %5, %6, %7 )")
                   .arg(sbn->value())
                   .arg(edp->text())
                   .arg(0.5*ez->text().toDouble())
                   .arg(0.5*eli->text().toDouble())
                   .arg(0.5*elo->text().toDouble())
                   .arg(0.5*eui->text().toDouble())
                   .arg(0.5*euo->text().toDouble()) );
}

#include <QTableWidget>
#include <QHeaderView>
AGeoPconDelegate::AGeoPconDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Polycone";

    ShapeHelp = "A shape constructed of an arbitrary number of cone sections\n"
            "\n"
            "A section is limited by the upper and lower planes,\n"
            "  shared by the neighboring sections.\n"
            "For each plane are defined:\n"
            "  Z position, and\n"
            "  outer and inner diameters.\n"
            "\n"
            "Phi from: in the range [0, 360) degrees,\n"
            "Phi to:   in the range (0, 360] degrees, Phi_to > Phi_from\n"
            "\n"
            "Implemented using TGeoPcon";

    lay = new QVBoxLayout();
    lay->setContentsMargins(50, 0, 50, 0);
    lay->setSpacing(3);

    lay->addWidget(new QLabel("Defined planes (should be monotonic in Z), all in mm:"));

        tab = new QTableWidget();
        tab->setColumnCount(3);
        tab->setHorizontalHeaderLabels(QStringList({"Z position", "Outer diameter", "Inner diameter"}));
        tab->setMaximumHeight(150);
        tab->verticalHeader()->setSectionsMovable(true);
        QObject::connect(tab->verticalHeader(), &QHeaderView::sectionMoved, this, [this](int /*logicalIndex*/, int oldVisualIndex, int newVisualIndex)
        {
            //qDebug() << logicalIndex << oldVisualIndex << newVisualIndex;
            tab->verticalHeader()->blockSignals(true);
            //tab->verticalHeader()->swapSections(oldVisualIndex, newVisualIndex);
            tab->verticalHeader()->moveSection(newVisualIndex, oldVisualIndex);
            tab->verticalHeader()->blockSignals(false);
            //swap table rows oldVisualIndex and newVisualIndex
            for (int i=0; i<3; i++)
            {
                QTableWidgetItem * from = tab->takeItem(oldVisualIndex, i);
                QTableWidgetItem * to = tab->takeItem(newVisualIndex, i);
                tab->setItem(oldVisualIndex, i, to);
                tab->setItem(newVisualIndex, i, from);
            }
        }, Qt::QueuedConnection);
        connect(tab, &QTableWidget::cellChanged, this, &AGeoPconDelegate::onLocalShapeParameterChange);

    lay->addWidget(tab);

        QHBoxLayout * hl = new QHBoxLayout();

        QPushButton * pbAddAbove = new QPushButton("Add above");
        connect(pbAddAbove, &QPushButton::clicked, [this]()
        {
            int row = tab->currentRow();
            if (row == -1) row = 0;
            tab->insertRow(row);
            tab->setRowHeight(row, rowHeight);
            onLocalShapeParameterChange();
        });
        hl->addWidget(pbAddAbove);
        QPushButton * pbAddBelow = new QPushButton("Add below");
        connect(pbAddBelow, &QPushButton::clicked, [this]()
        {
            const int num = tab->rowCount();
            int row = tab->currentRow();
            if (row == -1) row = num-1;
            row++;
            tab->insertRow(row);
            tab->setRowHeight(row, rowHeight);
            onLocalShapeParameterChange();
        });
        hl->addWidget(pbAddBelow);
        QPushButton * pbRemoveRow = new QPushButton("Remove plane");
        connect(pbRemoveRow, &QPushButton::clicked, [this]()
        {
            int row = tab->currentRow();
            if (row != -1) tab->removeRow(row);
            onLocalShapeParameterChange();
        });
        hl->addWidget(pbRemoveRow);

    lay->addLayout(hl);

    QGridLayout * gr = new QGridLayout();
        gr->setContentsMargins(0, 0, 0, 3);
        gr->setVerticalSpacing(1);

        gr->addWidget(new QLabel("Phi from:"), 0, 0);
        gr->addWidget(new QLabel("Phi to:"),   1, 0);

        ep0 = new QLineEdit(); gr->addWidget(ep0, 0, 1);
        epe = new QLineEdit(); gr->addWidget(epe, 1, 1);

        gr->addWidget(new QLabel("°"),  0, 2);
        gr->addWidget(new QLabel("°"),  1, 2);

    lay->addLayout(gr);

    addLocalLayout(lay);

    QVector<QLineEdit*> l = {ep0, epe};
    for (QLineEdit * le : l)
        QObject::connect(le, &QLineEdit::textChanged, this, &AGeoPconDelegate::onLocalShapeParameterChange);
}

void AGeoPconDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoPcon * pcon = dynamic_cast<const AGeoPcon*>(tmpShape ? tmpShape : obj->Shape);
    if (pcon)
    {
        ep0->setText(QString::number(pcon->phi));
        epe->setText(QString::number(pcon->dphi));

        tab->clearContents();
        const int numPlanes = pcon->Sections.size();
        tab->setRowCount(numPlanes);
        for (int iP = 0; iP < numPlanes; iP++)
        {
            const APolyCGsection & Section = pcon->Sections.at(iP);
            QTableWidgetItem * item = new QTableWidgetItem(QString::number(Section.z)); item->setTextAlignment(Qt::AlignCenter);
            tab->setItem(iP, 0, item);
            item = new QTableWidgetItem(QString::number(Section.rmax * 2.0)); item->setTextAlignment(Qt::AlignCenter);
            tab->setItem(iP, 1, item);
            item = new QTableWidgetItem(QString::number(Section.rmin * 2.0)); item->setTextAlignment(Qt::AlignCenter);
            tab->setItem(iP, 2, item);
            tab->setRowHeight(iP, rowHeight);
        }
    }
    delete tmpShape;
}

void AGeoPconDelegate::onLocalShapeParameterChange()
{
    QString s = QString("TGeoPcon( %1, %2")
            .arg(ep0->text())
            .arg(epe->text());

    if (!tab) return;
    const int rows = tab->rowCount();
    for (int ir = 0; ir < rows; ir++)
    {
        if (!tab->item(ir, 0) || !tab->item(ir, 1) || !tab->item(ir, 2)) continue;
        s += QString(", { %1 : %2 : %3 }")
                .arg(tab->item(ir, 0)->text())
                .arg(0.5*tab->item(ir, 2)->text().toDouble())
                .arg(0.5*tab->item(ir, 1)->text().toDouble());
    }
    s += " )";

    updatePteShape(s);
}

AGeoPgonDelegate::AGeoPgonDelegate(const QStringList &materials, QWidget *parent)
    : AGeoPconDelegate(materials, parent)
{
    DelegateTypeName = "Polygon";

    ShapeHelp = "A shape constructed of an arbitrary number of polygon sections\n"
            "\n"
            "A section is limited by the upper and lower planes,\n"
            "  shared by the neighboring sections.\n"
            "For each plane are defined:\n"
            "  Z position, and\n"
            "  outer and inner diameters of circles, inscribed in the polygon.\n"
            "\n"
            "Phi from: in the range [0, 360) degrees,\n"
            "Phi to:   in the range (0, 360] degrees, Phi_to > Phi_from\n"
            "\n"
            "Implemented using TGeoPgon";

    QHBoxLayout * h = new QHBoxLayout();
    h->setContentsMargins(0, 0, 0, 1);
    h->setSpacing(1);
    QLabel * lab = new QLabel("Edges:");
    h->addWidget(lab);
        sbn = new QSpinBox();
        sbn->setMinimum(3);
        sbn->setMaximum(100000);
    h->addWidget(sbn);
    h->addStretch();

    lay->insertLayout(0, h);

    tab->setHorizontalHeaderLabels(QStringList({"Z position", "Outer size", "Inner size"}));

    QObject::connect(sbn, SIGNAL(valueChanged(int)), this, SLOT(onLocalShapeParameterChange()));
}

void AGeoPgonDelegate::Update(const AGeoObject *obj)
{
    AGeoPconDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoPgon * pgon = dynamic_cast<const AGeoPgon*>(tmpShape ? tmpShape : obj->Shape);
    if (pgon)
        sbn->setValue(pgon->nedges);
    delete tmpShape;
}

void AGeoPgonDelegate::onLocalShapeParameterChange()
{
    QString s = QString("TGeoPgon( %1, %2, %3")
            .arg(ep0->text())
            .arg(epe->text())
            .arg(sbn->value());

    if (!tab) return;
    const int rows = tab->rowCount();
    for (int ir = 0; ir < rows; ir++)
    {
        if (!tab->item(ir, 0) || !tab->item(ir, 1) || !tab->item(ir, 2)) continue;
        s += QString(", { %1 : %2 : %3 }")
                .arg(tab->item(ir, 0)->text())
                .arg(0.5*tab->item(ir, 2)->text().toDouble())
                .arg(0.5*tab->item(ir, 1)->text().toDouble());
    }
    s += " )";

    updatePteShape(s);
}

#include <QFont>
AGeoCompositeDelegate::AGeoCompositeDelegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Composite shape";

    ShapeHelp = "Composite shape\n"
            "\n"
            "Composite shapes are boolean combinations\n"
            "  of two or more shape components. The supported operations are:\n"
            "\n"
            "  (+) - union\n"
            "  (*) - intersection\n"
            "  (-) - subtraction.\n"
            "\n"
            "If nested structures are needed, brackets can be used, e.g.,\n"
            "   (Shape1 + Shape2) - (Shape3 * (Shape4 + Shape5))\n"
            "\n"
            "Implemented using TGeoCompositeShape\n"
            "\n"
            "WARNING!\n"
            "Use composite shapes only as the last resort.\n"
            "Navigation in geometries containing composite shapes are very inefficient!";

    QVBoxLayout * v = new QVBoxLayout();
    v->setContentsMargins(50, 0, 50, 3);

    v->addWidget(new QLabel("Use logical volume names and\n'+', '*', and '-' operands; brackets for nested"));
        te = new QPlainTextEdit();
        QFont font = te->font();
        font.setPointSize(te->font().pointSize() + 2);
        te->setFont(font);
    v->addWidget(te);
    connect(te, &QPlainTextEdit::textChanged, this, &AGeoCompositeDelegate::onLocalShapeParameterChange);

    cbScale->setChecked(false);
    cbScale->setVisible(false);

    addLocalLayout(v);
}

void AGeoCompositeDelegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoComposite * combo = dynamic_cast<const AGeoComposite *>(tmpShape ? tmpShape : obj->Shape);
    if (combo)
    {
        QString s = combo->getGenerationString().simplified();
        s.remove("TGeoCompositeShape(");
        s.chop(1);

        te->clear();
        te->appendPlainText(s.simplified());
    }
    delete tmpShape;
}

void AGeoCompositeDelegate::onLocalShapeParameterChange()
{
    updatePteShape(QString("TGeoCompositeShape( %1 )").arg(te->document()->toPlainText()));
}

AGeoArb8Delegate::AGeoArb8Delegate(const QStringList &materials, QWidget *parent)
    : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Arb8";

    ShapeHelp = "An Arb8 shape of Cern Root\n"
                "\n"
                "The shape is defined by two quadrilaterals sitting on parallel planes,\n"
                "   at +0.5*Height and -0.5*Height.\n"
                "Quadrilaterals are defined each by 4 vertices\n"
                "   with the coordinates (Xi,Yi,+/-0.5*Height), i from 0 to 3.\n"
                "\n"
                "The lateral surface of the Arb8 is defined by the 4 pairs of\n"
                "   edges corresponding to vertices (i,i+1) on both planes.\n"
                "If M and M' are the middles of the segments (i,i+1)\n"
                "   at the different planes,\n"
                "   a lateral surface is obtained by sweeping the edge at\n"
                "   -0.5*Height along MM',\n"
                "   so that it will match the corresponding one at +0.5*Height.\n"
                "Since the points defining the edges are arbitrary,\n"
                "   the lateral surfaces are not necessary planes –\n"
                "   but twisted planes having a twist angle linear-dependent on Z.\n"
                "\n"
                "Vertices have to be defined clockwise in the XY pane at both planes!\n"
                "\n"
                "Any two or more vertices in each plane\n"
                "   can have the same (X,Y) coordinates!\n"
                "   It this case, the top and bottom quadrilaterals become triangles,\n"
                "   segments or points.\n"
                "\n"
                "The lateral surfaces are not necessary defined by a pair of segments,\n"
                "   but by pair segment-point (making a triangle)\n"
                "   or point-point (making a line).\n"
                "Any choice is valid as long as at one of the end-caps is at least a triangle.\n";

    QVBoxLayout * v = new QVBoxLayout();
    v->setContentsMargins(50, 0, 50, 0);
    v->setSpacing(3);
    QGridLayout * gr = new QGridLayout();
        gr->setContentsMargins(0, 0, 0, 0);
        gr->addWidget(new QLabel("Height:"), 0, 0);
        ez = new QLineEdit(); gr->addWidget(ez,  0, 1);
        connect(ez, &QLineEdit::textChanged, this, &AGeoArb8Delegate::onLocalShapeParameterChange);
        gr->addWidget(new QLabel("mm"), 0, 2);
    v->addLayout(gr);

    ve.clear();
    for (int iul = 0; iul < 2; iul++)
    {
        v->addWidget(new QLabel(iul == 0 ? "Lower plane (positions clockwise!):" : "Upper plane (positions clockwise!):"));

        QVector<AEditEdit> tmpV(4);

        QGridLayout * gri = new QGridLayout();
        gri->setContentsMargins(0, 0, 0, 0);
        gri->setVerticalSpacing(3);

        for (int i=0; i < 4; i++)
        {
            gri->addWidget(new QLabel("  x:"),    i, 0);
            tmpV[i].X = new QLineEdit("");
            connect(tmpV[i].X, &QLineEdit::textChanged, this, &AGeoArb8Delegate::onLocalShapeParameterChange);
            gri->addWidget(tmpV[i].X,             i, 1);
            gri->addWidget(new QLabel("mm   y:"), i, 2);
            tmpV[i].Y = new QLineEdit("");
            connect(tmpV[i].Y, &QLineEdit::textChanged, this, &AGeoArb8Delegate::onLocalShapeParameterChange);
            gri->addWidget(tmpV[i].Y,             i, 3);
            gri->addWidget(new QLabel("mm"),      i, 4);
        }
        ve.push_back(tmpV);
        v->addLayout(gri);
    }
    addLocalLayout(v);
}

void AGeoArb8Delegate::Update(const AGeoObject *obj)
{
    AGeoObjectDelegate::Update(obj);

    const AGeoShape * tmpShape = getBaseShapeOfObject(obj); //non-zero only if scaled shape!
    const AGeoArb8 * arb = dynamic_cast<const AGeoArb8 *>(tmpShape ? tmpShape : obj->Shape);
    if (arb)
    {
        ez->setText(QString::number(2.0 * arb->dz));

        for (int iul = 0; iul < 2; iul++)
        {
            for (int i = 0; i < 4; i++)
            {
                const int iInVert = iul * 4 + i;
                const QPair<double, double> & V = arb->Vertices.at(iInVert);
                AEditEdit & CEE = ve[iul][i];
                CEE.X->setText(QString::number(V.first));
                CEE.Y->setText(QString::number(V.second));
            }
        }
    }
    delete tmpShape;
}

void AGeoArb8Delegate::onLocalShapeParameterChange()
{
    QString s = QString("TGeoArb8( %1").arg(0.5 * ez->text().toDouble());
    for (int iul = 0; iul < 2; iul++)
    {
        for (int i = 0; i < 4; i++)
        {
            AEditEdit & CEE = ve[iul][i];
            s += QString(", %1,%2").arg(CEE.X->text()).arg(CEE.Y->text());
        }
    }
    s += ")";
    updatePteShape(s);
}

AGeoArrayDelegate::AGeoArrayDelegate(const QStringList &materials, QWidget *parent)
   : AGeoObjectDelegate(materials, parent)
{
    DelegateTypeName = "Array";

    QVBoxLayout * v = new QVBoxLayout();
    v->setContentsMargins(50, 0, 50, 0);

    QGridLayout *grAW = new QGridLayout();
    grAW->setContentsMargins(5, 3, 5, 3);
    grAW->setVerticalSpacing(0);

    QLabel *la = new QLabel("Number in X:");
    grAW->addWidget(la, 0, 0);
    la = new QLabel("Number in Y:");
    grAW->addWidget(la, 1, 0);
    la = new QLabel("Number in Z:");
    grAW->addWidget(la, 2, 0);
    la = new QLabel("Step in X:");
    la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grAW->addWidget(la, 0, 2);
    la = new QLabel("Step in Y:");
    la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grAW->addWidget(la, 1, 2);
    la = new QLabel("Step in Z:");
    la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grAW->addWidget(la, 2, 2);
    la = new QLabel("mm");
    grAW->addWidget(la, 0, 4);
    la = new QLabel("mm");
    grAW->addWidget(la, 1, 4);
    la = new QLabel("mm");
    grAW->addWidget(la, 2, 4);

    sbNumX = new QSpinBox(Widget);
    sbNumX->setMaximum(100);
    sbNumX->setMinimum(0);
    sbNumX->setContextMenuPolicy(Qt::NoContextMenu);
    grAW->addWidget(sbNumX, 0, 1);
    connect(sbNumX, SIGNAL(valueChanged(int)), this, SLOT(onContentChanged()));
    sbNumY = new QSpinBox(Widget);
    sbNumY->setMaximum(100);
    sbNumY->setMinimum(0);
    sbNumY->setContextMenuPolicy(Qt::NoContextMenu);
    grAW->addWidget(sbNumY, 1, 1);
    connect(sbNumY, SIGNAL(valueChanged(int)), this, SLOT(onContentChanged()));
    sbNumZ = new QSpinBox(Widget);
    sbNumZ->setMaximum(100);
    sbNumZ->setMinimum(0);
    sbNumZ->setContextMenuPolicy(Qt::NoContextMenu);
    grAW->addWidget(sbNumZ, 2, 1);
    connect(sbNumZ, SIGNAL(valueChanged(int)), this, SLOT(onContentChanged()));
    ledStepX = new QLineEdit(Widget);
    ledStepX->setContextMenuPolicy(Qt::NoContextMenu);
    ledStepX->setMaximumWidth(75);
    connect(ledStepX, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
    grAW->addWidget(ledStepX, 0, 3);
    ledStepY = new QLineEdit(Widget);
    ledStepY->setMaximumWidth(75);
    ledStepY->setContextMenuPolicy(Qt::NoContextMenu);
    connect(ledStepY, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
    grAW->addWidget(ledStepY, 1, 3);
    ledStepZ = new QLineEdit(Widget);
    ledStepZ->setMaximumWidth(75);
    ledStepZ->setContextMenuPolicy(Qt::NoContextMenu);
    connect(ledStepZ, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
    grAW->addWidget(ledStepZ, 2, 3);

    addLocalLayout(grAW);

    cbScale->setChecked(false);
    cbScale->setVisible(false);

    lMat->setVisible(false);
    cobMat->setVisible(false);
    ledPhi->setText("0");
    ledPhi->setEnabled(false);
    ledTheta->setText("0");
    ledTheta->setEnabled(false);

    pbTransform->setVisible(false);
    pbShapeInfo->setVisible(false);
}

void AGeoArrayDelegate::Update(const AGeoObject * obj)
{
    AGeoObjectDelegate::Update(obj);

    if (obj->ObjectType->isArray())
    {
        ATypeArrayObject* array = static_cast<ATypeArrayObject*>(obj->ObjectType);
        sbNumX->setValue(array->numX);
        sbNumY->setValue(array->numY);
        sbNumZ->setValue(array->numZ);
        ledStepX->setText(QString::number(array->stepX));
        ledStepY->setText(QString::number(array->stepY));
        ledStepZ->setText(QString::number(array->stepZ));
    }
}

AGeoSetDelegate::AGeoSetDelegate(const QStringList &materials, QWidget *parent)
   : AGeoObjectDelegate(materials, parent)
{
     pbTransform->setVisible(false);
     pbShapeInfo->setVisible(false);

     cbScale->setChecked(false);
     cbScale->setVisible(false);
}

void AGeoSetDelegate::Update(const AGeoObject *obj)
{
    if (obj->ObjectType->isCompositeContainer())
    {
        DelegateTypeName = "Container of logical shapes";
        pbShow->setVisible(false);
        pbChangeAtt->setVisible(false);
        pbScriptLine->setVisible(false);
    }
    else
        DelegateTypeName = ( obj->ObjectType->isStack() ? "Stack" : "Group" );

    AGeoObjectDelegate::Update(obj);
}

#include "slabdelegate.h"
AGeoSlabDelegate::AGeoSlabDelegate(const QStringList & definedMaterials, int State, QWidget * ParentWidget)
    : AGeoBaseDelegate(ParentWidget)
{
    QFrame * frMainFrame = new QFrame();
    frMainFrame->setFrameShape(QFrame::Box);

    Widget = frMainFrame;
    Widget->setContextMenuPolicy(Qt::CustomContextMenu);

    QPalette palette = frMainFrame->palette();
    palette.setColor( Widget->backgroundRole(), QColor( 255, 255, 255 ) );
    frMainFrame->setPalette(palette);
    frMainFrame->setAutoFillBackground( true );
    QVBoxLayout * lMF = new QVBoxLayout();
        lMF->setContentsMargins(0,5,0,5);

        //object type
        labType = new QLabel("Slab");
        labType->setAlignment(Qt::AlignCenter);
        QFont font = labType->font();
        font.setBold(true);
        labType->setFont(font);
        lMF->addWidget(labType);

        SlabDel = new ASlabDelegate();
        SlabDel->leName->setMinimumWidth(50);

        ASlabXYDelegate::ShowStates XYDelState = static_cast<ASlabXYDelegate::ShowStates>(State);
        switch (XYDelState)
        {
        case (0): //ASandwich::CommonShapeSize
            SlabDel->XYdelegate->SetShowState(ASlabXYDelegate::ShowNothing); break;
        case (1): //ASandwich::CommonShape
            SlabDel->XYdelegate->SetShowState(ASlabXYDelegate::ShowSize); break;
        case (2): //ASandwich::Individual
            SlabDel->XYdelegate->SetShowState(ASlabXYDelegate::ShowAll); break;
        default:
            qWarning() << "Unknown DetectorSandwich state!"; break;
        }

        SlabDel->frMain->setPalette( palette );
        SlabDel->frMain->setAutoFillBackground( true );
        SlabDel->frMain->setMaximumHeight(80);
        SlabDel->frMain->setFrameShape(QFrame::NoFrame);
        connect(SlabDel->XYdelegate, SIGNAL(ContentChanged()), SlabDel->XYdelegate, SLOT(updateComponentVisibility()));
        connect(SlabDel, &ASlabDelegate::ContentChanged, this, &AGeoSlabDelegate::onContentChanged);

        SlabDel->comMaterial->addItems(definedMaterials);

        //SlabDel->frMain->setMinimumHeight(65);

     lMF->addWidget(SlabDel->frMain);

     // bottom line buttons
     QHBoxLayout * abl = createBottomButtons();
     abl->setContentsMargins(5,0,5,0);
     lMF->addLayout(abl);

     Widget->setLayout(lMF);
}

const QString AGeoSlabDelegate::getName() const
{
    return SlabDel->leName->text();
}

bool AGeoSlabDelegate::isValid(AGeoObject * /*obj*/)
{
    return true;
}

void AGeoSlabDelegate::updateObject(AGeoObject * obj) const
{
    ASlabModel * model = obj->getSlabModel();

    obj->Name = getName();
    bool fCenter = model->fCenter;
    SlabDel->UpdateModel(model);
    model->fCenter = fCenter; //delegate does not remember center status
}

void AGeoSlabDelegate::Update(const AGeoObject *obj)
{
    QString str = "Slab";
    if (obj->ObjectType->isLightguide())
    {
        if (obj->ObjectType->isUpperLightguide()) str += ", Upper lightguide";
        else if (obj->ObjectType->isLowerLightguide()) str += ", Lower lightguide";
        else str = "Lightguide error!";
    }
    labType->setText(str);

    ASlabModel* SlabModel = (static_cast<ATypeSlabObject*>(obj->ObjectType))->SlabModel;
    SlabDel->UpdateGui(*SlabModel);

    SlabDel->frCenterZ->setVisible(false);
    SlabDel->fCenter = false;
    SlabDel->frColor->setVisible(false);
    SlabDel->cbOnOff->setEnabled(!SlabModel->fCenter);
}

void AGeoSlabDelegate::onContentChanged()
{
    emit ContentChanged();
}

void AGeoTreeWidget::objectMembersToScript(AGeoObject* Master, QString &script, int ident, bool bExpandMaterial, bool bRecursive)
{
    for (AGeoObject* obj : Master->HostedObjects)
        objectToScript(obj, script, ident, bExpandMaterial, bRecursive);
}

void AGeoTreeWidget::objectToScript(AGeoObject *obj, QString &script, int ident, bool bExpandMaterial, bool bRecursive)
{
    const QString Starter = "\n" + QString(" ").repeated(ident);

    if (obj->ObjectType->isLogical())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_basicObject(obj, bExpandMaterial);
    }
    else if (obj->ObjectType->isCompositeContainer())
    {
         //nothing to do
    }
    else if (obj->ObjectType->isSlab() || obj->ObjectType->isSingle() )
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_basicObject(obj, bExpandMaterial);
        script += "\n" + QString(" ").repeated(ident)+ makeLinePropertiesString(obj);
        if (obj->ObjectType->isLightguide())
        {
            script += "\n";
            script += "\n" + QString(" ").repeated(ident)+ "//=== Lightguide object is not supported! ===";
            script += "\n";
        }
        if (bRecursive) objectMembersToScript(obj, script, ident + 2, bExpandMaterial, bRecursive);
    }
    else if (obj->ObjectType->isComposite())
    {
        script += "\n" + QString(" ").repeated(ident) + "//-->-- logical volumes for " + obj->Name;
        objectMembersToScript(obj->getContainerWithLogical(), script, ident + 4, bExpandMaterial, bRecursive);
        script += "\n" + QString(" ").repeated(ident) + "//--<-- logical volumes end for " + obj->Name;

        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_basicObject(obj, bExpandMaterial);
        script += "\n" + QString(" ").repeated(ident)+ makeLinePropertiesString(obj);
        if (bRecursive) objectMembersToScript(obj, script, ident + 2, bExpandMaterial, bRecursive);
    }
    else if (obj->ObjectType->isArray())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_arrayObject(obj);
        script += "\n" + QString(" ").repeated(ident)+ "//-->-- array elements for " + obj->Name;
        objectMembersToScript(obj, script, ident + 2, bExpandMaterial, bRecursive);
        script += "\n" + QString(" ").repeated(ident)+ "//--<-- array elements end for " + obj->Name;
    }
    else if (obj->ObjectType->isMonitor())
    {
        script += Starter + makeScriptString_monitorBaseObject(obj);
        script += Starter + makeScriptString_monitorConfig(obj);
    }
    else if (obj->ObjectType->isStack())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_stackObjectStart(obj);
        script += "\n" + QString(" ").repeated(ident)+ "//-->-- stack elements for " + obj->Name;
        script += "\n" + QString(" ").repeated(ident)+ "// Values of x, y, z only matter for the stack element, refered to at InitializeStack below";
        script += "\n" + QString(" ").repeated(ident)+ "// For the rest of elements they are calculated automatically";
        objectMembersToScript(obj, script, ident + 2, bExpandMaterial, bRecursive);
        script += "\n" + QString(" ").repeated(ident)+ "//--<-- stack elements end for " + obj->Name;
        if (!obj->HostedObjects.isEmpty())
            script += "\n" + QString(" ").repeated(ident)+ makeScriptString_stackObjectEnd(obj);
    }
    else if (obj->ObjectType->isGroup())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_groupObjectStart(obj);
        script += "\n" + QString(" ").repeated(ident)+ "//-->-- group elements for " + obj->Name;
        objectMembersToScript(obj, script, ident + 2, bExpandMaterial, bRecursive);
        script += "\n" + QString(" ").repeated(ident)+ "//--<-- group elements end for " + obj->Name;
    }
    else if (obj->ObjectType->isGrid())
    {
        script += "\n";
        script += "\n" + QString(" ").repeated(ident)+ "//=== Optical grid object is not supported! Make a request to the developers ===";
        script += "\n";
    }
}

const QString AGeoTreeWidget::makeScriptString_basicObject(AGeoObject* obj, bool bExpandMaterials) const
{
    return  QString("geo.TGeo( ") +
            "'" + obj->Name + "', " +
            "'" + obj->Shape->getGenerationString() + "', " +
            (bExpandMaterials && obj->Material < Sandwich->GetMaterials().size() ?
                 Sandwich->GetMaterials().at(obj->Material) + "_mat" : QString::number(obj->Material)) + ", "
            "'"+obj->Container->Name + "',   "+
            QString::number(obj->Position[0]) + ", " +
            QString::number(obj->Position[1]) + ", " +
            QString::number(obj->Position[2]) + ",   " +
            QString::number(obj->Orientation[0]) + ", " +
            QString::number(obj->Orientation[1]) + ", " +
            QString::number(obj->Orientation[2]) + " )";
}

QString AGeoTreeWidget::makeScriptString_arrayObject(AGeoObject *obj)
{
    ATypeArrayObject* a = dynamic_cast<ATypeArrayObject*>(obj->ObjectType);
    if (!a)
    {
        qWarning() << "It is not an array!";
        return "Error accessing object as array!";
    }

    return  QString("geo.Array( ") +
            "'" + obj->Name + "', " +
            QString::number(a->numX) + ", " +
            QString::number(a->numY) + ", " +
            QString::number(a->numZ) + ",   " +
            QString::number(a->stepX) + ", " +
            QString::number(a->stepY) + ", " +
            QString::number(a->stepZ) + ", " +
            "'" + obj->Container->Name + "',   " +
            QString::number(obj->Position[0]) + ", " +
            QString::number(obj->Position[1]) + ", " +
            QString::number(obj->Position[2]) + ",   " +
            QString::number(obj->Orientation[2]) + " )";
}

const QString AGeoTreeWidget::makeScriptString_monitorBaseObject(const AGeoObject * obj) const
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
            .arg(2.0*c.size1)
            .arg(2.0*c.size2)
            .arg("'" + obj->Container->Name + "'")
            .arg(obj->Position[0])
            .arg(obj->Position[1])
            .arg(obj->Position[2])
            .arg(obj->Orientation[0])
            .arg(obj->Orientation[1])
            .arg(obj->Orientation[2])
            .arg(c.bUpper ? "true" : "false")
            .arg(c.bLower ? "true" : "false")
            .arg(c.bStopTracking ? "true" : "false");
}

const QString AGeoTreeWidget::makeScriptString_monitorConfig(const AGeoObject *obj) const
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

QString AGeoTreeWidget::makeScriptString_stackObjectStart(AGeoObject *obj)
{
    return  QString("geo.MakeStack(") +
            "'" + obj->Name + "', " +
            "'" + obj->Container->Name + "' )";
}

QString AGeoTreeWidget::makeScriptString_groupObjectStart(AGeoObject *obj)
{
    return  QString("geo.MakeGroup(") +
            "'" + obj->Name + "', " +
            "'" + obj->Container->Name + "' )";
}

QString AGeoTreeWidget::makeScriptString_stackObjectEnd(AGeoObject *obj)
{
    return QString("geo.InitializeStack( ") +
           "'" + obj->Name + "',  " +
           "'" + obj->HostedObjects.first()->Name + "' )";
}

QString AGeoTreeWidget::makeLinePropertiesString(AGeoObject *obj)
{
    return "geo.SetLine( '" +
            obj->Name +
            "',  " +
            QString::number(obj->color) + ",  " +
            QString::number(obj->width) + ",  " +
            QString::number(obj->style) + " )";
}

AWorldDelegate::AWorldDelegate(const QStringList & materials, QWidget * ParentWidget) :
    AGeoBaseDelegate(ParentWidget)
{
    QFrame * frMainFrame = new QFrame();
    frMainFrame->setFrameShape(QFrame::Box);

    Widget = frMainFrame;
    Widget->setContextMenuPolicy(Qt::CustomContextMenu);

    QPalette palette = frMainFrame->palette();
    palette.setColor( Widget->backgroundRole(), QColor( 255, 255, 255 ) );
    frMainFrame->setPalette( palette );
    frMainFrame->setAutoFillBackground( true );

    QVBoxLayout * lMF = new QVBoxLayout();
      lMF->setContentsMargins(5,5,5,2);

      QLabel * labType = new QLabel("World");
      labType->setAlignment(Qt::AlignCenter);
      QFont font = labType->font();
      font.setBold(true);
      labType->setFont(font);
      lMF->addWidget(labType);

      QHBoxLayout* hl = new QHBoxLayout();
        hl->setContentsMargins(2,0,2,0);

        QLabel * lMat = new QLabel();
        lMat->setText("Material:");
        lMat->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lMat->setMaximumWidth(60);
        hl->addWidget(lMat);

        cobMat = new QComboBox();
        cobMat->setContextMenuPolicy(Qt::NoContextMenu);
        cobMat->addItems(materials);
        //connect(cobMat, &QComboBox::activated, this, &AWorldDelegate::onContentChanged);
        connect(cobMat, SIGNAL(activated(int)), this, SLOT(onContentChanged()));
        cobMat->setMinimumWidth(120);
        hl->addWidget(cobMat);
      lMF->addLayout(hl);

      QHBoxLayout * h = new QHBoxLayout();
            h->addStretch();
            cbFixedSize = new QCheckBox("Fixed size");
            connect(cbFixedSize, &QCheckBox::clicked, this, &AWorldDelegate::onContentChanged);
            h->addWidget(cbFixedSize);

            QVBoxLayout * v1 = new QVBoxLayout();
                v1->setContentsMargins(2,0,2,0);
                v1->addWidget(new QLabel("Size XY:"));
                v1->addWidget(new QLabel("Size Z:"));
            h->addLayout(v1);

            QVBoxLayout * v2 = new QVBoxLayout();
                v2->setContentsMargins(2,0,2,0);
                ledSizeXY = new QLineEdit();
                connect(ledSizeXY, &QLineEdit::textChanged, this, &AWorldDelegate::onContentChanged);
                v2->addWidget(ledSizeXY);
                ledSizeZ  = new QLineEdit();
                connect(ledSizeZ, &QLineEdit::textChanged, this, &AWorldDelegate::onContentChanged);
                v2->addWidget(ledSizeZ);
            h->addLayout(v2);
            h->addStretch();
    lMF->addLayout(h);

    frMainFrame->setLayout(lMF);

    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    ledSizeXY->setValidator(dv);
    ledSizeZ->setValidator(dv);
}

const QString AWorldDelegate::getName() const
{
    return "World";
}

bool AWorldDelegate::isValid(AGeoObject *)
{
    return true;
}

void AWorldDelegate::updateObject(AGeoObject * obj) const
{
    obj->Material = cobMat->currentIndex();
    if (obj->Material == -1) obj->Material = 0; //protection

    AGeoBox * box = static_cast<AGeoBox*>(obj->Shape);
    box->dx = 0.5 * ledSizeXY->text().toDouble();
    box->dz = 0.5 * ledSizeZ->text().toDouble();
    ATypeWorldObject * typeWorld = static_cast<ATypeWorldObject *>(obj->ObjectType);
    typeWorld->bFixedSize = cbFixedSize->isChecked();
}

void AWorldDelegate::Update(const AGeoObject *obj)
{
    int imat = obj->Material;
    if (imat < 0 || imat >= cobMat->count())
    {
        qWarning() << "Material index out of bounds!";
        imat = -1;
    }
    cobMat->setCurrentIndex(imat);

    const AGeoBox * box = static_cast<const AGeoBox*>(obj->Shape);
    ledSizeXY->setText(QString::number(box->dx*2.0));
    ledSizeZ->setText(QString::number(box->dz*2.0));
    ATypeWorldObject * typeWorld = static_cast<ATypeWorldObject *>(obj->ObjectType);
    cbFixedSize->setChecked(typeWorld->bFixedSize);
}

void AWorldDelegate::onContentChanged()
{
    emit ContentChanged();
}
