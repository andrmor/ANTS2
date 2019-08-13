#ifndef ALRFWINDOW_H
#define ALRFWINDOW_H

#include "ui_alrfwindow.h"
#include "aguiwindow.h"

#include <map>
#include <memory>
#include <QJsonObject>

#include "idclasses.h"

#include "TMathBase.h"

class MainWindow;
class AStateInterface;
namespace LRF {
class ARepository;
class ASensorGroup;

namespace LrfWindowWidgets {
class AInstructionListWidgetItem;
class AInstructionListWidget;
class AHistoryWidget;
}
}

class AMouseWheelFilter : public QObject {
  Q_OBJECT
public:
  bool eventFilter(QObject *watched, QEvent *e) override
  {
    if(e->type() == QEvent::Wheel) {
      QWidget *widget = qobject_cast<QWidget*>(watched);
      return widget && !widget->hasFocus();
    }
    return false;
  }
};

class ALrfWindow : public AGuiWindow, private Ui::ALrfWindow
{
  Q_OBJECT
  int right_column_last_size;
  MainWindow *mw;
  LRF::ARepository *repo;
  LRF::ARecipeID right_column_rid;
  LRF::ARecipeVersionID right_column_vid;
  std::unique_ptr<QWidget> wid_right_column;

public:
  explicit ALrfWindow(QWidget *parent, MainWindow *mw, LRF::ARepository *lrf_repo);

  void writeInstructionsToJson(QJsonObject &json);

public slots:
  void readInstructionsFromJson(const QJsonObject &state);
  void onRequestShowPMs(QString selection);
  void onReadyStatusChanged(bool fReady);
  void onBusyStatusChanged(bool fStatus);
  void on_pbUpdateInstructionsJson_clicked();

private:
  void setSplitterRightColumn(QWidget *new_widget = nullptr);
  void addWidgetToInstructionList(QListWidget *list, LRF::LrfWindowWidgets::AInstructionListWidgetItem *widget);
  bool fillInstructionList(QListWidget *list, std::vector<LRF::AInstructionID> instructions);
  LRF::ARecipeID getRecipeFromInstructionList(QListWidget *list, QString &qname, bool *is_new = nullptr);

private slots:
  void onInstructionsListUpdate(QListWidget *list);

  void on_tw_recipes_itemClicked(QTreeWidgetItem *item, int column);
  void on_tw_recipes_itemDoubleClicked(QTreeWidgetItem *item, int column);
  void on_tw_recipes_customContextMenuRequested(const QPoint &pos);

  void onUnsetSecondaryLrfs();
  void onClearRepository();
  void onRecipeChanged(LRF::ARecipeID rid);

  void onRecipeContextMenuActionSetAsPrimary();
  void onRecipeContextMenuActionSetAsSecondary();
  void onRecipeContextMenuActionAppendToEditor();
  void onRecipeContextMenuActionCopyToEditor();
  void onRecipeContextMenuActionSave();
  void onRecipeContextMenuActionRename();
  void onRecipeContextMenuActionDelete();

  void onVersionContextMenuActionSetAsPrimary();
  void onVersionContextMenuActionSetAsSecondary();
  void onVersionContextMenuActionSave();
  void onVersionContextMenuActionRename();
  void onVersionContextMenuActionDelete();

  void onSaveLrfsPressed(AStateInterface *interface, LRF::ALrfTypeID tid,
                         LRF::ARecipeID rid, LRF::ARecipeVersionID vid, int isensor, int ilayer);

  void on_pb_add_instruction_clicked();
  void on_pb_remove_instruction_clicked();
  void on_pb_make_lrfs_clicked();
  void on_pbSTOP_clicked();

  void on_pb_save_repository_clicked();
  void on_pb_load_repository_clicked();
  void on_pb_save_current_clicked();
  void on_pb_load_current_clicked();

  void on_pb_show_radial_clicked();
  void on_cobShowLRF_LayerSelection_currentIndexChanged(int index);
  void on_pb_show_xy_clicked();
  void on_cbAddData_toggled(bool checked);
  void on_cbAddDiff_toggled(bool checked);
  void on_sbPM_valueChanged(int arg1);
  void on_actionLRF_explorer_triggered();
  void on_action_add_empty_recipe_triggered();
  void on_action_unset_secondary_lrfs_triggered();
  void on_action_clear_repository_triggered();
  void on_action_clear_instruction_list_triggered();

  void on_pb_close_right_column_clicked();
private:
  bool doLrfRadialProfile(QVector<double> &radius, QVector<double> &LRF);
private slots:
  void on_pbShowRadialForXY_clicked();
  void on_pbExportLrfVsRadial_clicked();
};

#endif // ALRFWINDOW_H
