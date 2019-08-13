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
    explicit ANeutronReactionsConfigurator(ANeutronInteractionElement* Element, QStringList DefinedParticles, QWidget *parent = 0);
    ~ANeutronReactionsConfigurator();

private slots:
    void on_pbAdd_clicked();
    void on_pbRemove_clicked();
    void on_pbConfirm_clicked();

public slots:
    void onResizeRequest();

signals:
    void RequestDraw(const QVector<double> & x, const QVector<double> & y, const QString & titleX, const QString & titleY);

private:
    Ui::ANeutronReactionsConfigurator *ui;
    ANeutronInteractionElement* Element;
    ANeutronInteractionElement  Element_LocalCopy;
    QStringList DefinedParticles;

    void updateDecayScenarios();
};

#endif // ANEUTRONREACTIONSCONFIGURATOR_H
