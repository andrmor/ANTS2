#include "aslablistwidget.h"
#include "slab.h"
#include "asandwich.h"
#include "arootlineconfigurator.h"
#include "ageoobject.h"

#include <QDropEvent>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QMessageBox>

ASlabListWidget::ASlabListWidget(ASandwich* Sandwich, QWidget *parent)
  : QListWidget(parent), Sandwich(Sandwich)
{
  tmpXY = 0;
  DefaultXYdelegate = 0;
  fIgnoreModelUpdateRequests = false;

  setViewMode(QListView::ListMode);
  //setMovement(QListView::Snap);
  setAcceptDrops(true);
  setDragDropMode(QAbstractItemView::InternalMove);
  setDefaultDropAction(Qt::TargetMoveAction);
  setDragEnabled(true);
  setDropIndicatorShown(true);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(customMenuRequested(const QPoint &)));

  //creating default XY delegate
  DefaultXYdelegate = new ASlabXYDelegate();
  DefaultXYdelegate->SetShowState(ASlabXYDelegate::ShowAll);
  DefaultXYdelegate->UpdateGui(*Sandwich->DefaultXY);
  connect(DefaultXYdelegate, SIGNAL(RequestModelUpdate()), this, SLOT(onDefaultXYpropertiesUpdateRequested()));

  setToolTip("Use context menu (Right mouse button) to add/remove/modify slabs!");

  connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onItemDoubleClicked(QListWidgetItem*)));
}

ASlabListWidget::~ASlabListWidget()
{
  if (tmpXY) delete tmpXY;
}

void ASlabListWidget::UpdateGui()
{
  //qDebug() << "--Updating SlabWidget GUI";

  //need to lock, otherwise delegate can trigger onEditingFinished etc
  fIgnoreModelUpdateRequests = true; // ==== //  unlock at the end!!!

  QList<int> RecommendedColors;
  RecommendedColors << 2 << 4 << 7 << 28 << 29 << 30 << 40 << 33 << 41
                    << 36 << 42 << 38 << 43 << 39 << 44 << 46 << 47 << 31 << 48 << 34 << 49;
  //see https://root.cern.ch/doc/master/classTColor.html

  int slabIndex = -1;
  for (int iHO=0; iHO<Sandwich->World->HostedObjects.size(); iHO++)
    {
      AGeoObject* obj = Sandwich->World->HostedObjects[iHO];
      if (!obj->ObjectType->isSlab()) continue;

      slabIndex++; //starts with 0
      //qDebug() << "slabIndex:"<<slabIndex;
      ASlabDelegate* ld;
      if (slabIndex > count()-1)
        {
          //qDebug() << "  --Not enough items, adding one";
          ld = AddNewSlabDelegate();
        }
      else
        {
          //qDebug() << "  --Updating existing one";
          QListWidgetItem* item = this->item(slabIndex);
          ld = static_cast<ASlabDelegate*>(this->itemWidget(item));
        }
      switch (Sandwich->SandwichState)
        {
        case (ASandwich::CommonShapeSize):
          ld->XYdelegate->SetShowState(ASlabXYDelegate::ShowNothing); break;
        case (ASandwich::CommonShape):
          ld->XYdelegate->SetShowState(ASlabXYDelegate::ShowSize); break;
        case (ASandwich::Individual):
          ld->XYdelegate->SetShowState(ASlabXYDelegate::ShowAll); break;
        default:
          qWarning() << "Unknown DetectorSandwich state!"; break;
        }

      //lightguide? drag&drop control
      QPalette palette;// = ld->frMain->palette();
      ld->frMain->setAutoFillBackground( true );
      if (obj->ObjectType->isLightguide())
        {
          palette.setColor(QPalette::Base, QColor( 225, 225, 225 ) );
          ld->frMain->setPalette( palette );
          ld->frMain->setToolTip("This slab is a lightguide! Use 'Advanced settings' to change the lightguide properties");
        }
      else
        {  //have to reset, sometimes we reuse old delegates :)
          palette.setColor(QPalette::Base, QColor( 255, 255, 255 ) );
          ld->frMain->setPalette( palette );
        }


      //updating material list
      ld->comMaterial->clear();
      ld->comMaterial->addItems(Sandwich->GetMaterials());

      if (Sandwich->SandwichState == ASandwich::CommonShape) ld->comMaterial->setMinimumWidth(100);
      else ld->comMaterial->setMinimumWidth(130);

      ASlabModel* SlabModel = (static_cast<ATypeSlabObject*>(obj->ObjectType))->SlabModel;
      //ensure slab material is not empty
      if (!Sandwich->isMaterialsEmpty())
        if (SlabModel->material == -1)
          SlabModel->material = 0;

      ReshapeDelegate(slabIndex);
      ld->UpdateGui(*SlabModel);

      //tooltip
      if (SlabModel->fCenter)
        {
          QString str;
          if (Sandwich->ZOriginType == -1) str = "Top";
          else if (Sandwich->ZOriginType == 1) str = "Bottom";
          else str = "Center";
          ld->setToolTip(str+" of this slab defines detector's Z = 0");
        }
      else ld->setToolTip("");

      //check which colors were already used and remove them from the recommended set
      int col = obj->color;
      if (col != -1) RecommendedColors.removeAll(col);
    }

  //COLORS
  //checking for default names - they are traditionally associated with specific colors
  for (int iHO=0; iHO<Sandwich->World->HostedObjects.size(); iHO++)
    {
      AGeoObject* obj = Sandwich->World->HostedObjects[iHO];
      if (!obj->ObjectType->isSlab()) continue;

      int col = obj->color;
      if (col != -1) continue;

      QString name = obj->Name;
      if (name.startsWith("Sp")) col = 28;
      if (name == "PrScint") col = 2;
      if (name == "SecScint") col = 6;
      if (name == "UpWin") col = 4;
      if (name == "LowWin") col = 4;
      if (col != -1)
        {
          obj->color = col;
          RecommendedColors.removeAll(col);
        }
    }
  //generating automatic colors
  for (int iHO=0; iHO<Sandwich->World->HostedObjects.size(); iHO++)
    {
      AGeoObject* obj = Sandwich->World->HostedObjects[iHO];
      if (!obj->ObjectType->isSlab()) continue;

      int col = obj->color;
      if (col == -1)
      {
          if (RecommendedColors.isEmpty())
            col = 1;
          else
            {
              col = RecommendedColors.first();
              RecommendedColors.removeFirst();
            }
          obj->color = col;
      }

      //update model color too
      obj->getSlabModel()->color = obj->color;
    }


  //removing excess of items
  for (int i=count()-1; i>=Sandwich->countSlabs(); i--)
    {
      //qDebug() << "Removing extra item:"<< i;
      delete takeItem(i);
    }

  DefaultXYdelegate->UpdateGui(*Sandwich->DefaultXY);
  UpdateDefaultXYdelegateVisibility();

  fIgnoreModelUpdateRequests = false; // ==== //
  //qDebug() << "SlabWidget updated!";
}

ASlabDelegate *ASlabListWidget::AddNewSlabDelegate()
{
  ASlabDelegate* ld = new ASlabDelegate();
  ld->leName->setText(ASlabModel::randomSlabName());
  QListWidgetItem* item = ASlabDelegate::CreateNewWidgetItem();
  addItem(item);
  setItemWidget(item, ld);
  connect(ld, SIGNAL(RequestModelUpdate(QWidget*)), this, SLOT(ItemRequestsModelUpdate(QWidget*)));
  return ld;
}

void ASlabListWidget::ReshapeDelegate(int row)
{
  if (row<0 || row>count()-1)
    {
      qWarning() << "Attempt to address non-existing SlabsListWidget row";
      return;
    }

  QListWidgetItem* item = this->item(row);
  ASlabDelegate* ld = static_cast<ASlabDelegate*>(this->itemWidget(item));

  switch (Sandwich->SandwichState)
    {
    case ASandwich::CommonShapeSize:
      ld->frMain->setGeometry(3,2, 358, 30);
      ld->frColor->setGeometry(362,2, 5, 30);
      ld->XYdelegate->setMaximumWidth(15);
      ld->XYdelegate->setMinimumWidth(15);
      item->setSizeHint(QSize(340, 34));
    break;
    case ASandwich::CommonShape:
      ld->frMain->setGeometry(3,2, 358, 30);
      ld->frColor->setGeometry(362,2, 5, 30);
      ld->XYdelegate->setMinimumWidth(100);
      item->setSizeHint(QSize(340, 34));
    break;
    case ASandwich::Individual:
      ld->frMain->setGeometry(3,2, 490, 50);
      ld->frColor->setGeometry(494,2, 5, 50);
      item->setSizeHint(QSize(300, 54));
      ld->XYdelegate->setMinimumWidth(165);
      ld->XYdelegate->setMaximumWidth(165);
    break;
    }
}

void ASlabListWidget::dropEvent(QDropEvent *event)
{
  QListWidgetItem* itemFrom = this->currentItem();
  if (itemFrom)
    {
      int rowFrom = row(itemFrom);
      //qDebug() << "Draggin from "<< rowFrom;
      QListWidgetItem* itemTo = this->itemAt(event->pos());
      int rowTo;
      if (itemTo)
        {
          rowTo = row(itemTo);
          if (event->pos().y() > visualItemRect(itemTo).center().y())
            {
              rowTo++;
              //qDebug() << " --corrected++ since drop position below widget mid";
            }
        }
      else
        rowTo = this->count(); //means after last   And there is no case when row=-1
      //qDebug() << "Drag from->to:"<<rowFrom<<rowTo;

      //finding dragged object in the WorldTree
      AGeoObject* draggedObj = Sandwich->findSlabByIndex(rowFrom);
      if (draggedObj->ObjectType->isLightguide())
        {
          QMessageBox::information(this, "", "Lightguide cannot be dragged!");
          return;
        }
      int dragFromIndex = Sandwich->World->HostedObjects.indexOf(draggedObj);

      AGeoObject* draggedToObj = Sandwich->findSlabByIndex(rowTo);
      int dragToIndex;
      if (!draggedToObj)  //to after last
      {
          draggedToObj = Sandwich->findSlabByIndex(rowTo-1);
          dragToIndex = Sandwich->World->HostedObjects.indexOf(draggedToObj);
          dragToIndex++;
      }
      else        
          dragToIndex = Sandwich->World->HostedObjects.indexOf(draggedToObj);
      if (draggedToObj->ObjectType->isLightguide())
        {
          QMessageBox::information(this, "", "Cannot insert a slab between a lightguide and\nthe corresponding PM array!");
          return;
        }

      Sandwich->World->HostedObjects.insert(dragToIndex, draggedObj);

      //remove record from where drag started
      if (dragToIndex < dragFromIndex) dragFromIndex++;
      Sandwich->World->HostedObjects.removeAt(dragFromIndex);

      UpdateGui();
      Sandwich->UpdateDetector();
    }
  event->accept();
}

void ASlabListWidget::ItemRequestsModelUpdate(QWidget *source)
{
  ASlabDelegate* sourceDelegate = static_cast<ASlabDelegate*>(source);
  //qDebug() << sourceDelegate->leName->text() << "slab requests model update";
  //cannot look by name, it could have changed

  if (Sandwich->countSlabs() != count())
    {
      qCritical() << "Error! Mismatch between count of slab delegates and model records!";
      return;
    }

  //finding the row and the object which delegate was triggered
  //checking was something actually changed (protection from just clicking on components without modifying them)
  int index = 0;
  int row = -1;
  AGeoObject* sourceObj = 0;
  for (int iHO=0; iHO<Sandwich->World->HostedObjects.size(); iHO++)
    {
      AGeoObject* obj = Sandwich->World->HostedObjects[iHO];
      if (!obj->ObjectType->isSlab()) continue;

      //ASlabDelegate* ld = static_cast<ASlabDelegate*>(itemWidget(item(i)));
      ASlabDelegate* ld = static_cast<ASlabDelegate*>(itemWidget(item(index)));
      if (ld == sourceDelegate)
      {
          //this is the one!
          row = index;
          sourceObj = obj;
          break;
      }
      index++;
    }

  if (row == -1)
    {
      qWarning() << "Source slab delegate not found in update, skipping!";
      return;
    }

  if ( *sourceObj->getSlabModel() == sourceDelegate->GetData() )
    {
      //qDebug() << "Nothing changed, request denied!";
      return;
    }

  sourceDelegate->UpdateModel(sourceObj->getSlabModel());
  sourceObj->UpdateFromSlabModel(sourceObj->getSlabModel());

  Sandwich->UpdateDetector();
}

void ASlabListWidget::customMenuRequested(const QPoint &pos)
{
  //qDebug() << "Context menu requested";
  QMenu Menu;
  int row = -1;
  QListWidgetItem* thisItem = itemAt(pos);

  Menu.addSeparator();  
  QAction* showLayer = 0;
  QAction* addLayer = 0;
  QAction* removeLayer = 0;
  QAction* copyXY = 0;
  QAction* pasteXY = 0;
  QAction* setAsSecScint = 0;
  QAction* SetZoriginTop = 0;
  QAction* SetZoriginMid = 0;
  QAction* SetZoriginBot = 0;
  QAction* SetLine = 0;
  if (thisItem)
    { //menu triggered at a valid item
      row = this->row(thisItem);
      AGeoObject* obj = Sandwich->findSlabByIndex(row);

      showLayer = Menu.addAction("Show this slab");
      SetLine = Menu.addAction("Change slab color and line width/style");

      Menu.addSeparator();

      addLayer = Menu.addAction("Insert slab");

      Menu.addSeparator();

      removeLayer = Menu.addAction("Remove this slab");

      Menu.addSeparator();


      Menu.addSeparator();
      copyXY = Menu.addAction("Copy XY properties");
      pasteXY = Menu.addAction("Paste XY properties");
      pasteXY->setEnabled( tmpXY );
      Menu.addSeparator();

      setAsSecScint = Menu.addAction("Set as secondary scintillator");
      Menu.addSeparator();

      //Z=0 control
      SetZoriginTop = Menu.addAction("Z=0 plane is defined by TOP of this slab");
      SetZoriginMid = Menu.addAction("Z=0 plane is defined by CENTER of this slab");
      SetZoriginBot = Menu.addAction("Z=0 plane is defined by BOTTOM of this slab");
      ASlabDelegate* ld = static_cast<ASlabDelegate*>(itemWidget(item(row)));
      bool fOn = ld->cbOnOff->isChecked();
      SetZoriginTop->setEnabled(fOn);
      SetZoriginMid->setEnabled(fOn);
      SetZoriginBot->setEnabled(fOn);
      SetZoriginTop->setCheckable(true);
      SetZoriginMid->setCheckable(true);
      SetZoriginBot->setCheckable(true);
      if (obj->getSlabModel()->fCenter)
        {
          if (Sandwich->ZOriginType == -1) SetZoriginTop->setChecked(true);
          if (Sandwich->ZOriginType == 0) SetZoriginMid->setChecked(true);
          if (Sandwich->ZOriginType == 1) SetZoriginBot->setChecked(true);
        }
    }
  else
    {
       //menu with no slabs selected
       addLayer = Menu.addAction("Insert layer");
    }
  Menu.addSeparator();

  QAction* SelectedAction = Menu.exec(mapToGlobal(pos));
  if (!SelectedAction) return; //nothing was selected

  AGeoObject* obj = Sandwich->findSlabByIndex(row);

  if (SelectedAction == copyXY)
    {
      if (!tmpXY) tmpXY = new ASlabXYModel();
      ASlabDelegate* ld = static_cast<ASlabDelegate*>(itemWidget(item(row)));
      *tmpXY = ld->XYdelegate->GetData();
    }
  else if (SelectedAction == pasteXY && tmpXY)
    {
      //Sandwich->Slabs[row]->XYrecord = *tmpXY;
      obj->getSlabModel()->XYrecord = *tmpXY;
      Sandwich->UpdateDetector();
    }
  else if (SelectedAction == showLayer)
    {
      if (obj) emit RequestHighlightObject(obj->Name);
    }
  else if (SelectedAction==removeLayer && row!=-1 && row<count())
    {
      //qDebug() << "Attempt to delete slab#"<<row;
      //qDebug() << obj->Name;
      if (obj && !obj->HostedObjects.isEmpty())
      {
          QMessageBox::information(this, "", "Cannot remove slab which contains objects inside");
          return;
      }
      else if (obj)
      {
          QMessageBox msgBox;
          //msgBox.setInformativeText("Remove the slab " + obj->Name + "?" );
          msgBox.setText("Remove the slab " + obj->Name + "?" );
          msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
          msgBox.setDefaultButton(QMessageBox::Cancel);
          msgBox.setIcon(QMessageBox::Question);
          int ret = msgBox.exec();

          if (ret == QMessageBox::Yes)
            {
              obj->Container->HostedObjects.removeOne(obj);
              delete obj;
              Sandwich->UpdateDetector();
            }
      }
    }
  else if (SelectedAction == addLayer)
    {
      //qDebug() << "Add slab triggered with row="<<row;
      int iCopyFrom = row;
      if (row != -1)
        {
          if (thisItem && pos.y()>visualItemRect(thisItem).center().y())
            row++; // cursor is after the middle Y -> inserting after, else before
        }

      ASlabModel* newRec = new ASlabModel();
      //try to copy properties of the neighbour
      if (count()>0)
        {
          if (iCopyFrom == -1) iCopyFrom = count()-1;
          //qDebug() << "Copy slab properties: index="<<iCopyFrom;
          AGeoObject* objCopyFrom = Sandwich->findSlabByIndex(iCopyFrom);
          //qDebug() <<"It is slab"<<objCopyFrom->Name;
          newRec->XYrecord = objCopyFrom->getSlabModel()->XYrecord;
          newRec->height = objCopyFrom->getSlabModel()->height;
          newRec->material = objCopyFrom->getSlabModel()->material;
        }

      if (row==-1 || row>count()-1)
        {
          if (Sandwich->getLowerLightguide())
            {
              //there is lower lightguide, should insert before
              //qDebug() << "Cannot append because of lower lightguide, adding before it";
              Sandwich->insertSlab(count()-1, newRec);
            }
          else Sandwich->appendSlab(newRec);
        }
      else
        {
          if (row==0 && Sandwich->getUpperLightguide())
            {
              //there is upper lightguide, should insert after
              //qDebug() << "Cannot insert as first slab because of defined lightguide, adding as next";
              row++;
            }
          Sandwich->insertSlab(row, newRec);
        }

      Sandwich->UpdateDetector();
    }
  else if (SelectedAction == setAsSecScint)
    {
          int slabIndex = -1;
          for (int iHO=0; iHO<Sandwich->World->HostedObjects.size(); iHO++)
            {
              AGeoObject* obj = Sandwich->World->HostedObjects[iHO];
              if (!obj->ObjectType->isSlab()) continue;
              slabIndex++;

              if (slabIndex == row)
                {
                  obj->Name = obj->getSlabModel()->name = "SecScint";
                  continue;
                }
              if (obj->Name == "SecScint")
                obj->Name = obj->getSlabModel()->name = ASlabModel::randomSlabName();
            }
          Sandwich->UpdateDetector();
    }
  else if (SelectedAction==SetZoriginTop || SelectedAction==SetZoriginMid || SelectedAction==SetZoriginBot)
    {
      if (SelectedAction==SetZoriginTop) Sandwich->ZOriginType = -1;
      else if (SelectedAction==SetZoriginBot) Sandwich->ZOriginType = 1;
      else Sandwich->ZOriginType = 0;

      for (int iHO=0; iHO<Sandwich->World->HostedObjects.size(); iHO++)
      {
          AGeoObject* obj1 = Sandwich->World->HostedObjects[iHO];
          if (!obj1->ObjectType->isSlab()) continue;
          obj1->getSlabModel()->fCenter = false;
      }
      obj->getSlabModel()->fCenter = true;
      Sandwich->UpdateDetector();     
    }
  else if (SelectedAction==SetLine)
    {
      AGeoObject* obj = Sandwich->findSlabByIndex(row);
      if (obj)
      {
          ARootLineConfigurator* rlc = new ARootLineConfigurator(&obj->getSlabModel()->color, &obj->getSlabModel()->width, &obj->getSlabModel()->style, this);
          int res = rlc->exec();
          if (res != 0)
          {
              //qDebug() << obj->Name << obj->color;
              obj->color = obj->getSlabModel()->color;
              obj->width = obj->getSlabModel()->width;
              obj->style = obj->getSlabModel()->style;
              Sandwich->UpdateDetector();
          }
      }
    }
}

void ASlabListWidget::onDefaultXYpropertiesUpdateRequested()
{
  //qDebug() << "Update default XY properties requested by GUI";
  if (*Sandwich->DefaultXY == DefaultXYdelegate->GetData())
    {
      //qDebug() << "Nothing changed, request denied";
      return;
    }
  *Sandwich->DefaultXY = DefaultXYdelegate->GetData();

  Sandwich->UpdateDetector();
}

void ASlabListWidget::onItemDoubleClicked(QListWidgetItem *item)
{
  int row = this->row(item);
  AGeoObject* obj = Sandwich->findSlabByIndex(row);
  if (obj)
    emit SlabDoubleClicked(obj->Name);
}

void ASlabListWidget::UpdateDefaultXYdelegateVisibility()
{
  //have to adjust controls visibility according to the CommonXY mode
  switch (Sandwich->SandwichState)
    {
      case (ASandwich::CommonShapeSize):
        break; //show all
      case (ASandwich::CommonShape):
        //hide all sizes
        DefaultXYdelegate->ledSize1->setVisible(false);
        DefaultXYdelegate->ledSize2->setVisible(false);
        break;
      default:
        //dont care in individual: delegate is not visible
        break;
    }
}
