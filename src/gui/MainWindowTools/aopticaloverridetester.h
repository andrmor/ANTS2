#ifndef AOPTICALOVERRIDETESTER_H
#define AOPTICALOVERRIDETESTER_H

#include <QMainWindow>

namespace Ui {
class AOpticalOverrideTester;
}

class AMaterialParticleCollection;
class AOpticalOverride;
class GraphWindowClass;

class AOpticalOverrideTester : public QMainWindow
{
    Q_OBJECT

public:
    explicit AOpticalOverrideTester(GraphWindowClass *GraphWindow, AMaterialParticleCollection *MPcollection, int matFrom, int matTo, QWidget *parent = 0);
    ~AOpticalOverrideTester();

private slots:
    void on_pbDirectionHelp_clicked();

    void on_pbST_RvsAngle_clicked();

private:
    Ui::AOpticalOverrideTester *ui;
    AMaterialParticleCollection* MPcollection;
    int MatFrom;
    int MatTo;
    AOpticalOverride* ov;
    GraphWindowClass* GraphWindow;
};

#endif // AOPTICALOVERRIDETESTER_H
