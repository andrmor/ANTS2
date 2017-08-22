#ifndef ANEUTRONREACTIONSCONFIGURATOR_H
#define ANEUTRONREACTIONSCONFIGURATOR_H

#include "aneutroninteractionelement.h"

#include <QDialog>
#include <QStringList>

namespace Ui {
class ANeutronReactionsConfigurator;
}

class QKeyEvent;

class ANeutronReactionsConfigurator : public QDialog
{
    Q_OBJECT

public:
    explicit ANeutronReactionsConfigurator(AAbsorptionElement* Element, QStringList DefinedParticles, QWidget *parent = 0);
    ~ANeutronReactionsConfigurator();

private slots:
    void on_pbAdd_clicked();
    void on_pbRemove_clicked();
    void on_pbConfirm_clicked();

public slots:
    void onResizeRequest();

private:
    Ui::ANeutronReactionsConfigurator *ui;
    AAbsorptionElement* Element;
    AAbsorptionElement  Element_LocalCopy;
    QStringList DefinedParticles;

    void updateReactions();
};

#endif // ANEUTRONREACTIONSCONFIGURATOR_H
