#include "neuralnetworkswindow.h"
#include "ui_neuralnetworkswindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include "neuralnetworksmodule.h"
#include "mainwindow.h"
//#include "classes.h"
#include "apositionenergyrecords.h"
#include "pms.h"
#include "eventsdataclass.h"
#include "reconstructionwindow.h"
#include "reconstructionmanagerclass.h"
#include <fstream>
#include <QJsonDocument>
#include <QTextStream>
#include <QDebug>

#define window_title "Neural Network Settings"

typedef QVector<double> cEvent;
typedef QVector<cEvent> cEvents;


cFANNDataSource FDataSource;

#define SIZE_NORMAL  450
#define SIZE_OPTIONS 830

//! check the connections at MainWindowInits(.cpp):
//! ** connect(NNmodule,SIGNAL(status(QString)),NNwindow,SLOT(status(QString)));
//! ** connect(NNwindow,SIGNAL(train_stop()),NNmodule,SLOT(train_stop()));
//! ** connect(Rwindow,SIGNAL(cb3DreconstructionChanged(bool)),NNwindow,SLOT(onReconstruct3D(bool)));
//! ** connect(Rwindow,SIGNAL(cbReconstructEnergyChanged(bool)),NNwindow,SLOT(onReconstructE(bool)));

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                            NeuralNetworksWindow                           */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*===========================================================================*/
NeuralNetworksWindow::NeuralNetworksWindow(QWidget *parent, MainWindow *mw, EventsDataClass* eventsDataHub):
QMainWindow(parent),ui(new Ui::NeuralNetworksWindow),MW(mw),EventsDataHub(eventsDataHub),
FChanged(false),FXYZAutoScale(true)
{
 ui->setupUi(this); this->setFixedSize(this->size());
 ui->twHideLayers->setSelectionMode(QAbstractItemView::SingleSelection);
 ui->lbTrainInput->setText((FTrainInput="default.train").c_str());
 ui->lbTrainOutput->setText((FTrainOutput="default.net").c_str());
 setMinimumWidth(SIZE_OPTIONS); setMaximumWidth(SIZE_OPTIONS);
 NeuralNetworksModule::FANNTypes.load_options(ui->cbNNType);
 NeuralNetworksModule::activationFunctions.load_options(ui->cbNNOutFunction);
 NeuralNetworksModule::activationFunctions.load_options(ui->cbNNHideFunction);
 NeuralNetworksModule::trainAlgorithms.load_options(ui->cbNNTrainAlg);
 NeuralNetworksModule::trainErrorFunctions.load_options(ui->cbNNTrainError);
 NeuralNetworksModule::trainStopFunctions.load_options(ui->cbNNTrainStop);
 NeuralNetworksModule::dataSource.load_options(ui->cbNNDataSrc);
 //............................................................................ 
 ui->cbNNHideFunction->setCurrentIndex(NeuralNetworksModule::activationFunctions.getNameIdx("LINEAR"));
 ui->cbNNOutFunction->setCurrentIndex(NeuralNetworksModule::activationFunctions.getNameIdx("LINEAR"));

 ui->cbNNTrainAlg->setCurrentIndex(NeuralNetworksModule::trainAlgorithms.getNameIdx("RETRO PROPAGATION"));
 //qDebug() << NeuralNetworksModule::trainErrorFunctions.getNameIdx("LINEAR");
 ui->cbNNTrainError->setCurrentIndex(NeuralNetworksModule::trainErrorFunctions.getNameIdx("LINEAR"));

 ui->cbNNTrainStop->setCurrentIndex(NeuralNetworksModule::trainStopFunctions.getNameIdx("BIT"));
 ui->cbNNDataSrc->setCurrentIndex(NeuralNetworksModule::dataSource.getNameIdx("Sim Data"));
 ui->cbNNType->setCurrentIndex(NeuralNetworksModule::FANNTypes.getNameIdx("CASCADE"));
 //............................................................................ 
 connect(ui->pbNNStop,SIGNAL(released()),this,SIGNAL(train_stop())); // forward the signal. 
 //............................................................................
 MW->ReconstructionManager->ANNmodule->setRecDimension(
  MW->Rwindow->isReconstructZ(),MW->Rwindow->isReconstructEnergy());

//qDebug() << "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

 FDataSource=NeuralNetworksModule::dataSource.QOption(ui->cbNNDataSrc->currentText()); /// THIS MAY NEED SOME CHANGE....


 setWindowTitle(window_title); 
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
NeuralNetworksWindow::~NeuralNetworksWindow(){
  delete ui;
}

/*===========================================================================*/
/*! fills 'hidden' from the GUI hidden layer table. */
void NeuralNetworksWindow::build_layers
(unsigned in, unsigned out, vector<unsigned> &hidden){
 if (ui->twHideLayers->rowCount()==0){ hidden.clear(); return; }
 hidden.resize(ui->twHideLayers->rowCount()+2);
 hidden.front()=in; hidden.back()=out;
 //............................................................................
 for (int i=0; i<ui->twHideLayers->rowCount(); ++i){
  hidden[i+1]=atoi(ui->twHideLayers->item(i,0)->text().toStdString().c_str());
  if (hidden[i+1]<=0){ hidden.clear(); QMessageBox::warning(this,"NN Module",
   "Check the number of hidden neurons!"); return; }
} }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! Get the position of 'event's 'hit'. The availability of data is defined
//! by 'FDataSource' (from simulation or a previous reconstruction).
double *NeuralNetworksWindow::getEvent_xyz(unsigned evt, unsigned hit){
 switch (FDataSource){ // check if source available!
  //case dsSimulation: return MW->Reconstructor->getScan(evt)->Points[hit].r; //ANDR
  case dsSimulation: return EventsDataHub->Scan.at(evt)->Points[hit].r;  //ANDR
  case dsReconstruction: return EventsDataHub->ReconstructionData[0][evt]->Points[hit].r;
  default: return 0; }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! Get the energy of 'event's 'hit'. The availability of data is defined
//! by 'FDataSource' (from simulation or a previous reconstruction).
double NeuralNetworksWindow::getEvent_ener(unsigned evt, unsigned hit){
 switch (FDataSource){ // check if source available.
  //case dsSimulation: return MW->Reconstructor->getScan(evt)->Points[hit].energy; //ANDR
  case dsSimulation: return EventsDataHub->Scan.at(evt)->Points[hit].energy;   //ANDR
  case dsReconstruction: return EventsDataHub->ReconstructionData[0][evt]->Points[hit].energy;
  default: return 0; }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! Get the 'GoodEvent' flag depending on the data source.
bool NeuralNetworksWindow::getEvent_good(unsigned evt){
 switch (FDataSource){ // check if source available!
  //case dsSimulation: return MW->Reconstructor->getScan(evt)->GoodEvent; //ANDR
  case dsSimulation: return EventsDataHub->Scan.at(evt)->GoodEvent;  //ANDR
  case dsReconstruction: return EventsDataHub->ReconstructionData[0][evt]->GoodEvent;
  default: return false; }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! Sets the training scaling based on the training data.
unsigned NeuralNetworksWindow::getTrain_scale(){
//unsigned nEvt=MW->Reconstructor->sizeOfEvents(), nGood=0; //ANDR
unsigned nEvt=EventsDataHub->Events.size(), nGood=0;   //ANDR
double xMin, xMax, yMin, yMax, zMin, zMax, *xyz;
 if (!FXYZAutoScale){ //+++++++++++++++++++++++++++++++++++++++++++++++++++++++
  for (unsigned iEvt=0; iEvt<nEvt; ++iEvt) if (getEvent_good(iEvt)) nGood++;
  MW->ReconstructionManager->ANNmodule->setRecScaling(0,1,0);
  MW->ReconstructionManager->ANNmodule->setRecScaling(1,1,0);
  MW->ReconstructionManager->ANNmodule->setRecScaling(2,1,0);
 } else { //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  xMin=yMin=zMin=DBL_MAX; xMax=yMax=zMax=-DBL_MAX;
  for (unsigned iEvt=0; iEvt<nEvt; ++iEvt)
   if (getEvent_good(iEvt)){ xyz=getEvent_xyz(iEvt); nGood++;
    if (xyz[0]<xMin) xMin=xyz[0]; if (xyz[0]>xMax) xMax=xyz[0];
    if (xyz[1]<yMin) yMin=xyz[1]; if (xyz[1]>yMax) yMax=xyz[1];
    if (xyz[2]<zMin) zMin=xyz[2]; if (xyz[2]>zMax) zMax=xyz[2];
  } //.........................................................................
  MW->ReconstructionManager->ANNmodule->setRecScaling(0,xMin!=xMax?xMax-xMin:1.,xMin);
  MW->ReconstructionManager->ANNmodule->setRecScaling(1,yMin!=yMax?yMax-yMin:1.,yMin);
  MW->ReconstructionManager->ANNmodule->setRecScaling(2,zMin!=zMax?zMax-zMin:1.,zMin);
 } return nGood;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Get the response of the photosensor 'ipm' for event 'evt'. */
double NeuralNetworksWindow::getEvent_signal(unsigned evt, unsigned ipm){
 //return (*MW->Reconstructor->getEvent(evt))[ipm];
 return EventsDataHub->Events[evt][ipm];  // Andr 11 02 2015
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! Set the source of data for 'getEvent_xyz' and 'getEvent_ener'
//! (No need to track changes - checked only when building the training file.
void NeuralNetworksWindow::on_cbNNDataSrc_currentIndexChanged(const QString &src){
 FDataSource=NeuralNetworksModule::dataSource.QOption(src);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Creates the train file from available data. Return number of inputs. */
unsigned NeuralNetworksWindow::build_trainFile(string filename){
unsigned iEvt, iPM, iC, last; double coleff=1, norm=1, val;
unsigned nEvt=EventsDataHub->Events.size(), nGood=0;
unsigned nIn=MW->ReconstructionManager->ANNmodule->getPMsDimension();
unsigned nOut=MW->ReconstructionManager->ANNmodule->getRecDimension();
ofstream outFile; // tmp file.
 outFile.open(filename.c_str(),ios_base::out|ios_base::trunc);
 nGood=getTrain_scale(); // set train limits and number of good events.
 if (outFile.is_open()){ //....................................................
  outFile << nGood << "\t" << nIn << "\t" << nOut << endl; // HEADER
  for (iEvt=0; iEvt<nEvt; ++iEvt, outFile << endl){
   if (!getEvent_good(iEvt)) continue; // skip bad events.
   // GET LIGHT COLLECTION EFFICIENCY (INPUT NEURONS) +++++++++++++++++++++++++
   if (MW->ReconstructionManager->ANNmodule->isRecE() // E is defined from normalized signal.
    || MW->ReconstructionManager->ANNmodule->norm_signals()==NeuralNetworksModule::nsToSum){
     for (coleff=0, iPM=0; iPM<nIn; ++iPM){ if (MW->ReconstructionManager->ANNmodule->isValidPM(iPM))
      coleff+=getEvent_signal(iEvt,iPM); } }
   //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   // NORMALIZE FACTORS (INPUT NEURONS) +++++++++++++++++++++++++++++++++++++++
   if (MW->ReconstructionManager->ANNmodule->norm_signals()==NeuralNetworksModule::nsToMax){
    for (norm=-DBL_MAX, iPM=0; iPM<nIn; ++iPM){ if (MW->ReconstructionManager->ANNmodule->isValidPM(iPM))
    if ((val=getEvent_signal(iEvt,iPM))>norm) norm=val; }
   } else if (MW->ReconstructionManager->ANNmodule->norm_signals()==NeuralNetworksModule::nsToSum){
    norm=coleff; // Same as light collection efficiency above.
   } else throw Exception("Invalid Operation",
    "NeuralNetworksWindow::build_trainFile(string)","Unknown Option");
   //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   // INPUTS ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   for (iPM=0, last=nIn-1; iPM<=last; ++iPM){ if (MW->ReconstructionManager->ANNmodule->isValidPM(iPM))
    outFile << (getEvent_signal(iEvt,iPM)/norm) << (iPM<last?"\t":""); }
   // OUTPUTS +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   for (outFile << endl, iC=0, last=MW->ReconstructionManager->ANNmodule->spaceDims()-1; iC<=last; ++iC)
    outFile <<  MW->ReconstructionManager->ANNmodule->scale_inv(iC,getEvent_xyz(iEvt)[iC]) << (iC<last?"\t":"");
   if (MW->ReconstructionManager->ANNmodule->isRecE()) outFile << "\t" << getEvent_ener(iEvt)/coleff;
  } outFile.close(); return nIn; //++++++++++++++++++++++++++++++++++++++++++++
 } else throw Exception("Fail To Open File",
  "NeuralNetworksWindow::build_trainFile(string)",filename);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! Writes 'data' into 'filename'
void NeuralNetworksWindow::write_File(string filename, QString &data) const{
ofstream out; out.open(filename.c_str(),ios_base::out|ios_base::trunc);
 if (out.is_open()){ out << data.toStdString(); out.close(); } else throw Exception
  ("Fail to open File","NeuralNetworksWindow::write_File(string, QString&)",filename);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! Read 'filename' into a QString.
QString NeuralNetworksWindow::read_File(string filename) const{
ifstream in; QString data; string ln;
 in.open(filename.c_str(),ios_base::in);
 if (in.is_open()){ while (in.good()){ getline(in,ln);
   if (!data.isEmpty() && !ln.empty()) data+='\n';
   data.append(ln.c_str()); } in.close(); return data;
 } else throw Exception ("Fail to open File",
  "NeuralNetworksWindow::read_File(string)",filename);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Propagate changes when the user changes the NN type. */
void NeuralNetworksWindow::on_cbNNType_currentIndexChanged(const QString &type){
bool gen_option=true; // condense option to toggle controls.
cFANNType_enum etype=NeuralNetworksModule::FANNTypes.QOption(type);
 //............................................................................
 if (etype==nnStandard || etype==nnShortcut){ gen_option=true;
  NeuralNetworksModule::trainAlgorithms.load_options(1,ui->cbNNTrainAlg);
 } else if (etype==nnCascade){ gen_option=false;
  NeuralNetworksModule::trainAlgorithms.load_options(2,ui->cbNNTrainAlg);
 } //..........................................................................
 ui->pbNNDelLayer->setEnabled(gen_option && ui->twHideLayers->rowCount()>0);
 ui->pbNNAddLayer->setEnabled(gen_option);
 ui->twHideLayers->setEnabled(gen_option);
 ui->cbNNHideFunction->setEnabled(gen_option);
 ui->tabCascade->setEnabled(!gen_option);
 userChangeUIOpt(true);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Propagate changes when the user changes the output activation function. */
void NeuralNetworksWindow::on_cbNNOutFunction_currentIndexChanged
(const QString &/*out*/){ userChangeUIOpt(true); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Propagate changes when the user changes the hidden activation function. */
void NeuralNetworksWindow::on_cbNNHideFunction_currentIndexChanged
(const QString &/*hide*/){ userChangeUIOpt(true); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Add a new hidden layer (GUI) and propagate the changes. */
void NeuralNetworksWindow::on_pbNNAddLayer_released(){
QList<QTableWidgetItem*> sel=ui->twHideLayers->selectedItems(); int at=-1;
 if (sel.count()==0) at=ui->twHideLayers->rowCount();
 else if (sel.count()==1) at=sel.front()->row();
 if (at<0) return; // check constructor: only one selectable item
 //............................................................................
 ui->twHideLayers->insertRow(at);
 ui->twHideLayers->setItem(at,0,new QTableWidgetItem("-"));
 ui->twHideLayers->setItem(at,1,new QTableWidgetItem("hidden"));
 ui->twHideLayers->item(at,1)->setFlags(Qt::ItemIsEnabled);
 //............................................................................
 ui->pbNNDelLayer->setEnabled(true);
 userChangeUIOpt(true);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Remove a hidden layer (GUI) and propagate the changes. */
void NeuralNetworksWindow::on_pbNNDelLayer_released(){
QList<QTableWidgetItem*> sel=ui->twHideLayers->selectedItems(); int at=-1;
 if (sel.count()==0) at=ui->twHideLayers->rowCount()-1; // remove last.
 else if (sel.count()==1) at=sel.front()->row(); // remove selected.
 if (at<0) return; // check constructor: only one selectable item
 //............................................................................
 ui->twHideLayers->removeRow(at); if (ui->twHideLayers->rowCount()==0)
  ui->pbNNDelLayer->setEnabled(false); // disable button if no items to select.
 userChangeUIOpt(true);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Propagate the changes when the user changes the training algorithm. */
void NeuralNetworksWindow::on_cbNNTrainAlg_currentIndexChanged
(const QString &alg){if (alg.isEmpty()) return; else {
fann_train_enum tAlg=NeuralNetworksModule::trainAlgorithms.QOption(alg);
 ui->tabQProp->setEnabled(tAlg==FANN_TRAIN_QUICKPROP);
 ui->tabRProp->setEnabled(tAlg==FANN_TRAIN_RPROP);
 userChangeUIOpt(true);
} }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Propagate the changes when the user changes the training error. */
void NeuralNetworksWindow::on_cbNNTrainError_currentIndexChanged
(const QString &/*err*/){ userChangeUIOpt(true); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Propagate the changes when the user changes the training stop. */
void NeuralNetworksWindow::on_cbNNTrainStop_currentIndexChanged
(const QString &stop){ if (stop.isEmpty()) return; else {
fann_stopfunc_enum tStp=NeuralNetworksModule::trainStopFunctions.QOption(stop);
 ui->bsNNBitLimit->setEnabled(tStp==FANN_STOPFUNC_BIT);
 userChangeUIOpt(true);
} }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Update the interface as a function of the trainig 'status'. */
void NeuralNetworksWindow::train_status(bool status){
 ui->pbNNTrain->setEnabled(!status); ui->pbNNStop->setEnabled(status);
 ui->pbNNLoad->setEnabled(!status); ui->twSubSettings->setEnabled(!status);
 ui->tabTrainTest->setEnabled(!status); ui->tabTrainInput->setEnabled(!status);
 ui->tabTrainOutput->setEnabled(!status); ui->bgGeneralSettings->setEnabled(!status);
 ui->cbNNDataSrc->setEnabled(!status);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Update the interface when the trainig is finished. */
void NeuralNetworksWindow::train_finished(){
 train_status(false);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::doTrain(bool resume){
 //if (FDataSource==dsSimulation && MW->Reconstructor->isEventsEmpty()){ //ANDR
 if (FDataSource==dsSimulation && EventsDataHub->isEmpty()){      //ANDR
  QMessageBox::warning(this,"ANTS FANN Module Warning", //! Sim Data
  "No Data Available! Please connect the train data first!"); return; }
 //else if (FDataSource==dsReconstruction && !MW->Reconstructor->isReconstructionDataReady()){  //ANDR
 else if (FDataSource==dsReconstruction && EventsDataHub->isReconstructionDataEmpty()){         //ANDR
  QMessageBox::warning(this,"ANTS FANN Module Warning", //! Rec Data.
  "No Data Available! Please reconstruct data first!"); return; }
unsigned nIn, nOut=MW->ReconstructionManager->ANNmodule->getRecDimension(); // number of inputs/outputs
cFANNType_enum etype=NeuralNetworksModule::FANNTypes.QOption(ui->cbNNType->currentText());
fann_train_enum tAlg=NeuralNetworksModule::trainAlgorithms.QOption(ui->cbNNTrainAlg->currentText());
fann_errorfunc_enum eFunc=NeuralNetworksModule::trainErrorFunctions.QOption(ui->cbNNTrainError->currentText());
fann_activationfunc_enum oFunc=NeuralNetworksModule::activationFunctions.QOption(ui->cbNNOutFunction->currentText());
fann_activationfunc_enum hFunc=NeuralNetworksModule::activationFunctions.QOption(ui->cbNNHideFunction->currentText());
fann_stopfunc_enum sFunc=NeuralNetworksModule::trainStopFunctions.QOption(ui->cbNNTrainStop->currentText());
 //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 if (resume) MW->ReconstructionManager->ANNmodule->cont_NNModule(); // resume training.
 else MW->ReconstructionManager->ANNmodule->init_NNModule(&FTrainOutput); // clear all!!
 nIn=build_trainFile(FTrainInput); // build train on disk.
 //@ ALG DEPENDENCIES @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 if (tAlg==FANN_TRAIN_QUICKPROP){ // QuickProp specific options.
  if (ui->cbQPropDecay->isChecked()) // default value is -0.0001
   MW->ReconstructionManager->ANNmodule->qprop_decay(ui->bsQPropDecay->value());
  if (ui->cbQPropMu->isChecked()) // default value is 1.75
   MW->ReconstructionManager->ANNmodule->qprop_mu(ui->bsQPropMu->value());
 //@ ALG DEPENDENCIES @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 } else if (tAlg==FANN_TRAIN_RPROP){ // RetroProp specific options.
  if (ui->cbRPropIncFactor->isChecked()) // default value is 1.2
   MW->ReconstructionManager->ANNmodule->rprop_incFactor(ui->bsRPropIncFactor->value());
  if (ui->cbRPropDecFactor->isChecked()) // default value is 0.5
   MW->ReconstructionManager->ANNmodule->rprop_decFactor(ui->bsRPropDecFactor->value());
  if (ui->cbRPropDeltaMin->isChecked()) // default value is 0
   MW->ReconstructionManager->ANNmodule->rprop_deltaMin(ui->bsRPropDeltaMin->value());
  if (ui->cbRPropDeltaMax->isChecked()) // default value is 50
   MW->ReconstructionManager->ANNmodule->rprop_deltaMax(ui->bsRPropDeltaMax->value());
  if (ui->cbRPropDeltaZero->isChecked()) // default value is 0.1
   MW->ReconstructionManager->ANNmodule->rprop_deltaZero(ui->bsRPropDeltaZero->value());
 } //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 //@ NN type dependencies @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 if (etype==nnCascade){ // Cascade specific options.
  if (!resume) MW->ReconstructionManager->ANNmodule->createCascade(nIn,nOut);
  MW->ReconstructionManager->ANNmodule->trainAlgorithm(tAlg);
  MW->ReconstructionManager->ANNmodule->outputActivationFunc(oFunc);
  MW->ReconstructionManager->ANNmodule->errorFunction(eFunc);
  MW->ReconstructionManager->ANNmodule->stopFunction(sFunc);
  if (ui->cbCascOutChangeFrac->isChecked()) // default is 0.01
   MW->ReconstructionManager->ANNmodule->cascade_outputChangeFraction(ui->bsCascOutChangeFrac->value());
  if (ui->cbCascCandChangeFrac->isChecked()) // default is 0.01
   MW->ReconstructionManager->ANNmodule->cascade_candidateChangeFraction(ui->bsCascCandChangeFrac->value());
  if (ui->cbCascOutStagnationEpochs->isChecked()) // default is 12
   MW->ReconstructionManager->ANNmodule->cascade_outputStagnationEpochs(ui->bsCascOutStagnationEpochs->value());
  if (ui->cbCascOutMinEpochs->isEnabled()) // default is 50
   MW->ReconstructionManager->ANNmodule->cascade_minOutEpochs(ui->bsCascOutMinEpochs->value());
  if (ui->cbCascOutMaxEpochs->isEnabled()) // default is 150
   MW->ReconstructionManager->ANNmodule->cascade_maxOutEpochs(ui->bsCascOutMaxEpochs->value());
  if (ui->cbCascCandStagnationEpochs->isChecked()) // default is 12
   MW->ReconstructionManager->ANNmodule->cascade_candidateStagnationEpochs(ui->bsCascCandStagnationEpochs->value());
  if (ui->cbCascCandMinEpochs->isEnabled()) // default is 50
   MW->ReconstructionManager->ANNmodule->cascade_minCandEpochs(ui->bsCascCandMinEpochs->value());
  if (ui->cbCascCandMaxEpochs->isEnabled()) // default is 150
   MW->ReconstructionManager->ANNmodule->cascade_maxCandEpochs(ui->bsCascCandMaxEpochs->value());
  if (ui->cbCascCandLimit->isChecked()) // default is 1000
   MW->ReconstructionManager->ANNmodule->cascade_candidateLimit(ui->bsCascCandLimit->value());
  if (ui->cbCascCandMult->isChecked()) // default is 0.4
   MW->ReconstructionManager->ANNmodule->cascade_weightMultiplier(ui->bsCascCandMult->value());
  if (ui->cbCascCandGroups->isChecked()) // default is 2
   MW->ReconstructionManager->ANNmodule->cascade_candidateGroups(ui->bsCascCandGroups->value());
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  if (ui->cbCascSteep->isChecked()){ // default is 0.25, 0.5, 0.75, 1
   vector<string> svalues; vector<fann_type> values;
   string text=ui->leCascSteep->text().toStdString();
   cSeparateValues(svalues,text,",",""); cConvertValues(svalues,values);
   MW->ReconstructionManager->ANNmodule->cascade_setActivationSteepness(values);
  } //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  if (ui->cbCascFunctions->isChecked()){ // default is SIGMOID,
   // SIGMOID SYMMETRIC, GAUSSIAN, GAUSSIAN SYMMETRIC, ELLIOT, ELLIOT SYMMETRIC
   vector<string> svalues; vector<fann_activationfunc_enum> values;
   string text=ui->leCascFunctions->text().toStdString();
   cSeparateValues(svalues,text,",",""); values.resize(svalues.size());
   for (unsigned i=0; i<svalues.size(); ++i)
    values[i]=NeuralNetworksModule::activationFunctions.option(cTrimSpaces(svalues[i]));
   MW->ReconstructionManager->ANNmodule->cascade_setActivationFunctions(values);
  } //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  if (sFunc==FANN_STOPFUNC_BIT) MW->ReconstructionManager->ANNmodule->bitFailLimit(ui->bsNNBitLimit->value());
  MW->ReconstructionManager->ANNmodule->train_maxStagEpochs(ui->bsNNTestStag->isEnabled()?ui->bsNNTestStag->value():0);
  MW->ReconstructionManager->ANNmodule->loadTrain(FTrainInput,ui->bsNNTestPerc->value()*0.01); // perc
  train_status(true); // toggle OFF GUI during train-
  MW->ReconstructionManager->ANNmodule->trainCascade(ui->bsNNMaxNeurons->value(),1,ui->bsNNTrainStop->value());
  train_status(false); // toggle ON GUI during train-
 //@ NN type dependencies @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 } else if (etype==nnStandard){ // Standard specific options.
  vector<unsigned> layers; build_layers(nIn,nOut,layers);
  if (layers.size()==0){ QMessageBox::warning(this,"NN Module",
   "Check hidden layers number/setting"); return; }
  if (!resume) MW->ReconstructionManager->ANNmodule->createStandard(layers);
  MW->ReconstructionManager->ANNmodule->trainAlgorithm(tAlg);
  MW->ReconstructionManager->ANNmodule->outputActivationFunc(oFunc);
  MW->ReconstructionManager->ANNmodule->hiddenActivationFunc(hFunc);
  MW->ReconstructionManager->ANNmodule->errorFunction(eFunc);
  MW->ReconstructionManager->ANNmodule->stopFunction(sFunc);
  if (sFunc==FANN_STOPFUNC_BIT) MW->ReconstructionManager->ANNmodule->bitFailLimit(ui->bsNNBitLimit->value());
  MW->ReconstructionManager->ANNmodule->train_maxStagEpochs(ui->bsNNTestStag->isEnabled()?ui->bsNNTestStag->value():0);
  MW->ReconstructionManager->ANNmodule->loadTrain(FTrainInput,ui->bsNNTestPerc->value()*0.01); // perc
  train_status(true); // toggle OFF GUI during train-
  MW->ReconstructionManager->ANNmodule->train(ui->bsNNMaxNeurons->value(),1,ui->bsNNTrainStop->value());
  train_status(false); // toggle ON GUI during train-
 //@ NN type dependencies @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 } else if (etype==nnShortcut){ // Shortcut specific options.
  vector<unsigned> layers; build_layers(nIn,nOut,layers);
  if (layers.size()==0){ QMessageBox::warning(this,"NN Module",
   "Check hidden layers number/setting"); return; }
  if (!resume) MW->ReconstructionManager->ANNmodule->createShortcut(layers);
  MW->ReconstructionManager->ANNmodule->trainAlgorithm(tAlg);
  MW->ReconstructionManager->ANNmodule->outputActivationFunc(oFunc);
  MW->ReconstructionManager->ANNmodule->hiddenActivationFunc(hFunc);
  MW->ReconstructionManager->ANNmodule->errorFunction(eFunc);
  MW->ReconstructionManager->ANNmodule->stopFunction(sFunc);
  if (sFunc==FANN_STOPFUNC_BIT) MW->ReconstructionManager->ANNmodule->bitFailLimit(ui->bsNNBitLimit->value());
  MW->ReconstructionManager->ANNmodule->train_maxStagEpochs(ui->bsNNTestStag->isEnabled()?ui->bsNNTestStag->value():0);
  MW->ReconstructionManager->ANNmodule->loadTrain(FTrainInput,ui->bsNNTestPerc->value()*0.01); // perc
  train_status(true); // toggle OFF GUI during train-
  MW->ReconstructionManager->ANNmodule->train(ui->bsNNMaxNeurons->value(),1,ui->bsNNTrainStop->value());
  train_status(false); // toggle ON GUI during train-
 } //..........................................................................
 MW->ReconstructionManager->ANNmodule->load(FTrainOutput); // load best (or last if test data is not used).
 emit MW->ReconstructionManager->ANNmodule->status("<BR>done!<BR><BR>");
 userChangeUIOpt(false);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! train a network. */
void NeuralNetworksWindow::on_pbNNTrain_released(){
 doTrain(false); ui->pbNNResume->setEnabled(true);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! Propagate the changes when the user changes the percentage of
//! training data to be used for testing.
void NeuralNetworksWindow::on_bsNNTestPerc_valueChanged(double val){
 ui->bsNNTestStag->setEnabled(val>0);
 userChangeUIOpt(true);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! Propagate the changes when the user changes the trainning bit limite
void NeuralNetworksWindow::on_bsNNBitLimit_valueChanged(double){
 userChangeUIOpt(true);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! Propagate the changes when the user changes the max number of
//! neurons to be used during training.
void NeuralNetworksWindow::on_bsNNMaxNeurons_valueChanged(int){
 userChangeUIOpt(true);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//! Propagate the changes when the user changes the train stop condition
void NeuralNetworksWindow::on_bsNNTrainStop_valueChanged(double){
 userChangeUIOpt(true);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! Loads a raw NN from disk. */

/*! ONGOING: this function is not yet completed and tested .... */

void NeuralNetworksWindow::on_pbNNLoad_released(){
QString file=QFileDialog::getOpenFileName(this,"Input Train File",
 FTrainOutput.c_str(),"NN Net File (*.net);;All Files (*)",0);
 if (!file.isEmpty()){

     /// !!!! SHOULD USE HERE THE JSON thing.


  MW->ReconstructionManager->ANNmodule->init_NNModule();
  MW->ReconstructionManager->ANNmodule->load(file.toStdString());

  // Probably it makes more sense to change options accordingly...
  // but there are things which are not saved in the file...
  // perhaps it would be a good idea to make a full json object at the NN
  // module to attach/include all the relevant info ... and easy the manipulation

  if (!MW->ReconstructionManager->ANNmodule->checkRecDimension()){
   QMessageBox::warning(this,"ANTS FANN Module Warning",
   "Check Reconstruction Dimensions (is Reconstruct Z/E (un)checked?)");
   MW->ReconstructionManager->ANNmodule->init_NNModule(); }
  ui->pbNNResume->setEnabled(true);
  userChangeUIOpt(false);
 }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_pbNNResume_released(){
 doTrain(true); ui->pbNNResume->setEnabled(true);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! User selection of the file build and used for training. */
void NeuralNetworksWindow::on_pbTrainInput_released(){
QString file=QFileDialog::getSaveFileName(this,"Output Train File",
 FTrainInput.c_str(),"Train File (*.train);;All Files (*)",0);
 if (!file.isEmpty()){ //......................................................
  FTrainInput=file.toStdString(); ui->lbTrainInput->setText(file);
  int w = ui->lbTrainInput->fontMetrics().width(file), W=ui->lbTrainInput->width();
  ui->lbTrainInput->setAlignment(w>W?Qt::AlignRight:Qt::AlignLeft);
} }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*! User selection of the output file from training. */
void NeuralNetworksWindow::on_pbTrainOutput_released(){
QString file=QFileDialog::getSaveFileName(this,"Input Train File",
 FTrainOutput.c_str(),"NN Net File (*.net);;All Files (*)",0);
 if (!file.isEmpty()){ //......................................................
  FTrainOutput=file.toStdString(); ui->lbTrainOutput->setText(file);
  int w = ui->lbTrainOutput->fontMetrics().width(file), W=ui->lbTrainOutput->width();
  ui->lbTrainOutput->setAlignment(w>W?Qt::AlignRight:Qt::AlignLeft);
} }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascOutChangeFrac_clicked(bool checked)
{ ui->bsCascOutChangeFrac->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascCandChangeFrac_clicked(bool checked)
{ ui->bsCascCandChangeFrac->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascOutStagnationEpochs_clicked(bool checked)
{ ui->bsCascOutStagnationEpochs->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascCandStagnationEpochs_clicked(bool checked)
{ ui->bsCascCandStagnationEpochs->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascCandLimit_clicked(bool checked)
{ ui->bsCascCandLimit->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascOutMinEpochs_clicked(bool checked)
{ ui->bsCascOutMinEpochs->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascOutMaxEpochs_clicked(bool checked)
{ ui->bsCascOutMaxEpochs->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascCandMinEpochs_clicked(bool checked)
{ ui->bsCascCandMinEpochs->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascCandMaxEpochs_clicked(bool checked)
{ ui->bsCascCandMaxEpochs->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascCandMult_clicked(bool checked)
{ ui->bsCascCandMult->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascSteep_clicked(bool checked)
{ ui->leCascSteep->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascFunctions_clicked(bool checked)
{ ui->leCascFunctions->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbCascCandGroups_clicked(bool checked)
{ ui->bsCascCandGroups->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbQPropDecay_clicked(bool checked)
{ ui->bsQPropDecay->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbQPropMu_clicked(bool checked)
{ ui->bsQPropMu->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbRPropIncFactor_clicked(bool checked)
{ ui->bsRPropIncFactor->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbRPropDecFactor_clicked(bool checked)
{ ui->bsRPropDecFactor->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbRPropDeltaMin_clicked(bool checked)
{ ui->bsRPropDeltaMin->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbRPropDeltaMax_clicked(bool checked)
{ ui->bsRPropDeltaMax->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::on_cbRPropDeltaZero_clicked(bool checked)
{ ui->bsRPropDeltaZero->setEnabled(checked); }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::status(QString mess){
 ui->ptOutput->appendHtml(mess);
 QCoreApplication::processEvents();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::userChangeUIOpt(bool isChanged){
 if (isChanged && MW->ReconstructionManager->ANNmodule && MW->ReconstructionManager->ANNmodule->isFANN()){
  setWindowTitle((string(window_title)+" (Changed)").c_str());
  ui->pbNNResume->setEnabled(false); FChanged=true;
 } else { FChanged=false; setWindowTitle(window_title); }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::onReconstruct3D(bool){
 if (MW->ReconstructionManager->ANNmodule){
 MW->ReconstructionManager->ANNmodule->setRecDimension(
  MW->Rwindow->isReconstructZ(),MW->Rwindow->isReconstructEnergy());
 if (MW->ReconstructionManager->ANNmodule->isFANN()) userChangeUIOpt(true);
} }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::onReconstructE(bool){
 if (MW->ReconstructionManager->ANNmodule){
  MW->ReconstructionManager->ANNmodule->setRecDimension(
   MW->Rwindow->isReconstructZ(),MW->Rwindow->isReconstructEnergy());
  if (MW->ReconstructionManager->ANNmodule->isFANN()) userChangeUIOpt(true);
} }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::setJSON(QJsonObject &json) const {
 json["XYZAutoScale"] = FXYZAutoScale;
 json["Type"] = ui->cbNNType->currentIndex();
 json["X_m"]=MW->ReconstructionManager->ANNmodule->scale_m(0);  json["X_b"]=MW->ReconstructionManager->ANNmodule->scale_b(0);
 json["Y_m"]=MW->ReconstructionManager->ANNmodule->scale_m(1);  json["Y_b"]=MW->ReconstructionManager->ANNmodule->scale_b(1);
 json["Z_m"]=MW->ReconstructionManager->ANNmodule->scale_m(2);  json["Z_b"]=MW->ReconstructionManager->ANNmodule->scale_b(2);
 json["HideActivationFunction"] = ui->cbNNHideFunction->currentIndex();
 json["OutActivationFunction"] = ui->cbNNOutFunction->currentIndex();
 json["TrainAlgorithm"] = ui->cbNNTrainAlg->currentIndex();
 json["TrainStopFunction"] = ui->cbNNTrainStop->currentIndex();
 json["TrainMaxNeurons"] = ui->bsNNMaxNeurons->value();
 json["TrainError"] = ui->cbNNTrainError->currentIndex();
 json["TrainStop"] = ui->bsNNTrainStop->value();
 json["BitLimit"] = ui->bsNNBitLimit->value();
 json["TestPercentage"] = ui->bsNNTestPerc->value();
 json["TestStagnation"] = ui->bsNNTestStag->value();
 json["TrainFileOut"] = ui->lbTrainOutput->text();
 json["TrainFileIn"] = ui->lbTrainInput->text();
 //----------------------------------------------------------------------------
 json["spaceDims"] = MW->ReconstructionManager->ANNmodule->spaceDims();
 json["energyDims"] = MW->ReconstructionManager->ANNmodule->energyDims();
 if (MW->ReconstructionManager->ANNmodule->isFANN()){
  cMemFile fanndata; fanndata.open();
  MW->ReconstructionManager->ANNmodule->save(fanndata); fanndata.flush();
  json["FANNData"] = QString(fanndata.text().c_str());
 } else json["FANNData"] = QString("");
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::saveJSON(QJsonObject &json) const { setJSON(json); }

/*===========================================================================*/
void NeuralNetworksWindow::saveJSON(string filename){
std::ofstream ofs (filename.c_str(),std::ofstream::out);
QJsonObject obj;  setJSON(obj);
 QJsonDocument saveDoc(obj);
 ofs << saveDoc.toJson().data();
 ofs.close();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::getJSON(QJsonObject &json){
JsonParser parser(json); bool b; int i, xyz, E; double d, m; QString s;
 parser.ParseObject("XYZAutoScale",b); FXYZAutoScale=b;
 parser.ParseObject("Type",i); ui->cbNNType->setCurrentIndex(i);
 parser.ParseObject("X_m",m); parser.ParseObject("X_b",d); MW->ReconstructionManager->ANNmodule->setRecScaling(0,m,d);
 parser.ParseObject("Y_m",m); parser.ParseObject("Y_b",d); MW->ReconstructionManager->ANNmodule->setRecScaling(1,m,d);
 parser.ParseObject("Z_m",m); parser.ParseObject("Z_b",d); MW->ReconstructionManager->ANNmodule->setRecScaling(2,m,d);
 parser.ParseObject("HideActivationFunction",i); ui->cbNNHideFunction->setCurrentIndex(i);
 parser.ParseObject("OutActivationFunction",i); ui->cbNNOutFunction->setCurrentIndex(i);
 parser.ParseObject("TrainAlgorithm",i); ui->cbNNTrainAlg->setCurrentIndex(i);
 parser.ParseObject("TrainStopFunction",i); ui->cbNNTrainStop->setCurrentIndex(i);
 parser.ParseObject("TrainMaxNeurons",i); ui->bsNNMaxNeurons->setValue(i);
 parser.ParseObject("TrainError",i); ui->cbNNTrainError->setCurrentIndex(i);
 parser.ParseObject("TrainStop",d); ui->bsNNTrainStop->setValue(d);
 parser.ParseObject("BitLimit",d); ui->bsNNBitLimit->setValue(d);
 parser.ParseObject("TestPercentage",d); ui->bsNNTestPerc->setValue(d);
 parser.ParseObject("TestStagnation",i); ui->bsNNTestStag->setValue(i);
 parser.ParseObject("TrainFileOut",s); ui->lbTrainOutput->setText(s);
 parser.ParseObject("TrainFileIn",s); ui->lbTrainInput->setText(s);
 //----------------------------------------------------------------------------
 parser.ParseObject("spaceDims",xyz);
 parser.ParseObject("energyDims",E);

 /// MUST BE UPDATED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 /// THIS WILL CAUSE TROUBLE --> ANDR CHANGE THE MEANING !!!!
 MW->ReconstructionManager->ANNmodule->setRecDimension(xyz,E);

 // MW->Rwindow->ui->cb

 /// From the main window, so this can be handle by the SIGNAL/SLOTS stuff
 /// MUST BE UPDATED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


 parser.ParseObject("FANNData",s);
 if (s.isEmpty()){ MW->ReconstructionManager->ANNmodule->init_NNModule(&FTrainOutput); } else {
  cMemFile fanndata; fanndata.open();
  fanndata.text(s.toStdString()); MW->ReconstructionManager->ANNmodule->load(fanndata); }
 userChangeUIOpt(false);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NeuralNetworksWindow::loadJSON(const QJsonObject &json){
 getJSON((QJsonObject&)json);
}

/*===========================================================================*/
void NeuralNetworksWindow::loadJSON(string filename){
QJsonObject obj; QFile loadFile(filename.c_str());
 loadFile.open(QIODevice::ReadOnly);
QByteArray saveData = loadFile.readAll();
QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
 obj=loadDoc.object(); getJSON(obj);
 loadFile.close();
}

/*===========================================================================*/
/*! Saves the log into a file. */
void NeuralNetworksWindow::on_pbNSaveOut_released(){
static QString pFile; QString file=QFileDialog::getSaveFileName(this,
 "Output Log File",pFile,"Log File (*.txt);;All Files (*)",0);
 if (!file.isEmpty()){ //......................................................
  QFile outfile; outfile.setFileName(pFile=file);
  outfile.open(QIODevice::Append|QIODevice::Text);
  QTextStream out(&outfile); out << ui->ptOutput->toPlainText() << endl;
} }
