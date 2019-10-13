#ifndef READDIFFUSIONVOLUME_H
#define READDIFFUSIONVOLUME_H
#include <vector>
#include <string>

// typedefs ------------------------------------------------------------

typedef std::vector<std::string> a_sentence; // vector of short strings
typedef std::vector<std::string> a_poem;     // vector of long srings

typedef std::vector<int>      a_vector;
typedef std::vector<a_vector> a_matrix;
typedef std::vector<a_matrix> a_tensor;

// structs -------------------------------------------------------------

struct VolumeInfo{
	
	int ID;             // ID of the volume in question
	int entry_line;     // beginning line of the entry in file
	double dV[3] = { }; // {delta_x, delta_y, delta_z} in mm
	double sz[3] = { }; // {sz_x, sz_y, sz_z} in number of cells
	
};
typedef struct VolumeInfo a_VolumeInfo;

struct Volume{ // Volume -> 3D

	struct VolumeInfo V_info;  // info pertaining to volume in question	
	a_tensor V; // volume in question

};
typedef struct Volume a_Volume;

struct Slice{ // Slice -> 2D (made along Z)
	
	struct VolumeInfo V_info; // info about volume to which slice belongs
	int ID; // ID of the slice (top of volume == 0. top-1 == 1, etc.)
	a_matrix S; // S_ID'th slice of V_ID
	
};
typedef struct Slice a_Slice;

// interface -----------------------------------------------------------

a_VolumeInfo getVolumeInfo(int V_ID, 
                   std::string filename = "final");
                               
a_Slice      getSlice(int V_ID, 
                      int S_ID, 
              std::string filename = "final");
              
a_Volume     getVolume(int V_ID,
               std::string filename = "final");
                              
void writeSliceToFile(a_Slice in_Slice, 
                         bool give_info = false,
                  std::string out_filename = "slice");

// tools: general ------------------------------------------------------

std::string cleanUpFilename(std::string filename,
                                   bool input = true); // give abs. address

void skipLines(std::ifstream& stream, 
                          int n); // skip n lines in *stream
                          
void split(const std::string& s, 
                         char c, 
                  a_sentence& v);
                  
a_matrix getSlice_S(std::ifstream& stream, 
                               int sz_y, 
                               int curr_line, 
                               int target_slice);
                               
// tools: type converters ----------------------------------------------
                               
a_vector a_sentence2a_vector(a_sentence iv);

std::string a_vector2string(a_vector in_vector);

a_poem a_matrix2a_poem(a_matrix in_matrix);

// tools: abbreviations ------------------------------------------------

a_vector as2av(a_sentence iv);

// error returns -------------------------------------------------------

a_VolumeInfo errorVolumeInfo(int V_ID);

a_Volume     errorVolume(int V_ID);

a_Slice      errorSlice(int V_ID, 
                        int S_ID);

#endif // READDIFFUSIONVOLUME_H

