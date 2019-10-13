#include <iostream>
#include <readDiffusionVolume.h>

#include <functional>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#include <algorithm> // for copy
#include <iterator>  // for ostream_iterator

using namespace std;

// ---------------------------------------------------------------------
// interface -----------------------------------------------------------
// ---------------------------------------------------------------------

a_VolumeInfo getVolumeInfo(int V_ID,
                        string filename){

	a_VolumeInfo V_info; // output struct

	V_info.ID = V_ID;

	// -- open file -- 
	
	string filename_absolute_address = cleanUpFilename(filename);
	ifstream infile(filename_absolute_address);

	// -- get file info (line 1 of file) --

	string file_info_string; // first line of file contains file info
	if(!std::getline(infile, file_info_string)){
		
		cout << "getVolumeInfo error: file not found";
		cout << endl;
		
		return errorVolumeInfo(V_info.ID);
		
	} 
	
	a_sentence file_info_sentence;
	split(file_info_string, ';', file_info_sentence);
	/*
	 * file_info_vector[0] == line number (1 in this case)
	 * file_info_vector[1] == lgthFile
	 * file_info_vector[2] == lgthHeader
	 * file_info_vector[3] == nBlobs
	 */
	
	int nBlobs = stoi(file_info_sentence[3]);
	
	if(V_info.ID > nBlobs){
		
		cout << "getVolumeInfo error: volume ID exceeds number of ";
		cout << "volumes in file";
		cout << endl;
		
		return errorVolumeInfo(V_info.ID);
		
	}

	// -- read V_ID's entry ref in index (line 2 + V_ID of file) --

	skipLines(infile, V_ID); // line 2 of file is a label, idx after
	
	string V_ID_index_entry_string;
	if(!std::getline(infile, V_ID_index_entry_string)){
		
		cout << "getVolumeInfo error: could not find entry reference ";
		cout << "in index";
		cout << endl;
		
		return errorVolumeInfo(V_info.ID);
		
	} 
	
	a_sentence V_ID_index_entry_sentence;
	split(V_ID_index_entry_string, ';', V_ID_index_entry_sentence);
	/*
	 * V_ID_index_entry_vector[0] == line number (2 + V_ID in this case)
	 * V_ID_index_entry_vector[1] == V_ID_entry_line_number
	 */
	
	V_info.entry_line = stoi(V_ID_index_entry_sentence[1]);

	// -- read V_ID's entry (line V_info.entry_line, 
	// infile currently at line 2 + V_ID) --
	
    skipLines(infile, V_info.entry_line - V_ID - 3);
    
	string V_dV_string; // first line of volume entry -> line;dx;dy;dz
	if(!std::getline(infile, V_dV_string)){
		
		cout << "getVolumeInfo error: could not find entry -> dV";
		cout << endl;
		
		return errorVolumeInfo(V_info.ID);
		
	} 	

	a_sentence V_dV_sentence;
	split(V_dV_string, ';', V_dV_sentence);

	// cout << "current line: " << V_dV_vector[0];
	// cout << " , intended line: " << V_info.entry_line << endl;
	
	//~ cout << "V_dV_sentence[1]: " << V_dV_sentence[1] << endl;
		
	std::replace(V_dV_sentence[1].begin(), V_dV_sentence[1].end(), '.', ',');
	V_info.dV[0] = stod(V_dV_sentence[1]);
	std::replace(V_dV_sentence[2].begin(), V_dV_sentence[2].end(), '.', ',');
	V_info.dV[1] = stod(V_dV_sentence[2]);
	std::replace(V_dV_sentence[3].begin(), V_dV_sentence[3].end(), '.', ',');
	V_info.dV[2] = stod(V_dV_sentence[3]);

	//~ cout << "stod(V_dV_sentence[1]): " << std::stod(V_dV_sentence[1]) << endl;
	//~ cout << "V_dV[0]: " << V_info.dV[0] << endl;

	string V_sz_string;
	if(!std::getline(infile, V_sz_string)){
		
		cout << "getVolumeInfo error: could not find entry -> sz";
		cout << endl;
		
		return errorVolumeInfo(V_info.ID);
		
	} 
	
	a_sentence V_sz_sentence;
	split(V_sz_string, ';', V_sz_sentence);
	
	// cout << "current line: " << V_sz_vector[0];
	// cout << " , intended line: " << V_info.entry_line << endl;

	V_info.sz[0] = stoi(V_sz_sentence[1]);
	V_info.sz[1] = stoi(V_sz_sentence[2]);
	V_info.sz[2] = stoi(V_sz_sentence[3]);

	infile.close();

	return V_info;
	
}

a_Slice getSlice(int V_ID, 
                 int S_ID,
              string filename){
	
	a_Slice S;
	
	S.V_info = getVolumeInfo(V_ID, filename);
	S.ID = S_ID;
	
	// setup parameters to be sent into getSlice_S and then run it
	
	// 	stream
	string filename_absolute_address = cleanUpFilename(filename);
	ifstream infile(filename_absolute_address);
	
	// 	sz_y
	int sz_y = S.V_info.sz[1];
	
	//  curr_line
	int e_l = S.V_info.entry_line;  // get desired curr_line
	skipLines(infile, e_l-1);       // update stream accordingly
	
	//  target_line
	int slice_thickness = sz_y + 1; // account for subdivider
	int target_line = e_l + 3 + (S_ID-1)*slice_thickness;
	
	S.S = getSlice_S(infile, sz_y, e_l, target_line);
	
	infile.close();
	
	return S;

}

a_Volume getVolume(int V_ID, 
                string filename){
	
	a_Volume V;
	
	cout << "getVolume: V_ID: " << V_ID << endl;
	
	V.V_info = getVolumeInfo(V_ID, filename);
	
	// setup parameters to be sent into getSlice_S and then run in loop
	
	//  stream
	string filename_absolute_address = cleanUpFilename(filename);
	ifstream infile(filename_absolute_address);
	
	//  sz_y
	int sz_y = V.V_info.sz[1];
	cout << "getVolume: sz_y: " << sz_y << endl;
	
	//  curr_line
	int e_l = V.V_info.entry_line;
	skipLines(infile, e_l + 2); // e_l + 1 so as to skip entry header
	int curr_line = e_l + 3;

	//  target_line
	int slice_thickness = sz_y + 1;
    // in this case, target_line = curr_line;

	int sz_z = V.V_info.sz[2];
	cout << "getVolume: sz_z: " << sz_z << endl;
	int old_line;
	for (int S_ID = 1 ; S_ID <= sz_z ; S_ID++){
	
		//~ cout << "getVolume: S_ID: " << S_ID << endl;
	
		V.V.push_back(getSlice_S(infile, sz_y, curr_line, curr_line));
		
		skipLines(infile, 1); // skip subdivider
		
		//~ old_line = curr_line;
		curr_line += slice_thickness;
		
		//~ cout << "getVolume: increment: " << curr_line-old_line << endl;
	}
	
	infile.close();
	
	return V;
	
}

void writeSliceToFile(a_Slice in_Slice,
                         bool give_info,
                       string out_filename){
						   
	a_poem txt_poem = a_matrix2a_poem(in_Slice.S);
	
	// make filename into address
	string address = cleanUpFilename(out_filename, false);
	
	// send txt_poem into file, line by line
	ofstream file;
	file.open(address);
	if(give_info){
	
		file << "volume ID: " << in_Slice.V_info.ID;
		file << " slice ID: " << in_Slice.ID << "\n";
		file << "dx: " << in_Slice.V_info.dV[0];
		file << " dy: " << in_Slice.V_info.dV[1] << "\n";
		file << "sz_x: " << in_Slice.V_info.sz[0];
		file << " sz_y: " << in_Slice.V_info.sz[1] << "\n";
		
	}
	
	for(int i = 0; i < txt_poem.size() ; i++){
	
		file << txt_poem[i] << "\n";
	}
	
	file.close();

}

// ---------------------------------------------------------------------
// tools: general ------------------------------------------------------
// ---------------------------------------------------------------------

string cleanUpFilename(string filename,
                         bool input){
	
	string address;
	
	a_sentence input_sentence;
	split(filename, '/', input_sentence);
	
	string filename_extension_string = input_sentence.back();
	a_sentence filename_extension_sentence;
	split(filename_extension_string, '.' , filename_extension_sentence);
	
	string clean_filename = filename_extension_sentence.front();

	if(input && filename_extension_sentence.size() == 1){
			
		address = "/home/andrey/Prog/ants2/inputFiles/" + clean_filename + ".txt";
		
	} else if (input && filename_extension_sentence.size() > 1){
		
		address = "/home/andrey/Prog/ants2/inputFiles/" + filename_extension_string;
		
	} else if (!input){ // output file
	
		address = "/home/andrey/Prog/ants2/inputFiles/" + clean_filename + ".txt";
		//~ cout << address << endl;
		
	} else {
	
		cout << "error: invalid filename";
		address = "ERROR";
		
	}

	return address;

}

void skipLines(ifstream& stream, 
                     int n){
	
	string line;
	int i = 1;
	
	while(i <= n && std::getline(stream, line)){
			
		i++;
	}
}

void split(const string& s, 
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

a_matrix getSlice_S(ifstream& stream, 
                          int sz_y, 
                          int curr_line, 
                          int target_slice){

	a_matrix target_matrix;

	a_matrix error_matrix = {{-1}};

	// reach appropriate line in the ifstream to read slice

	//~ cout << "getSlice_S: curr_line: " << curr_line << endl;
	//~ cout << "getSlice_S: target_slice: " << target_slice << endl;
	//~ cout << "getSlice_S: sz_y/2: " << sz_y/2 << endl;


	if (curr_line < target_slice){
	
		skipLines(stream, target_slice - curr_line);
		
	} else if (curr_line > target_slice){ // FIND HOWTO BACKTRACK STREAM
	
		cout << "getSlice_S error: feature not present for now. Sorry.";
		cout << endl;
		
		return error_matrix;
    }

	// pass the slice into target_matrix
	
	/* curr_line   = lineNumber;sliceVector */
	/* sliceVector = num\tnum\tnum\t... */
	
	string     curr_line_string;
	a_sentence curr_line_sentence;
	
	string     curr_sliceVector_string;
	a_sentence curr_sliceVector_sentence;
	a_vector   curr_sliceVector_vector; 
	
	for (int j = 0 ; j < sz_y ; j++){
	
		// read line from file
	
		if(!std::getline(stream, curr_line_string)){
			
			cout << "getSlice_S error: could not find line ";
			cout << j << " of slice at line ";
			cout << stoi(curr_line_sentence[0]) + 1;
			cout << endl;
			
			return error_matrix;
			
		} 	
		
		//~ cout << "getSlice_S: curr_line_string: " << curr_line_string << endl;
		
		// interpret the line, withdraw the relevant data
		
		curr_line_sentence.clear();
		split(curr_line_string,';',curr_line_sentence);
		
		//~ if(j == 0){		
			//~ cout << "getSline_S: j: " << j;
			//~ cout << " curr_line_sentence[0]: ";
			//~ cout << curr_line_sentence[0] << endl;
		//~ }
		
		//~ if(j == sz_y/2){		
			//~ cout << "getLine_S: j: " << j;
			//~ cout << " curr_line_sentence[0]: ";
			//~ cout << curr_line_sentence[0] << endl;
		//~ }
		
		curr_sliceVector_string = curr_line_sentence[1];	
		
		//~ if(j == sz_y/2){
		
			//~ cout << "getLine_S: curr_line_sentence[1]: ";
			//~ cout << curr_line_sentence[1] << endl;
			
		//~ }
			
		curr_sliceVector_sentence.clear();	
		split(curr_sliceVector_string, '\t', curr_sliceVector_sentence);		
		curr_sliceVector_vector = as2av(curr_sliceVector_sentence);
		
		// append to target_matrix
	
		target_matrix.push_back(curr_sliceVector_vector);
		
	}

	return target_matrix;

}

// ---------------------------------------------------------------------
// tools: type onverters -----------------------------------------------
// ---------------------------------------------------------------------

a_vector a_sentence2a_vector(a_sentence input_vector){

	a_vector target_vector;
	
	for(auto &s : input_vector){
		
		std::stringstream parser(s);
		int x = 0;
		parser >> x;
			
		target_vector.push_back(x);
	}
	
	return target_vector;
	
}

string a_vector2string(a_vector in_vector){

	a_vector iv = in_vector;
	string out_string;
	stringstream stream;
	
	copy( iv.begin(), iv.end(), ostream_iterator<int>(stream, "\t") );
	
	out_string = stream.str();
	
	return out_string.substr(0, out_string.length()-1); // trailing \t
	
}

a_poem a_matrix2a_poem(a_matrix in_matrix){
	
	a_poem out_poem;
	string curr_line;
	
	for(int i = 0 ; i < in_matrix.size() ; i++){
	
		curr_line = a_vector2string(in_matrix[i]);
		out_poem.push_back( curr_line );
		
	}
	
	return out_poem;
	
}

// ---------------------------------------------------------------------
// tools: abbreviations ------------------------------------------------
// ---------------------------------------------------------------------

a_vector as2av(a_sentence iv){

	return a_sentence2a_vector(iv);
	
}

// ---------------------------------------------------------------------
// error returns -------------------------------------------------------
// ---------------------------------------------------------------------

a_VolumeInfo errorVolumeInfo(int V_ID){

	a_VolumeInfo error_VolumeInfo;
	
	error_VolumeInfo.ID = V_ID;
	
	error_VolumeInfo.entry_line = -1;
	
	error_VolumeInfo.dV[0] = -1;
	error_VolumeInfo.dV[1] = -1;
	error_VolumeInfo.dV[2] = -1;
		
	error_VolumeInfo.sz[0] = -1;
	error_VolumeInfo.sz[1] = -1;
	error_VolumeInfo.sz[2] = -1;
	
	return error_VolumeInfo;
	
}

a_Volume errorVolume(int V_ID){

	a_Volume error_Volume;
	
	error_Volume.V_info = errorVolumeInfo(V_ID);
	error_Volume.V = {{{-1}}};
	
	return error_Volume;
	
}

a_Slice errorSlice(int V_ID, 
                   int S_ID){

	a_Slice error_Slice;
	
	error_Slice.V_info = errorVolumeInfo(V_ID);
	error_Slice.ID = S_ID;
	error_Slice.S = {{-1}};
	
	return error_Slice;
	
}
