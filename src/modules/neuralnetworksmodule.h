#ifndef NEURALNETWORKSMODULE_H
#define NEURALNETWORKSMODULE_H

#include "reconstructionsettings.h"

#include <QObject>
#include <QComboBox>
#include <typeinfo>
#include <vector>
#include <iostream>
#include <sstream>

#include "fann.h"
#include <stdio.h>
#include <cstdio>

class pms;
class APmGroupsManager;
class reconstructor;
class EventsDataClass;

using namespace std;

void cSeparateValues(vector<string>&, string&, string, string);
string cTrimSpaces(string text);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                                   Exception                               */
/*         Francisco Neves @ 2006.06.11 ( Last modified 2008.09.01 )         */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
class Exception{
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    string FType, FOrigin, FComment;
public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    Exception(string sType, string sOrigin, string sComment);
    template <class T> Exception(string sType, T& Obj, string f, string sComment);
    Exception(const Exception &Arg);
    inline string Comment(){ return FComment; }
    inline string Origin(){ return FOrigin; }
    inline string Type(){ return FType; }
    string Message();
    //.........................................................................
    friend inline ostream& operator<<(ostream& os, Exception& Arg){
      os << Arg.Message(); return os; }
};

/*===========================================================================*/
// If T is a template then the specialization will be printed.
template <class T> Exception::Exception(string sType, T& Obj,
string f, string sComment){ string C(typeid(Obj).name());
 FType=sType; FOrigin=C+"::"+f; FComment=sComment;
}

/*===========================================================================*/
// Writes to 'std::string' through 'std::ostringstream'.
template <class Type> string cString(Type Data){
ostringstream out; out << Data; return out.str();
}

/*=============================================================================*/
template <class Type> Type cValue(string text){
Type value; istringstream in(text); in >> value;
 if (in.fail()) throw Exception("Invalid Conversion","cValue(string)",
  "fail to read from string"); else return value;
}

/*===========================================================================*/
template <class Type> inline Type cAbs(Type arg){
 if (arg<0) return -arg; else return arg;
}

/*===========================================================================*/
template <class Type> void cConvertValues(vector<string> &i, vector<Type> &o){
 o.resize(i.size()); for (unsigned k=0; k<i.size(); ++k) o[k]=cValue<Type>(i[k]);
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                                 cMemFile                                  */
/*       Francisco Neves @ 2014.09.24 ( Last modified 2014.11.17 )           */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


#ifdef Q_OS_WIN32 //###########################################################
class cMemFile{
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    FILE *FHandler;
protected: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    cMemFile():FHandler(NULL){ ; }
    ~cMemFile(){ if (FHandler) fclose(FHandler); }
    //.........................................................................
    void close(){ if (FHandler){ fclose(FHandler); FHandler=NULL; } }
    void open(){ close(); FHandler=tmpfile(); }
    inline void flush(){ fflush(FHandler); }
    //.........................................................................
    inline void text(string text){ fwrite(text.data(),1,text.size(),FHandler); }
    inline FILE* handler(){ return FHandler; }
    void save(string name);
    void load(string name);
    string text();
};
#else //#######################################################################
class cMemFile{
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    size_t FSize;
    char *FBuffer;
    FILE *FHandler;
protected: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    int seekdir(ios_base::seekdir dir);
public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    cMemFile():FSize(0),FBuffer(NULL),FHandler(NULL){ ; }
    ~cMemFile(){ if (FHandler) fclose(FHandler); if (FBuffer) free(FBuffer); }
    //.........................................................................
    void close(); /*! closes and reset all data structures. */
    void save(string filename); /*! Saves the buffer to disk. */
    void open(){ close(); FHandler=open_memstream(&FBuffer,&FSize); }
    inline void flush(){ fflush(FHandler); }
    //.........................................................................
    inline void text(string text){ fwrite(text.data(),1,text.size(),FHandler); }
    inline string text(){ return string(FBuffer,FSize); } /*! see flush(). */
    inline char* buffer(){ return FBuffer; } /*! see flush(). */
    inline size_t size(){ return FSize; } /*! see flush(). */
    inline FILE* handler(){ return FHandler; }
    inline bool is_open(){ return FHandler; }
    inline bool is_eof(){ return feof(FHandler); }
    inline void rewind(){ fflush(FHandler); }
};

#endif //######################################################################


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                           cStaticMapChoices                               */
/*         Francisco Neves @ 2014.08.21 ( Last modified 2014.08.24 )         */
/*        Cross check version with mathetic library (Francisco Neves).       */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
// Inherit to implement a list of choices (set at the constructor)
// The inherited class can have a set of indexes (or options) corresponding
// to a specific values of members from the listed property.
template <class Type_, unsigned nOptions_> class cStaticMapChoices{
protected: typedef vector<unsigned> cGroup;
protected: struct cOption{ string MName; Type_ MOption; cGroup MGroup; };
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    cOption FOptions[nOptions_];
protected: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    void set(unsigned n, Type_ O_, string N_);
    void set(unsigned G_n, unsigned n, Type_ O_, string N_);
    void set(unsigned n, Type_ O_, string N_, unsigned G_, ...);
public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    explicit cStaticMapChoices(){ }
    virtual ~cStaticMapChoices(){ }
    //.........................................................................
    inline unsigned nOptions(){ return nOptions_; }
    inline string idxName(unsigned n){ return FOptions[n].MName; }
    inline Type_ idxOption(unsigned n){ return FOptions[n].MOption; }
    inline unsigned nGroups(unsigned n){ return FOptions[n].MGroup.size(); }
    inline int idxGroup(unsigned n, unsigned g){ return FOptions[n].MGroup[g]; }
    //......................................................................
    bool nameContains(string name_, int g);
    bool optionContains(Type_ opt_, int g);
    bool idxContains(unsigned n, int g);
    int getNameIdx(string name_);
    int getOptionIdx(Type_ opt_);
    Type_ option(string name_);
    string name(Type_ opt_);
    //......................................................................
    inline Type_ QOption(const QString &name_){ return option(name_.toStdString()); }
    inline int getQNameIdx(QString name_){ return getNameIdx(name_.toStdString()); }
    void load_options(unsigned g, QComboBox *combo);
    void load_options(QComboBox *combo);
};

/*===========================================================================*/
template <class Type_, unsigned nOptions_> void cStaticMapChoices
<Type_,nOptions_>::set(unsigned n, Type_ O_, string N_){
 FOptions[n].MName=N_; FOptions[n].MOption=O_;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// set 'n'th options ('O_','N_') and reuse the group options from 'G_n'
template <class Type_, unsigned nOptions_> void cStaticMapChoices
<Type_,nOptions_>::set(unsigned G_n, unsigned n, Type_ O_, string N_){
 FOptions[n].MName=N_; FOptions[n].MOption=O_;
 FOptions[n].MGroup(FOptions[G_n].MGroup);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
// set 'nth' options ('O_','N_',[G1,G2,...G'G_']).
template <class Type_, unsigned nOptions_> void cStaticMapChoices
<Type_,nOptions_>::set(unsigned n, Type_ O_, string N_,unsigned G_, ...){
va_list vList; va_start(vList,G_); FOptions[n].MGroup.resize(G_);
 for (unsigned i=0; i<G_; ++i) FOptions[n].MGroup[i]=va_arg(vList,int);
 FOptions[n].MName=N_; FOptions[n].MOption=O_; va_end(vList);
}

/*===========================================================================*/
template <class Type_, unsigned nOptions_>
bool cStaticMapChoices<Type_,nOptions_>::nameContains(string name_, int g){
 for (unsigned n=0; n<nOptions_; ++n) if (name_==FOptions[n].MName){
  for (unsigned i=0; i<FOptions[n].MGroup.size(); ++i) // single option
   if (FOptions[n].MGroup[i]==g) return true; } return false;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
template <class Type_, unsigned nOptions_>
bool cStaticMapChoices<Type_,nOptions_>::idxContains(unsigned n, int g){
 for (unsigned i=0; i<FOptions[n].MGroup.size(); ++i) // single option
  if (FOptions[n].MGroup[i]==g) return true; return false;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
template <class Type_, unsigned nOptions_>
bool cStaticMapChoices<Type_,nOptions_>::optionContains(Type_ opt_, int g){
 for (unsigned n=0; n<nOptions_; ++n) if (opt_==FOptions[n].MOption){
  for (unsigned i=0; i<FOptions[n].MGroup.size(); ++i) // single option
   if (FOptions[n].MGroup[i]==g) return true; } return false;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
template <class Type_, unsigned nOptions_>
Type_ cStaticMapChoices<Type_,nOptions_>::option(string name_){
 for (unsigned n=0; n<nOptions_; ++n) if (name_==FOptions[n].MName)
  return FOptions[n].MOption; throw Exception("Unknown Option",
   "cStaticMapChoices::option(string)",name_);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
template <class Type_, unsigned nOptions_>
string cStaticMapChoices<Type_,nOptions_>::name(Type_ opt_){
 for (unsigned n=0; n<nOptions_; ++n) if (opt_==FOptions[n].MOption)
  return FOptions[n].MName; throw Exception("Unknown Option",
   "cStaticMapChoices::option(string)",cString(opt_));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
template <class Type_, unsigned nOptions_>
int cStaticMapChoices<Type_,nOptions_>::getNameIdx(string name_){
 for (unsigned n=0; n<nOptions_; ++n) if (name_==FOptions[n].MName)
  return n; return -1;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
template <class Type_, unsigned nOptions_>
int cStaticMapChoices<Type_,nOptions_>::getOptionIdx(Type_ opt_){
 for (unsigned n=0; n<nOptions_; ++n) if (opt_==FOptions[n].MOption)
  return n; return -1;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
template <class Type_, unsigned nOptions_> void
cStaticMapChoices<Type_,nOptions_>::load_options(unsigned g, QComboBox *combo){
unsigned i; QString curr=combo->currentText(); combo->blockSignals(true);
 for (combo->clear(), i=0; i<nOptions_; ++i){ // add 'g' group.
  if (idxContains(i,g)) combo->addItem(FOptions[i].MName.c_str()); }
 for (i=0; i<combo->count(); ++i) // keep current text if available.
  if (combo->itemText(i)==curr){ combo->setCurrentIndex(i); break; }
 combo->blockSignals(false);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
template <class Type_, unsigned nOptions_> void
cStaticMapChoices<Type_,nOptions_>::load_options(QComboBox *combo){
unsigned i; combo->blockSignals(true);
 for (combo->clear(), i=0; i<nOptions_; ++i){ // add 'g' group.
  combo->addItem(FOptions[i].MName.c_str()); }
 combo->blockSignals(false);
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                                cFANNWrapper                               */
/*         Francisco Neves @ 2012.12.07 ( Last modified 2014.12.15 )         */
/*        Cross check version with mathetic library (Francisco Neves).       */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
enum cFANNDataSource { dsSimulation=1, dsReconstruction=2 };
enum cFANNType_enum { nnStandard=1, nnShortcut=2, nnCascade=3 };
class cFANNType: public cStaticMapChoices<cFANNType_enum,3>{ public: cFANNType(); };
class cTrainAlgorithm: public cStaticMapChoices<fann_train_enum,4>{ public: cTrainAlgorithm(); };
class cTrainStopFunction: public cStaticMapChoices<fann_stopfunc_enum,2>{ public: cTrainStopFunction(); };
class cTrainErrorFunction: public cStaticMapChoices<fann_errorfunc_enum,2>{ public: cTrainErrorFunction(); };
class cActivationFunction: public cStaticMapChoices<fann_activationfunc_enum,16>{ public: cActivationFunction(); };
class cDataSource: public cStaticMapChoices<cFANNDataSource,2>{ public: cDataSource(); };

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
class cFANNWrapper{
public: typedef vector<fann_type> cLayer;
public: typedef vector<unsigned> cLayers;
public: typedef vector<unsigned> cCandidateGroup;
public: typedef vector<fann_connection> cConnections;
public: typedef vector<fann_type> cActivationSteepness;
public: typedef vector<fann_activationfunc_enum> cActivationFunctions;    
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    bool FForceStop;
    struct fann *FANN;
    struct fann_train_data *FDataTrain, *FDataTest;
    static int FANN_API FUserCallback // see: ::userCallback.
     (fann*, fann_train_data*, unsigned, unsigned, float, unsigned);
protected: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    void clear();
    void train(fann_train_data **data, string file);
    void merge(fann_train_data **data, string file);
    inline void force_stop(){ FForceStop=true; } // not thread safe!
    //.........................................................................
    virtual int userCallback(fann_train_data* train, unsigned max_epochs,
     unsigned epochs_between_reports, float desired_error, unsigned epochs);
    virtual void testData(double *trainMSE, double *trainFailBits,
      double *testMSE, double *testFailBits);
    //.........................................................................
    inline void trainEpoch(){ fann_train_epoch(FANN,FDataTrain); }
    inline double testDataTest(){ return fann_test_data(FANN,FDataTest); } // see 'testData'
    inline double testDataTrain(){ return fann_test_data(FANN,FDataTrain); } // see 'testData'
public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    explicit cFANNWrapper();
    explicit cFANNWrapper(cFANNWrapper &O);
    virtual ~cFANNWrapper();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    virtual void createStandard(cLayers &layers);
    void createStandard(unsigned nLayers,...);
    virtual void createShortcut(cLayers &layers);
    void createShortcut(unsigned nLayers,...);
    void createSparse(cLayers &layers, double rate);
    void createSparse(unsigned nLayers, double rate,...);
    virtual void createCascade(unsigned nInputs, unsigned nOutputs);
    //.........................................................................
    inline void initWeights(){ fann_init_weights(FANN,FDataTrain); }
    inline void randomizeWeights(fann_type Wmin, fann_type Wmax){ fann_randomize_weights(FANN,Wmin,Wmax); }
    inline void setWeight(unsigned fromNeuron, unsigned toNeuron, fann_type W){ fann_set_weight(FANN,fromNeuron,toNeuron,W); }
    void setWeights(cConnections &connections_);
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    inline bool isFANN(){ return FANN; }
    struct fann* getFANN(){ return FANN; }
    inline bool isDataTest(){ return FDataTest; }
    inline bool isDataTrain(){ return FDataTrain; }
    inline struct fann_train_data* getDataTest(){ return FDataTest; }
    inline struct fann_train_data* getDataTrain(){ return FDataTrain; }
    inline unsigned lengthDataTest(){ return fann_length_train_data(FDataTest); }
    inline unsigned lengthDataTrain(){ return fann_length_train_data(FDataTrain); }
    // Train ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    void save(string file);
    void save(cMemFile &file);
    virtual void load(string file);
    virtual void load(cMemFile &file);
    void loadTrainData(string trainFile);
    void loadTrainData(string trainFile, string testFile);
    void loadTrain(string file, double testPercentage);
    inline void mergeDataTest(string testFile){ merge(&FDataTest,testFile); }
    inline void mergeDataTrain(string trainFile){ merge(&FDataTrain,trainFile); }
    inline void saveDataTest(string testFile){ fann_save_train(FDataTest,testFile.c_str()); }
    inline void saveDataTrain(string trainFile){ fann_save_train(FDataTrain,trainFile.c_str()); }
    // (Basic) Train config ...................................................
    inline fann_train_enum trainAlgorithm(){ return fann_get_training_algorithm(FANN); }
    inline void trainAlgorithm(fann_train_enum a){ fann_set_training_algorithm(FANN,a); }
    inline fann_type activationSteepness(unsigned layer, unsigned neuron){ return fann_get_activation_steepness(FANN,layer,neuron); }
    inline void activationSteepness(unsigned layer, unsigned neuron, fann_type v){ fann_set_activation_steepness(FANN,v,layer,neuron); }
    inline void layerActivationSteepness(unsigned layer, fann_type v){ fann_set_activation_steepness_layer(FANN,v,layer); }
    inline void hiddenActivationSteepness(fann_type v){ fann_set_activation_steepness_hidden(FANN,v); }
    inline void outputActivationSteepness(fann_type v){ fann_set_activation_steepness_output(FANN,v); }
    inline fann_activationfunc_enum activationFunction(unsigned layer, unsigned neuron){ return fann_get_activation_function(FANN,layer,neuron); }
    inline void activationFunction(unsigned layer, unsigned neuron, fann_activationfunc_enum f){ fann_set_activation_function(FANN,f,layer,neuron); }
    inline void layerActivationFunc(unsigned layer, fann_activationfunc_enum f){ fann_set_activation_function_layer(FANN,f,layer); }
    inline void outputActivationFunc(fann_activationfunc_enum f){ fann_set_activation_function_output(FANN,f); }
    inline void hiddenActivationFunc(fann_activationfunc_enum f){ fann_set_activation_function_hidden(FANN,f); }
    inline double learningRate(){ return fann_get_learning_rate(FANN); }
    inline void learningRate(double v){ fann_set_learning_rate(FANN,v); }
    inline double learningMomentum(){ return fann_get_learning_momentum(FANN); }
    inline void learningMomentum(double v){ return fann_set_learning_momentum(FANN,v); }
    inline fann_errorfunc_enum errorFunction(){ return fann_get_train_error_function(FANN); }
    inline void errorFunction(fann_errorfunc_enum e){ fann_set_train_error_function(FANN,e); }
    inline fann_stopfunc_enum stopFunction(){ return fann_get_train_stop_function(FANN); }
    inline void stopFunction(fann_stopfunc_enum f){ fann_set_train_stop_function(FANN,f); }
    inline fann_type bitFailLimit(){ return fann_get_bit_fail_limit(FANN); } // see: ''.
    inline void bitFailLimit(fann_type v){ fann_set_bit_fail_limit(FANN,v); }
    // Scaling ................................................................
    inline void clearScaling(){ fann_clear_scaling_params(FANN); }
    inline void scaleInputParameters(double min, double max){ fann_set_input_scaling_params(FANN,FDataTrain,min,max); }
    inline void scaleOutputParameters(double min, double max){ fann_set_output_scaling_params(FANN,FDataTrain,min,max); }
    inline void scaleparameters(double iMin, double iMax, double oMin, double oMax){ fann_set_scaling_params(FANN,FDataTrain,iMin,iMax,oMin,oMax); }
    inline void scaleInput(cLayer &in){ fann_scale_input(FANN,in.data()); }
    inline void scaleOutput(cLayer &out){ fann_scale_output(FANN,out.data()); }
    inline void descaleInput(cLayer &in){ fann_descale_input(FANN,in.data()); }
    inline void descaleOutput(cLayer &out){ fann_descale_output(FANN,out.data()); }
    void scaleInputs(double min, double max); // For train & test data sets.
    void scaleOutputs(double min, double max); // For train & test data sets.
    void scale(double min, double max); // for train & test data sets.
    void scale(); // for train & test data sets.
    // Train execution ........................................................
    void train(unsigned max_epochs, unsigned epochs_per_reports, float error);
    void trainCascade(unsigned max_neurons, unsigned neurons_between_reports, float error);
    void resumeTrain(unsigned max_epochs, unsigned epochs_per_reports, float error);
    void resumeTrainCascade(unsigned max_neurons, unsigned neurons_between_reports, float error);
    // Train control ..........................................................
    inline unsigned bitFail(){ return fann_get_bit_fail(FANN); } // see: 'Limit'
    inline fann_type MSE(){ return fann_get_MSE(FANN); }
    inline void resetMSE(){ fann_reset_MSE(FANN); }
    // Retro propagation ......................................................
    inline double rprop_incFactor(){ return fann_get_rprop_increase_factor(FANN); }
    inline void rprop_incFactor(double v){ fann_set_rprop_increase_factor(FANN,v); }
    inline double rprop_decFactor(){ return fann_get_rprop_decrease_factor(FANN); }
    inline void rprop_decFactor(double v){ fann_set_rprop_decrease_factor(FANN,v); }
    inline double rprop_deltaMin(){ return fann_get_rprop_delta_min(FANN); }
    inline void rprop_deltaMin(double v){ fann_set_rprop_delta_min(FANN,v); }
    inline double rprop_deltaMax(){ return fann_get_rprop_delta_max(FANN); }
    inline void rprop_deltaMax(double v){ fann_set_rprop_delta_max(FANN,v); }
    inline double rprop_deltaZero(){ return fann_get_rprop_delta_zero(FANN); }
    inline void rprop_deltaZero(double v){ fann_set_rprop_delta_zero(FANN,v); }
    // Quick propagation ......................................................
    inline double qprop_decay(){ return fann_get_quickprop_decay(FANN); }
    inline void qprop_decay(double v){ return fann_set_quickprop_decay(FANN,v); }
    inline double qprop_mu(){ return fann_get_quickprop_mu(FANN); }
    inline void qprop_mu(double v){ return fann_set_quickprop_mu(FANN,v); }
    // SAR propagation ........................................................
    inline double sarprop_weightDecayShit(){ return 0; /*fann_get_sarprop_weight_decay_shift(FANN)*/; }
    inline void sarprop_weightDecayShit(double){ /*fann_set_sarprop_weight_decay_shift(FANN,v); V2.1.0*/ }
    inline double sarprop_stepErrorThresholdFactor(){ return 0/*fann_get_sarprop_step_error_threshold_factor(FANN) V2.1.0*/; }
    inline void sarprop_stepErrorThresholdFactor(double){ /*fann_set_sarprop_step_error_threshold_factor(FANN,v) V2.1.0*/; }
    inline double sarprop_stepErrorShift(){ return 0/*fann_get_sarprop_step_error_shift(FANN) V2.1.0*/; }
    inline void sarprop_stepErrorShift(double){ /*fann_set_sarprop_step_error_shift(FANN,v) V2.1.0*/; }
    inline double sarprop_temperature(){ return 0/*fann_get_sarprop_temperature(FANN) V2.1.0*/; }
    inline void sarprop_temperature(double){ /*fann_set_sarprop_temperature(FANN,v) V2.1.0*/; }
    // cascade training .......................................................
    inline double cascade_outputChangeFraction(){ return fann_get_cascade_output_change_fraction(FANN); }
    inline void cascade_outputChangeFraction(double v){ fann_set_cascade_output_change_fraction(FANN,v); }
    inline unsigned cascade_outputStagnationEpochs(){ return fann_get_cascade_output_stagnation_epochs(FANN); }
    inline void cascade_outputStagnationEpochs(unsigned v){ fann_set_cascade_output_stagnation_epochs(FANN,v); }
    inline double cascade_candidateChangeFraction(){ return fann_get_cascade_candidate_change_fraction(FANN); }
    inline void cascade_candidateChangeFraction(double v){ fann_set_cascade_candidate_change_fraction(FANN,v); }
    inline unsigned cascade_candidateStagnationEpochs(){ return fann_get_cascade_candidate_stagnation_epochs(FANN); }
    inline void cascade_candidateStagnationEpochs(unsigned v){ fann_set_cascade_candidate_stagnation_epochs(FANN,v); }
    inline fann_type cascade_weightMultiplier(){ return fann_get_cascade_weight_multiplier(FANN); }
    inline void cascade_weightMultiplier(fann_type v){ fann_set_cascade_weight_multiplier(FANN,v); }
    inline fann_type cascade_candidateLimit(){ return fann_get_cascade_candidate_limit(FANN); }
    inline void cascade_candidateLimit(fann_type v){ fann_set_cascade_candidate_limit(FANN,v); }
    inline unsigned cascade_maxOutEpochs(){ return fann_get_cascade_max_out_epochs(FANN); }
    inline void cascade_maxOutEpochs(unsigned v){ fann_set_cascade_max_out_epochs(FANN,v); }
    inline unsigned cascade_minOutEpochs(){ return 0/*fann_get_cascade_min_out_epochs(FANN) V2.2.0*/; }
    inline void cascade_minOutEpochs(unsigned){ /*fann_set_cascade_min_out_epochs(FANN,v) V2.2.0*/; }
    inline unsigned cascade_maxCandEpochs(){ return fann_get_cascade_max_cand_epochs(FANN); }
    inline void cascade_maxCandEpochs(unsigned v){ fann_set_cascade_max_cand_epochs(FANN,v); }
    inline unsigned cascade_minCandEpochs(){ return 0/*fann_get_cascade_min_cand_epochs(FANN) V2.2.0*/; }
    inline void cascade_minCandEpochs(unsigned){ /*fann_set_cascade_min_cand_epochs(FANN,v) V2.2.0*/; }
    inline unsigned cascade_numCandidates(){ return fann_get_cascade_num_candidates(FANN); }
    inline unsigned cascade_nActivationFunctions(){ return fann_get_cascade_activation_functions_count(FANN); }
    void cascade_getActivationFunctions(cActivationFunctions &f);
    void cascade_setActivationFunctions(cActivationFunctions &f);
    inline unsigned cascade_nActivationSteepness(){ return fann_get_cascade_activation_steepnesses_count(FANN); }
    void cascade_getActivationSteepness(cActivationSteepness &f);
    void cascade_setActivationSteepness(cActivationSteepness &f);
    inline unsigned cascade_candidateGroups(){ return fann_get_cascade_num_candidate_groups(FANN); }
    inline void cascade_candidateGroups(unsigned v){ fann_set_cascade_num_candidate_groups(FANN,v); }
    // Output .................................................................
    void run(cLayer &in, cLayer &out);
    fann_type* run(cLayer &in);
    // Structure ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    inline fann_nettype_enum type(){ return fann_get_network_type(FANN); }
    inline float connectionRate(){ return fann_get_connection_rate(FANN); }
    //.........................................................................
    void connections(cConnections &connections_);
    void layers(cLayers &layers_);
    void bias(cLayers &bias_);
    //.........................................................................
    inline unsigned nInputs(){ return fann_get_num_input(FANN); }
    inline unsigned nLayers(){ return fann_get_num_layers(FANN); }
    inline unsigned nOutputs(){ return fann_get_num_output(FANN); }
    inline unsigned nTotalNeurons(){ return fann_get_total_neurons(FANN); }
    inline unsigned nTotalConnections(){ return fann_get_total_connections(FANN); }
    unsigned nHiddenNeurons(){ return nTotalNeurons()-nInputs()-nOutputs(); }
    unsigned nHiddenLayers(){ return nLayers()>2?nLayers()-2:0; }
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                           NeuralNetworksModule                            */
/*             ( Last modified by Francisco Neves @ 2015.04.27 )             */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
class NeuralNetworksWindow;
class NeuralNetworksModule : public QObject, public cFANNWrapper{
  Q_OBJECT
public: enum cNormSignals { nsToMax=0, nsToSum };
private: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
   pms* PMs;
   APmGroupsManager* PMgroups;
   cNormSignals FNorm;
   string *FTrainOutput;
   int FEnergyDims, FSpaceDims;
   unsigned FMaxEpochsStag, FnEpochsStag;
   double FTrainMSE, FTrainFailBit, FTestMSE, FTestFailBit;
   double FmScale[3], FbScale[3];
   ReconstructionSettings *RecSet;
   EventsDataClass* EventsDataHub;
   vector<fann_type> FVIn, FVOut;
protected: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
   int userCallback(fann_train_data* train, unsigned max_epochs,
    unsigned epochs_between_reports, float desired_error, unsigned epochs);
public: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  explicit NeuralNetworksModule(pms *Pms, APmGroupsManager *PMgroups, EventsDataClass *eventsDataHub, QObject *parent = 0);
  void Configure(ReconstructionSettings *recSet) {RecSet = recSet;}
  bool ReconstructEvent(int ievent, int itime, double (*r)[3], double *energy);
  //...........................................................................
  bool checkPMsDimension();
  unsigned getPMsDimension();
  bool isValidPM(unsigned pm);
  //...........................................................................
  bool checkRecDimension();
  unsigned getRecDimension();
  void setRecDimension(bool isRecZ, bool isRecE);
  void setRecScaling(unsigned XYZ, double m, double b);
  inline int spaceDims(){ return FSpaceDims; }
  inline int energyDims(){ return FEnergyDims; }
  inline bool isRecZ(){ return FSpaceDims==3; } // see 'spaceDims'.
  inline bool isRecE(){ return FEnergyDims==1; } // see 'energyDims'.
  inline ReconstructionSettings &reconstructionSettings(){ return *RecSet; }
  //...........................................................................
  inline cNormSignals norm_signals(){ return FNorm; }
  inline void norm_signals(cNormSignals s){ FNorm=s; }
  //...........................................................................
  inline unsigned train_maxStagEpochs(){ return FMaxEpochsStag; }
  inline void train_maxStagEpochs(unsigned se){ FMaxEpochsStag=se; }
  inline unsigned train_nStagEpochs(){ return FnEpochsStag; }
  static cFANNType FANNTypes; // available possibilities
  static cTrainAlgorithm trainAlgorithms; // available possibilities
  static cTrainStopFunction trainStopFunctions; // available possibilities
  static cTrainErrorFunction trainErrorFunctions; // available possibilities
  static cActivationFunction activationFunctions; // available possibilities
  static cDataSource dataSource; // available possibilities.
  //...........................................................................
  inline double scale_out(unsigned d, double nn){ return FmScale[d]*nn+FbScale[d]; }
  inline double scale_inv(unsigned d, double x){ return (x-FbScale[d])/FmScale[d]; }
  inline double scale_m(unsigned d){ return FmScale[d]; }
  inline double scale_b(unsigned d){ return FbScale[d]; }
  void createCascade(unsigned nInputs, unsigned nOutputs);
  void createStandard(cLayers &layers);
  void createShortcut(cLayers &layers);
  void load(cMemFile &file);
  void load(string file);
  void init_NNModule(string *trainOutput=NULL);
  void cont_NNModule();
signals: //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  void status(QString mess);
public slots: //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
  void train_stop(){ cFANNWrapper::force_stop(); }
};

#endif // NEURALNETWORKSMODULE_H
