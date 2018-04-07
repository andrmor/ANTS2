#ifndef PMS_H
#define PMS_H

#include "apm.h"
#include "pmtypeclass.h"

#include <QVector>
#include <QString>
#include <QPair>

class TRandom2;
class AMaterialParticleCollection;
class GeneralSimSettings;
class APmPosAngTypeRecord;
class AGammaRandomGenerator;
class QJsonObject;

class pms
{   
public:
    pms(AMaterialParticleCollection* materialCollection, TRandom2* randGen);
    ~pms();

    void configure(GeneralSimSettings* SimSet); // must do before simulation

    // top level json save/load
    void writePMtypesToJson(QJsonObject &json);
    bool readPMtypesFromJson(QJsonObject &json);
    void writeInividualOverridesToJson(QJsonObject &json);
    bool readInividualOverridesFromJson(QJsonObject &json);
    void writeElectronicsToJson(QJsonObject &json);
    bool readElectronicsFromJson(QJsonObject &json);

      //recalculate PDE for selected wave binning
    void RebinPDEs(); //triggered by changing between true and false for WavelenegthResolved on the MainWindow
    void RebinPDEsForType(int itype);
    void RebinPDEsForPM(int ipm);
    //recalculate angular for selected angle binning
    void RecalculateAngular();
    void RecalculateAngularForType(int typ);
    void RecalculateAngularForPM(int ipm);
      //prepare pulse height spectra hists
    void preparePHSs();
      // prepare MCcrosstalk
    void prepareMCcrosstalk();
    void prepareMCcrosstalkForPM(int ipm);
      //prepare ADC
    void updateADClevels();

      // Single PM signal using photoelectron spectrum
    double GenerateSignalFromOnePhotoelectron(int ipm);  //used only in GUI of mainwindow!
      // used during simulation:
    double getActualPDE(int ipm, int WaveIndex) const; //returns partial probability to be detected (wave-resolved PDE)
    double getActualAngularResponse(int ipm, double cosAngle) const; //returns partial probability to be detected (vs angular response - cos of refracted beam)
    double getActualAreaResponse(int ipm, double x, double y); //returns partial probability to be detected (vs area response - x, y in local coordinates of the PM)

    int count() const {return numPMs;} //returns number of PMs
    void clear();
    void resetOverrides();
    void add(int upperlower, double xx, double yy, double zz, double Psi, int typ);
    void add(int upperlower, APmPosAngTypeRecord* pat);
    void insert(int ipm, int upperlower, double xx, double yy, double zz, double Psi, int typ);
    void remove(int ipm); //removes PM number i

    //New memory layout stuff
    bool saveCoords(const QString &filename);
    bool saveAngles(const QString &filename);
    bool saveTypes(const QString &filename);
    bool saveUpperLower(const QString &filename);

    inline APm& at(int ipm) {return PMs[ipm];}
    inline const APm& at(int ipm) const {return PMs.at(ipm);}

    //accelerator
    double getMaxQE() const {return MaxQE;}
    double getMaxQEvsWave(int iWave) const {return MaxQEvsWave.at(iWave);}

    //individual PMs
    inline double X(int ipm) const       {return PMs.at(ipm).x;}
    inline double Y(int ipm) const       {return PMs.at(ipm).y;}
    inline double Z(int ipm) const       {return PMs.at(ipm).z;}
    inline int    PixelsX(int ipm) const {return PMtypes.at(PMs.at(ipm).type)->PixelsX;}
    inline int    PixelsY(int ipm) const {return PMtypes.at(PMs.at(ipm).type)->PixelsY;}
    inline double SizeX(int ipm) const   {return PMtypes.at(PMs.at(ipm).type)->SizeX;}
    inline double SizeY(int ipm) const   {return PMtypes.at(PMs.at(ipm).type)->SizeY;}
    inline double SizeZ(int ipm) const   {return PMtypes.at(PMs.at(ipm).type)->SizeZ;}
    inline bool   isSiPM(int ipm) const  {return PMtypes.at(PMs.at(ipm).type)->SiPM;}

    //PDE
    bool   isPDEwaveOverriden(int ipm) const;
    bool   isPDEwaveOverriden() const;
    bool   isPDEeffectiveOverriden() const;
    void   writePDEeffectiveToJson(QJsonObject &json);
    bool   readPDEeffectiveFromJson(QJsonObject &json);
    void   writePDEwaveToJson(QJsonObject &json);
    bool   readPDEwaveFromJson(QJsonObject &json);
    void   setPDEwave(int ipm, QVector<double>* x, QVector<double>* y);

    //Angular response
    bool   isAngularOverriden(int ipm) const;
    bool   isAngularOverriden() const;
    void   writeAngularToJson(QJsonObject &json);
    bool   readAngularFromJson(QJsonObject &json);
    void   setAngular(int ipm, QVector<double>* x, QVector<double>* y);

    //Area response
    bool   isAreaOverriden(int ipm) const;
    bool   isAreaOverriden() const;
    void   writeAreaToJson(QJsonObject &json);
    bool   readAreaFromJson(QJsonObject &json);
    void   setArea(int ipm, QVector<QVector <double> > *vec, double xStep, double yStep);

    //type properties    
    const PMtypeClass* getType(int itype) const {return PMtypes.at(itype);}
    PMtypeClass* getType(int itype) {return PMtypes[itype];}
    const PMtypeClass* getTypeForPM(int ipm) const {return PMtypes.at(PMs.at(ipm).type);}
    bool removePMtype(int itype);
    void appendNewPMtype(PMtypeClass* tp);
    int  countPMtypes() const {return PMtypes.size();}
    int  findPMtype(QString typeName) const;
    void clearPMtypes();
    void replaceType(int itype, PMtypeClass *newType);
    void updateTypePDE(int typ, QVector<double> *x, QVector<double> *y);
    void scaleTypePDE(int typ, double factor);
    void updateTypeAngular(int typ, QVector<double> *x, QVector<double> *y);
    void updateTypeAngularN1(int typ, double val);
    void updateTypeArea(int typ, QVector<QVector <double> > *vec, double xStep, double yStep);
    void clearTypeArea(int typ);

    void CalculateElChannelsStrength();

    void SetWave(bool wavelengthResolved, double waveFrom, double waveStep, int waveNodes);
    void setDoPHS(bool flag) {fDoPHS = flag;}
    void setDoMCcrosstalk(bool flag) {fDoMCcrosstalk = flag;}
    void setDoElNoise(bool flag) {fDoElNoise = flag;}
    void setDoADC(bool flag) {fDoADC = flag;}
    void setMeasurementTime(double time) {MeasurementTime = time;}

    bool   isWaveRes() const {return WavelengthResolved;}
    double getWaveFrom() const  {return WaveFrom;}
    double getWaveStep() const  {return WaveStep;}
    int    getWaveNodes() const {return WaveNodes;}

    bool isDoPHS() const {return fDoPHS;}
    bool isDoMCcrosstalk() const {return fDoMCcrosstalk;}
    bool isDoElNoise() const {return fDoElNoise;}
    bool isDoADC() const {return fDoADC;}

    double getMeasurementTime() const {return MeasurementTime;}

    QVector<QPair<double, int> > getPMsSortedByR() const;

private:
    TRandom2* RandGen;
    AMaterialParticleCollection* MaterialCollection;
    AGammaRandomGenerator* GammaRandomGen;

    int numPMs = 0;
    QVector<APm> PMs;

    double MeasurementTime = 150;  // measurement time to calculate dark counts for SiPMs

    //flags for the current simulation mode
    bool WavelengthResolved = false;
    bool AngularResolved = false;
    bool AreaResolved = false;
    bool fDoPHS = false;
    bool fDoMCcrosstalk = false;
    bool fDoElNoise = false;
    bool fDoADC = false;

    // binning settings for wave-resolved data
    double WaveFrom = 200.0;
    double WaveStep = 5.0;
    int    WaveNodes = 121;

    int    CosBins = 1000;  //number of bins for angular response

    //defined base PM types
    QVector<PMtypeClass*> PMtypes;

    double MaxQE = 0; //accelerator, not wave-resolved value
    QVector<double> MaxQEvsWave; //vs wavelength
    void calculateMaxQEs(); // vector has to be rebinned first according to the actual wave properties (see configure())

    void writeRelQE_PDE(QJsonObject &json);
    void readRelQE_PDE(QJsonObject &json);
};

#endif // PMS_H
