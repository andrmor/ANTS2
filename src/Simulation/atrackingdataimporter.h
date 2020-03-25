#ifndef ATRACKINGDATAIMPORTER_H
#define ATRACKINGDATAIMPORTER_H

#include <vector>
#include <QString>
#include <QMap>

class AEventTrackingRecord;
class AParticleTrackingRecord;
class TrackHolderClass;
class ATrackBuildOptions;
class QFile;
class QTextStream;

class ATrackingDataImporter
{
public:
    ATrackingDataImporter(const ATrackBuildOptions & TrackBuildOptions,
                          const QStringList & ParticleNames,
                          std::vector<AEventTrackingRecord*> * History,
                          std::vector<TrackHolderClass *> * Tracks,
                          int maxTracks);
    ~ATrackingDataImporter();

    const QString processFile(const QString & FileName, int StartEvent, bool bBinary = false);

private:
    const ATrackBuildOptions & TrackBuildOptions;
    const QStringList ParticleNames;
    std::vector<AEventTrackingRecord *> * History = nullptr; // if nullptr - do not collect history
    std::vector<TrackHolderClass *> * Tracks = nullptr;      // if nullptr - do not extract tracks
    bool bBinaryInput = false;
    int MaxTracks = 1000;

    AEventTrackingRecord * CurrentEventRecord = nullptr;      // history of the current event
    AParticleTrackingRecord * CurrentParticleRecord = nullptr;  // current particle - can be primary or secondary
    QMap<int, AParticleTrackingRecord *> PromisedSecondaries;   // <index in file, secondary AEventTrackingRecord *>
    TrackHolderClass * CurrentTrack = nullptr;

    enum Status {ExpectingEvent, ExpectingTrack, ExpectingStep, TrackOngoing};

    Status CurrentStatus = ExpectingEvent;
    int ExpectedEvent;

    QString Error;

    //resources for ascii input
    QFile *       inTextFile = nullptr;
    QTextStream * inTextStream = nullptr;
    QString       currentLine;

    //resources for binary input
    std::ifstream * inStream = nullptr;
    int           G4NextEventId = -1;

private:
    bool isEndReached() const;
    void readBuffer();
    bool isNewEvent();
    bool isNewTrack();

    void processNewEvent();
    void processNewTrack();
    void processNewStep();

    bool isErrorInPromises();

    void prepareImportResources(const QString & FileName);
    void clearImportResources();

};

#endif // ATRACKINGDATAIMPORTER_H
