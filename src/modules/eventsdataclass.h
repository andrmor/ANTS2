#ifndef EVENTSDATACLASS_H
#define EVENTSDATACLASS_H

#ifdef SIM
#include "asimulationstatistics.h"
#include "ahistoryrecords.h"
#endif

#include "generalsimsettings.h"
#include "reconstructionsettings.h"
#include "manifesthandling.h"

#include <QVector>
#include <QObject>
#include <TString.h>

class TTree;
class APmHub;
struct AScanRecord;
struct AReconRecord;
class TRandom2;
class QJsonObject;

class EventsDataClass : public QObject
{
  Q_OBJECT
public:
    EventsDataClass(const TString nameID = ""); //nameaddon to make unique hist names in multithread
    ~EventsDataClass();

    // True/calibration positions
    QVector<AScanRecord*> Scan;
    int ScanNumberOfRuns; //number of runs performed at each position (node) - see simulation module
    bool isScanEmpty() const {return Scan.isEmpty();}
    void clearScan();
    void purge1e10events();  //added after introduction of multithread to remove nodes outside the defined volume - they are marked as true x and y = 1e10

    // PM signal data
    QVector< QVector <float> > Events; //[event][pm]  - remember, if events energy is loaded, one MORE CHANNEL IS ADDED: last channel is numPMs+1
    QVector< QVector < QVector <float> > > TimedEvents; //event timebin pm
    int  countEvents() const {return Events.size();}
    bool isEmpty() const {return Events.isEmpty();}
    bool isTimed() const {return !TimedEvents.isEmpty();}
    int  getTimeBins() const;
    int  getNumPMs() const;
    const QVector<float> *getEvent(int iev) const;
    const QVector<QVector<float> > *getTimedEvent(int iev);

#ifdef SIM
    // Logs
    QVector<EventHistoryStructure*> EventHistory;
    void saveEventHistoryToTree(const QString& fileName) const;
    QVector<GeneratedPhotonsHistoryStructure> GeneratedPhotonsHistory;

    //Detection statistics
    ASimulationStatistics* SimStat;
    bool isStatEmpty() const {return SimStat->isEmpty();}
    void initializeSimStat(QVector<const AGeoObject *> monitorRecords, int numBins, int waveNodes);
#endif

    //Reconstruction data
    QVector< QVector<AReconRecord*> > ReconstructionData;  // [sensor_group] : [event]
    //QVector<bool> fReconstructionDataReady; // true if reconstruction was already performed for the group
    bool fReconstructionDataReady; // true if reconstruction was already performed for the group
    bool isReconstructionDataEmpty(int igroup = 0) const;  // container is empty
    bool isReconstructionReady(int igroup = 0) const;  // reconstruction was not yet performed
    void clearReconstruction();
    void clearReconstruction(int igroup);
    void createDefaultReconstructionData(int igroup = 0); //recreates new container - clears data completely!
    void resetReconstructionData(int numGgroups);         //reinitialize numPoints to 1 for each event. Keeps eventId intact!
    bool BlurReconstructionData(int type, double sigma, TRandom2 *RandGen, int igroup = -1); // 0 - uniform, 1 - gauss; igroup<0 -> apply to all groups
    bool BlurReconstructionDataZ(int type, double sigma, TRandom2 *RandGen, int igroup = -1); // 0 - uniform, 1 - gauss; igroup<0 -> apply to all groups
    void PurgeFilteredEvents(int igroup = 0);
    void Purge(int OnePer, int igroup = 0);    
    int  countGoodEvents(int igroup = 0) const;
    void copyTrueToReconstructed(int igroup = 0);
    void copyReconstructedToTrue(int igroup = 0);
    void prepareStatisticsForEvents(const bool isAllLRFsDefined, int &GoodEvents, double &AvChi2, double &AvDeviation, int igroup = 0);

    bool packEventsToByteArray(int from, int to, QByteArray &ba) const;
    bool unpackEventsFromByteArray(const QByteArray &ba);  // run by server - so no events range selection
    bool packReconstructedToByteArray(QByteArray &ba) const;
    bool unpackReconstructedFromByteArray(int from, int to, const QByteArray &ba);  //run by client -> should respect the event range

    //load data can have manifest file with holes/slits
    QVector<ManifestItemBaseClass*> Manifest;
    void clearManifest();

    // simulation setting for events / scan
    GeneralSimSettings LastSimSet;   
    bool fSimulatedData; // true if events data were simulated (false if loaded data)

    // reconstruction settings used during last reconstruction
    QVector<ReconstructionSettings> RecSettings;

    //Trees
    TTree *ReconstructionTree; //tree with reconstruction data
    void clearReconstructionTree();
    bool createReconstructionTree(APmHub* PMs,
                                  bool fIncludePMsignals=true,
                                  bool fIncludeRho=true,
                                  bool fIncludeTrue=true,
                                  int igroup = 0);
    TTree* ResolutionTree; //tree with resolution data
    void clearResolutionTree();
    bool createResolutionTree(int igroup = 0);

    //Data Save
    bool saveReconstructionAsTree(QString fileName,
                                  APmHub *PMs,
                                  bool fIncludePMsignals=true,
                                  bool fIncludeRho = true,
                                  bool fIncludeTrue = true,                                  
                                  int igroup = 0);
    bool saveReconstructionAsText(QString fileName, int igroup=0);
    bool saveSimulationAsTree(QString fileName);
    bool saveSimulationAsText(const QString &fileName, bool addNumPhotons, bool addPositions);

    //Data Load - ascii
    bool fLoadedEventsHaveEnergyInfo;
    int loadEventsFromTxtFile(QString fileName, QJsonObject &jsonPreprocessJson, APmHub *PMs); //returns -1 if failed, otherwise number of events added

    //data load - Tree
    int loadSimulatedEventsFromTree(QString fileName, const APmHub &PMs, int maxEvents = -1); //returns -1 if failed, otherwise number of events added
    bool overlayAsciiFile(QString fileName, bool fAddMulti, APmHub *PMs); //true = success, if not, see ErrorString

    //Misc
    QString ErrorString;

    void squeeze();          

public slots:
    void clear();
    void onRequestStopLoad();

private:
    int fStopLoadRequested;

signals:
    void loaded(int events, int progress);
    void requestClearKNNfilter();
    void cleared();
    void requestEventsGuiUpdate();

    void requestFilterEvents(); //use by remote rec event loader
};

#endif // EVENTSDATACLASS_H
