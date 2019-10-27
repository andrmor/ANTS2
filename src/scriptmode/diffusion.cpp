#include <iostream>
#include "diffusion.h"
#include <vector> // for a_sentence
#include <string> 

#include <algorithm>
#include <iterator>
#include <functional>
#include <sstream>
#include <fstream>
#include <cmath> 

// Eigen
#include <Eigen/Dense>

// ROOT
#include "TRandom.h"
#include "TMath.h"

using namespace std;

// === MAIN FUNCTION ===================================================

e_list diffusion(std::string filename,
                      double depthFrac,    // (0 == top, 1 == cathode)
                      double h,            // mm
                      double v_d,          // mm/us
                      double sigma_Tcath,  // mm
                      double sigma_Lcath){ // us
	
    string diagnosticsFilename = "/home/vova/Work/ANTS2/ANTS2-LXe/";
    diagnosticsFilename       += "ANTS2/diffusionDiagnostics.txt";

    Params theParams = constructTheParams(depthFrac, h, v_d,
	                                      sigma_Tcath, sigma_Lcath);
	
    e_list eventList = getEventList(filename, diagnosticsFilename);
	
	e_list dispList = getDisplacementList(eventList.rows(), theParams);

	e_list output = displace(eventList, dispList);

	return output;

}

// === HIGH LEVEL TOOLS ================================================

// --- input -----------------------------------------------------------

e_list getEventList(string filename,
                    string diagnosticsFilename){
	
	e_list eventList;
	
	int eventListRows = getEventListRows(filename);
	//~ cout << "GEL --> eventListRows: " << eventListRows << endl;
	eventList.resize(eventListRows,3);
	
	int cR = 0;
	
	ifstream infile(filename);
    ofstream diagnosticsFile;
    diagnosticsFile.open(diagnosticsFilename,
                         std::ofstream::out | std::ofstream::trunc);


	//~ int count = 0;
	
	while(true){
		
		//~ count++;
		
		//~ cout << "GEL --> count: " << count << endl;
		
        e_list thisRowEventList = getThisRowEventList(infile,
                                                      diagnosticsFile);
		
		if(thisRowEventList.cols() > 1){ // not yet the end of file
			
			for(int i = cR ; i < cR+thisRowEventList.rows(); i++){
				
				//~ cout << "GEL --> i-currRow: " << i-cR << endl;
				
				eventList.row(i) = thisRowEventList.row(i-cR);
				
			}
			
			cR += thisRowEventList.rows();
			//~ cout << "GEL --> currRow: " << currRow << endl;
			
		} else { // reached end of file
			
			cout << "reached end of file" << endl;

			break;
			
		}		
	}
	
	infile.close();
    diagnosticsFile.close();
	
	return eventList;
	
}

// --- displacement vector ---------------------------------------------

e_list getDisplacementList(int n_electrons,
                        Params theParams){

	e_list displacementList(n_electrons, 3);
	TRandom *rnd = new TRandom(time(0));

	for(int i = 0 ; i < n_electrons ; i++){
		
		displacementList.row(i) = getDisplacementVector(theParams, rnd);

	}

	return displacementList;
							
}

e_pos getDisplacementVector(Params theParams,
                          TRandom* rnd){

	e_pos displacementVector;

	e_xy Tdisp = getTdisplacement(theParams, rnd);
	e_z  Ldisp = getLdisplacement(theParams, rnd);
	
	displacementVector = cat2displacement(Tdisp, Ldisp);
	
	return displacementVector;
}

// === MID LEVEL TOOLS =================================================

// --- reading input ---------------------------------------------------

int getEventListRows(std::string filename){

	int eventListRows = 0;
	ifstream infile(filename);
	
	string thisRow_str;
	a_sentence thisRow_sen;
	
	while(std::getline(infile, thisRow_str)){
		
		split2(thisRow_str,'\t',thisRow_sen);
		if(thisRow_sen.size() > 1){
			//~ cout << "GELR --> stod tR_s: " << std::stod(thisRow_sen[4]);
			//~ cout << endl;
            /*std::replace(thisRow_sen[4].begin(), thisRow_sen[4].end(),
                         '.', ',');*/
			eventListRows += std::stod(thisRow_sen[4]);
			//~ cout << "GELR --> eventListRows: " << eventListRows << endl;
			
		}
		thisRow_sen.clear();
		
	}
	
	infile.close();
	return eventListRows;
	
}

e_list getThisRowEventList(std::ifstream& stream,
                           std::ofstream& diagnosticsStream){
	
	e_pos thisRow_pos;
	int thisN_electrons;
	
	string thisRow_str;
	
	a_sentence thisRow_sen;
	
	std::getline(stream, thisRow_str);
	//~ cout << "GTREL --> thisRow_str: " << thisRow_str << endl;
	split2(thisRow_str,'\t',thisRow_sen);
	//~ cout << "GTREL --> thisRow_sen[1]: " << thisRow_sen[1] << endl;
	
	e_list thisRowEventList;
	
	if(thisRow_sen.size() > 1){ // not end of input file yet
	
        /*
		std::replace(thisRow_sen[1].begin(), thisRow_sen[1].end(),
			         '.', ',');
		std::replace(thisRow_sen[2].begin(), thisRow_sen[2].end(),
			         '.', ',');
		std::replace(thisRow_sen[3].begin(), thisRow_sen[3].end(),
			         '.', ',');
		std::replace(thisRow_sen[4].begin(), thisRow_sen[4].end(),
			         '.', ',');
        */
		thisRow_pos << std::stod(thisRow_sen[1]),
		               std::stod(thisRow_sen[2]),
		               std::stod(thisRow_sen[3]);
		thisN_electrons = std::stod(thisRow_sen[4]);
		
		//~ cout << "GTREL--> thisN_electrons: " << thisN_electrons << endl;
		
        gTREL_diagnostics(thisRow_pos, diagnosticsStream);

		thisRowEventList.resize(thisN_electrons, 3);
		
		for(int i = 0; i < thisN_electrons ; i++){
			
			thisRowEventList.row(i) = thisRow_pos;
			
		}

	} else { // reached end of input file
		
		cout << "reached end of file" << endl;
		
		e_list thisRowEventList(1,1);
		thisRowEventList << 0;
		
	}

	return thisRowEventList;

}

// --- diffusion function ----------------------------------------------

e_xy getTdisplacement(Params theParams,
                    TRandom* rnd){

	e_xy Tdisplacement;
	
    double x;
    double y;
	
	double s_T2 = pow(theParams.s_T,2);
	double B = sqrt(2 * s_T2);
	
    x = rnd->Gaus(0,B);
    y = rnd->Gaus(0,B);
    
	Tdisplacement << x,y;
	
	return Tdisplacement;
	
}

e_z  getLdisplacement(Params theParams,
                    TRandom* rnd){

	e_z Ldisplacement;
	
	double s_L2 = pow(theParams.s_L,2);
	double B = sqrt(2 * s_L2);
	Ldisplacement = rnd->Gaus(0,B);
	
	return Ldisplacement;
	
}

// === LOW LEVEL TOOLS =================================================

// --- operations ------------------------------------------------------

void split2(const string& s, 
                    char c, 
             a_sentence& v){ // from O'reilly
	
	string::size_type i = 0;
	string::size_type j = s.find(c);

	while (j != string::npos) {
		v.push_back(s.substr(i, j-i));
		i = ++j;
		j = s.find(c, j);

		if (j == string::npos)
			v.push_back(s.substr(i, s.length()));
	}
	
	if(v.size() < 1){ // if no matches, return the entire string
      
		v.push_back(s);
       
	}
	
}

e_pos cat2displacement(e_xy Tdisplacement,
                        e_z Ldisplacement){

	e_pos displacementVector;
	
	displacementVector << Tdisplacement, Ldisplacement;
	
	return displacementVector;

}

e_list displace(e_list eventList,
                e_list displacementList){

	e_list displacedElectronList;
	
	displacedElectronList = eventList + displacementList;
	
	return displacedElectronList;
					
}

// --- converters ------------------------------------------------------



// --- prepare parameters ----------------------------------------------

Params constructTheParams(double depthFrac,
                          double h,            // mm
                          double v_d,          // mm/us
                          double sigma_Tcath,  // mm
                          double sigma_Lcath){ // us

	Params theParams;
	
	theParams.depthFrac = depthFrac;
	theParams.h = h;
	theParams.v_d = v_d;
	theParams.s_T = getSigma_T(depthFrac, sigma_Tcath);
	theParams.s_L = getSigma_L(depthFrac, v_d, sigma_Lcath);	
	
	return theParams;

}

sigma_T getSigma_T(double depthFrac, // transverse diffusion coefficient
                   double sigma_Tcath){

	sigma_T s_T;
	
	s_T = sigma_Tcath * depthFrac;
	
	return s_T;

}

sigma_L getSigma_L(double depthFrac, // longitudinal diffusion coeff
                   double v_d,
                   double sigma_Lcath){

	sigma_L s_L;
	
	s_L = sigma_Lcath * v_d * depthFrac;
	
	return s_L;

}

// ------ diagnostics --------------------------------------------------

void gTREL_diagnostics(e_pos thisRow_pos,
              std::ofstream& diagnosticsStream){

    diagnosticsStream << to_string(thisRow_pos(0)) << '\t';
    diagnosticsStream << to_string(thisRow_pos(1)) << '\t';
    diagnosticsStream << to_string(thisRow_pos(2)) << '\t';
    diagnosticsStream << '\n';
}
