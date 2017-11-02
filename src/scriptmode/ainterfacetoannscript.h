#ifndef AINTERFACETOANNSCRIPT_H
#define AINTERFACETOANNSCRIPT_H

#include "ascriptinterface.h"
#include "neuralnetworksmodule.h"
#include "eventsdataclass.h"

#include <QObject>
#include <QVariant>
#include <QVector>
#include <QJsonObject>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                           NeuralNetworksScript                            */
/*             ( Last modified by Francisco Neves @ 2017.10.30 )             */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
class NeuralNetworksScript : public cFANNWrapper{
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    unsigned FnEpochsStag, FMaxEpochsStag;
    double FTrainMSE, FTrainFailBit, FTestMSE, FTestFailBit;
protected: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
   int userCallback(fann_train_data* train, unsigned max_epochs,
    unsigned epochs_between_reports, float desired_error, unsigned epochs);
public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
   explicit NeuralNetworksScript():FMaxEpochsStag(20){ }
   void init_train();
};


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                          AInterfaceToANNScript                            */
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

  /// void create_train(QVector<QVector<float>> &in, QVector<QVector<float>> &out);

public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  AInterfaceToANNScript(EventsDataClass* EventsDataHub);
  ~AInterfaceToANNScript();
public slots: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  void newNetwork();
  void clearTrainingData();
  QVariant getConfig();
  void resetConfigToDefault();
  QString configure(QVariant configObject);
  void addTrainingInput(QVariant arrayOfArrays);
  void addTrainingOutput(QVariant arrayOfArrays);
  QString train();
  QVariant process(QVariant arrayOfArrays);
  QVariant processEvents(int firstEvent, int lastEvent);
};

#endif // AINTERFACETOANNSCRIPT_H
