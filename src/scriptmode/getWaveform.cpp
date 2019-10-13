#include <iostream>
#include "getWaveform.h"

//~ #include <vector> // used only for string vectors
#include <QVariant> // used only for string vectors
#include <string>

using namespace std;

// === MAIN FUNCTION ===================================================

//~ vector<double> getWaveform(vector<double> signal,
                           //~ vector<double> impulseResponse){
	
	//~ int L_h = impulseResponse.size();
	//~ int L_x = signal.size();
	//~ int L_y = L_h + L_x - 1;
	
	//~ vector<double> y(L_y, 0);
	
	//~ vector<double> h = getHpad(impulseResponse, L_x, L_h);
	
	//~ for(int n = 0; n < L_y ; n++){
		
		//~ for(int k  = 0 ; k < L_x; k++){
			
			//~ y[n] += signal[k] * h[L_x+n-k];
			
		//~ }
		
	//~ }
			
	//~ return y;
//~ }

QVariantList getWaveform(QVariant signal,
                         QVariant impulseResponse){
	
	QVariantList signal_qvl = signal.toList();
	QVariantList impulseResponse_qvl = impulseResponse.toList();
	
	QVariantList x;
	QVariantList h_noPad;
	
	if(signal_qvl.size() >= impulseResponse_qvl.size()){
		
		x       = impulseResponse_qvl;
		h_noPad = signal_qvl;
		
	} else {
		
		x       = signal_qvl;
		h_noPad = impulseResponse_qvl;
		
	}
	
	int L_h = h_noPad.size();
	int L_x = x.size();
	int L_y = L_h + L_x - 1;
	
	QVariantList y = aZeros(L_y);
	
	QVariantList h = getHpad(h_noPad, L_x, L_h);
	cout << "GW: L_x + 2*L_h = " << L_x + 2*L_h << ", h.size() = " << h.size() << endl; 
	cout << "GW: L_y = " << L_y << ", y.size() = " << y.size() << endl; 
	
	//~ int count = 0;
	
	for(int n = 0; n < L_y ; n++){
		
		for(int k  = 0 ; k < L_x; k++){
			
			//~ cout << "GW: L_y-1 - n = " << L_y-1 - n << ", L_x-1 - k = " << L_x-1 - k << ", h.size()-1 - (L_x+n-k) = " << h.size()-1 - (L_x+n-k) << endl;
			//~ count++;
			//~ cout << "GW: count = " << count << endl;
			
			//~ cout << "GW: y[" << n << "] = " << y[n].toDouble();
			//~ cout << ", GW: h[" << L_x+n-k << "] = " << h[L_x+n-k].toDouble() << endl;
			y[n] = y[n].toDouble() + x[k].toDouble() * h[L_x+n-k].toDouble();
			
		}
		
	}
			
	return y;
}

// === HIGH LEVEL TOOLS ================================================

//~ vector<double> getHpad(vector<double> impResp,
                                  //~ int L_x,
                                  //~ int L_h){

	//~ vector<double> h(L_x,0);
	//~ h.insert(h.end(), impResp.begin(), impResp.end());
	//~ vector<double> hPadR(L_h,0);
	//~ h.insert(h.end(), hPadR.begin(),hPadR.end());

	//~ return h;

//~ }

QVariantList getHpad(QVariantList hNoPad,
                              int L_x,
                              int L_h){

	cout << "GHP: impResp.size() = " << hNoPad.size() << ", L_h = " << L_h << endl; 

	QVariantList h = aZeros(L_x);
	h.append(hNoPad);
	cout << "GHP: L_x + L_h = " << L_x + L_h << ", h.size() = " << h.size() << endl; 
	
	QVariantList hPadR = aZeros(L_h);
	h.append(hPadR);
	cout << "GHP: L_x + 2*L_h = " << L_x + 2*L_h << ", h.size() = " << h.size() << endl; 
	
	return h;

}

QVariantList aZeros(int length){

	QVariantList out;
	
	for(int i = 0 ; i < length; i++){
	
		out.push_back(0);
		
	}
	
	cout << "aZeros: length = " << length << ", out.size() = " << out.size() << endl;
	return out;
}

// === MID LEVEL TOOLS =================================================


// === LOW LEVEL TOOLS =================================================

