#ifndef ACORESCRIPTINTERFACE_H
#define ACORESCRIPTINTERFACE_H

#include "ascriptinterface.h"

/// WATER'S INCLUDES
#include "readDiffusionVolume.h"
#include "diffusion.h"
//#include "hits2idealWaveform.h"
//#include "getImpulseResponse.h"
//#include "getWaveform.h"

#include <QVariant>
#include <QSet>
#include <QString>
//#include <QVector>

class AScriptManager;
class TRandom2;
class CurveFit;

//WATER'S TEST TYPEDEF
//typedef QVector<QString> a_sentence;
//typedef QVector<int> a_test; // compiles just fine
//typedef std::vector<int> another_test; // again, compiles just fine

//WATER'S ACTUAL TYPEDEFS

// not necessary apparently :^)

//WATER'S STRUCTS

// not necessary apparently :^)

class ACore_SI : public AScriptInterface
{
  Q_OBJECT

public:
  explicit ACore_SI(AScriptManager *ScriptManager);
  explicit ACore_SI(const ACore_SI& other);

  virtual bool IsMultithreadCapable() const override {return true;}

  void SetScriptManager(AScriptManager *NewScriptManager) {ScriptManager = NewScriptManager;}

public slots:
  //abort execution of the script
  void abort(QString message = "Aborted!");

  QVariant evaluate(QString script);

  //time
  void          sleep(int ms);
  int           elapsedTimeInMilliseconds();

  //output part of the script window
  //void print(QString text);
  void print(QVariant message);
  void printHTML(QVariant message);
  void clearText();
  QString str(double value, int precision);
  bool strIncludes(QString str, QString pattern);

  //time stamps
  QString GetTimeStamp();
  QString GetDateTimeStamp();

  //save to file
  bool createFile(QString fileName, bool AbortIfExists = true);
  bool isFileExists(QString fileName);
  bool deleteFile(QString fileName);
  bool createDir(QString path);
  QString getCurrentDir();
  bool setCirrentDir(QString path);
  bool save(QString fileName, QString str);
  bool saveArray(QString fileName, QVariant array);
  bool saveObject(QString FileName, QVariant Object, bool CanOverride);

  //load from file
  QVariant loadColumn(QString fileName, int column = 0); //load column of doubles from file and return it as an array
  QVariant loadArray(QString fileName, int columns);
  QVariant loadArray(QString fileName);
  QString  loadText(QString fileName);
  QVariant loadObject(QString fileName);

  QVariant loadArrayFromWeb(QString url, int msTimeout = 3000);

  //dirs
  QString GetWorkDir();
  QString GetScriptDir();
  QString GetExamplesDir();

  //file finder
  QVariant SetNewFileFinder(const QString dir, const QString fileNamePattern);
  QVariant GetNewFiles();

  //misc
  void processEvents();
  void reportProgress(int percents);

  void setCurveFitter(double min, double max, int nInt, QVariant x, QVariant y);
  double getFitted(double x);
  const QVariant getFittedArr(const QVariant array);
  
  // ===================================================================
  // WATER'S FUNCTIONS =================================================
  // ===================================================================
  
  // WATER'S TEST MEMBERS ----------------------------------------------
  
  // -- for readDiffusionVolume.h --------------------------------------
  
 // a_sentence makeSentenceWithWord(QString word, int times);
  //QString repeatAndJoin(QString word, int times);
  QVariant giveMap();
  
  // -- for diffusion.h ------------------------------------------------

  // -- for debugging --------------------------------------------------

  void shellMarker(QString in); // sends QString to shell

  // USEFUL STUFF ------------------------------------------------------
  
  inline QString getVarQtype(QVariant in){return in.typeName();};
  inline void await(int t_sec){sleep(t_sec*1000);};
  double minimumValueInList(QVariant in);
  //~ bool saveObj2Table(QString FileName, 
                    //~ QVariant Object, 
                        //~ bool CanOverride);

  // WATER'S DIFFUSION VOLUME ORIGINAL MEMBERS -------------------------
  
  // not necessary apparently :^)
  
  // WATER'S TRANSLATORS -----------------------------------------------
  
  // -- for readDiffusionVolume.h --------------------------------------
  
  QVariant a_VolumeInfo2QVariant(a_VolumeInfo in);
  QVariant a_Volume2QVariant(a_Volume in);
  QVariant a_Slice2QVariant(a_Slice in);
  
  // -- for diffusion.h ------------------------------------------------
  
  QVariant e_list2QVariantList(e_list in);
  
  // WATER'S INTERFACE -------------------------------------------------
  
  // -- for readDiffusionVolume.h --------------------------------------
  
  QVariant antsGetVolumeInfo(int V_ID, 
                         QString q_filename = "final"); // convert to std::string
  QVariant antsGetSlice(int V_ID,
                        int S_ID,
                    QString q_filename = "final");
  QVariant antsGetVolume(int V_ID,
                     QString q_filename = "final");
  
  // -- for diffusion.h ------------------------------------------------

  QVariant antsGetVolume(QString q_filename,
                          double depthFrac, // (0 == top, 1 == cathode)
                          double h = 1456,            // mm
                          double v_d = 2.0,           // mm/us
                          double sigma_Tcath = 0.59,  // mm
                          double sigma_Lcath = 0.85); // us

  // FOR CONVOLUTION OF IMPULSE RESPONSE WITH THE IDEAL WAVEFORM -------
  
  QVariant antsHits2idealWaveform(QVariant PMT_hits_lst, // QVariantList
                                    double ADC_reso = 5);// ns
	// hits2idealWaveform outputs a QVariantList
                                
  QVariant antsGetImpulseResponse  (QString PMTfilename,
                                     double ADC_reso = 5); // ns
	// getImpulseResponse outputs a QVariantList
  
  QVariant antsGetWaveform(QVariant idealWaveform,    // QVariantList
                           QVariant impulseResponse); // QVariantList
    // getWaveform outputs a QVariantList

  // ===================================================================
  // END OF WATER'S FUNCTIONS ==========================================
  // ===================================================================

  const QString StartExternalProcess(QString command, QVariant arguments, bool waitToFinish, int milliseconds);

private:
  AScriptManager* ScriptManager;

  //file finder
  QSet<QString>   Finder_FileNames;
  QString         Finder_Dir;
  QString         Finder_NamePattern = "*.*";

  CurveFit* CurF = 0;

  void addQVariantToString(const QVariant & var, QString & string) const;

};

#endif // ACORESCRIPTINTERFACE_H
