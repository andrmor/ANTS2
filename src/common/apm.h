#ifndef APM_H
#define APM_H

#include <QVector>

class QTextStream;
class ACustomRandomSampling;
class TH1D;

class APm
{
public:
    APm(double x, double y, double z, double psi, int type) : x(x), y(y), z(z), psi(psi), type(type) {}
    APm() {}
    ~APm(); //can be triggered on resize of the vector!!!

    void setCoords(const double *const xyz) { x = xyz[0]; y = xyz[1]; z = xyz[2]; }
    void saveCoords(QTextStream &file) const;
    void saveCoords2D(QTextStream &file) const;
    void setAngles(const double *const phithepsi) { phi = phithepsi[0]; theta = phithepsi[1]; psi = phithepsi[2]; }
    void saveAngles(QTextStream &file) const;

    void preparePHS();
    void clearSPePHSCustomDist();

    double x = 0;
    double y = 0;
    double z = 0;

    double phi = 0;
    double theta = 0;
    double psi = 0;

    int    type = 0;

    int    upperLower = 0;               // 0 - upper array, 1 - lower

    double relQE_PDE = 1.0;
    double relElStrength = 1.0;

    // preprocessing for ascii signal import
    double PreprocessingAdd = 0;
    double PreprocessingMultiply = 1.0;

    //  -- ELECTRONICS --
    // optical cross-talk for SiPM
    int    MCmodel = 0;
    QVector<double> MCcrosstalk;        //empty = not defined; otherwise should contain probabilityes of 1, 2, etc photoelectrons. Normalized to 1!
    ACustomRandomSampling* MCsampl = 0; //calculated before sim
    double MCtriggerProb = 0;           //calculated before sim
    // electronic noise
    double ElNoiseSigma = 0;
    // ADC
    double ADCmax = 65535;
    double ADCbits = 16;
    double ADCstep = 1.0;
    int    ADClevels = 65535;
    // PHS
    int    SPePHSmode = 0; //0 - use average value; 1 - normal distr; 2 - Gamma distr; 3 - custom distribution
    double AverageSigPerPhE = 1.0;
    double SPePHSsigma = 0;
    double SPePHSshape = 2.0;
    QVector<double> SPePHS_x; //custom distribution
    QVector<double> SPePHS;   //custom distribution
    TH1D* SPePHShist = 0;
    // ----

};


#endif // APM_H
