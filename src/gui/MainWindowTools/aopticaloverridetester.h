#ifndef AOPTICALOVERRIDETESTER_H
#define AOPTICALOVERRIDETESTER_H

#include "atracerstateful.h"

#include <QMainWindow>

#include "TVector3.h"

namespace Ui {
class AOpticalOverrideTester;
}

struct AReportForOverride
{
    double abs = 0;
    double back = 0;
    double forw = 0;
    double notTrigger = 0;
    double error = 0;

    double Bspike = 0;
    double Blobe = 0;
    double Blamb = 0;

    double waveChanged = 0;
    double timeChanged = 0;
};
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
    void on_pbST_RvsAngle_clicked();
    void on_pbCSMtestmany_clicked();
    void on_pbST_showTracks_clicked();
    void on_pbST_uniform_clicked();

    void on_cbWavelength_toggled(bool checked);

    void on_ledST_wave_editingFinished();

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
    ATracerStateful* Resources;                 //local

    const int maxNumTracks = 1000;

    bool testOverride();
    int getWaveIndex();
    const TVector3 getPhotonVector();
    void reportStatistics(const AReportForOverride& rep, int numPhot);

};

#endif // AOPTICALOVERRIDETESTER_H
