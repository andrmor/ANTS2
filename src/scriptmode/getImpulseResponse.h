#ifndef GETIMPULSERESPONSE_H
#define GETIMPULSERESPONSE_H

//~ #include <Eigen/Dense>
//~ #include <vector> // used only for string vectors
#include <QVariant> // used only for string vectors
#include <string>

// === TYPEDEFS ========================================================



// === STRUCTS =========================================================

struct PMT{
	
	//~ std::vector<double> v; // signal
	//~ std::vector<double> t; // time
	QVariantList v; // signal
	QVariantList t; // time
	
};
typedef struct PMT AS_PMT;

// === MAIN FUNCTION ===================================================

//~ std::vector<double> getImpulseResponse(std::string filename,
                                            //~ double resolution);

QVariantList getImpulseResponse(std::string filename,
                                     double resolution);

// === HIGH LEVEL TOOLS ================================================

AS_PMT getInputPMT(std::string filename);

AS_PMT getSamplPMT(AS_PMT inPMT,
                   double resolution);
                          
// === MID LEVEL TOOLS =================================================

//~ std::vector<double> getPMTrawV(std::string filename);
QVariantList getPMTrawV(std::string filename);

//~ std::vector<double> getPMTt_vec(int size,
                             //~ double interval); 
QVariantList getPMTt_vec(int size,
                      double interval);                              

double interpolation(int idx,
                  AS_PMT inPMT,
                  AS_PMT outPMT);

// === LOW LEVEL TOOLS =================================================

double getInterval(double total_time,
                      int t_vec_size);

int getNearestIn_idx(double time,
                     AS_PMT inPMT);

inline int aCeil(double d){return (int)d + 1;};

// === ERROR RETURNS ===================================================

// ERRORS GO HERE

#endif // DIFFUSION_H 
