#include <iostream>
#include "hits2idealWaveform.h"
#include <vector>
#include <string>

// Eigen includes
#include <Eigen/Core>
#include <Eigen/Dense>

using namespace std;

// === MAIN FUNCTION ===================================================

vector<int> hits2IdealWaveform(vector<double> hits,  // times in ns
                                       double resolution){ // in ns
													 
	vector<int> out;
	
	a_hits_vec hits_vec = stdVecD2a_hits_vec(hits);
	a_idxs_vec idxs_vec = a_hits_vec2a_idxs_vec(hits_vec, resolution);
	//a_hist_vec hist_vec = a_idxs_vec2a_hist_vec(idxs_vec);
	//out = a_hist_vec2stdVecI(hist_vec);
	out = a_idxs_vec2hist_stdVecI(idxs_vec);
	
	return out;
													 
}

// === HIGH LEVEL TOOLS ================================================

a_hits_vec stdVecD2a_hits_vec(vector<double> in){

	double* ptr = &in[0];
	Eigen::Map<a_hits_vec> out(ptr,in.size());
	
	return out;
	
}

a_idxs_vec a_hits_vec2a_idxs_vec(a_hits_vec in,
                                     double resolution){

	a_idxs_vec out(in.size());
	
	//~ cout << "a_hits_vec2a_idxs_vec: in.maxCoeff() = " << in.maxCoeff();
	//~ cout << endl;
	//~ cout << "a_hits_vec2a_idxs_vec: in.minCoeff() = " << in.minCoeff();
	//~ cout << endl;
		
	double invReso = 1.0/resolution;
	//~ in = in * 1.3424; // DEBUG
	double min = in.minCoeff();
	in = in.array() - min; // allowing negative idxs caused bin error
	in = in*invReso;
	
	out << in.unaryExpr(ptr_fun(aFloor));
	
	return out;

}


vector<int> a_idxs_vec2hist_stdVecI(a_idxs_vec in){
	
	int max_in = in.maxCoeff();
	//~ cout << "a_idxs_vec2hist_stdVecI: max_in = " << max_in << endl;
	
	int min_in = in.minCoeff();
	//~ cout << "a_idxs_vec2hist_stdVecI: min_in = " << min_in << endl;
	
	int size_out = max_in - min_in + 1;
	//~ cout << "a_idxs_vec2hist_stdVecI: size_out = " << size_out << endl;
	
	
	vector<int> out(size_out,0);
	
	for(int i = 0; i < in.size() ; i++){
		
		//~ out[in(i)-min_in] += 1;
		out[in(i)] += 1;
		
	}
	
	return out;
	
}

//~ vector<int> a_hist_vec2stdVecI(a_hist_vec in){

	//~ vector<int> out(in.data(), in.data() + in.rows() * in.cols());
	
	//~ return out;
	
//~ }

// === MID LEVEL TOOLS =================================================



// === LOW LEVEL TOOLS =================================================





// === ERROR RETURNS ===================================================
