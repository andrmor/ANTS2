#ifndef GETWAVEFORM_H
#define GETWAVEFORM_H

//~ #include <Eigen/Dense>
//~ #include <vector> // used only for string vectors
#include <QVariant> // used only for string vectors
#include <string>

// === TYPEDEFS ========================================================



// === STRUCTS =========================================================



// === MAIN FUNCTION ===================================================

//~ std::vector<double> getWaveform(std::vector<double> signal,
                                //~ std::vector<double> impulseResponse);
QVariantList getWaveform(QVariant signal,
                         QVariant impulseResponse);                                
  // convolves the two signals, but needed a fancy name so that it
  // wouldn't clash with any other functions that may exist that do the
  // same thing.

// === HIGH LEVEL TOOLS ================================================

//~ std::vector<double> getHpad(std::vector<double> impResp,
                                            //~ int L_x,
                                            //~ int L_h);

QVariantList getHpad(QVariantList impResp,
                              int L_x,
                              int L_h);
                              
QVariantList aZeros(int length);
                          
// === MID LEVEL TOOLS =================================================



// === LOW LEVEL TOOLS =================================================



// === ERROR RETURNS ===================================================

// ERRORS GO HERE

#endif // DIFFUSION_H 
