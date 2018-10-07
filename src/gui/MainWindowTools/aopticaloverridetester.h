#ifndef AOPTICALOVERRIDETESTER_H
#define AOPTICALOVERRIDETESTER_H

#include "atracerstateful.h"

#include <QMainWindow>

namespace Ui {
class AOpticalOverrideTester;
}

class AMaterialParticleCollection;
class AOpticalOverride;
class MainWindow;
class GraphWindowClass;
class GeometryWindowClass;
class TRandom2;
class QJsonObject;

class AOpticalOverrideTester : public QMainWindow
{
    Q_OBJECT

public:
    explicit AOpticalOverrideTester(AOpticalOverride ** ovLocal, MainWindow* MW, int matFrom, int matTo, QWidget *parent = 0);
    ~AOpticalOverrideTester();

    void writeToJson(QJsonObject& json) const;
    void readFromJson(const QJsonObject& json);

public slots:
    void updateGUI();
    void showGeometry();

private slots:
    void on_pbDirectionHelp_clicked();
    void on_pbST_RvsAngle_clicked();
    void on_pbCSMtestmany_clicked();
    void on_pbST_showTracks_clicked();
    void on_pbST_uniform_clicked();

    void on_cbWavelength_toggled(bool checked);

    void on_ledST_wave_editingFinished();

    void on_ledST_i_editingFinished();

    void on_ledST_j_editingFinished();

    void on_ledST_k_editingFinished();

    void on_ledAngle_editingFinished();

private:
    Ui::AOpticalOverrideTester *ui;
    AOpticalOverride ** pOV;                    //external
    int MatFrom;
    int MatTo;
    MainWindow* MW;                             //external
    AMaterialParticleCollection* MPcollection;  //external
    GraphWindowClass* GraphWindow;              //external
    GeometryWindowClass* GeometryWindow;        //external
    TRandom2* RandGen;                          //local
    ATracerStateful* Resources;                 //loacl

    bool testOverride();
    int getWaveIndex();

};

#endif // AOPTICALOVERRIDETESTER_H
