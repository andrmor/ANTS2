#ifndef ATRACKINGDATAIMPORTER_H
#define ATRACKINGDATAIMPORTER_H

#include <vector>
#include <QString>
#include <QMap>

class AEventTrackingRecord;
class AParticleTrackingRecord;
class TrackHolderClass;
class ATrackBuildOptions;

class ATrackingDataImporter
{
public:
    ATrackingDataImporter(const ATrackBuildOptions & TrackBuildOptions, std::vector<AEventTrackingRecord*> * History, std::vector<TrackHolderClass *> * Tracks);

    const QString processFile(const QString & FileName, int StartEvent);

private:
    const ATrackBuildOptions & TrackBuildOptions;
    std::vector<AEventTrackingRecord *> * History = nullptr; // if 0 - do not collect history
    std::vector<TrackHolderClass *> * Tracks = nullptr;      // if 0 - do not extract tracks

    QString currentLine;
    AEventTrackingRecord * CurrentEventRecord = nullptr;      // history of the current event
    AParticleTrackingRecord * CurrentParticleRecord = nullptr;  // current particle - can be primary or secondary
    QMap<int, AParticleTrackingRecord *> PromisedSecondaries;   // <index in file, secondary AEventTrackingRecord *>
    TrackHolderClass * CurrentTrack = nullptr;

    enum Status {ExpectingEvent, ExpectingTrack, ExpectingStep, TrackOngoing};

    Status CurrentStatus = ExpectingEvent;
    int ExpectedEvent;

    QString Error;

    void processNewEvent();
    void processNewTrack();
    void processNewStep();

    bool isErrorInPromises();

};

#endif // ATRACKINGDATAIMPORTER_H
