#include "ainterfacetoannscript.h"
#include "ajsontools.h"
#include "apositionenergyrecords.h"

#include <QJsonArray>
//#include <QJsonObject>
#include <QDebug>

#include <fstream>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                           NeuralNetworksScript                            */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*===========================================================================*/
int NeuralNetworksScript::userCallback(fann_train_data* train, unsigned max_epochs,
unsigned epochs_between_reports, float desired_error, unsigned epochs){
double TrainMSE, TrainFailBit, TestMSE, TestFailBit; QString mess;
 testData(&TrainMSE,&TrainFailBit,&TestMSE,&TestFailBit);

 if (TrainMSE<FTrainMSE && TrainFailBit<FTrainFailBit
  && TestMSE<FTestMSE && TestFailBit<FTestFailBit){ //+++++++++++++++++++++++++
   FTrainMSE=TrainMSE; FTrainFailBit=TrainFailBit;
   FTestMSE=TestMSE; FTestFailBit=TestFailBit;
   FnEpochsStag=0;

      save("best.fann");

 } else { FnEpochsStag++; }
 if (FMaxEpochsStag>0 && FnEpochsStag>FMaxEpochsStag){ //++++++++++++++++++++++
   qDebug() << "Train stopped";
   return -1; // exit
 } else { //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   qDebug() << "Epochs = " << cString(epochs).c_str();
   qDebug() << "Hidden Neurons,Layers = " << cString(nHiddenNeurons()).c_str()
            << ", " << cString(nHiddenLayers()).c_str();
   if (nHiddenLayers()>0){ // In case we have hidden layers ...................
    qDebug() << "\tActivation steepness = " << cString(activationSteepness(nHiddenLayers(),0)).c_str();
    qDebug() << "\tActivation function = " << FANN_ACTIVATIONFUNC_NAMES[activationFunction(nHiddenLayers(),0)];
   } //........................................................................
   qDebug() << "\tTrain Data: MSE = " << cString(TrainMSE).c_str()
            << (isDataTest()?(" ("+cString(TestMSE)+")").c_str():"");
   qDebug() << "\tbit fail = " << cString(TrainFailBit).c_str()
            << (isDataTest()?(" ("+cString(TestFailBit)+")").c_str():"");
   //..........................................................................
   if (FnEpochsStag==0) qDebug () << ">>> Train is improving...";
   else qDebug() << ">>> Train is stagnated!";
 } //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 return cFANNWrapper::userCallback(train,max_epochs,
  epochs_between_reports,desired_error,epochs);
}

/*===========================================================================*/
void NeuralNetworksScript::init_train(){
 FTrainMSE=FTrainFailBit=FTestMSE=FTestFailBit=DBL_MAX;
 FnEpochsStag=0; cFANNWrapper::clear();
}

/*===========================================================================*/
AInterfaceToANNScript::AInterfaceToANNScript(EventsDataClass *EventsDataHub) :
EventsDataHub(EventsDataHub){ resetConfigToDefault(); }

/*===========================================================================*/
AInterfaceToANNScript::~AInterfaceToANNScript(){
  clearTrainingData();
}

/*===========================================================================*/
void AInterfaceToANNScript::clearTrainingData(){
  Input.clear(); Input.squeeze();
  Output.clear(); Output.squeeze();

}

/*===========================================================================*/
//! Reset the training data.
void AInterfaceToANNScript::newNetwork(){
  clearTrainingData();
  qDebug() << "New NN created.";
}

/*===========================================================================*/
//! Config.
QString AInterfaceToANNScript::configure(QVariant configObject){
  qDebug() << configObject.typeName();

  if ( QString(configObject.typeName()) != "QVariantMap"){  // ++++++++++++++++
      abort("ANN script: configure function requirs an object-type argument");
      return "";
  }

  QVariantMap map = configObject.toMap();
  Config = QJsonObject::fromVariantMap(map);
  qDebug() << "User config:" << Config;

  if (!Config.contains("type")){ //++++++++++++++++++++++++++++++++++++++++++++
   abort("FANN Type - ('type' key) has to be defined in ANN config");
   return "";
  }

  if (!Config.contains("trainingInput")){ //+++++++++++++++++++++++++++++++++++
   abort("FANN Type - ('trainingInput' key) has to be defined in ANN config");
   return "";
  }

  if (!Config.contains("trainingOutput")){ //++++++++++++++++++++++++++++++++++
   abort("FANN Type - ('trainingOutput' key) has to be defined in ANN config");
   return "";
  }

  return "Config OK!";
}

/*===========================================================================*/
//! Default config.
void AInterfaceToANNScript::resetConfigToDefault(){
  Config = QJsonObject();

  Config["trainingInput"]="EVENTS"; // can be "DIRECT","EVENTS" (from )
  Config["trainingOutput"]="SCAN_POSITIONS"; // can be "DIRECT", "RECONSTRUCTED_POSITIONS", "SCAN_POSITIONS"
  Config["currentSensorGroup"]=0;
  Config["trainingTestPer"]=0.5;
  //...........................................................................
  Config["type"]="CASCADE";
  Config["trainAlgorithm"]="RETRO PROPAGATION";
  Config["outputActivationFunc"]="SIGMOID";
  Config["errorFunction"]="LINEAR";
  Config["stopFunction"]="BIT";
  //...........................................................................
  Config["bitFailLimit"]=0.1; // no default.
  Config["cascade_outputChangeFraction"]=0.01; // default
  Config["cascade_candidateChangeFraction"]=0.01; // default
  Config["cascade_outputStagnationEpochs"]=12; // default
  Config["cascade_minOutEpochs"]=50; // default
  Config["cascade_maxOutEpochs"]=150; // default
  Config["cascade_candidateStagnationEpochs"]=12; // default
  Config["cascade_minCandEpochs"]=50; // default
  Config["cascade_maxCandEpochs"]=150; // default
  Config["cascade_candidateLimit"]=1000; // default
  Config["cascade_candidateGroups"]=2; // default
  Config["cascade_weightMultiplier"]=0.4; // default
  Config["cascade_setActivationSteepness"]=QJsonArray({ 0.25, 0.5, 0.75, 1. });
  Config["cascade_setActivationFunctions"]=QJsonArray({"SIGMOID","SIGMOID SYMMETRIC",
   "GAUSSIAN","GAUSSIAN SYMMETRIC","ELLIOT","ELLIOT SYMMETRIC"});
}

/*===========================================================================*/
QVariant AInterfaceToANNScript::getConfig(){
  const QVariantMap res = Config.toVariantMap();
  return res;
}

/*===========================================================================*/
//! Adding training Input from script. Note that for each input there most
//! be a correspondent output command in the script.
void AInterfaceToANNScript::addTrainingInput(QVariant arrayOfArrays){
  //unpacking data
  int fixedSize = Input.isEmpty() ? -1 : Input.first().size();
  if ( !convertQVariantToVectorOfVectors(&arrayOfArrays, &Input, fixedSize) ) return;
  qDebug() << "New training input is set to:" << Input;

}

/*===========================================================================*/
//! Adding training from from script. Note that for each output there most
//! be a correspondent input command in the script.
void AInterfaceToANNScript::addTrainingOutput(QVariant arrayOfArrays){
  //unpacking data
  int fixedSize = Output.isEmpty() ? -1 : Output.first().size();
  if ( !convertQVariantToVectorOfVectors(&arrayOfArrays, &Output, fixedSize) ) return;
  qDebug() << "New training output is set to:" << Output;
}

/*===========================================================================*/
//! Set cascade's specific settings.
void AInterfaceToANNScript::setConfig_cascade(){

 // !!!!!!!!!!!!!!!!!!!!!!!!!!! replace EventsDataHub->Events.at(0).size() by a parameter !!!!!!
 afann.createCascade(EventsDataHub->Events.at(0).size(),1);

 //.......................................................................
 afann.cascade_outputChangeFraction(Config["cascade_outputChangeFraction"].toDouble());
 afann.cascade_candidateChangeFraction(Config["cascade_candidateChangeFraction"].toDouble());
 afann.cascade_outputStagnationEpochs(Config["cascade_outputStagnationEpochs"].toInt());
 afann.cascade_minOutEpochs(Config["cascade_minOutEpochs"].toInt());
 afann.cascade_maxOutEpochs(Config["cascade_maxOutEpochs"].toInt());
 afann.cascade_candidateStagnationEpochs(Config["cascade_candidateStagnationEpochs"].toInt());
 afann.cascade_minCandEpochs(Config["cascade_minCandEpochs"].toInt());
 afann.cascade_maxCandEpochs(Config["cascade_maxCandEpochs"].toInt());
 afann.cascade_candidateLimit(Config["cascade_candidateLimit"].toInt());
 afann.cascade_candidateGroups(Config["cascade_candidateGroups"].toInt());
 afann.cascade_weightMultiplier(Config["cascade_weightMultiplier"].toDouble());

 //.......................................................................
 QVector<QVariant> qStp=Config["cascade_setActivationSteepness"].toArray().toVariantList().toVector();
 cFANNWrapper::cActivationSteepness stp(qStp.size());
 for (int s=0; s<qStp.size(); ++s) stp[s]=qStp[s].toDouble();
 afann.cascade_setActivationSteepness(stp);

 //.......................................................................
 cFANNWrapper::cActivationFunctions act=NeuralNetworksModule::activationFunctions
  .QOption(Config["cascade_setActivationFunctions"]);
 afann.cascade_setActivationFunctions(act);
}

/*===========================================================================*/
//! Set common settings.
void AInterfaceToANNScript::setConfig_commom(){
 afann.trainAlgorithm(NeuralNetworksModule::trainAlgorithms
  .option(Config["trainAlgorithm"].toString().toStdString()));
 afann.outputActivationFunc(NeuralNetworksModule::activationFunctions
  .option(Config["outputActivationFunc"].toString().toStdString()));
 afann.errorFunction(NeuralNetworksModule::trainErrorFunctions
  .option(Config["errorFunction"].toString().toStdString()));
 afann.stopFunction(NeuralNetworksModule::trainStopFunctions
  .option(Config["stopFunction"].toString().toStdString()));

 //...........................................................................
 afann.bitFailLimit(Config["bitFailLimit"].toDouble());
}

/*===========================================================================*/
//! Normalize vector 'data' to 'to' (note that the vector contents are changed)
void AInterfaceToANNScript::norm(QVector<float> &data, double to){
float sum=0, norm; for (float &d: data) sum+=d;
 norm=to/sum; for (float &d: data) d*=norm;
}

/*===========================================================================*/
//void AInterfaceToANNScript::create_train(QVector<QVector<float>> &in,
//QVector<QVector<float>> &out){

//}

/*===========================================================================*/
// in the case of InputSource == "EVENTS"
// input vector for event iEvent is EventsDataHub->Events.at(iEvent)   (size is equal to the number of PMTs)
// use only those events which are bool true: EventsDataHub->ReconstructionData.at(CurrentSensorGroup).at(iEvent)->GoodEvent
// total number of events to make the for cyckle is: EventsDataHub->ReconstructionData.at(CurrentSensorGroup).size()

// in the case of OutputSource == "SCAN_POSITIONS"
// the output training vector for event iEvent is EventsDataHub->Scan.at(iEvent)->Points.at(0).r - it is double[3]
// use only those events which are bool true: EventsDataHub->ReconstructionData.at(CurrentSensorGroup).at(iEvent)->GoodEvent
// not a misstype - we use ReconstructionData container for event filters
// total number of events to make the for cyckle is: EventsDataHub->ReconstructionData.at(CurrentSensorGroup).size()

// in the case of OutputSource == "RECONSTRUCTED_POSITIONS"
// the output training vector for event iEvent is EventsDataHub->ReconstructionData.at(CurrentSensorGroup).at(iEvent)->Points.at(0).r - it is double[3]
// use only those events which are bool true: EventsDataHub->ReconstructionData.at(CurrentSensorGroup).at(iEvent)->GoodEvent
// total number of events to make the for cyckle is: EventsDataHub->ReconstructionData.at(CurrentSensorGroup).size()
QString AInterfaceToANNScript::train() { try {

 //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 QString InputSource=Config["trainingInput"].toString();
 QString OutputSource=Config["trainingOutput"].toString();
 int CurrentSensorGroup=Config["currentSensorGroup"].toInt();
 const QVector<AReconRecord*> &rec_=EventsDataHub->ReconstructionData.at(CurrentSensorGroup);
 QVector<QVector<float>> *in_=NULL;
 QVector<float> data;

  // Input data: check consistency of the training data +++++++++++++++++++++++
  int InputDataSize = -1;
  if (InputSource  == "DIRECT"){
      in_=&Input;
      InputDataSize = Input.size();
  } else if (InputSource == "EVENTS"){
      in_=&EventsDataHub->Events;
      InputDataSize = EventsDataHub->countGoodEvents(CurrentSensorGroup);
  } else return "Unknown data source for training input";

  // Output data: check consistency of the training data ++++++++++++++++++++++
  int OutputDataSize = -1;
  if (OutputSource == "DIRECT") {
      OutputDataSize = Output.size();
  } else if (OutputSource == "SCAN_POSITIONS"){
      if (EventsDataHub->Scan.isEmpty()) OutputDataSize = 0;
      else OutputDataSize = EventsDataHub->countGoodEvents(CurrentSensorGroup);
  } else if (OutputSource == "RECONSTRUCTED_POSITIONS"){
      OutputDataSize = EventsDataHub->countGoodEvents(CurrentSensorGroup);
  } else return "Unknown data source for training output";

  //additional watchdogs ++++++++++++++++++++++++++++++++++++++++++++++++++++++
  if (InputDataSize  == 0) return "Training input is empty";
  if (OutputDataSize == 0) return "Training output is empty";
  if (InputDataSize !=  OutputDataSize) return "Training data size mismatch";

  //???????????????????????????????????????????????????????????????????????????

  /// GOOD FOR SCAN, FLOOD, ...
  FILE * outnn;
  outnn = fopen ("test.fann","w");
  fprintf(outnn,"%d %d %d\n",InputDataSize,in_->at(0).size(),1);
  for (int i, evt=0; evt<rec_.size(); ++evt) if (rec_.at(evt)->GoodEvent){
   for (norm(data=in_->at(evt)), i=0; i<data.size(); ++i) fprintf(outnn,"%f ",data[i]);
   fprintf(outnn,"\n%f\n",EventsDataHub->Scan.at(evt)->Points.at(0).r[2]);
  }
   fclose(outnn);


  //???????????????????????????????????????????????????????????????????????????

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  afann.init_train(); // reset treain data, etc
  if (Config["type"].toString()=="CASCADE") setConfig_cascade();
  setConfig_commom();

  //...........................................................................
  afann.loadTrain("test.fann",Config["trainingTestPer"].toDouble());
//   afann.saveDataTrain("train.fann"); // optional, useful for debug
//   afann.saveDataTest("test.fann"); // optional, useful for debug.

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  if (Config["type"].toString()=="CASCADE")
   afann.trainCascade(100,1,0.5); //! >>>>>>>>>>>>> This 2 inputs should be options !!!!!!!!!!!!

  // re-load the best configuration!
  afann.load("best.fann");

  return "OK"; //success

 } catch (Exception &err){ return err.Message().c_str();
 } catch (exception &err){ return err.what();
 } catch (...){ return strerror(errno);
} }

/*===========================================================================*/
QVariant AInterfaceToANNScript::process(QVariant arrayOfArrays)
{
  //unpacking data
  QVector< QVector<float> > Data;
  int fixedSize = Input.isEmpty() ? -1 : Input.first().size();
  if ( !convertQVariantToVectorOfVectors(&arrayOfArrays, &Data, fixedSize) ) return QVariant();
  qDebug() << "received to process:" << Data;

  //.....

  //packing the results to send back to the script
  QVector< QVector<float> > dummy; dummy << QVector<float>(5,0) << QVector<float>(5,1) << QVector<float>(5, 2);
  QVariantList l;
  for (int i=0; i<dummy.size(); i++)
    {
      const QVector<float>& thisOne = dummy.at(i);
      QVariantList ll;
      for (int ii=0; ii<thisOne.size(); ii++) ll << thisOne.at(ii);
      l << QVariant(ll);
    }
  return l;
}

QVariant AInterfaceToANNScript::processEvents(int firstEvent, int lastEvent)
{
  requestPrint("Working..."); //this is waht you asked for printout

  // Use EventsDataHub->Events, without checking for good (all events in the range from firstEvent, until and including lastEvent):


  //packing the results to send back to the script
  QVector< QVector<float> > dummy; dummy << QVector<float>(5,0) << QVector<float>(5,1) << QVector<float>(5, 2);
  QVariantList l;
  for (int i=0; i<dummy.size(); i++)
    {
      const QVector<float>& thisOne = dummy.at(i);
      QVariantList ll;
      for (int ii=0; ii<thisOne.size(); ii++) ll << thisOne.at(ii);
      l << QVariant(ll);
    }
  return l;
}

/*===========================================================================*/
bool AInterfaceToANNScript::convertQVariantToVectorOfVectors(QVariant *var, QVector<QVector<float> > *vec, int fixedSize)
{
  if ( QString(var->typeName()) != "QVariantList")
  {
      abort("ANN script: array (or array of arrays) is required as an argument");
      return false;
  }
  QVariantList tmp = var->toList();
  QJsonArray arr = QJsonArray::fromVariantList(tmp);
  for (int i=0; i<arr.size(); i++)
    {
      QJsonArray el = arr[i].toArray();
      if (  el.isEmpty() || (fixedSize!=-1 && el.size()!=fixedSize)  )
        {
          abort("ANN script: Invalid argument or size mismatch");
          return false;
        }
      QVector<float> tmp;
      for (int ii=0; ii<el.size(); ii++) tmp << el[ii].toDouble();
      vec->append(tmp);
    }
  return true;
}
