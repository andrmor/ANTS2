#ifndef NEURALNETWORKSWINDOW_H
#define NEURALNETWORKSWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QString>
#include "jsonparser.h"

class MainWindow;
class EventsDataClass;

namespace Ui { class NeuralNetworksWindow; }
using namespace std;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                            NeuralNetworksWindow                           */
/*         Francisco Neves @ 2013.11.15 ( Last modified 2015.04.28 )         */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
class NeuralNetworksWindow : public QMainWindow{
  Q_OBJECT
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    string FTrainInput, FTrainOutput;
    Ui::NeuralNetworksWindow *ui;
    MainWindow *MW;
    EventsDataClass *EventsDataHub;
    bool FChanged, FXYZAutoScale;
protected: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    void getJSON(QJsonObject &json);
    void setJSON(QJsonObject &json) const;
    unsigned build_trainFile(string filename);
    QString read_File(string filename) const;
    void write_File(string filename, QString &data) const;
    void build_layers(unsigned in, unsigned out, vector<unsigned> &hidden);
    double *getEvent_xyz(unsigned evt, unsigned hit=0);
    double getEvent_ener(unsigned evt, unsigned hit=0);
    double getEvent_signal(unsigned evt, unsigned ipm);
    bool getEvent_good(unsigned evt);
    unsigned getTrain_scale();
    void doTrain(bool resume);
public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    explicit NeuralNetworksWindow(QWidget* parent, MainWindow *mw, EventsDataClass *eventsDataHub);
    ~NeuralNetworksWindow();
    void saveJSON(QJsonObject &json) const;
    void loadJSON(const QJsonObject &json);
    void saveJSON(string filename);
    void loadJSON(string filename);
public slots: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    void status(QString mess);
    void userChangeUIOpt(bool isChanged);
    void onReconstruct3D(bool checked);
    void onReconstructE(bool checked);
private slots: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    void on_cbNNType_currentIndexChanged(const QString &arg1);
    void on_cbNNOutFunction_currentIndexChanged(const QString &arg1);
    void on_cbNNHideFunction_currentIndexChanged(const QString &arg1);
    void on_cbNNTrainAlg_currentIndexChanged(const QString &arg1);
    void on_cbNNTrainError_currentIndexChanged(const QString &arg1);
    void on_cbNNTrainStop_currentIndexChanged(const QString &arg1);
    void on_cbCascOutChangeFrac_clicked(bool checked);
    void on_cbCascOutStagnationEpochs_clicked(bool checked);
    void on_cbCascCandChangeFrac_clicked(bool checked);
    void on_cbCascCandStagnationEpochs_clicked(bool checked);
    void on_cbCascCandLimit_clicked(bool checked);
    void on_cbCascOutMinEpochs_clicked(bool checked);
    void on_cbCascOutMaxEpochs_clicked(bool checked);
    void on_cbCascCandMaxEpochs_clicked(bool checked);
    void on_cbCascCandMinEpochs_clicked(bool checked);
    void on_cbCascCandMult_clicked(bool checked);
    void on_cbCascSteep_clicked(bool checked);
    void on_cbCascFunctions_clicked(bool checked);
    void on_cbCascCandGroups_clicked(bool checked);
    void on_cbQPropDecay_clicked(bool checked);
    void on_cbQPropMu_clicked(bool checked);
    void on_cbRPropIncFactor_clicked(bool checked);
    void on_cbRPropDecFactor_clicked(bool checked);
    void on_cbRPropDeltaMin_clicked(bool checked);
    void on_cbRPropDeltaMax_clicked(bool checked);
    void on_cbRPropDeltaZero_clicked(bool checked);
    void on_pbNNAddLayer_released();
    void on_pbNNDelLayer_released();
    void on_pbNNTrain_released();
    void on_pbNNLoad_released();
    void on_pbTrainInput_released();
    void on_pbTrainOutput_released();
    void on_bsNNTestPerc_valueChanged(double arg1);
    void on_bsNNBitLimit_valueChanged(double arg1);
    void on_bsNNMaxNeurons_valueChanged(int arg1);
    void on_bsNNTrainStop_valueChanged(double arg1);
    void on_cbNNDataSrc_currentIndexChanged(const QString &arg1);
    void train_status(bool status);
    void train_finished();
    void on_pbNNResume_released();
    void on_pbNSaveOut_released();
signals: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    void train_stop();
};

#endif // NEURALNETWORKSWINDOW_H
