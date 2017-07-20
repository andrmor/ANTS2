#include "ageotreewidget.h"
#include "ageoobject.h"
#include "ashapehelpdialog.h"
#include "arootlineconfigurator.h"
#include "aslablistwidget.h"
#include "slab.h"
#include "asandwich.h"
#include "agridelementdialog.h"

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

AGeoTreeWidget::AGeoTreeWidget(ASandwich *Sandwich) : Sandwich(Sandwich)
{
  World = Sandwich->World;

  connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
  setHeaderLabels(QStringList() << "Tree of geometry objects");

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
  //w->setSizeHint(0, QSize(50,50));

  //qDebug() << "New world WidgetItem created";

  populateTreeWidget(w, World);
  //expandAll(); // less blunt later - e.g. on exapand remember the status in AGeoObject
  if (topLevelItemCount()>0)
    updateExpandState(this->topLevelItem(0));

  if (!selected.isEmpty())
    {
      //qDebug() << "Selection:"<<selected;
      QList<QTreeWidgetItem*> list = findItems(selected, Qt::MatchExactly | Qt::MatchRecursive);
        if (!list.isEmpty())
        {
           //qDebug() << "Attempting to focus:"<<list.first()->text(0);
           list.first()->setSelected(true);
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

    AGridElementDialog* d = new AGridElementDialog(this);
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

    int res = d->exec();

    if (res != 0)
    {
        //qDebug() << "Accepted!";
        switch (d->shape())
        {
        case 0: Sandwich->shapeGrid(obj, 0, d->pitch(), d->length(), d->diameter()); break;
        case 1: Sandwich->shapeGrid(obj, 1, d->pitchX(), d->pitchY(), d->diameter()); break;
        case 2: Sandwich->shapeGrid(obj, 2, d->outer(), d->inner(), d->height()); break;
        default:
            qWarning() << "Unknown grid type!";
        }

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
  QList<QTreeWidgetItem*> selected = selectedItems();

  QTreeWidgetItem* itemTo = this->itemAt(event->pos());
  if (!itemTo)
    {
      qDebug() << "No item on drop position - rejected!";
      event->ignore();
      return;
    }
  QString DraggedTo = itemTo->text(0);
  //qDebug() << "To:"<< DraggedTo;
  AGeoObject* objTo = World->findObjectByName(DraggedTo);
  if (!objTo)
  {
      qWarning() << "Error: objTo not found!";
      event->ignore();
      return;
  }


  QStringList selNames;

  bool fAfter = false;
  if (event->pos().y() > visualItemRect(itemTo).center().y())
    fAfter = true;

  //if Ctrl or Alt is pressed, its rearrangment event
  if (event->keyboardModifiers() == Qt::ALT)
    {
      // Rearranging event!
      //qDebug() << "Rearrange order event triggered";
      if (selected.size() != 1)
        {
          //qDebug() << "Only one item should be selected to use rearrangment!";
          event->ignore();
          return;
        }
      QTreeWidgetItem* DraggedItem = this->selectedItems().first();
      if (!DraggedItem)
        {
          //qDebug() << "Drag source item invalid, ignore";
          event->ignore();
          return;
        }
      QString DraggedName = DraggedItem->text(0);
      selNames << DraggedName;

      AGeoObject* obj = World->findObjectByName(DraggedName);
      if (obj)
        obj->repositionInHosted(objTo, fAfter);

      if (obj->Container && obj->Container->ObjectType->isStack())
        obj->updateStack();

      //qDebug() << "Affected items:"<< DraggedName<<DraggedTo;
      //Model->swapObjects(DraggedName, DraggedTo);
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
  //qDebug() << "Drag enter. Selection size:"<< selectedItems().size();
  //attempt to drag items contaning locked objects should be canceled!
  for (int i=0; i<selectedItems().size(); i++)
    {
      QTreeWidgetItem* DraggedItem = selectedItems().at(i);
      QString DraggedName = DraggedItem->text(0);
      //qDebug() << "Draggin item:"<< DraggedName;
      AGeoObject* obj = World->findObjectByName(DraggedName);
      if (obj->fLocked || obj->isContainsLocked() || obj->ObjectType->isGridElement() || obj->ObjectType->isCompositeContainer())
        {
           qDebug() << "Drag forbidden for one of the items!";
           event->ignore();
           return;
        }
    }
  event->accept();
}

//void AGeoTreeWidget::dragMoveEvent(QDragMoveEvent *event)
//{
//  QTreeWidget::dragMoveEvent(event);
//}

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

  QAction* newA  = Action(menu, "Add object");
  QFont f = newA->font();
  f.setBold(true);
  newA->setFont(f);
  QAction* newArrayA  = Action(menu, "Add array");
  QAction* newCompositeA  = Action(menu, "Add composite object");
  QAction* newGridA = Action(menu, "Add optical grid");
  QAction* newMonitorA = Action(menu, "Add monitor");

  menu.addSeparator();

  QAction* copyA = Action(menu, "Duplicate this object");

  menu.addSeparator();

  QAction* removeA = Action(menu, "Remove");
  QAction* removeThisAndHostedA = Action(menu, "Remove object recursively");
  QAction* removeHostedA = Action(menu, "Remove objects inside");

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

      newA->setEnabled(fNotGridNotMonitor);
      enableDisableA->setEnabled(true);      
      enableDisableA->setText( (obj->isDisabled() ? "Enable object" : "Disable object" ) );
      if (obj->getSlabModel())
          if (obj->getSlabModel()->fCenter) enableDisableA->setEnabled(false);

      newCompositeA->setEnabled(fNotGridNotMonitor);
      newArrayA->setEnabled(fNotGridNotMonitor && !ObjectType.isArray());
      newMonitorA->setEnabled(fNotGridNotMonitor && !ObjectType.isArray());
      newGridA->setEnabled(fNotGridNotMonitor);
      copyA->setEnabled( ObjectType.isSingle() || ObjectType.isSlab());  //supported so far only Single and Slab
      removeHostedA->setEnabled(fNotGridNotMonitor);
      removeThisAndHostedA->setEnabled(fNotGridNotMonitor);
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
  else if (SelectedAction == newA) // ADD NEW OBJECT
     menuActionAddNewObject(selected.first()->text(0));
  else if (SelectedAction == newCompositeA) //ADD NEW COMPOSITE
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

void AGeoTreeWidget::menuActionRemove()
{
  QList<QTreeWidgetItem*> selected = selectedItems();

  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle("Locked objects will NOT be removed!");
  QString str = "\nThis command, executed on a slab or lightguide removes it too!";
  if (selected.size() == 1)  msgBox.setText("Remove "+selected.first()->text(0)+"?"+str);
  else msgBox.setText("Remove selected objects?"+str);
  QPushButton *remove = msgBox.addButton("Remove", QMessageBox::ActionRole);
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
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle("Locked objects will NOT be removed!");
  msgBox.setText("Remove the object and all objects hosted inside?\nSlabs and lightguides are NOT removed.");
  QPushButton *remove = msgBox.addButton("Remove", QMessageBox::ActionRole);
  QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
  msgBox.setDefaultButton(cancel);

  msgBox.exec();

  if (msgBox.clickedButton() == remove)
    {
      //emit ObjectSelectionChanged("");
      AGeoObject* obj = World->findObjectByName(ObjectName);
      if (obj) obj->recursiveSuicide();
      UpdateGui();
      emit RequestRebuildDetector();
    }
}

void AGeoTreeWidget::menuActionRemoveHostedObjects(QString ObjectName)
{
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle("Locked objects will NOT be deleted!");
  msgBox.setText("Delete objects hosted inside " + ObjectName + "?\nSlabs and lightguides are NOT removed.");
  QPushButton *remove = msgBox.addButton("Delete", QMessageBox::ActionRole);
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

  if ( !(ObjToCopy->ObjectType->isSingle() || ObjToCopy->ObjectType->isSlab()) ) return; //supported so far only Single and Slab

  if (ObjToCopy->ObjectType->isSlab())
  {
    ATypeSlabObject* slab = static_cast<ATypeSlabObject*>(ObjToCopy->ObjectType);
    ObjToCopy->UpdateFromSlabModel(slab->SlabModel);
  }

  AGeoObject* newObj = new AGeoObject(ObjToCopy);
  while (World->isNameExists(newObj->Name))
    newObj->Name = AGeoObject::GenerateRandomObjectName();

  AGeoObject* container = ObjToCopy->Container;
  if (!container) container = World;
  container->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!

  QString name = newObj->Name;
  UpdateGui(name);
  emit RequestRebuildDetector();
  emit RequestHighlightObject(name);
  UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewObject(QString ContainerName)
{
  AGeoObject* ContObj = World->findObjectByName(ContainerName);
  if (!ContObj) return;

  AGeoObject* newObj = new AGeoObject();
  while (World->isNameExists(newObj->Name))
    newObj->Name = AGeoObject::GenerateRandomObjectName();

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
    qDebug() << "Add new monitor requested";

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
    UpdateGui(name);
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
  if (obj)
    {
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
          emit RequestRebuildDetector();
          UpdateGui(ObjectName);
      }
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
  CurrentObject = 0;
  GeoObjectDelegate = 0;
  SlabDelegate = 0;
  GridDelegate = 0;
  MonitorDelegate = 0;
  fIgnoreSignals = true;
  fEditingMode = false;

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
  sa->setToolTip("Use the context menu of the onbject box to manipulate it");

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

  while(ObjectLayout->count()>0)
    {
        QLayoutItem* item = ObjectLayout->takeAt(0);
//          if (item->layout()) {
//              clearLayout(item->layout());
//              delete item->layout();
//          }
        if (item->widget())
          delete item->widget();
        delete item;
    }


  if (GeoObjectDelegate)
    {
      //delete CurrentObjectDelegate->Widget;
      delete GeoObjectDelegate;
      GeoObjectDelegate = 0;
    }    
  if (SlabDelegate)
    {
      //delete CurrentSlabDelegate->frMain;
      delete SlabDelegate;
      SlabDelegate = 0;
    }
  if (MonitorDelegate)
    {
      delete MonitorDelegate;
      MonitorDelegate = 0;
    }

  fIgnoreSignals = false;

  //if update triggered during editing
  exitEditingMode();
}

void AGeoWidget::UpdateGui()
{  
  //qDebug() << "UpdateGui triggered";
  ClearGui(); //deletes CurrentObjectDelegate

  if (!CurrentObject) return;

  pbConfirm->setEnabled(true);
  pbCancel->setEnabled(true);

  AGeoObject* contObj = CurrentObject->Container;
  if (!contObj) return; //true only for World

  if (CurrentObject->ObjectType->isSlab()) // SLAB and LIGHGUIDE
    {   //Slab-based
        QString str = "Slab";
        if (CurrentObject->ObjectType->isLightguide())
          {
            if (CurrentObject->ObjectType->isUpperLightguide()) str += ", Upper lightguide";
            else if (CurrentObject->ObjectType->isLowerLightguide()) str += ", Lower lightguide";
            else str = "Lightguide error!";
          }
        addInfoLabel(str);

        SlabDelegate = createAndAddSlabDelegate(CurrentObject);
        SlabDelegate->frMain->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(SlabDelegate->frMain, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnCustomContextMenuTriggered_forMainObject(QPoint)));
    }
  else if (CurrentObject->ObjectType->isGridElement())
    {
        // special for grid element delegate
        addInfoLabel("Elementary block of the grid");
        GridDelegate = createAndAddGridElementDelegate(CurrentObject);
    }
  else if (CurrentObject->ObjectType->isMonitor())
    {
        addInfoLabel("Monitor");
        QStringList particles;
        emit tw->RequestListOfParticles(particles);
        MonitorDelegate = createAndAddMonitorDelegate(CurrentObject, particles);
    }
  else
  {   // Normal AGeoObject based

      if (CurrentObject->isCompositeMemeber())
          {
            addInfoLabel("Logical object");
            GeoObjectDelegate = createAndAddGeoObjectDelegate(CurrentObject);
          }
      else if (CurrentObject->ObjectType->isSingle())  // NORMAL
          {
            addInfoLabel("Object"+getSuffix(contObj));
            GeoObjectDelegate = createAndAddGeoObjectDelegate(CurrentObject);
            connect(GeoObjectDelegate->Widget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnCustomContextMenuTriggered_forMainObject(QPoint)));
          }
      else if (CurrentObject->ObjectType->isGrid())  // Grid
          {
            addInfoLabel("Grid bulk"+getSuffix(contObj));
            GeoObjectDelegate = createAndAddGeoObjectDelegate(CurrentObject);
            connect(GeoObjectDelegate->Widget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnCustomContextMenuTriggered_forMainObject(QPoint)));
          }
      else if (CurrentObject->ObjectType->isHandlingSet())  // SET
          {
            if (CurrentObject->ObjectType->isCompositeContainer()) return;
            QString str = ( CurrentObject->ObjectType->isStack() ? "Stack" : "Group" );
            addInfoLabel( str );
            GeoObjectDelegate = createAndAddGeoObjectDelegate(CurrentObject);
          }
      else if (CurrentObject->ObjectType->isComposite())
          {
            addInfoLabel("Composite object"+getSuffix(contObj));
            GeoObjectDelegate = createAndAddGeoObjectDelegate(CurrentObject);
            connect(GeoObjectDelegate->Widget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnCustomContextMenuTriggered_forMainObject(QPoint)));
          }
      else if (CurrentObject->ObjectType->isArray())
        {
          addInfoLabel("Array"+getSuffix(contObj));
          GeoObjectDelegate = createAndAddGeoObjectDelegate(CurrentObject);
        }
  }
}

AGeoObjectDelegate* AGeoWidget::createAndAddGeoObjectDelegate(AGeoObject* obj)
{
    AGeoObjectDelegate* Del = new AGeoObjectDelegate(obj->Name, tw->Sandwich->Materials);    
    Del->Update(obj);
    Del->Widget->setEnabled(!CurrentObject->fLocked);  //CurrentObject here!!!
    ObjectLayout->addWidget(Del->Widget);
    ObjectLayout->addStretch();    
    connect(Del, SIGNAL(ContentChanged()), this, SLOT(onStartEditing()));
    return Del;
}

ASlabDelegate* AGeoWidget::createAndAddSlabDelegate(AGeoObject* obj)
{
    ASlabDelegate* Del = new ASlabDelegate();
    Del->leName->setMinimumWidth(50);
    QPalette palette = Del->frMain->palette();
    palette.setColor( backgroundRole(), QColor( 255, 255, 255 ) );
    Del->frMain->setPalette( palette );
    Del->frMain->setAutoFillBackground( true );
    Del->frMain->setMaximumHeight(80);
    connect(Del->XYdelegate, SIGNAL(ContentChanged()), Del->XYdelegate, SLOT(updateComponentVisibility()));

    switch (tw->Sandwich->SandwichState)
      {
      case (0): //ASandwich::CommonShapeSize
        Del->XYdelegate->SetShowState(ASlabXYDelegate::ShowNothing); break;
      case (1): //ASandwich::CommonShape
        Del->XYdelegate->SetShowState(ASlabXYDelegate::ShowSize); break;
      case (2): //ASandwich::Individual
        Del->XYdelegate->SetShowState(ASlabXYDelegate::ShowAll); break;
      default:
        qWarning() << "Unknown DetectorSandwich state!"; break;
      }

    //updating material list
    Del->comMaterial->clear();
    Del->comMaterial->addItems(tw->Sandwich->Materials);

    ASlabModel* SlabModel = (static_cast<ATypeSlabObject*>(obj->ObjectType))->SlabModel;
    Del->UpdateGui(*SlabModel);

    Del->frCenterZ->setVisible(false);
    Del->fCenter = false;
    Del->frColor->setVisible(false);
    Del->cbOnOff->setEnabled(!SlabModel->fCenter);

    Del->frMain->setMinimumHeight(75);
    ObjectLayout->addWidget(Del->frMain);
    ObjectLayout->addStretch();
    connect(Del, SIGNAL(ContentChanged()), this, SLOT(onStartEditing()));
    return Del;
}

AGridElementDelegate *AGeoWidget::createAndAddGridElementDelegate(AGeoObject *obj)
{
    AGridElementDelegate* Del = new AGridElementDelegate(obj->Name);
    Del->Update(obj);
    Del->Widget->setEnabled(!CurrentObject->fLocked);
    ObjectLayout->addWidget(Del->Widget);
    ObjectLayout->addStretch();
    connect(Del, SIGNAL(ContentChanged()), this, SLOT(onStartEditing()));
    connect(Del, SIGNAL(RequestReshapeGrid(QString)), tw, SLOT(onGridReshapeRequested(QString)));
    return Del;
}

AMonitorDelegate *AGeoWidget::createAndAddMonitorDelegate(AGeoObject *obj, QStringList particles)
{
    AMonitorDelegate* Del = new AMonitorDelegate(particles);
    Del->Update(obj);
    Del->Widget->setEnabled(!CurrentObject->fLocked);
    ObjectLayout->addWidget(Del->Widget);
    ObjectLayout->addStretch();
    connect(Del, SIGNAL(ContentChanged()), this, SLOT(onStartEditing()));
    //connect(Del, SIGNAL(RequestReshapeGrid(QString)), tw, SLOT(onGridReshapeRequested(QString)));
    return Del;
}

QString AGeoWidget::getSuffix(AGeoObject* objCont)
{
  if (!objCont) return "";

  if (objCont->ObjectType->isHandlingStandard())
    return "";
  if (objCont->ObjectType->isHandlingSet())
    {
      if (objCont->ObjectType->isGroup())
        return ", group member";
      else
        return ", stack member";
    }
  return "";
}

void AGeoWidget::onObjectSelectionChanged(const QString SelectedObject)
{  
  CurrentObject = 0;
  //qDebug() << "Object selection changed! ->" << SelectedObject;
  ClearGui();

  AGeoObject* obj = World->findObjectByName(SelectedObject);
  if (!obj) return;

  if (obj->ObjectType->isWorld()) return;

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
  else if (SelectedAction == showA)
  {
      QString name = CurrentObject->Name;
      emit tw->RequestHighlightObject(name);
      tw->UpdateGui(name);
  }
  else if (SelectedAction == clipA)
  {
      QString c = "TGeo( 'N_A_M_E', '" + CurrentObject->Shape->getGenerationString() + "', " +
              QString::number(CurrentObject->Material) + ", " +
              "'"+CurrentObject->Container->Name + "',   "+
              QString::number(CurrentObject->Position[0]) + ", " +
              QString::number(CurrentObject->Position[1]) + ", " +
              QString::number(CurrentObject->Position[2]) + ",   " +
              QString::number(CurrentObject->Orientation[0]) + ", " +
              QString::number(CurrentObject->Orientation[1]) + ", " +
              QString::number(CurrentObject->Orientation[2]) + " )";

      QClipboard *clipboard = QApplication::clipboard();
      clipboard->setText(c);
  }
  else if (SelectedAction == setLineA)
    {
          ARootLineConfigurator* rlc = new ARootLineConfigurator(&CurrentObject->color, &CurrentObject->width, &CurrentObject->style, this);
          int res = rlc->exec();
          if (res != 0)
          {
              if (CurrentObject->ObjectType->isSlab())
              {
                  ATypeSlabObject* slab = static_cast<ATypeSlabObject*>(CurrentObject->ObjectType);
                  slab->SlabModel->color = CurrentObject->color;
                  slab->SlabModel->width = CurrentObject->width;
                  slab->SlabModel->style = CurrentObject->style;
              }
              QStringList names;
              names << CurrentObject->Name;
              emit tw->RequestRebuildDetector();
              tw->SelectObjects(names);
          }
  } 
}

void AGeoWidget::addInfoLabel(QString text)
{
  ObjectLayout->addStretch(0);

  QLabel* l1 = new QLabel(text);
  l1->setMaximumHeight(20);
  l1->setAlignment(Qt::AlignCenter);
  ObjectLayout->addWidget(l1);
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
  if (SlabDelegate)
    {
      confirmChangesForSlab();  // SLAB including LIGHTGUIDE goes here
      return;
    }
  else if (GridDelegate)
    {
      confirmChangesForGridDelegate(); // Grid element processed here
      return;
    }
  else if (MonitorDelegate)
    {
      confirmChangesForMonitorDelegate(); // Monitor processed here
      return;
    }
  else if (!GeoObjectDelegate)
    {
      qWarning() << "Confirm triggered without CurrentObject!";
      exitEditingMode();
      tw->UpdateGui();
      return;
    }

  //qDebug() << "Validating update data for object" << CurrentObject->Name;
  bool ok = checkNonSlabObjectDelegateValidity(CurrentObject);
  if (!ok) return;

  //qDebug() << "Validation success, can assign new values!";
  getValuesFromNonSlabDelegates(CurrentObject);

  //finalizing
  exitEditingMode();
  QString name = CurrentObject->Name;
  tw->UpdateGui(name);
  emit tw->RequestRebuildDetector();
  tw->UpdateGui(name);
}

void AGeoWidget::getValuesFromNonSlabDelegates(AGeoObject* objMain)
{
    QString oldName = objMain->Name;
    QString newName = GeoObjectDelegate->leName->text();
    objMain->Name = newName;

    if ( objMain->ObjectType->isHandlingSet() )
      {
        //set container object does not have material and shape
      }
    else
      {
        objMain->Material = GeoObjectDelegate->cobMat->currentIndex();
        if (objMain->Material == -1) objMain->Material = 0; //protection

        //inherit materials for composite members
        if (objMain->isCompositeMemeber())
        {
           if (objMain->Container && objMain->Container->Container)
             objMain->Material = objMain->Container->Container->Material;
        }

        QString newShape = GeoObjectDelegate->pteShape->document()->toPlainText();
        objMain->readShapeFromString(newShape);

        //if it is a set member, need old values of position and angle
        QVector<double> old;
        old << objMain->Position[0]    << objMain->Position[1]    << objMain->Position[2]
            << objMain->Orientation[0] << objMain->Orientation[1] << objMain->Orientation[2];

        objMain->Position[0] = GeoObjectDelegate->ledX->text().toDouble();
        objMain->Position[1] = GeoObjectDelegate->ledY->text().toDouble();
        objMain->Position[2] = GeoObjectDelegate->ledZ->text().toDouble();
        objMain->Orientation[0] = GeoObjectDelegate->ledPhi->text().toDouble();
        objMain->Orientation[1] = GeoObjectDelegate->ledTheta->text().toDouble();
        objMain->Orientation[2] = GeoObjectDelegate->ledPsi->text().toDouble();

        // checking was there a rotation of the main object
        bool fWasRotated = false;
        for (int i=0; i<3; i++)
          if (objMain->Orientation[i] != old[3+i])
            {
              fWasRotated =true;
              break;
            }
        //qDebug() << "--Was rotated?"<< fWasRotated;

        //for grouped object, taking into accound the shift
        if (objMain->Container && objMain->Container->ObjectType->isGroup())
          {
            for (int iObj=0; iObj<objMain->Container->HostedObjects.size(); iObj++)
              {
                AGeoObject* obj = objMain->Container->HostedObjects[iObj];
                if (obj == objMain) continue;

                //center vector for rotation
                //in TGeoRotation, first rotation iz around Z, then new X(manual is wrong!) and finally around new Z
                TVector3 v(obj->Position[0]-old[0], obj->Position[1]-old[1], obj->Position[2]-old[2]);

                //first rotate back to origin in rotation
                rotate(&v, -old[3+0], 0, 0);
                rotate(&v, 0, -old[3+1], 0);
                rotate(&v, 0, 0, -old[3+2]);
                rotate(&v, objMain->Orientation[0], objMain->Orientation[1], objMain->Orientation[2]);

                for (int i=0; i<3; i++)
                  {
                    double delta = objMain->Position[i] - old[i]; //shift in position

                    if (fWasRotated)
                      {
                        //shift due to rotation  +  global shift
                        obj->Position[i] = old[i]+v[i] + delta;
                        //rotation of the object
                        double deltaAng = objMain->Orientation[i] - old[3+i];
                        obj->Orientation[i] += deltaAng;
                      }
                    else
                      obj->Position[i] += delta;
                  }
              }
          }
        //for stack:
        if (objMain->Container && objMain->Container->ObjectType->isStack())
            objMain->updateStack();
      }

    //additional post-processing
    if ( objMain->ObjectType->isArray() )
    {
        //additional properties for array
        ATypeArrayObject* array = static_cast<ATypeArrayObject*>(objMain->ObjectType);
        array->numX = GeoObjectDelegate->sbNumX->value();
        array->numY = GeoObjectDelegate->sbNumY->value();
        array->numZ = GeoObjectDelegate->sbNumZ->value();
        array->stepX = GeoObjectDelegate->ledStepX->text().toDouble();
        array->stepY = GeoObjectDelegate->ledStepY->text().toDouble();
        array->stepZ = GeoObjectDelegate->ledStepZ->text().toDouble();
    }
    else if (objMain->ObjectType->isComposite())
    {
        AGeoObject* logicals = objMain->getContainerWithLogical();
        if (logicals)
            logicals->Name = "CompositeSet_"+objMain->Name;
    }
    else if (objMain->ObjectType->isGrid())
    {
        AGeoObject* GE = objMain->getGridElement();
        if (GE) 
		{
            GE->Name = "GridElement_"+objMain->Name;
			GE->Material = objMain->Material;
		}
    }
    else if (objMain->isCompositeMemeber())
    {
        AGeoObject* cont = objMain->Container;
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

bool AGeoWidget::checkNonSlabObjectDelegateValidity(AGeoObject* obj)
{
    //name of the main object
    QString newName = GeoObjectDelegate->leName->text();
    if (newName!=obj->Name && World->isNameExists(newName))
      {
        QMessageBox::warning(this, "", "This name already exists: "+newName);
        return false;
      }

    if ( obj->ObjectType->isHandlingSet())
      {
        //for Set object there is no shape to check
      }
    else
      { // this is normal or composite object then
        //if composite, first check all members
        QString newShape = GeoObjectDelegate->pteShape->document()->toPlainText();
        //qDebug() << "--> attempt to set shape using string:"<< newShape;

        bool fValid = obj->readShapeFromString(newShape, true); //only checks, no change!
        if (!fValid)
          {
            if (newShape.simplified().startsWith("TGeoArb8"))
              QMessageBox::warning(this, "", "Error parsing shape string for "+newName+"\nCould be non-clockwise order of nodes!");
            else
              QMessageBox::warning(this, "", "Error parsing shape string for "+newName);
            return false;
          }
      }
    return true;
}

void AGeoWidget::confirmChangesForSlab()
{
  //verification
  QString newName = SlabDelegate->leName->text();
  if (newName!=CurrentObject->Name && World->isNameExists(newName))
    {
      QMessageBox::warning(this, "", "This name already exists: "+newName);
      return;
    }

  CurrentObject->Name = newName;
  bool fCenter = CurrentObject->getSlabModel()->fCenter;
  SlabDelegate->UpdateModel(CurrentObject->getSlabModel());
  CurrentObject->getSlabModel()->fCenter = fCenter; //delegate does not remember center status

  exitEditingMode();
  QString name = CurrentObject->Name;
  tw->UpdateGui(name);
  emit tw->RequestRebuildDetector();
  tw->UpdateGui(name);
}

void AGeoWidget::confirmChangesForGridDelegate()
{
    if (!CurrentObject) return;
    if (!CurrentObject->ObjectType->isGridElement()) return;

    ATypeGridElementObject* GE = static_cast<ATypeGridElementObject*>(CurrentObject->ObjectType);
    int shape = GridDelegate->cobShape->currentIndex();
    if (shape == 0)
    {
        if (GE->shape == 2) GE->shape = 1;
    }
    else
    {
        if (GE->shape != 2) GE->shape = 2;
    }
    GE->dz = GridDelegate->ledDZ->text().toDouble();
    GE->size1 = GridDelegate->ledDX->text().toDouble();
    GE->size2 = GridDelegate->ledDY->text().toDouble();
    CurrentObject->updateGridElementShape();

    exitEditingMode();
    QString name = CurrentObject->Name;
    tw->UpdateGui(name);
    emit tw->RequestRebuildDetector();
    tw->UpdateGui(name);
}

void AGeoWidget::confirmChangesForMonitorDelegate()
{
  if (!CurrentObject) return;
  if (!CurrentObject->ObjectType->isMonitor()) return;


  //verification
  QString newName = MonitorDelegate->getName();
  if (newName != CurrentObject->Name && World->isNameExists(newName))
    {
      QMessageBox::warning(this, "", "This name already exists: "+newName);
      return;
    }

  MonitorDelegate->updateObject(CurrentObject);

  exitEditingMode();

  tw->UpdateGui(newName);
  emit tw->RequestRebuildDetector();
  tw->UpdateGui(newName);
}

void AGeoWidget::rotate(TVector3* v, double dPhi, double dTheta, double dPsi)
{
  v->RotateZ( TMath::Pi()/180.0* dPhi );
  TVector3 X(1,0,0);
  X.RotateZ( TMath::Pi()/180.0* dPhi );
  //v.RotateX( TMath::Pi()/180.0* Theta);
  v->Rotate( TMath::Pi()/180.0* dTheta, X);
  TVector3 Z(0,0,1);
  Z.Rotate( TMath::Pi()/180.0* dTheta, X);
 // v.RotateZ( TMath::Pi()/180.0* Psi );
  v->Rotate( TMath::Pi()/180.0* dPsi, Z );
}

void AGeoWidget::onCancelPressed()
{
  exitEditingMode();
  tw->UpdateGui( (CurrentObject) ? CurrentObject->Name : "" );
}

AGeoObjectDelegate::AGeoObjectDelegate(QString name, QStringList materials)
{
  CurrentObject = 0;

  frMainFrame = new QFrame();
  frMainFrame->setFrameShape(QFrame::Box);
  QPalette palette = frMainFrame->palette();
  palette.setColor( backgroundRole(), QColor( 255, 255, 255 ) );
  frMainFrame->setPalette( palette );
  frMainFrame->setAutoFillBackground( true );
  frMainFrame->setMinimumHeight(150);
  frMainFrame->setMaximumHeight(200);
    QVBoxLayout* lMF = new QVBoxLayout();
    lMF->setContentsMargins(5,5,5,2);

    //name and material line
    QHBoxLayout* hl = new QHBoxLayout();
      hl->setContentsMargins(2,0,2,0);
      QLabel* lname = new QLabel();
      lname->setText("Name:");
      lname->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      lname->setMaximumWidth(50);
      hl->addWidget(lname);
      leName = new QLineEdit();
      connect(leName, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      leName->setMaximumWidth(100);
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

    //Shape box and help
    QHBoxLayout* h2 = new QHBoxLayout();
      h2->setContentsMargins(2,0,2,0);

      pteShape = new QPlainTextEdit();
      pteShape->setContextMenuPolicy(Qt::NoContextMenu);
      connect(pteShape, SIGNAL(textChanged()), this, SLOT(onContentChanged()));
      connect(pteShape, SIGNAL(cursorPositionChanged()), this, SLOT(onCursorPositionChanged()));
      pteShape->setMaximumHeight(50);
      new AShapeHighlighter(pteShape->document());
      h2->addWidget(pteShape);

      pbHelp = new QPushButton();
      pbHelp->setText("Help");
      pbHelp->setMaximumWidth(50);
      connect(pbHelp, SIGNAL(pressed()), this, SLOT(onHelpRequested()));
      h2->addWidget(pbHelp);

      //only for arrays
      QFrame* frArr = new QFrame();
      frArr->setFrameShape(QFrame::Box);
      ArrayWid = frArr;
      ArrayWid->setContentsMargins(0,0,0,0);
      ArrayWid->setMaximumHeight(80);
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

      sbNumX = new QSpinBox(this);
      sbNumX->setMaximum(100);
      sbNumX->setMinimum(0);
      sbNumX->setContextMenuPolicy(Qt::NoContextMenu);
      grAW->addWidget(sbNumX, 0, 1);
      connect(sbNumX, SIGNAL(valueChanged(int)), this, SLOT(onContentChanged()));
      sbNumY = new QSpinBox(this);
      sbNumY->setMaximum(100);
      sbNumY->setMinimum(0);
      sbNumY->setContextMenuPolicy(Qt::NoContextMenu);
      grAW->addWidget(sbNumY, 1, 1);
      connect(sbNumY, SIGNAL(valueChanged(int)), this, SLOT(onContentChanged()));
      sbNumZ = new QSpinBox(this);
      sbNumZ->setMaximum(100);
      sbNumZ->setMinimum(0);
      sbNumZ->setContextMenuPolicy(Qt::NoContextMenu);
      grAW->addWidget(sbNumZ, 2, 1);
      connect(sbNumZ, SIGNAL(valueChanged(int)), this, SLOT(onContentChanged()));
      ledStepX = new QLineEdit(this);
      ledStepX->setContextMenuPolicy(Qt::NoContextMenu);
      ledStepX->setMaximumWidth(75);
      connect(ledStepX, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      grAW->addWidget(ledStepX, 0, 3);
      ledStepY = new QLineEdit(this);
      ledStepY->setMaximumWidth(75);
      ledStepY->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledStepY, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      grAW->addWidget(ledStepY, 1, 3);
      ledStepZ = new QLineEdit(this);
      ledStepZ->setMaximumWidth(75);
      ledStepZ->setContextMenuPolicy(Qt::NoContextMenu);
      connect(ledStepZ, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      grAW->addWidget(ledStepZ, 2, 3);
      ArrayWid->setLayout(grAW);

      h2->addWidget(ArrayWid);
    lMF->addLayout(h2);


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

  Widget = frMainFrame;
  Widget->setContextMenuPolicy(Qt::CustomContextMenu);
}

void AGeoObjectDelegate::onCursorPositionChanged()
{
  QString text = pteShape->document()->toPlainText().simplified();
  QList<AGeoShape*> AvailableShapes = AGeoObject::GetAvailableShapes();

  QString templ = "";
  for (AGeoShape* s : AvailableShapes)
    {
      QString type = s->getShapeType() + "(";
      if (text.contains(type))
        {
          templ = s->getShapeTemplate();
          break;
        }
    }

  if (!templ.isEmpty())
    {
      QToolTip::showText( pteShape->viewport()->mapToGlobal( QPoint(pteShape->x(), pteShape->y()) ),
                          templ,
                          pteShape,
                          pteShape->rect(),
                          100000);
      pteShape->setToolTip(templ);
      pteShape->setToolTipDuration(100000);
    }
}

void AGeoObjectDelegate::onHelpRequested()
{
  QList<AGeoShape*> AvailableShapes = AGeoObject::GetAvailableShapes();
  //AvailableShapes << new AGeoBox << new AGeoTrd1 << new AGeoTube << new AGeoSphere << new AGeoPara << new AGeoPgon << new AGeoComposite;

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

  //d->setParent(0);
  for (int i=0; i<AvailableShapes.size(); i++) delete AvailableShapes[i];
  delete d;
}

void AGeoObjectDelegate::Update(const AGeoObject *obj)
{  
    CurrentObject = obj;
    leName->setText(obj->Name);

    if (obj->ObjectType->isHandlingSet())
    {
      pteShape->setVisible(false);
      lMat->setVisible(false);
      cobMat->setVisible(false);
      pbHelp->setVisible(false);
      PosOrient->setVisible(false);
    }

  if (obj->ObjectType->isArray())
  {
    pteShape->setVisible(false);
    lMat->setVisible(false);
    cobMat->setVisible(false);
    pbHelp->setVisible(false);

    ledPhi->setEnabled(false);
    ledTheta->setEnabled(false);

    ATypeArrayObject* array = static_cast<ATypeArrayObject*>(obj->ObjectType);
    sbNumX->setValue(array->numX);
    sbNumY->setValue(array->numY);
    sbNumZ->setValue(array->numZ);
    ledStepX->setText(QString::number(array->stepX));
    ledStepY->setText(QString::number(array->stepY));
    ledStepZ->setText(QString::number(array->stepZ));
  }
  else
     ArrayWid->setVisible(false);

  int imat = obj->Material;
  if (imat<0 || imat>cobMat->count()-1)
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

  if (obj->Container && obj->Container->ObjectType->isStack())
    {
      ledPhi->setEnabled(false);
      ledTheta->setEnabled(false);
      ledPhi->setText("0");
      ledTheta->setText("0");
    }

  if (obj->isCompositeMemeber())
  {
      cobMat->setEnabled(false);
      cobMat->setCurrentIndex(-1);      
  }
  if (obj->ObjectType->isArray())
  {
      ledPhi->setEnabled(false);
      ledTheta->setEnabled(false);
      ledPhi->setText("0");
      ledTheta->setText("0");
  }
}

void AGeoObjectDelegate::onContentChanged()
{
  emit ContentChanged();
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

AGridElementDelegate::AGridElementDelegate(QString name)
{
    CurrentObject = 0;

    frMainFrame = new QFrame(this);
    frMainFrame->setFrameShape(QFrame::Box);
    QPalette palette = frMainFrame->palette();
    palette.setColor( backgroundRole(), QColor( 255, 255, 255 ) );
    frMainFrame->setPalette( palette );
    frMainFrame->setAutoFillBackground( true );
    frMainFrame->setMinimumHeight(150);
    frMainFrame->setMaximumHeight(150);

    QVBoxLayout* vl = new QVBoxLayout();

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

      cobShape = new QComboBox(this);
      cobShape->addItem("Rectangular");
      cobShape->addItem("Hexagonal");
      connect(cobShape, SIGNAL(activated(int)), this, SLOT(onContentChanged()));
      lay->addWidget(cobShape, 0, 1);

      ledDZ = new QLineEdit(this);
      connect(ledDZ, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      lay->addWidget(ledDZ, 0, 3);
      ledDX = new QLineEdit(this);
      connect(ledDX, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      lay->addWidget(ledDX, 1, 1);
      ledDY = new QLineEdit(this);
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
    pbAuto->setText("Open generation dialog");
    pbAuto->setMinimumWidth(200);
    pbAuto->setMaximumWidth(200);
    vl->addWidget(pbAuto);
    vl->setAlignment(pbAuto, Qt::AlignHCenter);

    frMainFrame->setLayout(vl);
    Widget = frMainFrame;

    //installing double validators for edit boxes
    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    ledDX->setValidator(dv);
    ledDY->setValidator(dv);
    ledDZ->setValidator(dv);
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
                "   grid element, so photons cannot cross the border \"sideways\"\n\n"
                "   Grid generation dialog generates a correct grid element automatically";

    QMessageBox::information(this, "", s);
}

#include "amonitordelegateform.h"
AMonitorDelegate::AMonitorDelegate(QStringList definedParticles)
{
    CurrentObject = 0;

    frMainFrame = new QFrame(this);
    frMainFrame->setFrameShape(QFrame::Box);
    QPalette palette = frMainFrame->palette();
    palette.setColor( backgroundRole(), QColor( 255, 255, 255 ) );
    frMainFrame->setPalette( palette );
    frMainFrame->setAutoFillBackground( true );
    frMainFrame->setMinimumHeight(340);
    frMainFrame->setMaximumHeight(340);

    QVBoxLayout* vl = new QVBoxLayout();
    vl->setContentsMargins(0,0,0,0);

    del = new AMonitorDelegateForm(definedParticles, this);
    del->UpdateVisibility();
    connect(del, SIGNAL(contentChanged()), this, SLOT(onContentChanged()));
    vl->addWidget(del);

    /*
    QGridLayout *lay = new QGridLayout();
    lay->setContentsMargins(20, 5, 20, 5);
    lay->setVerticalSpacing(3);

    QLabel *la = new QLabel("Name:");
    lay->addWidget(la, 0, 0);
    leName = new QLineEdit(name, this);
    connect(leName, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
    lay->addWidget(leName, 0, 1);

    la = new QLabel("    Shape:");
    la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lay->addWidget(la, 0, 2);
      cobShape = new QComboBox(this);
      cobShape->addItem("Rectangular");
      cobShape->addItem("Round");
      connect(cobShape, SIGNAL(activated(int)), this, SLOT(onContentChanged()));
      connect(cobShape, SIGNAL(currentIndexChanged(int)), this, SLOT(updateVisibility()));
      lay->addWidget(cobShape, 0, 3);

      lSize1 = new QLabel("dX, mm:");
      lay->addWidget(lSize1, 1, 0);
      ledDX = new QLineEdit(this);
      connect(ledDX, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      lay->addWidget(ledDX, 1, 1);

      lSize2 = new QLabel("    dY, mm:");
      lSize2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
      lay->addWidget(lSize2, 1, 2);
      ledDY = new QLineEdit(this);
      connect(ledDY, SIGNAL(textChanged(QString)), this, SLOT(onContentChanged()));
      lay->addWidget(ledDY, 1, 3);
    vl->addLayout(lay);

    QWidget* PosOrient = new QWidget();
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
    vl->addWidget(PosOrient);

    QHBoxLayout* msLay = new QHBoxLayout();
       la = new QLabel("Monitoring target:");
       la->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
       msLay->addWidget(la);
       cobTarget = new QComboBox();
       cobTarget->addItem("Optical photons");
       cobTarget->addItem("Particles");
       connect(cobTarget, SIGNAL(activated(int)), this, SLOT(onContentChanged()));
       connect(cobTarget, SIGNAL(currentIndexChanged(int)), this, SLOT(updateVisibility()));
       msLay->addWidget(cobTarget);
    vl->addLayout(msLay);

*/
    frMainFrame->setLayout(vl);
    Widget = frMainFrame;

    //installing double validators for edit boxes
//    QDoubleValidator* dv = new QDoubleValidator(this);
//    dv->setNotation(QDoubleValidator::ScientificNotation);
//    ledDX->setValidator(dv);
    //    ledDY->setValidator(dv);
}

QString AMonitorDelegate::getName() const
{
    return del->getName();
}

void AMonitorDelegate::updateObject(AGeoObject *obj)
{
    del->updateObject(obj);
}

//void AMonitorDelegate::updateVisibility()
//{
//    return;
//  if (cobShape->currentIndex() == 0)
//  {  //rectangular
//     lSize1->setText("dX, mm:");
//     lSize2->setVisible(true);
//     ledDY->setVisible(true);
//  }
//  else
//  {
//      lSize1->setText("dR, mm:");
//      lSize2->setVisible(false);
//      ledDY->setVisible(false);
//  }
//}

void AMonitorDelegate::Update(const AGeoObject *obj)
{    
    bool bOK = del->updateGUI(obj);
    if (!bOK) return;

    CurrentObject = obj;
}

void AMonitorDelegate::onContentChanged()
{
    emit ContentChanged();
}
