#include <iostream>
#include "getImpulseResponse.h"

//~ #include <vector> // used only for string vectors
#include <QVariant> // used only for string vectors
#include <string>

#include <algorithm>
#include <iterator>
#include <functional>
#include <sstream>
#include <fstream>
#include <cmath> 

using namespace std;

// === MAIN FUNCTION ===================================================

//~ vector<double> getImpulseResponse(string filename,
                                  //~ double resolution){

	//~ vector<double> out;
	
	//~ AS_PMT inPMT  = getInputPMT(filename);
	//~ AS_PMT outPMT = getSamplPMT(inPMT, resolution);
	//~ out = outPMT.v;
	
	//~ return out;

//~ }

QVariantList getImpulseResponse(string filename,
                                double resolution){

	QVariantList out;
	
	//~ cout << "gIR->gIR: getInputPMT" << endl;
	AS_PMT inPMT  = getInputPMT(filename);
	//~ cout << "gIR->gIR: getSamplPMT" << endl;
	AS_PMT outPMT = getSamplPMT(inPMT, resolution);
	out = outPMT.v;
	cout << "gIR->gIR: exiting" << endl;
	return out;

}

// === HIGH LEVEL TOOLS ================================================

AS_PMT getInputPMT(string filename){

	double total_time = 100; // ns
	
	AS_PMT inputPMT;
	//~ cout << "gIR->gIPMT: getPMTrawV" << endl;
	inputPMT.v = getPMTrawV(filename);
	
	//~ cout << "gIR->gIPMT: getInterval" << endl;
	double inpPMT_interval = getInterval(total_time, inputPMT.v.size());
	//~ cout << "gIR->gIPMT: getPMTt_vec" << endl;
	inputPMT.t = getPMTt_vec(inputPMT.v.size(), inpPMT_interval);
	
	return inputPMT;

}

AS_PMT getSamplPMT(AS_PMT inPMT,
                   double resolution){

	AS_PMT outPMT;
	
	//~ int size = aCeil(inPMT.t.back()/resolution);
	//~ cout << "gIR->gSPMT: aCeil" << endl;
	int size = aCeil(inPMT.t.back().toDouble()/resolution);
	//~ cout << "gIR->gSPMT: getPMTt_vec" << endl;
	outPMT.t = getPMTt_vec(size, resolution); 
	
	cout << "gIR->gSPMT: populating sampled vec" << endl;
	outPMT.v.push_back(inPMT.v[0]);// 0th elememt needs no interpolation
	
	for(int i = 1 ; i < outPMT.t.size() ; i++){ // i = 1 bc of prev line
		
		cout << "gIR->gSPMT: i = " << i << ", interpolation" << endl;
		outPMT.v.push_back(interpolation(i, inPMT, outPMT));
		
	}
	
	return outPMT;

}

// === MID LEVEL TOOLS =================================================

//~ vector<double> getPMTrawV(string filename){
	
	//~ vector<double> PMTrawV;
	
	//~ ifstream infile(filename);
	//~ string thisRow_str;
	//~ while(std::getline(infile, thisRow_str)){
		//~ std::replace(thisRow_str.begin(),thisRow_str.end(),'.',',');
		//~ PMTrawV.push_back(std::stod(thisRow_str));
		
	//~ }
	
	//~ infile.close();
	
	//~ return PMTrawV;
	
//~ }

QVariantList getPMTrawV(string filename){
	
	QVariantList PMTrawV;
	
	//~ cout << "gIR->gPMTrV: importing PMT.txt" << endl;
	ifstream infile(filename);
	string thisRow_str;
	while(std::getline(infile, thisRow_str)){
		std::replace(thisRow_str.begin(),thisRow_str.end(),'.',',');
		PMTrawV.push_back(std::stod(thisRow_str));
		
	}
	
	infile.close();
	
	return PMTrawV;
	
}

//~ vector<double> getPMTt_vec(int size, 
                        //~ double interval){
	
	//~ vector<double> PMTt_vec;
	
	//~ for(int i = 0; i < size ; i++){
		
		//~ PMTt_vec.push_back(i*interval);
		
	//~ }
	
	//~ return PMTt_vec;
	
//~ }

QVariantList getPMTt_vec(int size, 
                      double interval){
	
	QVariantList PMTt_vec;
	//~ cout << "gIR->gPMTt_vec: making time vector" << endl;
	
	for(int i = 0; i < size ; i++){
		
		PMTt_vec.push_back(i*interval);
		
	}
	
	return PMTt_vec;
	
}

double interpolation(int idx,
                  AS_PMT inPMT,
                  AS_PMT outPMT){

	double v;
	
	//~ double t_ns = outPMT.t[idx];
	double t_ns = outPMT.t[idx].toDouble();
	
	//~ cout << "gIR->I: getNearestIn_idx" << endl;
	int inL_idx = getNearestIn_idx(t_ns, inPMT);
	//~ cout << "gIR->I: inL_idx = " << inL_idx << endl;
	
	//~ double inL_t = inPMT.t[inL_idx];
	//~ double inL_v = inPMT.v[inL_idx];
	//~ double inR_v = inPMT.v[inL_idx+1];
	//~ cout << "gIR->I: inL_t = inPMT.t[inL_idx]" << endl;
	double inL_t = inPMT.t[inL_idx].toDouble();
	//~ cout << "gIR->I: inL_v = inPMT.v[inL_idx]" << endl;
	double inL_v = inPMT.v[inL_idx].toDouble();
	cout << "gIR->I: inR_v = inPMT.v[" << inL_idx+1 << "] = " << inPMT.v[inL_idx+1].toDouble() << endl;
	cout << "gIR->I: inR_v = inPMT.v.size() = " << inPMT.v.size() << endl;
	double inR_v = inPMT.v[inL_idx+1].toDouble();
	
	//~ cout << "gIR->I: calculating interpolated value" << endl;
	v = inL_v + (inR_v-inL_v)*(t_ns - inL_t);
	
	return v;
}

// === LOW LEVEL TOOLS =================================================

double getInterval(double total_time,
                      int t_vec_size){

	return (total_time/(t_vec_size-1));
						  
}

int getNearestIn_idx(double aTime,
                     AS_PMT inPMT){

	int nearestIn_idx;
	
	//~ cout << "gIR->gNI_idx: getting input resolution" << endl;
	//~ double in_reso = inPMT.t[1];
	double in_reso = inPMT.t[1].toDouble();
	
	//~ cout << "gIR->gNI_idx: aCeil" << endl;
	nearestIn_idx = aCeil(aTime/in_reso) - 2;
	//~ cout << "gIR->gNI_idx: exited aCeil" << endl;
	return nearestIn_idx;
					  
}
