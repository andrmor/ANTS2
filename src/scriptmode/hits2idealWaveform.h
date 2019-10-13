#ifndef HITS2IDEALWAVEFORM_H
#define HITS2IDEALWAVEFORM_H

#include <Eigen/Dense>
#include <vector> // used only for string vectors
#include <string>

// === TYPEDEFS ========================================================

typedef Eigen::VectorXd a_hits_vec; // hits vector
typedef Eigen::VectorXi a_idxs_vec; // vector of indices
//typedef Eigen::VectorXi a_hist_vec; // histogram vector

// === STRUCTS =========================================================



// === MAIN FUNCTION ===================================================

std::vector<int> hits2IdealWaveform(std::vector<double> hits,       //ns
                                                 double resolution);//ns

// === HIGH LEVEL TOOLS ================================================

a_hits_vec stdVecD2a_hits_vec(std::vector<double> in);

a_idxs_vec a_hits_vec2a_idxs_vec(a_hits_vec in,
                                     double resolution);
                             
//a_hist_vec a_idxs_vec2a_hist_vec(a_idxs_vec in);
std::vector<int> a_idxs_vec2hist_stdVecI(a_idxs_vec in);

//std::vector<int> a_hist_vec2stdVecI(a_hist_vec in);

                          
// === MID LEVEL TOOLS =================================================

inline int aFloor(double d){return (int)d;};

// === LOW LEVEL TOOLS =================================================



// === ERROR RETURNS ===================================================

// ERRORS GO HERE

#endif // DIFFUSION_H 
