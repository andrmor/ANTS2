#include "neuralnetworkswindow.h"
#include "neuralnetworksmodule.h"
#include "eventsdataclass.h"
#include "pms.h"
#include "apmgroupsmanager.h"

/*===========================================================================*/
void cSeparateValues(vector<string>& keys, string& text, string sep, string no){
unsigned S0=0, S=0, i, ni; // values separated by 'sep' (excluding '[no]sep')
 while ((i=text.find(sep,S))!=(unsigned)string::npos){
  if (!no.empty() && (ni=text.rfind(no,i))!=(unsigned)string::npos){
   if (i==no.length()+ni) goto skip; } keys.push_back(text.substr(S0,i-S0));
  S0=i+sep.length(); skip:S=i+sep.length(); // guides.
 } keys.push_back(text.substr(S0,text.length()-S0));
}

/*===========================================================================*/
string cTrimSpaces(string text){
size_t b=text.find_first_not_of(" \t"), e=text.find_last_not_of(" \t");
 return (b==string::npos || e==string::npos)?"":text.substr(b,e-b+1);
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                                   Exception                               */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*===========================================================================*/
// Constructor.
Exception::Exception(string sType, string sOrigin, string sComment){
 FType=sType; FOrigin=sOrigin; FComment=sComment;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Copy Constructor.
Exception::Exception(const Exception &Arg){
 FType=Arg.FType; FOrigin=Arg.FOrigin; FComment=Arg.FComment;
}

/*===========================================================================*/
// Compose error message.
string Exception::Message(){
 return "ERROR: "+FType+" ("+FComment+") @ '"+FOrigin+"'.";
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                                 cMemFile                                  */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/



#ifdef Q_OS_WIN32 //###########################################################

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cMemFile::save(string name){
FILE *file = fopen(name.c_str(),"w"); char c;
 if (file){ fseek(FHandler,0,SEEK_SET); while (true) //........................
  if ((c=fgetc(FHandler))==EOF) break; else fputc(c,file); fclose(file);
 } else throw Exception("Fail to open file","cMemFile::save(string)",name);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cMemFile::load(string name){
FILE *file = fopen(name.c_str(),"r"); char c;
 if (file){ while (true) //....................................................
  if ((c=fgetc(file))==EOF) break; else fputc(c,FHandler); fclose(file);
 } else throw Exception("Fail to open file","cMemFile::load(string)",name);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
string cMemFile::text(){
 fseek(FHandler,0,SEEK_SET); string r; char c;
 while (true) if ((c=fgetc(FHandler))==EOF) break; else r+=c;
 return r; // pointer is again at the right place for writing!
}

#else //#######################################################################

/*===========================================================================*/
int cMemFile::seekdir(ios_base::seekdir dir){
 switch (dir){ // convert c++ -> c types.
  case ios_base::beg: return SEEK_SET;
  case ios_base::cur: return SEEK_CUR;
  case ios_base::end: return SEEK_END;
  default: throw Exception("Invalid Operation",
  "cMemFile::seekdir(ios_base::seekdir)","Unknown option");
} }

/*===========================================================================*/
void cMemFile::close(){
 if (FHandler){ fclose(FHandler); FHandler=NULL; }
 if (FBuffer){ free(FBuffer); FBuffer=NULL; }
 FSize=0;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cMemFile::save(string filename){
FILE *p=fopen(filename.c_str(),"w"); unsigned c; fflush(FHandler);
 if (p!=NULL){ for (c=0; c<FSize; ++c) fputc(FBuffer[c],p); fclose (p);
 } else throw Exception("Fail to write to file","cMemFile::save(string)",filename);
}

#endif //######################################################################

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                                cFANNWrapper                               */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


/*===========================================================================*/
cFANNType::cFANNType():cStaticMapChoices<cFANNType_enum,3>(){
 set(0,nnStandard,"STANDARD");
 set(1,nnShortcut,"SHORTCUT");
 set(2,nnCascade,"CASCADE");
}

/*===========================================================================*/
#define GRP_TRAIN_ALL 1
#define GRP_TRAIN_CASCADE 2
cTrainAlgorithm::cTrainAlgorithm():cStaticMapChoices<fann_train_enum,4>(){
 set(0,FANN_TRAIN_BATCH,"BATCH",1,GRP_TRAIN_ALL);
 set(1,FANN_TRAIN_INCREMENTAL,"INCREMENTAL",1,GRP_TRAIN_ALL);
 set(2,FANN_TRAIN_RPROP,"RETRO PROPAGATION",2,GRP_TRAIN_ALL,GRP_TRAIN_CASCADE);
 set(3,FANN_TRAIN_QUICKPROP,"QUICK PROPAGATION",2,GRP_TRAIN_ALL,GRP_TRAIN_CASCADE);
}

/*===========================================================================*/
cTrainStopFunction::cTrainStopFunction():cStaticMapChoices<fann_stopfunc_enum,2>(){
 set(0,FANN_STOPFUNC_BIT,"BIT");
 set(1,FANN_STOPFUNC_MSE,"MSE");
}

/*===========================================================================*/
cTrainErrorFunction::cTrainErrorFunction():cStaticMapChoices<fann_errorfunc_enum,2>(){
 set(0,FANN_ERRORFUNC_TANH,"TANH");
 set(1,FANN_ERRORFUNC_LINEAR,"LINEAR");
}

/*===========================================================================*/
cActivationFunction::cActivationFunction():cStaticMapChoices<fann_activationfunc_enum,16>(){
 set(0,FANN_SIN,"SIN");
 set(1,FANN_COS,"COS");
 set(2,FANN_LINEAR,"LINEAR");
 set(3,FANN_ELLIOT,"ELLIOT");
 set(4,FANN_SIGMOID,"SIGMOID");
 set(5,FANN_GAUSSIAN,"GAUSSIAN");
 set(6,FANN_THRESHOLD,"THRESHOLD");
 set(7,FANN_LINEAR_PIECE,"LINEAR PIECE");
 set(8,FANN_SIN_SYMMETRIC,"SIN SYMMETRIC");
 set(9,FANN_COS_SYMMETRIC,"COS SYMMETRIC");
 set(10,FANN_SIGMOID_STEPWISE,"SIGMOID STEPWISE");
 set(11,FANN_ELLIOT_SYMMETRIC,"ELLIOT SYMMETRIC");
 set(12,FANN_SIGMOID_SYMMETRIC,"SIGMOID SYMMETRIC");
 set(13,FANN_GAUSSIAN_SYMMETRIC,"GAUSSIAN SYMMETRIC");
 set(14,FANN_THRESHOLD_SYMMETRIC,"THRESHOLD SYMMETRIC");
 set(15,FANN_LINEAR_PIECE_SYMMETRIC,"LINEAR PIECE SYMMETRIC");
}

/*===========================================================================*/
cDataSource::cDataSource():cStaticMapChoices<cFANNDataSource,2>(){
 set(0,dsSimulation,"Sim Data");
 set(1,dsReconstruction,"Rec Data");
}

/*===========================================================================*/
int FANN_API cFANNWrapper::FUserCallback( // Wrapper.
fann *a, fann_train_data *b, unsigned c, unsigned d, float e, unsigned f){
 return ((cFANNWrapper*)fann_get_user_data(a))->userCallback(b,c,d,e,f); }

/*===========================================================================*/
void cFANNWrapper::train(fann_train_data **data, string file){
 if (*data){ fann_destroy_train(*data); *data=NULL; }
 *data=fann_read_train_from_file(file.c_str());
 if (fann_num_input_train_data(*data)!=nInputs()) throw Exception("Invalid Operation",
  "cFANNWrapper::train(fann_train_data **,string)","Wrong number of inputs");
 if (fann_num_output_train_data(*data)!=nOutputs()) throw Exception("Invalid Operation",
  "cFANNWrapper::train(fann_train_data **,string)","Wrong number of outputs");
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::merge(fann_train_data **data, string file){
fann_train_data *tmp=NULL; train(&tmp,file);
 if (*data){ // deletes the original training data.
  fann_train_data *res=fann_merge_train_data(*data,tmp);
  fann_destroy_train(*data); fann_destroy_train(tmp); *data=res;
 } else *data=tmp;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// 'getBitFail' and 'getMSE' relates to the last operation on train/test data.
// --BE CAREFULL-- with the value --MSE/BIT FAIL-- value trasnposed to the FANN
// code if testing the test data on the callback function.
int cFANNWrapper::userCallback(fann_train_data*, unsigned,
unsigned, float, unsigned){ return FForceStop?-1:0; } // not thread safe!

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// 'bit_fail' and 'MSE' are calculated during training or testing, and can therefore
// sometimes be a bit off if the weights have been changed since the last calculation
// of the value. Use 'testDataTest' and 'testDataTrain' to update 'bit_fail' and 'MSE'
// !!!!!!!!! BUT always 'resetMSE' before returning on 'userCallback' !!!!!!!!!
void cFANNWrapper::testData(double *trainMSE,
double *trainFailBits, double *testMSE, double *testFailBits){
 if (FDataTrain){ *trainMSE=testDataTrain(); *trainFailBits=bitFail(); }
 if (FDataTest){ *testMSE=testDataTest(); *testFailBits=bitFail(); }
 resetMSE(); // important before returning to the training loop.
}

/*===========================================================================*/
void cFANNWrapper::clear(){
 if (FDataTrain){ fann_destroy_train(FDataTrain); FDataTrain=NULL; }
 if (FDataTest){ fann_destroy_train(FDataTest); FDataTest=NULL; }
 if (FANN){ fann_destroy(FANN); FANN=NULL; }
}

/*===========================================================================*/
cFANNWrapper::cFANNWrapper():
FANN(NULL),FDataTrain(NULL),FDataTest(NULL){ }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
cFANNWrapper::cFANNWrapper(cFANNWrapper &):
FANN(NULL),FDataTrain(NULL),FDataTest(NULL){
 /* FANN=fann_copy(O.FANN); V2.2.0 */
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
cFANNWrapper::~cFANNWrapper(){ clear(); }

/*===========================================================================*/
void cFANNWrapper::createStandard(unsigned nLayers,...){
va_list vList; va_start(vList,nLayers); cLayers layers(nLayers);
 for (unsigned i=0; i<nLayers; ++i) layers[i]=va_arg(vList,unsigned);
 va_end(vList); createStandard(layers);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::createStandard(cLayers &layers){
 clear(); // Force cleanup of all structrures.
 FANN=fann_create_standard_array(layers.size(),layers.data());
 fann_set_callback(FANN,(fann_callback_type)&FUserCallback);
 fann_set_user_data(FANN,this);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::createShortcut(unsigned nLayers,...){
va_list vList; va_start(vList,nLayers); cLayers layers(nLayers);
 for (unsigned i=0; i<nLayers; ++i) layers[i]=va_arg(vList,unsigned);
 va_end(vList); createShortcut(layers);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::createShortcut(cLayers &layers){
 clear(); // Force cleanup of all structrures.
 FANN=fann_create_shortcut_array(layers.size(),layers.data());
 fann_set_callback(FANN,(fann_callback_type)&FUserCallback);
 fann_set_user_data(FANN,this);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::createSparse(cLayers &layers, double rate){
 clear(); // Force cleanup of all structrures.
 FANN=fann_create_sparse_array(rate,layers.size(),layers.data());
 fann_set_callback(FANN,(fann_callback_type)&FUserCallback);
 fann_set_user_data(FANN,this);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::createSparse(unsigned nLayers, double rate,...){
va_list vList; va_start(vList,rate); cLayers layers(nLayers);
 for (unsigned i=0; i<nLayers; ++i) layers[i]=va_arg(vList,unsigned);
 va_end(vList); createSparse(layers,rate);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::createCascade(unsigned nInputs, unsigned nOutputs){
 clear(); // Force cleanup of all structrures.
 createShortcut(2,nInputs,nOutputs);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Check success using '::isFANN'.
void cFANNWrapper::load(string file){ 
 clear(); // Force cleanup of all structrures.
 FANN=fann_create_from_file(file.c_str());
 fann_set_callback(FANN,(fann_callback_type)&FUserCallback);
 fann_set_user_data(FANN,this);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Check success using '::isFANN'.
void cFANNWrapper::load(cMemFile &file){
 clear(); // Force cleanup of all structrures.
#ifdef Q_OS_WIN32 //###########################################################
//! WINDOWS - THIS CODE IS NOT SAFE WHEN RUNNING MULTIPLE INSTANTS OF ANTS.
string file_name="fann_file_load.tmp";
 file.save(file_name);
 FANN=fann_create_from_file(file_name.c_str());
 DeleteFileA(file_name.c_str());
#else //#######################################################################
 FANN=fann_create_from_fd(file.handler(),0);
 fann_set_callback(FANN,(fann_callback_type)&FUserCallback);
 fann_set_user_data(FANN,this);
#endif //######################################################################
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::save(string file){
 fann_save(FANN,file.c_str());
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::save(cMemFile &file){
#ifdef Q_OS_WIN32 //###########################################################
//! WINDOWS - THIS CODE IS NOT SAFE WHEN RUNNING MULTIPLE INSTANTS OF ANTS.
string file_name="fann_file_save.tmp";
 fann_save(FANN,file_name.c_str());
 file.load(file_name);
 DeleteFileA(file_name.c_str());
#else //#######################################################################
 fann_save_internal_fd(FANN,file.handler(),0,0);
#endif //######################################################################
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// check success using '::isDataTrain'.
void cFANNWrapper::loadTrainData(string trainFile){
 if (FDataTest){ fann_destroy_train(FDataTest); FDataTest=NULL; }
 train(&FDataTrain,trainFile);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// check success using '::isDataTrain' and '::isDataTest'.
void cFANNWrapper::loadTrainData(string trainFile, string testFile){
 train(&FDataTrain,trainFile); train(&FDataTest,testFile);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::loadTrain(string file, double testPercentage){
fann_train_data *data=NULL; train(&data,file); // load & check.
unsigned total=fann_length_train_data(data), pos=total*testPercentage;
 if (FDataTest){ fann_destroy_train(FDataTest); FDataTest=NULL; }
 if (FDataTrain){ fann_destroy_train(FDataTrain); FDataTrain=NULL; }
 fann_shuffle_train_data(data); // avoid ordered training data.
 FDataTrain=fann_subset_train_data(data,pos,total-pos);
 if (pos>0) FDataTest=fann_subset_train_data(data,0,pos);
 fann_destroy_train(data);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::scaleInputs(double min, double max){
 fann_scale_input_train_data(FDataTest,min,max);
 fann_scale_input_train_data(FDataTrain,min,max);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::scaleOutputs(double min, double max){
 fann_scale_output_train_data(FDataTest,min,max);
 fann_scale_output_train_data(FDataTrain,min,max);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::scale(double min, double max){
 fann_scale_train_data(FDataTest,min,max);
 fann_scale_train_data(FDataTrain,min,max);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::scale(){
 fann_scale_train(FANN,FDataTest);
 fann_descale_train(FANN,FDataTrain);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Continues the tranning of the FANN (see: 'save'/'load').
void cFANNWrapper::train(unsigned max_epochs,
unsigned epochs_per_reports, float error){ FForceStop=false;
 fann_train_on_data(FANN,FDataTrain,max_epochs,epochs_per_reports,error);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// Continues the tranning of the FANN (see: 'save'/'load').
void cFANNWrapper::trainCascade(unsigned max_neurons,
unsigned neurons_between_reports, float error){ FForceStop=false;
 fann_cascadetrain_on_data(FANN,FDataTrain,max_neurons,neurons_between_reports,error);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! resume training on a previous trained network (which can be loaded from file)
//! Given that the NN types are compatible, we can use different types of training.
//! For instance, one 'resumeTrain' on a previous 'trainCascade' trained NN.
void cFANNWrapper::resumeTrain(unsigned max_epochs, unsigned epochs_per_reports,
float error){ train(max_epochs,epochs_per_reports,error); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! resume training on a previous trained network (which can be loaded from file)
//! Given that the NN types are compatible, we can use different types of training.
//! For instance, one 'resumeTrainCascade' on a previous 'train' trained NN.
void cFANNWrapper::resumeTrainCascade(unsigned max_neurons, unsigned neurons_between_reports,
float error){ trainCascade(max_neurons,neurons_between_reports,error); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::cascade_getActivationFunctions(cActivationFunctions &f){
 f.resize(cascade_nActivationFunctions());
 memmove(f.data(),fann_get_cascade_activation_functions(FANN),
  f.size()*sizeof(fann_activationfunc_enum));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::cascade_setActivationFunctions(cActivationFunctions &f){
 fann_set_cascade_activation_functions(FANN,f.data(),f.size());
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::cascade_getActivationSteepness(cActivationSteepness &f){
 f.resize(cascade_nActivationSteepness());
 memmove(f.data(),fann_get_cascade_activation_steepnesses(FANN),
  f.size()*sizeof(fann_type));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::cascade_setActivationSteepness(cActivationSteepness &f){
 fann_set_cascade_activation_steepnesses(FANN,f.data(),f.size());
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::run(cLayer &in, cLayer &out){
 out.resize(nOutputs()); memmove(out.data(),run(in),nOutputs()*sizeof(fann_type));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
fann_type* cFANNWrapper::run(cLayer &in){
 if (in.size()==nInputs()) return fann_run(FANN,in.data()); else
  throw Exception("Invalid Operation","cFANNWrapper::run(cLayer&)",
   "Wrong number of inputs");
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::bias(cLayers &bias_){
 bias_.resize(nLayers()); fann_get_bias_array(FANN,bias_.data());
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::layers(cLayers &layers_){
 layers_.resize(nLayers()); fann_get_layer_array(FANN,layers_.data());
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::connections(cConnections &connections_){
 connections_.resize(nTotalConnections());
 fann_get_connection_array(FANN,connections_.data());
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void cFANNWrapper::setWeights(cConnections &connections_){
 if (connections_.size()>nTotalConnections()) throw Exception("Invalid Operation",
  "cFANNWrapper::weights(cConnections&)","The array of weights is too long");
 else fann_set_weight_array(FANN,connections_.data(),connections_.size());
}


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                           NeuralNetworksModule                            */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

cFANNType NeuralNetworksModule::FANNTypes;
cTrainAlgorithm NeuralNetworksModule::trainAlgorithms;
cTrainStopFunction NeuralNetworksModule::trainStopFunctions;
cTrainErrorFunction NeuralNetworksModule::trainErrorFunctions;
cActivationFunction NeuralNetworksModule::activationFunctions;
cDataSource NeuralNetworksModule::dataSource;

/*===========================================================================*/
void NeuralNetworksModule::init_NNModule(string *trainOutput){
 cFANNWrapper::clear(); FnEpochsStag=0;
 FTrainMSE=FTrainFailBit=FTestMSE=FTestFailBit=DBL_MAX;
 for (unsigned i=0; i<3; ++i){ FmScale[i]=1; FbScale[i]=0; }
 FTrainOutput=trainOutput;
}

/*===========================================================================*/
void NeuralNetworksModule::cont_NNModule(){
 FnEpochsStag=0; FTrainMSE=FTrainFailBit=FTestMSE=FTestFailBit=DBL_MAX;
}

/*===========================================================================*/
void NeuralNetworksModule::createCascade(unsigned nInputs, unsigned nOutputs){
 cFANNWrapper::createCascade(nInputs,nOutputs); // parent.
 FVIn.resize(nInputs); FVOut.resize(nOutputs);
}

/*===========================================================================*/
void NeuralNetworksModule::createStandard(cLayers &layers){
 cFANNWrapper::createStandard(layers); // parent.
 FVIn.resize(layers.front()); FVOut.resize(layers.back());
}

/*===========================================================================*/
void NeuralNetworksModule::createShortcut(cLayers &layers){
 cFANNWrapper::createShortcut(layers); // parent.
 FVIn.resize(layers.front()); FVOut.resize(layers.back());
}

/*===========================================================================*/
void NeuralNetworksModule::load(cMemFile &file){
 cFANNWrapper::load(file); // parent.
 FVIn.resize(nInputs()); FVOut.resize(nOutputs());
}

/*===========================================================================*/
void NeuralNetworksModule::load(string file){
 cFANNWrapper::load(file); // parent.
 FVIn.resize(nInputs()); FVOut.resize(nOutputs());
}

/*===========================================================================*/
int NeuralNetworksModule::userCallback(fann_train_data* train, unsigned max_epochs,
unsigned epochs_between_reports, float desired_error, unsigned epochs){
double TrainMSE, TrainFailBit, TestMSE, TestFailBit; QString mess;
 testData(&TrainMSE,&TrainFailBit,&TestMSE,&TestFailBit);
 if (TrainMSE<FTrainMSE && TrainFailBit<FTrainFailBit
  && TestMSE<FTestMSE && TestFailBit<FTestFailBit){
  FTrainMSE=TrainMSE; FTrainFailBit=TrainFailBit;
  FTestMSE=TestMSE; FTestFailBit=TestFailBit;
  FnEpochsStag=0; save(*FTrainOutput);
 } else { FnEpochsStag++; }
 if (FMaxEpochsStag>0 && FnEpochsStag>FMaxEpochsStag){ //++++++++++++++++++++++
  (mess+="<B>Train stopped<font color=\"Red\">");
   emit status(mess+="</font></B><BR>"); return -1;
 } else { //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 (mess+="<B>Epochs = <font color=\"Red\">")+=cString(epochs).c_str();
 (mess+="</font></B><BR><B>Hidden Neurons,Layers = </B>")+=cString(nHiddenNeurons()).c_str();
 (mess+=" ,")+=cString(nHiddenLayers()).c_str();
 if (nHiddenLayers()>0){ // In case we have hidden layers.
  (mess+="<BR><B>Activation steepness = </B>")+=cString(activationSteepness(nHiddenLayers(),0)).c_str();
  (mess+="<BR><B>Activation function = </B>")+=FANN_ACTIVATIONFUNC_NAMES[activationFunction(nHiddenLayers(),0)];
 } //..........................................................................
 (mess+="<BR><B>Train Data: MSE = <font color=\"Red\">")+=cString(TrainMSE).c_str();
 if (isDataTest()) (mess+=" / ")+=cString(TestMSE).c_str();
 (mess+="</font></B><BR><B>bit fail = <font color=\"Red\">")+=cString(TrainFailBit).c_str();
 if (isDataTest()) (mess+=" / ")+=cString(TestFailBit).c_str();
 //............................................................................
 if (FnEpochsStag==0) mess+="<BR><B><font color=\"Green\">Train is improving...";
 else { double c=double(FnEpochsStag)/double(FMaxEpochsStag); string clr;
  if (c>0.75) clr="Red"; else if (c>0.5) clr="Orange";
  else if (c>0.25) clr="Yellow"; else clr="Black";
  ((mess+="<BR><B><font color=\"")+=clr.c_str())+="\">Train is stagnated!";
 } //..........................................................................
 emit status(mess+="</font></B><BR>");
 return cFANNWrapper::userCallback(train,max_epochs,
  epochs_between_reports,desired_error,epochs);
} }

/*===========================================================================*/
NeuralNetworksModule::NeuralNetworksModule(pms* Pms, APmGroupsManager *PMgroups, EventsDataClass* eventsDataHub, QObject *parent)
:QObject(parent),PMs(Pms),PMgroups(PMgroups), FNorm(nsToSum),FTrainOutput(NULL),FEnergyDims(-1),
FSpaceDims(-1),FMaxEpochsStag(1),FnEpochsStag(0), RecSet(NULL), EventsDataHub(eventsDataHub) { } //ANDR

/*===========================================================================*/
bool NeuralNetworksModule::checkPMsDimension(){
 return getPMsDimension()==nInputs();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
unsigned NeuralNetworksModule::getPMsDimension(){
 return PMs->count() - PMgroups->countStaticPassives();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool NeuralNetworksModule::isValidPM(unsigned pm){
 return !PMgroups->isStaticPassive(pm);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool NeuralNetworksModule::checkRecDimension(){
 return getRecDimension()==nOutputs();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! check the connections at neuralNetworkWindow -> MainWindowInits(.cpp):
unsigned NeuralNetworksModule::getRecDimension(){
 return FSpaceDims+FEnergyDims; // total number of output dims
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! check the connections at neuralNetworkWindow -> MainWindowInits(.cpp):
void NeuralNetworksModule::setRecDimension(bool isRecZ, bool isRecE){
 FSpaceDims=(isRecZ?3:2); FEnergyDims=(isRecE?1:0);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksModule::setRecScaling(unsigned XYZ, double m, double b){
 FmScale[XYZ]=m; FbScale[XYZ]=b;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//reconstruct event# ievent, time bin itime (will be passed -1 if time is
// not resolved or time-itegrated data to be used) and returns reconstructed
// coordinates r and event energy - the pointers are to already existent
// variables bool = true if success.
bool NeuralNetworksModule::ReconstructEvent(int ievent, int, double (*r)[3],
double *energy){ int nIn=nInputs(), iX, iP; double coleff=1, norm=1, val;
 // GET LIGHT COLLECTION EFFICIENCY (INPUT NEURONS) +++++++++++++++++++++++++
 if (isRecE() || FNorm==nsToSum){
  for (coleff=0, iP=0; iP<nIn; ++iP){ if (isValidPM(iP))
   //coleff+=(*Reconstructor->getEvent(ievent))[iP]; } }  ANDR
   coleff+=(*EventsDataHub->getEvent(ievent))[iP]; } }   // ANDR
 // NORMALIZE FACTORS (INPUT NEURONS) +++++++++++++++++++++++++++++++++++++++++
 if (FNorm==nsToMax){
  for (norm=-DBL_MAX, iP=0; iP<nIn; ++iP) if (isValidPM(iP)){
   if ((val=(*EventsDataHub->getEvent(ievent))[iP])>norm) norm=val; }
 } else if (FNorm==nsToSum){ norm=coleff; }
 //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 if (norm<=0){ return false; } else
  for (iP=0; iP<nIn; ++iP) if (isValidPM(iP)){
   FVIn[iP]=(*EventsDataHub->getEvent(ievent))[iP]/norm; }
 //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 for (run(FVIn,FVOut), iX=0; iX<spaceDims(); iX++) (*r)[iX]=scale_out(iX,FVOut[iX]);
 //if (!isRecZ()) (*r)[2]=Reconstructor->getSuggestedZ();
 if (!isRecZ()) (*r)[2]=RecSet->SuggestedZ;
 //*energy=isRecE()?FVOut.back()*coleff:Reconstructor->getSuggestedEnergy();
 *energy=isRecE()?FVOut.back()*coleff : RecSet->SuggestedEnergy;
 return true;
}
