#ifndef ALRFMOUSEEXPLORER_H
#define ALRFMOUSEEXPLORER_H

#include <QDialog>

class ALrfModuleSelector;
class pms;
class APmGroupsManager;
class Viewer2DarrayObject;
class myQGraphicsView;
class QPointF;
class QComboBox;
class QLineEdit;

class ALrfMouseExplorer : public QDialog
{
  Q_OBJECT

public:
  ALrfMouseExplorer(ALrfModuleSelector *LRFs, pms *PMs, APmGroupsManager *PMgroups, double SuggestedZ = 0, QWidget *parent = 0);
  ~ALrfMouseExplorer();

  void Start();

private:
  Viewer2DarrayObject *LRFviewObj;
  myQGraphicsView* gv;

  ALrfModuleSelector* LRFs;
  pms* PMs;
  APmGroupsManager* PMgroups;

  QComboBox *cobSG;
  QLineEdit *ledZ;

public slots:
  void paintLRFonDialog(QPointF* pos);
  void onCobActivated(int);

};

#endif // ALRFMOUSEEXPLORER_H
