#ifndef ALRFWINDOWWIDGETS_H
#define ALRFWINDOWWIDGETS_H

#include <memory>
#include <functional>

#include <QWidget>
#include <QListWidget>
#include <QTreeWidget>
#include <QAbstractItemModel>

#include "idclasses.h"

class QComboBox;
class QPushButton;
class QVBoxLayout;
class QGridLayout;
class QButtonGroup;
class QCheckBox;
class QFrame;
class QStackedWidget;
class QTabWidget;
class QGroupBox;
class ACollapsibleGroupBox;
class QLineEdit;

class EventsDataClass;
class DetectorClass;

namespace LRF {

class ALrf;
class ASensor;
class ASensorGroup;
class AInstruction;
class ARecipeVersion;
class ARepository;
class ALrfSettingsWidget;

namespace Instructions {
class FitLayer;
}


namespace LrfWindowWidgets {

class AInstructionEditorWidget : public QWidget {
  Q_OBJECT
public:
  AInstructionEditorWidget(QWidget *parent = nullptr) : QWidget(parent) {}
  virtual void loadInstruction(const AInstruction &instruction) = 0;
  virtual AInstruction *makeInstruction() = 0;
  virtual QJsonObject saveState() = 0;
  virtual void loadState(const QJsonObject &state) = 0;
signals:
  void requestShowPMs(QString selection);
  void requestShowTextOnSensors(QVector<QString> text);
  void elementChanged();
};


class FitLayerWidget : public AInstructionEditorWidget {
  Q_OBJECT
  const DetectorClass *detector;

  //LrfSelector *lrf_selection;
  QComboBox *cb_lrf_category;
  QComboBox *cb_lrf_type;
  QGridLayout *layout_lrf_type_selection;
  QWidget *current_lrf_widget;

  QGridLayout *layout_control;
  QComboBox *cb_sensor_group;
  QComboBox *cb_pms;
  QLineEdit *le_pms;
  QComboBox *cb_stack_operation;
  ACollapsibleGroupBox *gb_groupping;
  QComboBox *group_type;
  QCheckBox *adjust_gains;

public:
  FitLayerWidget(const DetectorClass *detector, QWidget *parent = nullptr);
  void loadInstruction(const AInstruction &instruction) override;
  AInstruction *makeInstruction() override;
  QJsonObject saveState() override;
  void loadState(const QJsonObject &state) override;
  void showTextOnSensors(int type);

private slots:
  void onLrfTypeGroupChange(const QString &group_name);
  void onLrfTypeChanged(int index);
  void on_cb_pms_changed(int index);
  void on_ShowButton_clicked();
  void on_ContextmenuRequested_pmSelection(const QPoint &pos);
};

class InheritWidget : public AInstructionEditorWidget {
  Q_OBJECT
  const ARepository *repo;
  QComboBox *sel_recipe;
  QComboBox *sel_version;
  QLineEdit *sel_sensors;

public:
  InheritWidget(const ARepository *repo, QWidget *parent = nullptr);
  void loadInstruction(const AInstruction &instruction) override;
  AInstruction *makeInstruction() override;
  QJsonObject saveState() override;
  void loadState(const QJsonObject &state) override;

private:
  void refreshRecipeList();
  void setCurrentRecipe(ARecipeID rid);
  void setCurrentVersion(ARecipeVersionID vid);

private slots:
  void onRepoChangedRecipe(ARecipeID rid);
  void onSelRecipeChanged(int index);
};



class AInstructionListWidgetItem : public QWidget {
  Q_OBJECT
  const ARepository *repo;
  QPushButton *pb_collapser;
  QLineEdit *le_name;
  QComboBox *cb_instruction;
  QStackedWidget *instructions_widget;
  bool is_expanded;
public:
  AInstructionListWidgetItem(const ARepository *repo, const EventsDataClass *events_data_hub,
                             const DetectorClass *detector, QWidget *parent = nullptr);
  void loadInstruction(const AInstruction &instruction);
  AInstruction *makeInstruction();

  QJsonObject saveState();
  void loadState(const QJsonObject &state);

  bool isExpanded() const;

signals:
  void requestShowPMs(QString selection);
  void requestShowTextOnSensors(QVector<QString> text);
  void elementChanged();
public slots:
  void setExpanded(bool expanded = true);
  void setEditable(bool editable = true);
private slots:
  void onInstructionChanged(int index);
private:
  void setupInstructionWidget(AInstructionEditorWidget *widget, const QString &name, const QString &ui_text);
  size_t getInstructionIndex(const std::string &type_name);
};


class ASensorWidget : public QWidget {
  Q_OBJECT
public:
  ASensorWidget(const ASensor &sensor);
signals:
  void coefficientEdited(int layer, double new_value);
};


class ARecipeVersionWidget : public QWidget {
  Q_OBJECT
public:
  ARecipeVersionWidget(ARecipeID rid, QString recipe_name, const ARecipeVersion &version);
signals:
  void saveCommentPressed(QString comment_html);
};



class AHistoryWidget : public QTreeWidget {
  Q_OBJECT
  ARepository *repo;
  ARecipeID rid;

  int selection_lineage_depth;
  QTreeWidgetItem *selection_lineage[4]; //Recipe->Version->Sensor->Layer
  int current_recipe, current_version;
  int secondary_recipe, secondary_version;

  std::pair<QColor, QColor> current_color;
  std::pair<QColor, QColor> secondary_color;
  std::pair<QColor, QColor> regular_color;

  void setItemColor(QTreeWidgetItem *item, const std::pair<QColor, QColor> &font_background);
  void setIndicesAndColors(ARecipeID rid, ARecipeVersionID vid, int &recipe, int &version, const std::pair<QColor, QColor> &font_background);
  void fillRecipeItem(QTreeWidgetItem *recipe_item, ARecipeID rid);

public:
  AHistoryWidget(QWidget *parent = nullptr);
  void setRepository(ARepository *repo) { this->repo = repo; }

  int findTopLevelItem(ARecipeID rid);
  QTreeWidgetItem *getSelectedItem(int depth = -1);
  int getSelectedItemDepth() const { return selection_lineage_depth; }

  ARecipeID getSelectedRecipeId() const;
  ARecipeVersionID getSelectedVersionId() const;
  int getSelectedSensor() const;
  int getSelectedLayer() const;

public slots:
  void onRecipeChanged(ARecipeID rid);
  void onCurrentLrfsChanged(ARecipeID rid, ARecipeVersionID vid);
  void onSecondaryLrfsChanged(ARecipeID rid, ARecipeVersionID vid);
  void autoResizeColumns();

protected:
  void resizeEvent(QResizeEvent *event) override;

private slots:
  void onItemSelectionChanged();
};

} } //namespace LRF::LrfWindowWidgets

#endif // ALRFWINDOWWIDGETS_H
