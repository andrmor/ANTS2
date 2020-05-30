#ifndef AINTERFACETOPHOTONSCRIPT_H
#define AINTERFACETOPHOTONSCRIPT_H

#include "localscriptinterfaces.h"
#include "ageneralsimsettings.h"
#include <QVector>
#include <vector>
#include <QVariant>

class AConfiguration;
class EventsDataClass;
class DetectorClass;
class APhotonTracer;
class TrackHolderClass;
class AOneEvent;
class TH1;

class APhoton_SI : public AScriptInterface
{
  Q_OBJECT
public:
    APhoton_SI (AConfiguration* Config, EventsDataClass *EventsDataHub);
    ~APhoton_SI();

    bool InitOnRun() override;

public slots:
    void ClearData();
    void ClearTracks();
    bool TracePhotons(int copies, double x, double y, double z, double vx, double vy, double vz, int iWave, double time, bool AddToPreviousEvent = false);
    bool TracePhotonsIsotropic(int copies, double x, double y, double z, int iWave, double time, bool AddToPreviousEvent = false);

    bool TracePhotonsS2Isotropic(int copies,
                                 double x, double y,
                                 double zStart, double zStop,
                                 int iWave, double time, double dZOverdT,
                                 bool AddToPreviousEvent = false);

    void SetBuildTracks(bool flag) {bBuildTracks = flag;}
    void SetTrackColor(int color) {TrackColor = color;}
    void SetTrackWidth(int width) {TrackWidth = width;}
    void SetMaxNumberTracks(int maxNumber) {MaxNumberTracks = maxNumber;}

    void SetHistoryFilters_Processes(QVariant MustInclude, QVariant MustNotInclude);
    void SetHistoryFilters_Volumes(QVariant MustInclude, QVariant MustNotInclude);
    void ClearHistoryFilters();

    void SetRandomGeneratorSeed(int seed);

    //photon loss statistics
    long GetBulkAbsorbed() const;  //Absorbed and BulkAbosrbed are the same
    long GetOverrideLoss() const;
    long GetHitPM() const;
    long GetHitDummy() const;
    long GetEscaped() const;
    long GetLossOnGrid() const;
    long GetTracingSkipped() const;
    long GetMaxCyclesReached() const;
    long GetGeneratedOutsideGeometry() const;
    long GetStoppedByMonitor() const;
    //optical processes
    long GetFresnelTransmitted() const;
    long GetFresnelReflected() const;
    long GetRayleigh() const;
    long GetReemitted() const;

    //history record
    int GetHistoryLength() const;    
    QVariant GetHistory() const;
    void DeleteHistoryRecord(int iPhoton);
    bool SaveHistoryToFile(QString FileName, bool AllowAppend, int StartFrom);

    void AddTrackFromHistory(int iPhoton, int TrackColor, int TrackWidth);
    int GetNumberOfTracks() const;

    QString GetProcessName(int NodeType);
    QString PrintRecord(int iPhoton, int iRecord);
    QString PrintAllDefinedProcessTypes();
    QString PrintAllDefinedRecordMemebers();

private:
    AConfiguration* Config;
    EventsDataClass *EventsDataHub;
    DetectorClass* Detector;

    APhotonTracer* Tracer;

    AGeneralSimSettings simSet;

    bool bBuildTracks;
    int TrackColor;
    int TrackWidth;
    int MaxNumberTracks;
    std::vector<TrackHolderClass *> Tracks;
    AOneEvent* Event;

    void clearTrackHolder();
    void normalizeVector(double *arr);
    bool initTracer();
    void processTracks();
    void handleEventData(bool AddToPreviousEvent);
};

#endif // AINTERFACETOPHOTONSCRIPT_H
