#include "alrfwindow.h"

#include <QSignalMapper>
#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>
#include <QDebug>
#include <QDoubleValidator>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QScrollBar>
#include <QJsonDocument>

#include "amessage.h"
#include "alrfmouseexplorer.h"
#include "reconstructionwindow.h"
#include "alrfdraw.h"
#include "acommonfunctions.h"
#include "geometrywindowclass.h"
#include "apmgroupsmanager.h"
#include "ajsontools.h"
#include "globalsettingsclass.h"
#include "graphwindowclass.h"
#include "windownavigatorclass.h"
#include "aconfiguration.h"

//New Lrf module
#include "alrfwindowwidgets.h"
#include "arepository.h"
#include "alrftypemanager.h"
#include "astateinterface.h"

//For "Make Lrfs"
#include "mainwindow.h"
#include "detectorclass.h"
#include "apmhub.h"
#include "eventsdataclass.h"
#include "ainstructioninput.h"

namespace Widgets = LRF::LrfWindowWidgets;
using namespace LRF;

ALrfWindow::ALrfWindow(QWidget *parent, MainWindow *mw, ARepository *lrf_repo) :
  QMainWindow(parent), mw(mw), repo(lrf_repo)
{
  setupUi(this);
  connect(repo, &ARepository::currentLrfsChangedReadyStatus, this, &ALrfWindow::onReadyStatusChanged);
  lw_instructions->setDragEnabled(false);

  pbUpdateInstructionsJson->setVisible(false);

  QButtonGroup *bg_data_selection = new QButtonGroup(this);
  bg_data_selection->addButton(rb_data_source_simulation);
  bg_data_selection->addButton(rb_data_source_reconstruction);
  rb_data_source_simulation->setChecked(true);

  tw_recipes->setRepository(repo);
  for(auto &rid : repo->getRecipeList())
    tw_recipes->onRecipeChanged(rid);

  right_column_rid = invalid_recipe_id;
  right_column_vid = ARecipeVersionID(-1);

  connect(repo, &ARepository::recipeChanged, this, &ALrfWindow::onRecipeChanged);
  connect(repo, &ARepository::repositoryCleared, [=](){ onRecipeChanged(right_column_rid); });

  connect(repo, &ARepository::nextUpdateConfigChanged, this, &ALrfWindow::readInstructionsFromJson);
  connect(repo, &ARepository::repositoryCleared, tw_recipes, &Widgets::AHistoryWidget::clear);
  connect(repo, &ARepository::recipeChanged, tw_recipes, &Widgets::AHistoryWidget::onRecipeChanged);
  connect(repo, &ARepository::currentLrfsChanged, tw_recipes, &Widgets::AHistoryWidget::onCurrentLrfsChanged);
  connect(repo, &ARepository::secondaryLrfsChanged, tw_recipes, &Widgets::AHistoryWidget::onSecondaryLrfsChanged);

  connect(repo, &ARepository::currentLrfsChanged, [this]() { pb_save_current->setEnabled(repo->isCurrentRecipeSet()); });

  QList<QWidget*> invis;
  invis << sbShowLRFs_StackLayer << cbScaleByLayerGain;
  for (QWidget* w : invis) w->setVisible(false);

  QDoubleValidator* dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);
  QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
  for (QLineEdit *w : list)
      if (w->objectName().startsWith("led")) w->setValidator(dv);

  splTop->setCollapsible(2, true);
  //fr_right_column->setVisible(false);
  pb_close_right_column->setVisible(false);
  fProgress->setVisible(false);
  connect(repo, &ARepository::reportProgress, prProgress, &QProgressBar::setValue);
  onReadyStatusChanged(repo->isAllLRFsDefined());
  setSplitterRightColumn(nullptr);
}

void ALrfWindow::writeInstructionsToJson(QJsonObject &json)
{
  QJsonArray current_instructions;
  for(int i = 0; i < lw_instructions->count(); i++) {
    auto *widget = static_cast<Widgets::AInstructionListWidgetItem*>(lw_instructions->itemWidget(lw_instructions->item(i)));
    current_instructions.append(widget->saveState());
  }

  json["instructions"] = current_instructions;
  json["name"] = le_current_recipe_name->text();
  json["use true data"] = rb_data_source_simulation->isChecked();
  json["scale by energy"] = cb_UseEventEnergy->isChecked();
  json["fit error"] = cb_FitError->isChecked();
  json["instruction list vpos"] = lw_instructions->verticalScrollBar()->value();
}

void ALrfWindow::readInstructionsFromJson(const QJsonObject &state)
{
  le_current_recipe_name->setText(state["name"].toString());
  rb_data_source_simulation->setChecked(state["use true data"].toBool());
  rb_data_source_reconstruction->setChecked(!state["use true data"].toBool());
  cb_UseEventEnergy->setChecked(state["scale by energy"].toBool());
  cb_FitError->setChecked(state["fit error"].toBool());

  lw_instructions->clear();
  QJsonArray current_instructions = state["instructions"].toArray();
  for(int i = 0; i < current_instructions.size(); i++) {
    auto *widget = new Widgets::AInstructionListWidgetItem(repo, mw->EventsDataHub, mw->Detector);
    widget->loadState(current_instructions[i].toObject());
    addWidgetToInstructionList(lw_instructions, widget);
  }

  lw_instructions->setFocus(); //Don't flicker when processing events
  //This one can't be here (unlike in onInstructionsListUpdate()), it crashes!
  //qApp->processEvents(); //ALlow for window to calculate items' sizes
  lw_instructions->verticalScrollBar()->setValue(state["instruction list vpos"].toInt());
  lw_instructions->setFocus();
}

void ALrfWindow::onRequestShowPMs(QString selection)
{
   QList<int> PMlist;
   bool fOk = ExtractNumbersFromQString(selection, &PMlist);
   if (!fOk) {
     if(selection == "-") {
       for(int i = 0; i < mw->PMs->count(); i++)
         PMlist<<i;
     } else {
       message("Error in extracting PM numbers", this);
       return;
     }
   }

   int numPms = mw->PMs->count();
   QVector<QString> Text(numPms, "");
   for (int ipm : PMlist)
   {
       if (ipm<0 || ipm>numPms-1)
       {
           message("Bad Pm number: "+QString::number(ipm), this);
           return;
       }
       Text[ipm] = QString::number(ipm);
   }

   mw->GeometryWindow->ShowTextOnPMs(Text, kBlack);
}

void ALrfWindow::onReadyStatusChanged(bool fReady)
{
  //status label
  QString status = fReady ? "LRFs ready" : "LRFs not ready";
  labReady->setText(status);

  if (fReady) labReady->setStyleSheet("QLabel { color : green; }");
  else labReady->setStyleSheet("QLabel { color : red; }");

  //As long as we have some lrf, we can draw them, no need for all lrfs defined
  //fDraw->setEnabled(fReady); //Plot enabled
}

void ALrfWindow::onBusyStatusChanged(bool fStatus)
{
   QList<QWidget*> wids;
   wids << frRecipe << tw_recipes << frUnderHistory << fDraw << fr_right_column;
   for (QWidget* w : wids)
       w->setEnabled(!fStatus);
}

void ALrfWindow::on_pbUpdateInstructionsJson_clicked()
{
    QJsonObject js;
    writeInstructionsToJson(js);
    repo->setNextUpdateConfig(js);
    mw->Config->UpdateLRFv3makeJson();
}

bool ALrfWindow::event(QEvent *event)
{
    if (!mw->WindowNavigator) return QMainWindow::event(event);

    if (event->type() == QEvent::Hide) mw->WindowNavigator->HideWindowTriggered("newLrf");
    if (event->type() == QEvent::Show) mw->WindowNavigator->ShowWindowTriggered("newLrf");

    return QMainWindow::event(event);
}

void ALrfWindow::setSplitterRightColumn(QWidget *new_widget)
{
  //if(fr_right_column->isVisible()) {
  //  delete wid_right_column;
  //  this->setMinimumWidth(this->minimumWidth()-fr_right_column->minimumWidth());
  //}
  //fr_right_column->setVisible(new_widget != nullptr);
  //pb_close_right_column->setVisible(new_widget != nullptr);
  wid_right_column = std::unique_ptr<QWidget>(new_widget);
  if(new_widget) {
    auto *layout = static_cast<QGridLayout*>(fr_right_column->layout());
    if(new_widget->layout())
      new_widget->layout()->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new_widget, 0, 0);
    //this->setMinimumWidth(this->minimumWidth()+fr_right_column->minimumWidth());
  }
}

//TODO: Make parcel editor, sensor editor (table with gains + remove lrfs), version editor (remove sensors)
void ALrfWindow::addWidgetToInstructionList(QListWidget *list, Widgets::AInstructionListWidgetItem *widget)
{
  //Looks like the list settings are forgotten along the way..
  //list->setFixedWidth(260); //Why? Because! That's why. (ask Qt guys if unsatisfied with answear)
  list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  list->setVerticalScrollMode(QListWidget::ScrollPerPixel); //setHorizontalPolicy screws up this one...

  //widget->setFixedWidth(250); //Looks like this doesn't matter for anything.
  widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
  connect(widget, &Widgets::AInstructionListWidgetItem::requestShowPMs, this, &ALrfWindow::onRequestShowPMs);
  connect(widget, &Widgets::AInstructionListWidgetItem::requestShowTextOnSensors, [=](QVector<QString> text) {
    mw->GeometryWindow->ShowTextOnPMs(text, kBlack);
  });

  connect(widget, &LrfWindowWidgets::AInstructionListWidgetItem::elementChanged, this, &ALrfWindow::on_pbUpdateInstructionsJson_clicked);
  QListWidgetItem *new_item = new QListWidgetItem;
  new_item->setSizeHint(widget->sizeHint());
  list->addItem(new_item);
  list->setItemWidget(new_item, widget);

  AMouseWheelFilter *filter = new AMouseWheelFilter;
  for(QWidget *child : widget->findChildren<QWidget*>()) {
    child->setFocusPolicy(Qt::StrongFocus);
    child->installEventFilter(filter);
  }
}

bool ALrfWindow::fillInstructionList(QListWidget *list, std::vector<AInstructionID> instructions)
{
  for(AInstructionID iid : instructions) {
    if(!repo->hasInstruction(iid)) {
      qDebug()<<"ALrfWindow::fillInstructionList(): failed to find instruction in repository.";
      continue;
    }
    const AInstruction &instruction = repo->getInstruction(iid);

    auto *widget = new Widgets::AInstructionListWidgetItem(repo, mw->EventsDataHub, mw->Detector);
    widget->loadInstruction(instruction);
    addWidgetToInstructionList(list, widget);
    widget->setEnabled(false); //onInstructionsListUpdate will clear this when we want to. Happy coincidence!
  }
  return true;
}

LRF::ARecipeID ALrfWindow::getRecipeFromInstructionList(QListWidget *list, QString &qname, bool *is_new)
{
  if(list->count() == 0) {
    message("No instructions are set.\nAdd at least one instruction to the current recipe", this);
    return invalid_recipe_id;
  }

  //Get instruction id list for current settings
  std::vector<AInstructionID> current_instructions(list->count());
  for(int i = 0; i < list->count(); i++) {
    auto *widget = static_cast<Widgets::AInstructionListWidgetItem*>(list->itemWidget(list->item(i)));
    current_instructions[i] = repo->addInstruction(std::unique_ptr<AInstruction>(widget->makeInstruction()));
  }
  ARecipeID recipe;
  bool is_new_recipe = !repo->findRecipe(current_instructions, &recipe);
  if(is_new) *is_new = is_new_recipe;
  if(is_new_recipe) {
    std::string name = qname.toLocal8Bit().data();
    ARecipeID old_rid;
    if(repo->findRecipe(name, &old_rid)) {
      QString msg = "Recipe with id "+QString::number(old_rid.val())+" is different and has the same name.\nWhat do you wish to do?";
      switch(QMessageBox::question(this, "Already existing recipe", msg, "Keep duplicate name", "Automatically rename", "Cancel"))
      {
        default: case 0: break;
        case 1: {
          auto id = repo->getNextRecipeId().val();
          do name = "Recipe "+std::to_string(id++);
          while(repo->findRecipe(name));
          qname = QString::fromStdString(name);
        } break;
        case 2:
          return invalid_recipe_id;
      }
    }
    recipe = repo->addRecipe(name, current_instructions);
    emit repo->recipeChanged(recipe);
  }
  return recipe;
}

void ALrfWindow::onInstructionsListUpdate(QListWidget *list)
{
  QVector<QJsonObject> current_instructions(list->count());
  for(int i = 0; i < list->count(); i++) {
    auto *widget = static_cast<Widgets::AInstructionListWidgetItem*>(list->itemWidget(list->item(i)));
    current_instructions[i] = widget->saveState();
  }
  int old_pos = list->verticalScrollBar()->value();
  list->clear();

  for(int i = 0; i < current_instructions.size(); i++) {
    auto *widget = new Widgets::AInstructionListWidgetItem(repo, mw->EventsDataHub, mw->Detector);
    widget->loadState(current_instructions[i]);
    addWidgetToInstructionList(list, widget);
  }

  list->setFocus(); //Don't flicker when processing events
  qApp->processEvents(); //ALlow for window to calculate items' sizes
  list->verticalScrollBar()->setValue(old_pos);
  list->setFocus();
}

void ALrfWindow::on_tw_recipes_itemClicked(QTreeWidgetItem *, int)
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  if(!repo->hasRecipe(rid)) return;
  right_column_vid = ARecipeVersionID(-1);
  switch(tw_recipes->getSelectedItemDepth()) {
    case 1: { //Recipe: Show instruction list on right panel
      QListWidget *instruction_editor = new QListWidget;
      auto &instructions = repo->getInstructionList(rid);
      setSplitterRightColumn(instruction_editor);
      if(!fillInstructionList(instruction_editor, instructions))
        setSplitterRightColumn(nullptr);
      else
        right_column_rid = rid;
    } break;

    case 2: { //Version: Show creation date + comments
      ARecipeVersionID vid = tw_recipes->getSelectedVersionId();
      if(!repo->hasVersion(rid, vid)) return;
      const ARecipeVersion &version = repo->getVersion(rid, vid);
      auto *widget = new Widgets::ARecipeVersionWidget(rid, QString::fromStdString(repo->getName(rid)), version);
      connect(widget, &Widgets::ARecipeVersionWidget::saveCommentPressed, [=](QString comment) {
        repo->setVersionComment(rid, vid, comment.toLocal8Bit().data());
      });
      setSplitterRightColumn(widget);
      right_column_rid = rid;
      right_column_vid = vid;
    } break;

    case 3: { //Sensor: Show layer count, type of lrfs
      ARecipeVersionID vid = tw_recipes->getSelectedVersionId();
      int isensor = tw_recipes->getSelectedSensor();
      if(!repo->hasVersion(rid, vid) || isensor < 0)
        return;
      const ASensor *sensor = repo->getVersion(rid, vid).sensors.getSensor(isensor);
      auto *widget = new Widgets::ASensorWidget(*sensor);
      connect(widget, &Widgets::ASensorWidget::coefficientEdited, [=](int layer, double new_val){
        repo->editSensorLayer(rid, vid, isensor, layer, &new_val, nullptr);
      });
      setSplitterRightColumn(widget);
      right_column_rid = rid;
      right_column_vid = vid;
    } break;

    case 4: { //Layer: Edit lrfs
      ARecipeVersionID vid = tw_recipes->getSelectedVersionId();
      int isensor = tw_recipes->getSelectedSensor();
      int layer = tw_recipes->getSelectedLayer();
      if(!repo->hasVersion(rid, vid) || isensor < 0 || layer < 0)
        return;
      const ASensor *sensor = repo->getVersion(rid, vid).sensors.getSensor(isensor);
      if(sensor == nullptr || (unsigned)layer >= sensor->deck.size()) return;
      std::shared_ptr<const ALrf> lrf = sensor->deck[layer].lrf;

      QWidget *widget = ALrfTypeManager::instance().getTypeFast(lrf->type()).newInternalsWidget();
      AStateInterface *interface = dynamic_cast<AStateInterface*>(widget);
      if(!interface) return;

      QJsonObject state = ALrfTypeManager::instance().lrfToJson(lrf.get());
      interface->loadState(state);

      QPushButton *pb_save_lrf = new QPushButton("Save lrf");
      connect(pb_save_lrf, &QPushButton::clicked, [=]() {
        onSaveLrfsPressed(interface, lrf->type(), rid, vid, isensor, layer);
      });

      QVBoxLayout *layout = new QVBoxLayout;
      layout->addWidget(widget);
      layout->addWidget(pb_save_lrf);
      QFrame *frame_sensor = new QFrame;
      frame_sensor->setLayout(layout);
      setSplitterRightColumn(frame_sensor);
      right_column_rid = rid;
      right_column_vid = vid;
    } break;
  }
}

void ALrfWindow::on_tw_recipes_itemDoubleClicked(QTreeWidgetItem *, int)
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  switch(tw_recipes->getSelectedItemDepth()) {
    case 1: { //Recipe: Set latest version as primary
      if(!repo->hasRecipe(rid)) return;
      repo->setCurrentRecipe(rid);
    } break;

    case 2: { //Version: Set this version as primary
      ARecipeVersionID vid = tw_recipes->getSelectedVersionId();
      if(vid == ARecipeVersionID(-1)) return;
      repo->setCurrentRecipe(rid, vid);
    } break;
  }
}

void ALrfWindow::on_tw_recipes_customContextMenuRequested(const QPoint &)
{
  QMenu menu(this);
  QAction *unset_secondary_lrfs = new QAction("Unset secondary", &menu);
  connect(unset_secondary_lrfs, &QAction::triggered, this, &ALrfWindow::onUnsetSecondaryLrfs);
  QAction *clear_repo = new QAction("Clear repository", &menu);
  connect(clear_repo, &QAction::triggered, this, &ALrfWindow::onClearRepository);
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  switch(tw_recipes->getSelectedItemDepth()) {
    case 1: { //Recipe: Select latest version
      if(!repo->hasRecipe(rid)) return;
      connect(menu.addAction("Set as primary"), &QAction::triggered, this, &ALrfWindow::onRecipeContextMenuActionSetAsPrimary);
      connect(menu.addAction("Set as secondary"), &QAction::triggered, this, &ALrfWindow::onRecipeContextMenuActionSetAsSecondary);
      menu.addAction(unset_secondary_lrfs);
      menu.addSeparator();
      connect(menu.addAction("Append to instruction editor"), &QAction::triggered, this, &ALrfWindow::onRecipeContextMenuActionAppendToEditor);
      connect(menu.addAction("Copy to instruction editor"), &QAction::triggered, this, &ALrfWindow::onRecipeContextMenuActionCopyToEditor);
      menu.addSeparator();
      connect(menu.addAction("Save"), &QAction::triggered, this, &ALrfWindow::onRecipeContextMenuActionSave);
      connect(menu.addAction("Rename"), &QAction::triggered, this, &ALrfWindow::onRecipeContextMenuActionRename);
      menu.addSeparator();
      connect(menu.addAction("Delete"), &QAction::triggered, this, &ALrfWindow::onRecipeContextMenuActionDelete);
      menu.addAction(clear_repo);
      menu.exec(QCursor::pos());
    } break;

    case 2: { //Version: Select this particular version of recipe
      if(tw_recipes->getSelectedVersionId() == ARecipeVersionID(-1)) return;
      connect(menu.addAction("Set as primary"), &QAction::triggered, this, &ALrfWindow::onVersionContextMenuActionSetAsPrimary);
      connect(menu.addAction("Set as secondary"), &QAction::triggered, this, &ALrfWindow::onVersionContextMenuActionSetAsSecondary);
      menu.addAction(unset_secondary_lrfs);
      menu.addSeparator();
      connect(menu.addAction("Save"), &QAction::triggered, this, &ALrfWindow::onVersionContextMenuActionSave);
      connect(menu.addAction("Rename"), &QAction::triggered, this, &ALrfWindow::onVersionContextMenuActionRename);
      menu.addSeparator();
      connect(menu.addAction("Delete"), &QAction::triggered, this, &ALrfWindow::onVersionContextMenuActionDelete);
      menu.addAction(clear_repo);
      menu.exec(QCursor::pos());
    } break;

    case 3: { //Sensor: Open sensor editor
      //const ASensor *sensor = tw_recipes->getSelectedSensor();
      //if(!sensor) return;
    } //no break;

    case 4: { //Layer: Open layer editor
      //const ALrf *lrf = tw_recipes->getSelectedLrf();
      //if(!lrf) return;
    } //no break;
    default:
      menu.addAction(unset_secondary_lrfs);
      menu.addSeparator();
      menu.addAction(clear_repo);
      menu.exec(QCursor::pos());
  }
}

void ALrfWindow::onUnsetSecondaryLrfs()
{
  repo->unsetSecondaryLrfs();
}

void ALrfWindow::onClearRepository()
{
  QString question = "Are you sure you wish to delete the repository?\n"
                     "This includes all recipes, versions, and instructions.\n\n"
                     "This action is irreversible!";
  if(QMessageBox::Yes == QMessageBox::question(this, "Delete repository?", question)) {
    repo->clear(mw->PMs->count());
  }
}

void ALrfWindow::onRecipeChanged(ARecipeID rid)
{
  if(right_column_rid != rid) return;
  if(!repo->hasRecipe(rid) || (right_column_vid != ARecipeVersionID(-1) && !repo->hasVersion(rid, right_column_vid))) {
    setSplitterRightColumn(nullptr);
    right_column_rid = invalid_recipe_id;
    right_column_vid = ARecipeVersionID(-1);
  }
}

void ALrfWindow::onRecipeContextMenuActionSetAsPrimary()
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  repo->setCurrentRecipe(rid);
}

void ALrfWindow::onRecipeContextMenuActionSetAsSecondary()
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  repo->setSecondaryRecipe(rid);
}

void ALrfWindow::onRecipeContextMenuActionAppendToEditor()
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  auto &instructions = repo->getInstructionList(rid);
  fillInstructionList(lw_instructions, instructions);
  onInstructionsListUpdate(lw_instructions);
}

void ALrfWindow::onRecipeContextMenuActionCopyToEditor()
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  auto &instructions = repo->getInstructionList(rid);
  lw_instructions->clear();
  fillInstructionList(lw_instructions, instructions);
  onInstructionsListUpdate(lw_instructions);
  le_current_recipe_name->setText(QString::fromStdString(repo->getName(rid)));
}

void ALrfWindow::onRecipeContextMenuActionSave()
{
  QString file_name = QFileDialog::getSaveFileName(this, "Save recipe", mw->GlobSet->LastOpenDir, "json files(*.json);;all files(*)");
  if (file_name.isEmpty()) return;
  mw->GlobSet->LastOpenDir = QFileInfo(file_name).dir().absolutePath();

  QJsonDocument doc(repo->toJson(tw_recipes->getSelectedRecipeId()));
  QFile save_file(file_name);
  if(!save_file.open(QFile::WriteOnly)) {
    message("Failed to open file \""+file_name+"\" for writing.", this);
    return;
  }
  save_file.write(doc.toJson());
  save_file.close();
}

void ALrfWindow::onRecipeContextMenuActionRename()
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  bool ok;
  QString text = QInputDialog::getText(this, "Rename recipe", "New name:",
    QLineEdit::Normal, QString::fromStdString(repo->getName(rid)), &ok, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
  if (!ok || text.isEmpty()) return;
  repo->setName(rid, text.toLocal8Bit().data());
  if(repo->getCurrentRecipeID() == rid)
    le_current_recipe_name->setText(text);
}

void ALrfWindow::onRecipeContextMenuActionDelete()
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  QString name = QString::fromStdString(repo->getName(rid));
  QString id = QString::number(rid.val());
  QString question = "Are you sure you wish to delete recipe \""+name+"\" (id "+id+")?";
  if(QMessageBox::Yes == QMessageBox::question(this, "Delete recipe?", question)) {
    repo->remove(rid);
  }
}

void ALrfWindow::onVersionContextMenuActionSetAsPrimary()
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  ARecipeVersionID vid = tw_recipes->getSelectedVersionId();
  repo->setCurrentRecipe(rid, vid);
}

void ALrfWindow::onVersionContextMenuActionSetAsSecondary()
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  ARecipeVersionID vid = tw_recipes->getSelectedVersionId();
  repo->setSecondaryRecipe(rid, vid);
}

void ALrfWindow::onVersionContextMenuActionSave()
{
  QString file_name = QFileDialog::getSaveFileName(this, "Save version", mw->GlobSet->LastOpenDir, "json files(*.json);;all files(*)");
  if (file_name.isEmpty()) return;
  mw->GlobSet->LastOpenDir = QFileInfo(file_name).dir().absolutePath();

  QJsonDocument doc(repo->toJson(tw_recipes->getSelectedRecipeId(), tw_recipes->getSelectedVersionId()));
  QFile save_file(file_name);
  if(!save_file.open(QFile::WriteOnly)) {
    message("Failed to open file \""+file_name+"\" for writing.", this);
    return;
  }
  save_file.write(doc.toJson());
  save_file.close();
}

void ALrfWindow::onVersionContextMenuActionRename()
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  ARecipeVersionID vid = tw_recipes->getSelectedVersionId();
  if(!repo->hasVersion(rid, vid)) {
    qDebug()<<"Failed to find selected recipe version. Should never happen!";
    return;
  }
  const ARecipeVersion &version = repo->getVersion(rid, vid);
  bool ok;
  QString text = QInputDialog::getText(this, "Rename version", "New name:",
    QLineEdit::Normal, QString::fromStdString(version.name),
    &ok, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
  if (!ok || text.isEmpty()) return;
  repo->renameVersion(rid, vid, text.toLocal8Bit().data());
}

void ALrfWindow::onVersionContextMenuActionDelete()
{
  ARecipeID rid = tw_recipes->getSelectedRecipeId();
  ARecipeVersionID vid = tw_recipes->getSelectedVersionId();
  repo->removeVersion(rid, vid);
}

void ALrfWindow::onSaveLrfsPressed(AStateInterface *interface, ALrfTypeID tid,
                                   ARecipeID rid, ARecipeVersionID vid, int isensor, int ilayer)
{
  QJsonObject state;
  interface->saveState(state);
  ALrf *new_lrf = ALrfTypeManager::instance().lrfFromJson(state, tid);
  if(!new_lrf) {
    message("Failed to create lrf with new settings.\nDid not save changes!", this);
    return;
  }
  repo->editSensorLayer(rid, vid, isensor, ilayer, nullptr, std::shared_ptr<ALrf>(new_lrf));
}

void ALrfWindow::on_pb_add_instruction_clicked()
{
  auto *widget = new Widgets::AInstructionListWidgetItem(repo, mw->EventsDataHub, mw->Detector, this);
  addWidgetToInstructionList(lw_instructions, widget);
  on_pbUpdateInstructionsJson_clicked();
}

void ALrfWindow::on_pb_remove_instruction_clicked()
{
  for(QListWidgetItem *item : lw_instructions->selectedItems()) {
    QWidget *widget = lw_instructions->itemWidget(item);
    lw_instructions->setItemWidget(item, nullptr);
    delete lw_instructions->takeItem(lw_instructions->row(item));
    delete widget;
  }
  on_pbUpdateInstructionsJson_clicked();
}

void ALrfWindow::on_pb_make_lrfs_clicked()
{
  if(mw->EventsDataHub->Events.isEmpty()) {
    message("There are no events loaded", this);
    return;
  }

  if(rb_data_source_simulation->isChecked()) {
    if(mw->EventsDataHub->Scan.isEmpty()) {
      message("There are no scan data available!", this);
      return;
    }
  } else if(!mw->EventsDataHub->isReconstructionReady()) {
    message("Reconstruction was not yet performed!", this);
    return;
  }

  bool is_new_recipe;
  QString name = le_current_recipe_name->text();
  ARecipeID recipe = getRecipeFromInstructionList(lw_instructions, name, &is_new_recipe);
  if(recipe == invalid_recipe_id)
    return;
  le_current_recipe_name->setText(name);

  std::vector<APoint> sensorPos;
  APmHub *PMs = mw->Detector->PMs;
  for(int i = 0; i < PMs->count(); i++) {
    APm &PM = PMs->at(i);
    sensorPos.push_back(APoint(PM.x, PM.y, PM.z));
  }

  //Preparing GUI
  prProgress->setValue(0);
  fProgress->setVisible(true);
  mw->WindowNavigator->ResetAllProgressBars();
  mw->WindowNavigator->BusyOn();
  qApp->processEvents();

  AInstructionInput data(repo,
                         sensorPos,
                         mw->Detector->PMgroups,
                         mw->EventsDataHub,
                         rb_data_source_simulation->isChecked(),
                         cb_FitError->isChecked(),
                         cb_UseEventEnergy->isChecked());

  if(!repo->updateRecipe(data, recipe)) {
    message("Failed to update recipe!", this);
    if(is_new_recipe) {
      repo->remove(recipe);
      repo->removeUnusedInstructions();
    }
  }
  else repo->setCurrentRecipe(recipe);

  fProgress->setVisible(false);
  mw->WindowNavigator->BusyOff(true);
}

void ALrfWindow::on_pbSTOP_clicked()
{
  repo->stopUpdate();
}

void ALrfWindow::on_pb_save_repository_clicked()
{
  QString file_name = QFileDialog::getSaveFileName(this, "Save repository", mw->GlobSet->LastOpenDir, "json files(*.json);;all files(*)");
  if (file_name.isEmpty()) return;
  mw->GlobSet->LastOpenDir = QFileInfo(file_name).dir().absolutePath();

  QJsonDocument doc(repo->toJson());
  QFile save_file(file_name);
  if(!save_file.open(QFile::WriteOnly)) {
    message("Failed to open file \""+file_name+"\" for writing.", this);
    return;
  }
  save_file.write(doc.toJson());
  save_file.close();
}

void ALrfWindow::on_pb_load_repository_clicked()
{
  QStringList file_names = QFileDialog::getOpenFileNames(this, "Append repository", mw->GlobSet->LastOpenDir, "json files (*.json);;all files(*)");
  if(file_names.isEmpty()) return;
  mw->GlobSet->LastOpenDir = QFileInfo(file_names.first()).absolutePath();

  for (int ifile = 0; ifile < file_names.size(); ifile++) {
    QString file_name = file_names[ifile];
    QFile load_file(file_name);
    if (!load_file.open(QIODevice::ReadOnly)) {
      message("Cannot open save file \""+file_name+"\".", this);
      continue;
    }

    QJsonDocument loadDoc(QJsonDocument::fromJson(load_file.readAll()));
    load_file.close();
    ARepository new_repo(loadDoc.object());
    ARecipeID old_rid = repo->getCurrentRecipeID();
    ARecipeVersionID old_vid = repo->getCurrentRecipeVersionID();
    repo->mergeRepository(new_repo);
    repo->setCurrentRecipe(old_rid, old_vid);
  }
}

void ALrfWindow::on_pb_save_current_clicked()
{
  QString file_name = QFileDialog::getSaveFileName(this, "Save current lrfs", mw->GlobSet->LastOpenDir, "json files(*.json);;all files(*)");
  if (file_name.isEmpty()) return;
  mw->GlobSet->LastOpenDir = QFileInfo(file_name).dir().absolutePath();

  QJsonDocument doc(repo->toJson(repo->getCurrentRecipeID()));
  QFile save_file(file_name);
  if(!save_file.open(QFile::WriteOnly)) {
    message("Failed to open file \""+file_name+"\" for writing.", this);
    return;
  }
  save_file.write(doc.toJson());
  save_file.close();
}

void ALrfWindow::on_pb_load_current_clicked()
{
  QStringList file_names = QFileDialog::getOpenFileNames(this, "Append repository and set as current", mw->GlobSet->LastOpenDir, "json files (*.json);;all files(*)");
  if(file_names.isEmpty()) return;
  mw->GlobSet->LastOpenDir = QFileInfo(file_names.first()).absolutePath();

  for (int ifile = 0; ifile < file_names.size(); ifile++) {
    QString file_name = file_names[ifile];
    QFile load_file(file_name);
    if (!load_file.open(QIODevice::ReadOnly)) {
      message("Cannot open save file \""+file_name+"\".", this);
      continue;
    }

    QJsonDocument loadDoc(QJsonDocument::fromJson(load_file.readAll()));
    load_file.close();
    ARepository new_repo(loadDoc.object());
    repo->mergeRepository(new_repo);
  }
}

void ALrfWindow::on_pb_show_radial_clicked()
{
  if(repo->getCurrentLrfs().size() == 0) {
    message("No sensors have been created.", this);
    return;
  }
  const ASensorGroup &sensors = repo->getCurrentLrfs();
  if((unsigned)sbPM->value() >= sensors.size()) {
    message("Invalid sensor selected.", this);
    return;
  }

  ALrfDraw lrfd(mw->Detector->LRFs, false, mw->EventsDataHub, mw->Detector->PMs, mw->GraphWindow);

  QJsonObject json;
  json["DataSource"] =  (rb_data_source_simulation->isChecked() ? 0 : 1 ) ; //0 - scan, 1 - rec
  json["PlotLRF"] = true;// ui->cb_LRF_2->isChecked();
  json["PlotSecondaryLRF"] = repo->isSecondaryRecipeSet();
  json["PlotData"] = cbAddData->isChecked();
  json["PlotDiff"] = cbAddDiff->isChecked();
  json["FixedMin"] = cbFixedMin->isChecked();
  json["FixedMax"] = cbFixedMax->isChecked();
  json["Min"] = ledFixedMin->text().toDouble();
  json["Max"] = ledFixedMax->text().toDouble();
  json["CurrentGroup"] = mw->Detector->PMgroups->getCurrentGroup();
  json["CheckZ"] = cb_zrange->isChecked();
  json["Z"] = led_zcenter->text().toDouble();
  json["DZ"] = led_zrange->text().toDouble();
  json["EnergyScaling"] = cb_UseEventEnergy->isChecked();
  json["FunctionPointsX"] = mw->GlobSet->FunctionPointsX;
  json["Bins"] = sbDataBins->value();
  json["ShowNodes"] = false;//ui->cbShowNodePositions->isChecked();

  QJsonObject js;
  //js["SecondIteration"] = secondIter;
  bool all_layers = cobShowLRF_LayerSelection->currentIndex() == 0;
  int layer = sbShowLRFs_StackLayer->value();
  if(!all_layers && !(0 <= layer && (unsigned)layer < sensors[sbPM->value()].deck.size())) {
    message("Invalid layer selected.", this);
    return;
  }
  js["AllLayers"] = all_layers;
  js["Layer"] = layer;
  json["ModuleSpecific"] = js;

  lrfd.DrawRadial(sbPM->value(), json);
  mw->GraphWindow->LastDistributionShown = "LRFvsR";
}

void ALrfWindow::on_cobShowLRF_LayerSelection_currentIndexChanged(int index)
{
  sbShowLRFs_StackLayer->setVisible(index != 0);
  cbScaleByLayerGain->setVisible(index != 0);
}

void ALrfWindow::on_pb_show_xy_clicked()
{
  if(repo->getCurrentLrfs().size() == 0) {
    message("No sensors have been created.", this);
    return;
  }
  const ASensorGroup &sensors = repo->getCurrentLrfs();
  if((unsigned)sbPM->value() >= sensors.size()) {
    message("Invalid sensor selected.", this);
    return;
  }
  ALrfDraw lrfd(mw->Detector->LRFs, false, mw->EventsDataHub, mw->PMs, mw->GraphWindow);

  QJsonObject json;
  json["DataSource"] = (rb_data_source_simulation->isChecked() ? 0 : 1 ) ; //0 - scan, 1 - rec
  json["PlotLRF"] = true; //ui->cb_LRF->isChecked();
  json["PlotSecondaryLRF"] = repo->isSecondaryRecipeSet();
  json["PlotData"] = cbAddData->isChecked();
  json["PlotDiff"] = cbAddDiff->isChecked();
  json["FixedMin"] = cbFixedMin->isChecked();
  json["FixedMax"] = cbFixedMax->isChecked();
  json["Min"] = ledFixedMin->text().toDouble();
  json["Max"] = ledFixedMax->text().toDouble();
  json["CurrentGroup"] = mw->Detector->PMgroups->getCurrentGroup();
  json["CheckZ"] = cb_zrange->isChecked();
  json["Z"] = led_zcenter->text().toDouble();
  json["DZ"] = led_zrange->text().toDouble();
  json["EnergyScaling"] = cb_UseEventEnergy->isChecked();
  json["FunctionPointsX"] = mw->GlobSet->FunctionPointsX;
  json["FunctionPointsY"] = mw->GlobSet->FunctionPointsY;
  json["Bins"] = sbDataBins->value();
  //json["ShowNodes"] = ui->cbShowNodePositions->isChecked();

  QJsonObject js;
  //js["SecondIteration"] = secondIter;
  bool all_layers = cobShowLRF_LayerSelection->currentIndex() == 0;
  int layer = sbShowLRFs_StackLayer->value();
  if(!all_layers && !(0 <= layer && (unsigned)layer < sensors[sbPM->value()].deck.size())) {
    message("Invalid layer selected.", this);
    return;
  }
  js["AllLayers"] = all_layers;
  js["Layer"] = layer;
  json["ModuleSpecific"] = js;

  lrfd.DrawXY(sbPM->value(), json);
  mw->GraphWindow->LastDistributionShown = "LRFvsXY";
}

void ALrfWindow::on_cbAddData_toggled(bool checked)
{
    if (checked) cbAddDiff->setChecked(false);

    bool fVis = cbAddData->isChecked() || cbAddDiff->isChecked();
    sbDataBins->setEnabled(fVis);
    labDataBins->setEnabled(fVis);
}

void ALrfWindow::on_cbAddDiff_toggled(bool checked)
{
    if (checked) cbAddData->setChecked(false);

    bool fVis = cbAddData->isChecked() || cbAddDiff->isChecked();
    sbDataBins->setEnabled(fVis);
    labDataBins->setEnabled(fVis);
}

void ALrfWindow::on_sbPM_valueChanged(int arg1)
{
    const ASensorGroup &sensors = repo->getCurrentLrfs();
    if((unsigned)arg1 >= sensors.size())
    {
        sbPM->setValue(0);
        return;  //update on valueChanged
    }

    if (mw->GraphWindow->LastDistributionShown == "LRFvsR")
       on_pb_show_radial_clicked();
    else if (mw->GraphWindow->LastDistributionShown == "LRFvsXY")
        on_pb_show_xy_clicked();
}

void ALrfWindow::on_actionLRF_explorer_triggered()
{
  if(!repo->isAllLRFsDefined()) {
    message("Not all Lrfs are defined!", this);
    return;
  }
  ALrfMouseExplorer* ME = new ALrfMouseExplorer(mw->Detector->LRFs, mw->PMs, mw->Detector->PMgroups, mw->Rwindow->getSuggestedZ(), this);
  ME->Start();
  ME->deleteLater();
}

void ALrfWindow::on_action_add_empty_recipe_triggered()
{
  QString name = le_current_recipe_name->text();
  ARecipeID recipe = getRecipeFromInstructionList(lw_instructions, name);
  if(recipe == invalid_recipe_id)
    return;
  le_current_recipe_name->setText(name);
}

void ALrfWindow::on_action_unset_secondary_lrfs_triggered()
{
  onUnsetSecondaryLrfs();
}

void ALrfWindow::on_action_clear_repository_triggered()
{
  onClearRepository();
}

void ALrfWindow::on_action_clear_instruction_list_triggered()
{
  lw_instructions->clear();
  on_pbUpdateInstructionsJson_clicked();
}

void ALrfWindow::on_pb_close_right_column_clicked()
{
  if(fr_right_column->isVisible()) {
    right_column_last_size = splTop->sizes().last();
    fr_right_column->setVisible(false);
    pb_close_right_column->setText("<");
    this->setMinimumWidth(this->minimumWidth()-fr_right_column->minimumWidth());
    this->resize(this->width() - right_column_last_size, this->height());
  } else {
    auto sizes = splTop->sizes();
    sizes.last() = right_column_last_size;
    fr_right_column->setVisible(true);
    splTop->setSizes(sizes);
    pb_close_right_column->setText(">");
    this->resize(this->width() + right_column_last_size, this->height());
    this->setMinimumWidth(this->minimumWidth()+fr_right_column->minimumWidth());
  }
}

bool ALrfWindow::doLrfRadialProfile(QVector<double> &radius, QVector<double> &LRF)
{
  int ipm = sbPM->value();
  const ASensor* sensor = repo->getCurrentLrfs().getSensor(ipm);
  bool use_single_layer = cobShowLRF_LayerSelection->currentIndex() == 1;
  size_t layer = sbShowLRFs_StackLayer->value();
  if(!sensor || (use_single_layer && layer >= sensor->deck.size()))
    return false;

  double step = ledStep->text().toDouble();
  if(step == 0)
    return false;

  EventsDataClass *EventsDataHub = mw->EventsDataHub;
  double energy = 1.0;
  if(cbScaleByAverageEnergy->isChecked()) {
    if(!EventsDataHub->isReconstructionReady() ) {
      message("Reconstruction was not yet performed, energy scaling is not possible!", this);
      return false;
    }

    int goodEv = 0;
    energy = 0;
    for(int iev = 0; iev < EventsDataHub->ReconstructionData[0].size(); iev++) {
      if(EventsDataHub->ReconstructionData[0][iev]->GoodEvent) {
        goodEv++;
        energy += EventsDataHub->ReconstructionData[0][iev]->Points[0].energy;
      }
    }
    if(goodEv == 0) {
      message("There are no events passing all the filters!", this);
      return false;
    }
    energy /= goodEv;
  }

  double gain = 1.0;
  if(use_single_layer && cbScaleByLayerGain->isChecked())
    gain = sensor->deck[layer].coeff;

  ASensor lrf = *sensor;
  if(use_single_layer) {
    LRF::ASensor::Parcel p = lrf.deck[layer];
    lrf.deck.clear();
    lrf.deck.push_back(p);
  }

  APm &PM = mw->PMs->at(ipm);
  int n_profiles = ledProfileCount->text().toDouble();
  const double angle_step = 2*3.141592653589793238462643383279/n_profiles;
  APoint pos(0, 0, mw->Rwindow->getSuggestedZ());
  double rmin, rmax;
  sensor->getAxialRange(rmin, rmax);

  for(double r = rmin; r <= rmax; r += step) {
    for(int i = 0; i < n_profiles; i++) {
        double angle = i*angle_step;
        pos.x() = PM.x + r*cos(angle);
        pos.y() = PM.y + r*sin(angle);
        double eval = lrf.eval(pos);
        radius << r;
        LRF << eval*gain*energy;
    }
  }
  return true;
}

void ALrfWindow::on_pbShowRadialForXY_clicked()
{
  QVector<double> Rad, LRF;
  if(!doLrfRadialProfile(Rad, LRF)) return;

  mw->GraphWindow->ShowAndFocus();
  TString str = "LRF of pm#";
  str += sbPM->value();
  mw->GraphWindow->MakeGraph(&Rad, &LRF, 4, "Radial distance, mm", "LRF", 6, 1, 0, 0, "");
}

void ALrfWindow::on_pbExportLrfVsRadial_clicked()
{
  QVector<double> Rad, LRF;
  if(!doLrfRadialProfile(Rad, LRF)) return;

  QString str = sbPM->text();
  QString fileName = QFileDialog::getSaveFileName(this, "Save LRF # " +str+ " vs radius", mw->GlobSet->LastOpenDir,
                                                  "Text files (*.txt);;Data files (*.dat);;All files (*.*)");
  if (fileName.isEmpty()) return;
  mw->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
  QFile outputFile(fileName);
  outputFile.open(QIODevice::WriteOnly);
  if(!outputFile.isOpen()) {
    message("Unable to open file " +fileName+ " for writing!", this);
    return;
  }
  QTextStream outStream(&outputFile);

  int n_profiles = ledProfileCount->text().toDouble();
  for(int i = 0; i < Rad.count(); i += n_profiles) {
    double lrfsum = 0;
    for(int p = 0; p < n_profiles; p++)
      lrfsum += LRF[i+p];
    lrfsum /= n_profiles;
    outStream << Rad[i] << " " <<lrfsum << "\n";
  }
  outputFile.close();
}
