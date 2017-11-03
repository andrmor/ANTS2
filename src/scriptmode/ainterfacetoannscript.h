#ifndef AINTERFACETOANNSCRIPT_H
#define AINTERFACETOANNSCRIPT_H

#include "ascriptinterface.h"
#include "neuralnetworksmodule.h"
#include "eventsdataclass.h"

#include <QObject>
#include <QVariant>
#include <QVector>
#include <QJsonObject>

class NeuralNetworksScript;
class AInterfaceToANNScript;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                           NeuralNetworksScript                            */
/*             ( Last modified by Francisco Neves @ 2017.11.03 )             */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
class NeuralNetworksScript : public cFANNWrapper{
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    string FOutFile;
    unsigned FnEpochsStag, FMaxEpochsStag;
    double FTrainMSE, FTrainFailBit, FTestMSE, FTestFailBit;
public:
    AInterfaceToANNScript* FParent; // used by ::requestPrint.
protected: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
   int userCallback(fann_train_data* train, unsigned max_epochs,
    unsigned epochs_between_reports, float desired_error, unsigned epochs);
public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
   explicit NeuralNetworksScript(AInterfaceToANNScript *parent);
   void init_train(string outFile);
};


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                          AInterfaceToANNScript                            */
/*             ( Last modified by Francisco Neves @ 2017.11.03 )             */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
class AInterfaceToANNScript : public AScriptInterface{
  Q_OBJECT
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  EventsDataClass* EventsDataHub;
  NeuralNetworksScript afann;
  QVector< QVector<float> > Input;
  QVector< QVector<float> > Output;
  QJsonObject Config;
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
  void clearTrainingData();
  QVariant getConfig();
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
