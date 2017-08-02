#ifndef AINTERFACETOPHOTONSCRIPT_H
#define AINTERFACETOPHOTONSCRIPT_H

#include "scriptinterfaces.h"
#include "generalsimsettings.h"
#include <QVector>
#include <QVariant>

class AConfiguration;
class EventsDataClass;
class DetectorClass;
class APhotonTracer;
class TrackHolderClass;
class OneEventClass;
class TH1;

class AInterfaceToPhotonScript : public AScriptInterface
{
  Q_OBJECT
public:
    AInterfaceToPhotonScript (AConfiguration* Config, EventsDataClass *EventsDataHub);
    ~AInterfaceToPhotonScript();

public slots:
    void ClearData();
    void ClearTracks();
    bool TracePhotons(int copies, double x, double y, double z, double vx, double vy, double vz, int iWave, double time);
    bool TracePhotonsIsotropic(int copies, double x, double y, double z, int iWave, double time);

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

    GeneralSimSettings simSet;

    bool bBuildTracks;
    int TrackColor;
    int TrackWidth;
    int MaxNumberTracks;
    QVector<TrackHolderClass *> Tracks;
    OneEventClass* Event;

    void clearTrackHolder();
    void normalizeVector(double *arr);
    bool initTracer();
    void processTracks();
};

#endif // AINTERFACETOPHOTONSCRIPT_H
