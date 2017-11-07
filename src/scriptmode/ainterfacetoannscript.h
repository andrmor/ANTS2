#ifndef AINTERFACETOANNSCRIPT_H
#define AINTERFACETOANNSCRIPT_H

#include "ascriptinterface.h"
#include "neuralnetworksmodule.h"
#include "eventsdataclass.h"

#include <QObject>
#include <QVariant>
#include <QVector>
#include <QJsonObject>
#include <QApplication>

class NeuralNetworksScript;
class AInterfaceToANNScript;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                           NeuralNetworksScript                            */
/*             ( Last modified by Francisco Neves @ 2017.11.06 )             */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
class NeuralNetworksScript : public cFANNWrapper{
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    bool FStop;
    string FOutFile;
    unsigned FnEpochsStag, FMaxEpochsStag;
    double FTrainMSE, FTrainFailBit, FTestMSE, FTestFailBit;
    AInterfaceToANNScript* FParent; // used by ::requestPrint.
protected: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
   int userCallback(fann_train_data* train, unsigned max_epochs,
    unsigned epochs_between_reports, float desired_error, unsigned epochs);
public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
   explicit NeuralNetworksScript(AInterfaceToANNScript *parent);
   //..........................................................................
   inline void stop_Train(){ FStop=true; }
   void init_train(string outFile);
   //..........................................................................
   void run(QVector<float> &in, QVector<float> &out);
};


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                          AInterfaceToANNScript                            */
/*             ( Last modified by Francisco Neves @ 2017.11.07 )             */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
class AInterfaceToANNScript : public AScriptInterface{
  Q_OBJECT
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  EventsDataClass* EventsDataHub;
  NeuralNetworksScript afann;
  QVector< QVector<float> > Input;
  QVector< QVector<float> > Output;
  QJsonObject Config;
  QString buildNN(QString from, cFANNWrapper::cActivationFunctions &funcs, cFANNWrapper::cActivationSteepness &steps);
  bool convertQVariantToVectorOfVectors(QVariant* var, QVector< QVector<float> >* vec , int fixedSize = -1);
  void setConfig_cascade();
  void setConfig_commom();
  void norm(QVector<float> &data, double to=1.);
protected: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  friend class NeuralNetworksScript; // enable use of ::requestPrint
public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  AInterfaceToANNScript(EventsDataClass* EventsDataHub);
  ~AInterfaceToANNScript();
  //...........................................................................
  void ForceStop(); // inherited from AScriptInterface
public slots: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  void newNetwork();
  QVariant getConfig();
  void clearTrainingData();
  void resetConfigToDefault();
  QString configure(QVariant configObject);
  //...........................................................................
  void addTrainingInput(QVariant arrayOfArrays);
  void addTrainingOutput(QVariant arrayOfArrays);
  //...........................................................................
  QString load(QString FANN_File);
  QString train(QString FANN_File);
  //...........................................................................
  QVariant process(QVariant arrayOfArrays);
  QVariant processEvents(int firstEvent, int lastEvent);
};

#endif // AINTERFACETOANNSCRIPT_H
