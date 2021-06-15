#include "ashapehelpdialog.h"
#include "ui_ashapehelpdialog.h"
#include "ageoobject.h"
#include "ageoshape.h"

AShapeHelpDialog::AShapeHelpDialog(AGeoShape *thisShape, QList<AGeoShape *> shapes, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::AShapeHelpDialog)
{
  ui->setupUi(this);
  SelectedTemplate = "";
  //this->setWindowModality(Qt::ApplicationModal);
  this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);

  AvailableShapes = shapes;
  for (int i=0; i<AvailableShapes.size(); i++)
      ui->lwTypes->addItem(AvailableShapes.at(i)->getShapeTemplate());

  if (thisShape)
    {
      for (int index=0; index<AvailableShapes.size(); index++)
        {
          if (thisShape->getShapeType() == AvailableShapes.at(index)->getShapeType())
            {
              ui->lwTypes->setCurrentRow(index);
              break;
            }
        }
    }
}

AShapeHelpDialog::~AShapeHelpDialog()
{
  delete ui;
}

void AShapeHelpDialog::on_pbSelect_clicked()
{
  SelectedTemplate = ui->leSelected->text();
  accept();
}

void AShapeHelpDialog::on_pbClose_clicked()
{
   reject();
}

void AShapeHelpDialog::on_lwTypes_currentRowChanged(int row)
{
  ui->leSelected->clear();
  ui->pteHelp->clear();

  if (row==-1) return;
  AGeoShape* s = AvailableShapes[row];

  ui->leSelected->setText(s->getShapeTemplate());
  ui->pteHelp->appendPlainText(s->getHelp());
}
