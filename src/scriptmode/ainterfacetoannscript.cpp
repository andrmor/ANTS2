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
//! TODO: exit (-1) when stopped is true (from AInterfaceToANNScript::ForceStop)
int NeuralNetworksScript::userCallback(fann_train_data* train, unsigned max_epochs,
unsigned epochs_between_reports, float desired_error, unsigned epochs){
double TrainMSE, TrainFailBit, TestMSE, TestFailBit; string mess, rMess;
 qApp->processEvents();
 if (FStop) return -1; // user stop it, nothing to do!
 //............................................................................
 testData(&TrainMSE,&TrainFailBit,&TestMSE,&TestFailBit);
 if (TrainMSE<FTrainMSE && TrainFailBit<FTrainFailBit
  && TestMSE<FTestMSE && TestFailBit<FTestFailBit){ //+++++++++++++++++++++++++
   FTrainMSE=TrainMSE; FTrainFailBit=TrainFailBit;
   FTestMSE=TestMSE; FTestFailBit=TestFailBit;
   // Save best config up do now and reset stagnation flag.
   save(FOutFile); FnEpochsStag=0;
 } else { FnEpochsStag++; }
 if (FMaxEpochsStag>0 && FnEpochsStag>FMaxEpochsStag){ //++++++++++++++++++++++
   FParent->requestPrint("Train stopped");
   qDebug() << "Train stopped";
   return -1; // exit
 } else { //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   mess+=("Epochs = "+cString(epochs)+"\n");
   mess+=("Hidden Neurons,Layers = "+cString(nHiddenNeurons())+", "+cString(nHiddenLayers())+"\n");
   if (nHiddenLayers()>0){ // In case we have hidden layers ...................
    mess+=("\tActivation steepness = "+cString(activationSteepness(nHiddenLayers(),0))+"\n");
    mess+=("\tActivation function = "+string(FANN_ACTIVATIONFUNC_NAMES[activationFunction(nHiddenLayers(),0)])+"\n");
   } //........................................................................
   rMess+=("\tTrain Data: MSE = "+cString(TrainMSE)+(isDataTest()?(" ("+cString(TestMSE)+")"):"")+"\n");
   rMess+=("\tbit fail = "+cString(TrainFailBit)+(isDataTest()?(" ("+cString(TestFailBit)+")"):"")+"\n");
   //..........................................................................
   if (FnEpochsStag==0) rMess+=">>> Train is improving...\n";
   else rMess+=">>> Train is stagnated!\n";
   mess+=rMess; qDebug() << mess.c_str();
   FParent->requestPrint(rMess.c_str());
   qApp->processEvents();
 } //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 return cFANNWrapper::userCallback(train,max_epochs,
  epochs_between_reports,desired_error,epochs);
}

/*===========================================================================*/
NeuralNetworksScript::NeuralNetworksScript(AInterfaceToANNScript *parent)
:FMaxEpochsStag(20),FParent(parent){ }

/*===========================================================================*/
//! Must be called before starting trainning.
void NeuralNetworksScript::init_train(string outFile){
 FTrainMSE=FTrainFailBit=FTestMSE=FTestFailBit=DBL_MAX;
 FnEpochsStag=0; FStop=false; FOutFile=outFile;
 cFANNWrapper::clear();
}

/*===========================================================================*/
//! Interface between ANTS QVector and FANNWrapper std vector.
void NeuralNetworksScript::run(QVector<float> &in, QVector<float> &out){
cLayer std_in=in.toStdVector(), std_out;
 cFANNWrapper::run(std_in,std_out);
 out=QVector<float>::fromStdVector(std_out);
}

/*===========================================================================*/
AInterfaceToANNScript::AInterfaceToANNScript(EventsDataClass *EventsDataHub) :
EventsDataHub(EventsDataHub),afann(this){ resetConfigToDefault(); }

/*===========================================================================*/
AInterfaceToANNScript::~AInterfaceToANNScript(){
  clearTrainingData();
}

/*===========================================================================*/
void AInterfaceToANNScript::ForceStop(){
 requestPrint("Waiting for NN to stop ... ");
 afann.stop_Train();
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
  //...........................................................................
  Config["trainingInput"]="EVENTS"; // can be "DIRECT","EVENTS" (from )
  Config["trainingOutput"]="SCAN_POSITIONS"; // can be "DIRECT", "RECONSTRUCTED_POSITIONS", "SCAN_POSITIONS"
  Config["currentSensorGroup"]=0;
  Config["trainingTestPer"]=0.5;
  //...........................................................................
  Config["type"]="CASCADE";
  Config["trainAlgorithm"]="RETRO PROPAGATION";
  Config["outputActivationFunc"]="LINEAR";
  Config["errorFunction"]="LINEAR";
  Config["stopFunction"]="BIT";
  Config["bitFailLimit"]=0.1; // no default.
  //...........................................................................
  Config["cascade_maxNeurons"]=100;
  Config["cascade_trainError"]=1.E-5;
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
  Config["cascade_setActivationSteepness"]=QJsonArray({ 0.2, 0.4, 0.6, 0.8, 1. });
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
//! TODO: replace EventsDataHub->Events.at(0).size() by a parameter !!!!!!
void AInterfaceToANNScript::setConfig_cascade(){
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
QString AInterfaceToANNScript::load(QString FANN_File){
 afann.load(FANN_File.toStdString());
 if (afann.isFANN()) return "Ok"; else {
  abort("Fail to load File: "+FANN_File);
  return "Failed";
} }

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
QString AInterfaceToANNScript::train(QString FANN_File) { try {
 //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 QString InputSource=Config["trainingInput"].toString();
 QString OutputSource=Config["trainingOutput"].toString();
 int CurrentSensorGroup=Config["currentSensorGroup"].toInt();
 const QVector<AReconRecord*> &rec_=EventsDataHub->ReconstructionData.at(CurrentSensorGroup);
 QVector<QVector<float>> *in_=NULL;
 QVector<float> data;

 // Input data: check consistency of the training data ++++++++++++++++++++++++
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
  //???????????????????????????????????????????????????????????????????????????

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  afann.init_train(FANN_File.toStdString()); // reset treain data, etc
  if (Config["type"].toString()=="CASCADE") setConfig_cascade();
  setConfig_commom();

  //...........................................................................
  afann.loadTrain("test.fann",Config["trainingTestPer"].toDouble());
//   afann.saveDataTrain("train.fann"); // optional, useful for debug
//   afann.saveDataTest("test.fann"); // optional, useful for debug.

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  if (Config["type"].toString()=="CASCADE") afann.trainCascade(
   Config["cascade_maxNeurons"].toInt(),1,Config["cascade_trainError"].toDouble());

  afann.load(FANN_File.toStdString()); // re-load the best configuration!
  return "OK"; //success

 } catch (Exception &err){ return err.Message().c_str();
 } catch (exception &err){ return err.what();
 } catch (...){ return strerror(errno);
} }

/*===========================================================================*/
QVariant AInterfaceToANNScript::process(QVariant arrayOfArrays) {

  if (!afann.isFANN()){ // need to load a valid FANN structure ++++++++++++++++
   abort("No valid FANN is available!");
   return QVariantList();
  }

  //unpacking data ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  QVector< QVector<float> > Data;
  int fixedSize = Input.isEmpty() ? -1 : Input.first().size();
  if ( !convertQVariantToVectorOfVectors(&arrayOfArrays, &Data, fixedSize) ) return QVariant();
  qDebug() << "received to process: " << Data.size() << " events";

  if (Data.size()==0) return QVariantList(); // nothing to do.
  else if ((int)afann.nInputs()!=Data[0].size()){ //+++++++++++++++++++++++++++
   abort("Loaded FANN and input data have different dimensions!");
   return QVariantList();
  }

  QVector<QVector<float>> res(Data.size()); // Process ++++++++++++++++++++++++
  for (int d=0; d<Data.size(); ++d){
    norm(Data[d]); afann.run(Data[d],res[d]);
  }

  QVariantList l; // Pack the result back  ++++++++++++++++++++++++++++++++++++
  for (int i=0; i<res.size(); i++){
   const QVector<float>& thisOne = res.at(i); QVariantList ll;
   for (int ii=0; ii<thisOne.size(); ii++) ll << thisOne.at(ii);
   l << QVariant(ll);
  }
  return l;
}

/*===========================================================================*/
//! all events in the range from firstEvent, until and including lastEvent
QVariant AInterfaceToANNScript::processEvents(int firstEvent, int lastEvent){

  if (!afann.isFANN()){ // need to load a valid FANN structure ++++++++++++++++
   abort("No valid FANN is available!");
   return QVariantList();
  }

  if (lastEvent<firstEvent) return QVariantList(); // nothing to do.
  else if ((int)afann.nInputs()!=EventsDataHub->Events.at(0).size()){ //+++++++
   abort("Loaded FANN and input data have different dimensions!");
   return QVariantList();
  }

  QVector<QVector<float>> res(lastEvent-firstEvent+1); // Process +++++++++++++
  for (int d=firstEvent; d<=lastEvent; ++d){
    QVector<float> Data=EventsDataHub->Events.at(d);
    norm(Data); afann.run(Data,res[d-firstEvent]);
  }

  QVariantList l; // packing the results to send back to the script +++++++++++
  for (int i=0; i<res.size(); i++){
   const QVector<float>& thisOne = res.at(i); QVariantList ll;
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
