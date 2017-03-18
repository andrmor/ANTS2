#include "alrfwindowwidgets.h"

#include "amessage.h"
#include "alrftypemanager.h"
#include "ainstruction.h"
#include "arecipe.h"
#include "arepository.h"
#include "ainstructioninput.h"
#include "astateinterface.h"
#include "acommonfunctions.h"
#include "detectorclass.h"
#include "apmgroupsmanager.h"
#include "afitlayersensorgroup.h"
#include "pms.h"
#include "acollapsiblegroupbox.h"

#include <array>

#include <QFormLayout>
#include <QVBoxLayout>
#include <QGridLayout>

#include <QRadioButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QTabWidget>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTableWidget>
#include <QScrollBar>
#include <QEvent>
#include <QResizeEvent>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

namespace LRF { namespace LrfWindowWidgets {

/***************************************************************************\
*                   Implementation of FitLayerWidget                        *
\***************************************************************************/
FitLayerWidget::FitLayerWidget(const DetectorClass *detector, QWidget *parent)
  : AInstructionEditorWidget(parent), detector(detector)
{
  layout_control = new QGridLayout();
  {
    QLabel *l_sensor_group = new QLabel("Sensor group:");
    l_sensor_group->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    layout_control->addWidget(l_sensor_group, 0, 0);

    cb_sensor_group = new QComboBox;
    int group_count = detector->PMgroups->countPMgroups();
    for(int i = 0; i < group_count; i++)
      cb_sensor_group->addItem(detector->PMgroups->getGroupName(i));
    layout_control->addWidget(cb_sensor_group, 0, 1);

    QLabel *l_pms = new QLabel("Composition:");
    l_pms->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    layout_control->addWidget(l_pms, 1, 0);

    cb_pms = new QComboBox;
    cb_pms->addItem("All PMs");
    cb_pms->addItem("Subset of PMs:");
    layout_control->addWidget(cb_pms, 1, 1);

    le_pms = new QLineEdit("0, 2-5");
    le_pms->setVisible(false);
    le_pms->setToolTip("PM selection: list, comma separated, of individual PM# or ranges (e.g. 10-12 is 10,11 and 12)\nUse right mouse click menu for more options.");
    le_pms->setContextMenuPolicy(Qt::CustomContextMenu);
    layout_control->addWidget(le_pms, 2, 1);

    QLabel *l_stack_op = new QLabel("Stack operation:");
    l_stack_op->setMaximumWidth(100);
    l_stack_op->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    layout_control->addWidget(l_stack_op, 3, 0);

    cb_stack_operation = new QComboBox;
    cb_stack_operation->addItem("Append to group");
    cb_stack_operation->addItem("Overwrite group");
    layout_control->addWidget(cb_stack_operation, 3, 1);

    layout_control->setContentsMargins(2,2,2,2);
    layout_control->setSpacing(3);
  }

  gb_groupping = new ACollapsibleGroupBox("Grouping");
  {
    group_type = new QComboBox;
    group_type->addItem("One for all");
    group_type->addItem("By distance to center");
    group_type->addItem("Square packing");
    group_type->addItem("Hexagonal packing");

    adjust_gains = new QCheckBox("Adjust gains?");
    adjust_gains->setChecked(true);

    QPushButton* ShowButton = new QPushButton("?");
    connect(ShowButton, &QPushButton::clicked, this, &FitLayerWidget::on_ShowButton_clicked);
    ShowButton->setMaximumWidth(20);

    auto *layout_group_type = new QHBoxLayout;
    layout_group_type->addWidget(group_type);    
    layout_group_type->addWidget(adjust_gains);
    layout_group_type->addWidget(ShowButton);
    layout_group_type->addSpacing(5);
    layout_group_type->setContentsMargins(2,2,2,2);

    gb_groupping->getClientAreaWidget()->setLayout(layout_group_type);
    gb_groupping->setChecked(false);
  }

  auto &lrf_types = ALrfTypeManager::instance();
  layout_lrf_type_selection = new QGridLayout;
  {
    cb_lrf_category = new QComboBox;
    auto groups = lrf_types.getGroups();
    for(auto &group : groups)
      cb_lrf_category->addItem(QString::fromStdString(group));
    cb_lrf_category->setCurrentIndex(0);

    cb_lrf_type = new QComboBox;

    layout_lrf_type_selection->setContentsMargins(5, 0, 5, 0);
    layout_lrf_type_selection->addWidget(new QLabel("Lrf Category:"), 0, 0, 1, 1);
    layout_lrf_type_selection->addWidget(cb_lrf_category, 0, 1, 1, 1);
    layout_lrf_type_selection->addWidget(new QLabel("Lrf Type:"), 1, 0, 1, 1);
    layout_lrf_type_selection->addWidget(cb_lrf_type, 1, 1, 1, 1);

    layout_lrf_type_selection->setVerticalSpacing(2);
    layout_lrf_type_selection->setColumnMinimumWidth(0, 40);
    layout_lrf_type_selection->setHorizontalSpacing(15);
    layout_lrf_type_selection->setColumnStretch(0, 0);
    layout_lrf_type_selection->setColumnStretch(1, 1);
  }
  current_lrf_widget = nullptr;

  auto *layout = new QVBoxLayout;
  layout->setSpacing(5);
  this->setLayout(layout);
  layout->addLayout(layout_control);
  layout->addWidget(gb_groupping);
  layout->addLayout(layout_lrf_type_selection);

  connect(cb_sensor_group, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &FitLayerWidget::elementChanged);
  connect(cb_pms, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &FitLayerWidget::elementChanged);
  connect(cb_pms, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &FitLayerWidget::on_cb_pms_changed);
  connect(le_pms, &QLineEdit::editingFinished, this, &FitLayerWidget::elementChanged);
  connect(le_pms, &QLineEdit::customContextMenuRequested, this, &FitLayerWidget::on_ContextmenuRequested_pmSelection);
  connect(cb_stack_operation, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &FitLayerWidget::elementChanged);
  connect(group_type, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &FitLayerWidget::elementChanged);
  connect(adjust_gains, &QCheckBox::toggled, this, &FitLayerWidget::elementChanged);
  connect(gb_groupping, &ACollapsibleGroupBox::clicked, this, &FitLayerWidget::elementChanged);
  connect(cb_lrf_category, &QComboBox::currentTextChanged, this, &FitLayerWidget::onLrfTypeGroupChange);
  connect(cb_lrf_type, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &FitLayerWidget::onLrfTypeChanged);

  onLrfTypeGroupChange(cb_lrf_category->currentText());
}

void FitLayerWidget::loadInstruction(const AInstruction &instruction)
{
  //If reference is used in dynamic_cast and fails, an exception is thrown
  if(dynamic_cast<const Instructions::FitLayer*>(&instruction))
    loadState(instruction.toJson()["data"].toObject());
}

AInstruction *FitLayerWidget::makeInstruction()
{
  return new Instructions::FitLayer(saveState());
}

QJsonObject FitLayerWidget::saveState()
{
  QJsonObject settings;
  QJsonObject lrf_state;
  lrf_state["Category"] = cb_lrf_category->currentText();
  const ALrfType *type = ALrfTypeManager::instance().getType(ALrfTypeID(cb_lrf_type->currentData().toUInt()));
  if(!type) return QJsonObject();
  lrf_state["Type"] = QString::fromStdString(type->name());
  AStateInterface *interface = dynamic_cast<AStateInterface*>(current_lrf_widget);
  if(interface) {
    QJsonObject lrf_state_settings;
    interface->saveState(lrf_state_settings);
    lrf_state["Settings"] = lrf_state_settings;
    settings["Lrf"] = lrf_state;
  }
  settings["EffectOnPrevious"] = cb_stack_operation->currentIndex();
  settings["SensorComposition"] = cb_pms->currentIndex();
  settings["SensorList"] = le_pms->text();
  settings["SensorGroup"] = cb_sensor_group->currentIndex();

  QJsonObject groupping;
  groupping["Enabled"] = gb_groupping->isChecked();
  groupping["Type"] = group_type->currentIndex();
  groupping["AdjustGains"] = adjust_gains->isChecked();
  settings["Groupping"] = groupping;
  return settings;
}

void FitLayerWidget::loadState(const QJsonObject &state)
{
  QJsonObject lrf_state = state["Lrf"].toObject();
  QString lrf_type_name = lrf_state["Type"].toString();
  std::string lrf_type_name_std = lrf_type_name.toLocal8Bit().data();
  auto *lrf_type = ALrfTypeManager::instance().getType(lrf_type_name_std);
  if(lrf_type == nullptr) {
    qDebug()<<"Unknown lrf type:"<<lrf_type_name<<". Maybe some plugin was not loaded?";
    lrf_type = ALrfTypeManager::instance().getType(ALrfTypeID(0));
    if(lrf_type == nullptr) {
      qDebug()<<"No lrf types exist! Something very serious going on... crash may be imminent.";
      return;
    }
  }

  QString category = lrf_state["Category"].toString();
  for(int i = 0; i < cb_lrf_category->count(); i++)
    if(cb_lrf_category->itemText(i) == category) {
      cb_lrf_category->setCurrentIndex(i);
      break;
    }

  QString type_ui_name = QString::fromStdString(lrf_type->nameUi());
  for(int i = 0; i < cb_lrf_type->count(); i++)
    if(cb_lrf_type->itemText(i) == type_ui_name) {
      cb_lrf_type->setCurrentIndex(i);
      break;
    }

  AStateInterface *interface = dynamic_cast<AStateInterface*>(current_lrf_widget);
  if(interface) {
    this->layout()->removeWidget(current_lrf_widget);
    interface->loadState(lrf_state["Settings"].toObject());
    this->layout()->addWidget(current_lrf_widget);
  }

  cb_stack_operation->setCurrentIndex(state["EffectOnPrevious"].toInt());
  cb_pms->setCurrentIndex(state["SensorComposition"].toInt());
  le_pms->setText(state["SensorList"].toString());

  int group = state["SensorGroup"].toInt();
  if(0 > group || group >= cb_sensor_group->count())
    group = 0;
  cb_sensor_group->setCurrentIndex(group);

  QJsonObject groupping = state["Groupping"].toObject();
  gb_groupping->setChecked(groupping["Enabled"].toBool());
  group_type->setCurrentIndex(groupping["Type"].toInt());
  adjust_gains->setChecked(groupping["AdjustGains"].toBool());
}

void FitLayerWidget::showTextOnSensors(int type)
{
  Instructions::FitLayer *instruction = static_cast<Instructions::FitLayer*>(makeInstruction());
  std::vector<APoint> sensor_pos;
  pms *PMs = detector->PMs;
  for(int i = 0; i < PMs->count(); i++) {
    pm &PM = PMs->at(i);
    sensor_pos.push_back(APoint(PM.x, PM.y, PM.z));
  }
  AInstructionInput sensors(nullptr, sensor_pos, detector->PMgroups, nullptr, false, false, false);

  QVector<QString> text(PMs->count());
  auto groups = instruction->getSymmetryGroups(sensors);
  for(unsigned int j = 0; j < groups.size(); j++) {
    AFitLayerSensorGroup &group = groups[j];
    for(unsigned int i = 0; i < group.size(); i++) {
      const ATransform &transf = *group[i].second;
      QString t;
      switch(type) {
        case 0: case 1:
          t = QString::number(transf.getShift()[type])+(transf.getFlip() ? "F" : "");
          break;
        case 2:
          t = QString::number(transf.getPhi()/3.1415926*180)+(transf.getFlip() ? "F" : "");
          break;
        case 3:
          t = QString::number(j);
          break;
      }
      text[group[i].first] = t;
    }
  }

  emit requestShowTextOnSensors(text);
}

void FitLayerWidget::onLrfTypeGroupChange(const QString &group_name)
{
  auto &lrf_types = ALrfTypeManager::instance();
  cb_lrf_type->clear();
  for(auto type_id : *lrf_types.getTypesInGroup(group_name.toLocal8Bit().data())) {
    auto &type = lrf_types.getTypeFast(type_id);
    const std::string &name_ui = type.nameUi();
    cb_lrf_type->addItem(QString::fromStdString(name_ui), type_id.val());
  }
  onLrfTypeChanged(0);
}

void FitLayerWidget::onLrfTypeChanged(int index)
{
  if(index == -1 || index >= cb_lrf_type->count()) return;
  ALrfTypeID tid(cb_lrf_type->itemData(index).toULongLong());
  delete current_lrf_widget;
  current_lrf_widget = ALrfTypeManager::instance().getTypeFast(tid).newSettingsWidget();
  connect(current_lrf_widget, SIGNAL(elementChanged()), this, SIGNAL(elementChanged()));
  if(this->layout())
    this->layout()->addWidget(current_lrf_widget);
  emit elementChanged();
}

void FitLayerWidget::on_cb_pms_changed(int index)
{
  layout_control->removeWidget(le_pms);
  le_pms->setVisible(index == 1);
  layout_control->addWidget(le_pms, 2, 1);
  emit elementChanged();
}

void FitLayerWidget::on_ShowButton_clicked()
{
    QDialog* d = new QDialog();
    d->setModal(true);

    QVBoxLayout* l = new QVBoxLayout();

    QPushButton* b = new QPushButton("Show sensor selection");
    connect(b, &QPushButton::clicked, [=]() {
      emit requestShowPMs(cb_pms->currentIndex() == 1 ? le_pms->text() : "-");
      d->close();
    });
    l->addWidget(b);

    b = new QPushButton("Show groups");
    connect(b, &QPushButton::clicked, [=]() { showTextOnSensors(3); d->close(); });
    l->addWidget(b);

    b = new QPushButton("Show X shift");
    connect(b, &QPushButton::clicked, [=]() { showTextOnSensors(0); d->close(); });
    l->addWidget(b);

    b = new QPushButton("Show Y shift");
    connect(b, &QPushButton::clicked, [=]() { showTextOnSensors(1); d->close(); });
    l->addWidget(b);

    b = new QPushButton("Show rotation/flip");
    connect(b, &QPushButton::clicked, [=]() { showTextOnSensors(2); d->close(); });
    l->addWidget(b);

    d->setLayout(l);
    d->exec();

    delete d;
}

void FitLayerWidget::on_ContextmenuRequested_pmSelection(const QPoint &pos)
{
    QMenu Menu;
    Menu.addSeparator();
    QAction* a_ShowPMs = Menu.addAction("Show PM selection");
    Menu.addSeparator();

    QAction* SelectedAction = Menu.exec(mapToGlobal(pos));

    if (SelectedAction == a_ShowPMs)
    {
        emit requestShowPMs(le_pms->text());
    }
}



/***************************************************************************\
*                     Implementation of InheritWidget                       *
\***************************************************************************/
InheritWidget::InheritWidget(const ARepository *repo, QWidget *parent)
  : AInstructionEditorWidget(parent), repo(repo)
{
  QGridLayout *layout = new QGridLayout;
  sel_recipe = new QComboBox;
  sel_version = new QComboBox;
  sel_sensors = new QLineEdit;

  layout->addWidget(new QLabel("Recipe:"), 0, 0, 1, 1);
  layout->addWidget(sel_recipe, 0, 1, 1, 1);
  layout->addWidget(new QLabel("Version:"), 1, 0, 1, 1);
  layout->addWidget(sel_version, 1, 1, 1, 1);
  layout->addWidget(new QLabel("Sensor selection:"), 2, 0, 1, 1);
  layout->addWidget(sel_sensors, 2, 1, 1, 1);
  //setLayout(layout);

  QVBoxLayout *hack_layout_for_proper_resizing = new QVBoxLayout;
  hack_layout_for_proper_resizing->addLayout(layout);
  setLayout(hack_layout_for_proper_resizing);
  hack_layout_for_proper_resizing->addStretch(1);

  refreshRecipeList();
  onSelRecipeChanged(0);

  connect(repo, &ARepository::recipeChanged, this, &InheritWidget::onRepoChangedRecipe);
  connect(sel_recipe, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &InheritWidget::onSelRecipeChanged);
  connect(sel_version, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &InheritWidget::elementChanged);
  connect(sel_sensors, &QLineEdit::editingFinished, this, &InheritWidget::elementChanged);
}

void InheritWidget::loadInstruction(const AInstruction &instruction)
{
  if(dynamic_cast<const Instructions::Inherit*>(&instruction))
    loadState(instruction.toJson()["data"].toObject());
}

AInstruction *InheritWidget::makeInstruction()
{
  return new Instructions::Inherit(saveState());
}

QJsonObject InheritWidget::saveState()
{
  QJsonObject save;
  save["recipe"] = sel_recipe->currentData().toInt();
  save["version"] = sel_version->currentData().toInt();
  save["sensors"] = sel_sensors->text();
  return save;
}

void InheritWidget::loadState(const QJsonObject &state)
{
  sel_sensors->setText(state["sensors"].toString());

  ARecipeID rid(state["recipe"].toVariant().toUInt());
  if(!repo->hasRecipe(rid)) return;
  setCurrentRecipe(rid);

  ARecipeVersionID vid(state["version"].toInt());
  if(!repo->hasVersion(rid, vid)) return;
  setCurrentVersion(vid);
}

void InheritWidget::refreshRecipeList()
{
  sel_recipe->clear();
  for(ARecipeID recipe : repo->getRecipeList())
    sel_recipe->addItem(QString::fromStdString(repo->getName(recipe)), recipe.val());
}

void InheritWidget::setCurrentRecipe(ARecipeID rid)
{
  for(int i = 0; i < sel_recipe->count(); i++)
    if(rid == ARecipeID(sel_recipe->itemData(i).toInt())) {
      sel_recipe->setCurrentIndex(i);
      break;
    }
}

void InheritWidget::setCurrentVersion(ARecipeVersionID vid)
{
  for(int i = 0; i < sel_version->count(); i++)
    if(vid == ARecipeVersionID(sel_version->itemData(i).toInt())) {
      sel_version->setCurrentIndex(i);
      break;
    }
}

void InheritWidget::onRepoChangedRecipe(ARecipeID rid)
{
  bool has_recipe = repo->hasRecipe(rid);
  if(sel_recipe->count() == 0) {
    refreshRecipeList();
    return;
  }
  ARecipeID sel_rid = ARecipeID(sel_recipe->currentData().toInt());
  ARecipeVersionID sel_vid = ARecipeVersionID(sel_version->currentData().toInt());
  if(sel_rid == rid) {
    if(has_recipe) {
      onSelRecipeChanged(sel_recipe->currentIndex());
      if(repo->hasVersion(sel_rid, sel_vid))
        setCurrentVersion(sel_vid);
    } else
      refreshRecipeList();
  } else {
    refreshRecipeList();
    setCurrentRecipe(sel_rid);
  }
}

void InheritWidget::onSelRecipeChanged(int index)
{
  sel_version->clear();
  sel_version->addItem("-- Most recent version --", -1);
  if(index >= sel_recipe->count()) return;
  ARecipeID id(sel_recipe->itemData(index).toULongLong());
  if(!repo->hasRecipe(id)) return;
  for(const ARecipeVersion &version : repo->getHistory(id))
    sel_version->addItem(QString::fromStdString(version.name), version.id.val());
  emit elementChanged();
}

/***************************************************************************\
*               Implementation of AInstructionListWidgetItem                *
\***************************************************************************/
AInstructionListWidgetItem::AInstructionListWidgetItem(const ARepository *repo, const EventsDataClass *,
                                                       const DetectorClass *detector, QWidget *parent)
  : QWidget(parent), repo(repo)
{
  is_expanded = false;
  pb_collapser = new QPushButton();
  pb_collapser->setCheckable(true);
  pb_collapser->setChecked(is_expanded);
  pb_collapser->setFixedSize(21, 21);
  connect(pb_collapser, &QPushButton::toggled, this, &AInstructionListWidgetItem::setExpanded);

  le_name = new QLineEdit("Name");

  instructions_widget = new QStackedWidget(this);
  instructions_widget->setFrameShape(QFrame::StyledPanel);
  cb_instruction = new QComboBox;
  connect(cb_instruction, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &AInstructionListWidgetItem::onInstructionChanged);
  setupInstructionWidget(new FitLayerWidget(detector, this), "Fit Layer", "Fit a layer of LRFs");
  setupInstructionWidget(new InheritWidget(repo, this), "Inherit", "Inherit a set of sensors");

  QHBoxLayout *layout_minimized = new QHBoxLayout;
  layout_minimized->setContentsMargins(0, 0, 0, 0);
  layout_minimized->setSpacing(2);
  layout_minimized->addWidget(pb_collapser);
  layout_minimized->addWidget(le_name);
  layout_minimized->addWidget(cb_instruction);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(layout_minimized);
  layout->addWidget(instructions_widget);
  this->setLayout(layout);

  onInstructionChanged(0);
}

void AInstructionListWidgetItem::loadInstruction(const AInstruction &instruction)
{
  int index = getInstructionIndex(instruction.typeName());
  if(0 > index || index >= instructions_widget->count()) {
    qDebug()<<"Unregistered instruction"<<QString::fromStdString(instruction.typeName())<<"in alrfwindowidgets.cpp";
    return;
  }
  static_cast<AInstructionEditorWidget*>(instructions_widget->widget(index))->loadInstruction(instruction);
  cb_instruction->setCurrentIndex(index);
}

AInstruction *AInstructionListWidgetItem::makeInstruction()
{
  return static_cast<AInstructionEditorWidget*>(instructions_widget->currentWidget())->makeInstruction();
}

QJsonObject AInstructionListWidgetItem::saveState()
{
  QJsonObject state;
  state["name"] = le_name->text();
  state["expanded"] = isExpanded();
  QJsonArray state_instructions;
  for(int i = 0; i < instructions_widget->count(); i++)
    state_instructions.append(static_cast<AInstructionEditorWidget*>(instructions_widget->widget(i))->saveState());
  state["instructions"] = state_instructions;
  state["current instruction"] = cb_instruction->currentIndex();
  return state;
}

void AInstructionListWidgetItem::loadState(const QJsonObject &state)
{
  le_name->setText(state["name"].toString());
  QJsonArray state_instructions = state["instructions"].toArray();
  for(int i = 0; i < state_instructions.count(); i++)
    static_cast<AInstructionEditorWidget*>(instructions_widget->widget(i))->loadState(state_instructions[i].toObject());
  cb_instruction->setCurrentIndex(state["current instruction"].toInt());
  setExpanded(state["expanded"].toBool());
}

bool AInstructionListWidgetItem::isExpanded() const
{
  return is_expanded;
}

void AInstructionListWidgetItem::setExpanded(bool expanded)
{
  if(expanded == isExpanded())
    return;
  this->is_expanded = expanded;
  pb_collapser->setChecked(expanded);
  if(expanded) {
    pb_collapser->setText("-");
    instructions_widget->setVisible(true);
  } else {
    pb_collapser->setText("+");
    instructions_widget->setVisible(false);
  }
  emit elementChanged();
}

void AInstructionListWidgetItem::setEditable(bool editable)
{
  cb_instruction->setEnabled(editable);
  instructions_widget->setEnabled(editable);
}

void AInstructionListWidgetItem::onInstructionChanged(int index)
{
  instructions_widget->setCurrentIndex(index);

  if(is_expanded) emit elementChanged();
  else setExpanded(true);
}

void AInstructionListWidgetItem::setupInstructionWidget(AInstructionEditorWidget *widget, const QString &name, const QString &ui_text)
{
  if(widget->layout()) //Let's hope they all have layouts... if not, add them!
    widget->layout()->setContentsMargins(5, 3, 5, 3);
  instructions_widget->addWidget(widget);
  cb_instruction->addItem(ui_text, name);
  connect(widget, &AInstructionEditorWidget::elementChanged, this, &AInstructionListWidgetItem::elementChanged);
  connect(widget, &AInstructionEditorWidget::requestShowPMs, this, &AInstructionListWidgetItem::requestShowPMs);
  connect(widget, &AInstructionEditorWidget::requestShowTextOnSensors, this, &AInstructionListWidgetItem::requestShowTextOnSensors);
}

size_t AInstructionListWidgetItem::getInstructionIndex(const std::string &type_name) {
  QString name = QString::fromStdString(type_name);
  for(int i = 0; i < cb_instruction->count(); i++) {
    if(cb_instruction->itemData(i).toString() == name)
      return i;
  }
  return -1;
}


/***************************************************************************\
*                      Implementation of ASensorWidget                      *
\***************************************************************************/
ASensorWidget::ASensorWidget(const ASensor &sensor)
{
  QDoubleValidator *dv = new QDoubleValidator;
  QTableWidget *table = new QTableWidget;
  table->setColumnCount(2);
  table->setRowCount(sensor.deck.size());
  table->setHorizontalHeaderLabels(QStringList() << "Type" << "Coefficient");
  QStringList hor_labels;
  for(unsigned int i = 0; i < sensor.deck.size(); i++) {
    hor_labels.append(QString::number(i));
    const ALrfType *type = ALrfTypeManager::instance().getType(sensor.deck[i].lrf->type());
    QLineEdit *name = new QLineEdit(QString::fromStdString(type ? type->nameUi() : "Unknown"));
    name->setReadOnly(true);
    table->setCellWidget(i, 0, name);
    QLineEdit *coeff = new QLineEdit;
    coeff->setValidator(dv);
    coeff->setAlignment(Qt::AlignRight);
    coeff->setText(QString::number(sensor.deck[i].coeff));
    table->setCellWidget(i, 1, coeff);
    connect(coeff, &QLineEdit::editingFinished,
            [=] { emit coefficientEdited(i, coeff->text().toDouble()); });
  }
  table->setVerticalHeaderLabels(hor_labels);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(new QLabel("Layers:"), 0, Qt::AlignLeft);
  layout->addWidget(table);
  this->setLayout(layout);

  for(int i = 0; i < table->columnCount(); i++)
    table->resizeColumnToContents(i);
}


/***************************************************************************\
*                   Implementation of ARecipeVersionWidget                  *
\***************************************************************************/
ARecipeVersionWidget::ARecipeVersionWidget(ARecipeID rid, QString recipe_name, const ARecipeVersion &version)
{
  QLineEdit *le_creation_time = new QLineEdit(QString::fromStdString(version.creation_time));
  le_creation_time->setReadOnly(true);
  QLineEdit *le_recipe_name = new QLineEdit(recipe_name);
  le_recipe_name->setReadOnly(true);
  QLineEdit *le_recipe_id = new QLineEdit(QString::number(rid.val()));
  le_recipe_id->setReadOnly(true);
  QLineEdit *le_version_name = new QLineEdit(QString::fromStdString(version.name));
  le_version_name->setReadOnly(true);
  QLineEdit *le_version_id = new QLineEdit(QString::number(version.id.val()));
  le_version_id->setReadOnly(true);

  QTextEdit *te_comment = new QTextEdit(QString::fromStdString(version.comment));

  QPushButton *pb_save_comment = new QPushButton("Save comment");
  connect(pb_save_comment, &QPushButton::clicked, [=]() { emit saveCommentPressed(te_comment->toHtml()); });

  QFormLayout *sub_layout = new QFormLayout;
  sub_layout->addRow("Creation date:", le_creation_time);
  sub_layout->addRow("Recipe name:", le_recipe_name);
  sub_layout->addRow("Recipe id:", le_recipe_id);
  sub_layout->addRow("Version name:", le_version_name);
  sub_layout->addRow("Version id:", le_version_id);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addLayout(sub_layout);
  layout->addWidget(new QLabel("Comments:"));
  layout->addWidget(te_comment);
  layout->addWidget(pb_save_comment);
  this->setLayout(layout);
}


/***************************************************************************\
*                     Implementation of AHistoryWidget                      *
\***************************************************************************/
void AHistoryWidget::setItemColor(QTreeWidgetItem *item, const std::pair<QColor, QColor> &font_background)
{
  if(!item) {
    qDebug()<<"AHistoryWidget::setItemColor was given item = nullptr. There's a bug!";
    return;
  }
  for(int i = 0; i < columnCount(); i++) {
    item->setTextColor(i, font_background.first);
    item->setBackgroundColor(i, font_background.second);
  }
}

void AHistoryWidget::setIndicesAndColors(ARecipeID rid, ARecipeVersionID vid, int &recipe, int &version, const std::pair<QColor, QColor> &font_background)
{
  if(recipe != -1) {
    QTreeWidgetItem *item = topLevelItem(recipe);
    setItemColor(item, regular_color);
    if(version != -1) {
      setItemColor(item->child(version), regular_color);
    }
  }

  if(!repo->hasRecipe(rid)) {
    recipe = -1;
    version = -1;
    return;
  }

  if(vid == ARecipeVersionID(-1))
    vid = repo->getVersion(rid, vid).id;

  for(int i = 0; i < topLevelItemCount(); i++)
    if(topLevelItem(i)->data(0, Qt::UserRole).toUInt() == rid.val()) {
      recipe = i;
      QTreeWidgetItem *recipe_item = topLevelItem(recipe);
      setItemColor(recipe_item, font_background);
      for(int j = 0; j < recipe_item->childCount(); j++)
        if(recipe_item->child(j)->data(0, Qt::UserRole).toUInt() == vid.val()) {
          version = j;
          setItemColor(recipe_item->child(j), font_background);
          break;
        }
      break;
    }
}

AHistoryWidget::AHistoryWidget(QWidget *parent) : QTreeWidget(parent)
{
  selection_lineage_depth = 0;
  current_recipe = -1;
  current_version = -1;
  secondary_recipe = -1;
  secondary_version = -1;

  current_color = std::make_pair(QColor(0, 192, 224), QColor(255, 255, 255));
  secondary_color = std::make_pair(QColor(0, 0, 0), QColor(0, 192, 224));
  regular_color = std::make_pair(QColor(0, 0, 0), QColor(255, 255, 255));

  setSelectionMode(QAbstractItemView::SingleSelection);
  setSortingEnabled(false);
  setExpandsOnDoubleClick(false);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &AHistoryWidget::itemSelectionChanged, this, &AHistoryWidget::onItemSelectionChanged);
  connect(this, &AHistoryWidget::itemChanged, this, &AHistoryWidget::autoResizeColumns);
}

int AHistoryWidget::findTopLevelItem(ARecipeID rid)
{
  for(int i = 0; i < topLevelItemCount(); i++) {
    if(rid == ARecipeID(topLevelItem(i)->data(0, Qt::UserRole).toULongLong()))
      return i;
  }
  return topLevelItemCount();
}

QTreeWidgetItem *AHistoryWidget::getSelectedItem(int depth)
{
  if(0 <= depth && depth < selection_lineage_depth)
    return selection_lineage[selection_lineage_depth-1];
  else return nullptr;
}

ARecipeID AHistoryWidget::getSelectedRecipeId() const {
  if(selection_lineage_depth < 1)
    return invalid_recipe_id;
  else
    return ARecipeID(selection_lineage[0]->data(0, Qt::UserRole).toUInt());
}

ARecipeVersionID AHistoryWidget::getSelectedVersionId() const
{
  if(selection_lineage_depth < 2)
    return ARecipeVersionID(-1);
  else
    return ARecipeVersionID(selection_lineage[1]->data(0, Qt::UserRole).toUInt());
}

int AHistoryWidget::getSelectedSensor() const
{
  if(selection_lineage_depth < 3)
    return -1;
  return selection_lineage[2]->data(0, Qt::UserRole).toInt();
}

int AHistoryWidget::getSelectedLayer() const
{
  if(selection_lineage_depth < 4)
    return -1;
  return selection_lineage[3]->data(0, Qt::UserRole).toInt();
}

void AHistoryWidget::fillRecipeItem(QTreeWidgetItem *recipe_item, ARecipeID rid)
{
  const std::string &name = repo->getName(rid);
  recipe_item->setText(0, QString::fromStdString(name));

  auto &hist = repo->getHistory(rid);
  for(int i = hist.size()-1; i >= 0; i--) {
    QTreeWidgetItem *version_item = new QTreeWidgetItem(recipe_item);
    version_item->setText(0, QString::fromStdString(hist[i].name));
    version_item->setData(0, Qt::UserRole, hist[i].id.val());
    version_item->setText(1, QString::number(hist[i].id.val()));
    version_item->setExpanded(false);

    const ASensorGroup &group = hist[i].sensors;
    for(size_t i = 0; i < group.size(); i++) {
      QString str_num = QString::number(group[i].ipm);
      QTreeWidgetItem *sensor_item = new QTreeWidgetItem(version_item);
      sensor_item->setText(0, "Sensor "+str_num);
      sensor_item->setData(0, Qt::UserRole, group[i].ipm);
      //sensor_item->setText(1, str_num);
      sensor_item->setExpanded(false);

      int deck_size = group[i].deck.size();
      for(int j = 0; j < deck_size; j++) {
        QString str_num_lrf = QString::number(j);
        QTreeWidgetItem *lrf_item = new QTreeWidgetItem(sensor_item);
        lrf_item->setText(0, "Lrf "+str_num_lrf);
        lrf_item->setData(0, Qt::UserRole, j);
        //lrf_item->setText(1, str_num_lrf);
      }
    }
  }
}

void AHistoryWidget::onRecipeChanged(ARecipeID rid)
{
  int recipe_index = findTopLevelItem(rid);
  if(!repo->hasRecipe(rid)) {
    if(recipe_index != topLevelItemCount()) {
      delete takeTopLevelItem(recipe_index);
      current_recipe = -1;
      current_version = -1;
      secondary_recipe = -1;
      secondary_version = -1;
    }
  } else {
    QTreeWidgetItem *recipe_item;
    QFont bold(this->font());
    bold.setBold(true);
    if(recipe_index == topLevelItemCount()) {
      recipe_item = new QTreeWidgetItem(this);
      recipe_item->setData(0, Qt::UserRole, rid.val());
      recipe_item->setFont(1, bold);
      recipe_item->setText(1, "R " + QString::number(rid.val()));
    } else {
      recipe_item = topLevelItem(recipe_index);
      //If we already have that recipe, clear it. Is there no better way to clear?
      for(auto *child : recipe_item->takeChildren())
        delete child;
    }
    fillRecipeItem(recipe_item, rid);
    recipe_item->setExpanded(true);
  }

  setIndicesAndColors(repo->getCurrentRecipeID(), repo->getCurrentRecipeVersionID(),
                      current_recipe, current_version, current_color);
  setIndicesAndColors(repo->getSecondaryRecipeID(), repo->getSecondaryRecipeVersionID(),
                      secondary_recipe, secondary_version, secondary_color);
}

void AHistoryWidget::onCurrentLrfsChanged(ARecipeID rid, ARecipeVersionID vid)
{
  setIndicesAndColors(rid, vid, current_recipe, current_version, current_color);
}

void AHistoryWidget::onSecondaryLrfsChanged(ARecipeID rid, ARecipeVersionID vid)
{
  setIndicesAndColors(rid, vid, secondary_recipe, secondary_version, secondary_color);
}

void AHistoryWidget::autoResizeColumns()
{
  setColumnWidth(0, width());
  resizeColumnToContents(1);
  int col1_width = width() - columnWidth(1) - 2;
  if(verticalScrollBar()->isVisible())
  col1_width -= verticalScrollBar()->width();
  setColumnWidth(0, col1_width);
}

void AHistoryWidget::resizeEvent(QResizeEvent *event)
{
  QTreeWidget::resizeEvent(event);
  autoResizeColumns();
}

void AHistoryWidget::onItemSelectionChanged()
{
  auto selection = this->selectedItems();
  if(selection.count() != 1) {
    selection_lineage_depth = 0;
    selection_lineage[0] = 0;
    return;
  }
  const int max_depth = sizeof(selection_lineage)/sizeof(*selection_lineage);

  selection_lineage[max_depth-1] = selection[0];
  selection_lineage_depth = 1;
  for(int i = max_depth-2; i >= 0; i--) {
    selection_lineage[i] = selection_lineage[i+1]->parent();
    if(selection_lineage[i]) {
      selection_lineage_depth++;
    }
    else {
      unsigned int j = 0;
      i++;
      while(i < max_depth) {
        selection_lineage[j++] = selection_lineage[i++];
      }
      for(; j < max_depth; j++) {
        selection_lineage[j] = nullptr;
      }
      break;
    }
  }
}


} } //namespace LRF::LrfWindowWidgets
