#ifndef ATRACKINGDATAIMPORTER_H
#define ATRACKINGDATAIMPORTER_H

#include <QString>
#include <QVector>
#include <vector>

class ATrackingHistory;
class TrackHolderClass;
class ATrackBuildOptions;

class ATrackingDataImporter
{
public:
    ATrackingDataImporter(const ATrackBuildOptions & TrackBuildOptions, std::vector<ATrackingHistory*> * History, QVector<TrackHolderClass*> * Tracks);

    const QString processFile(const QString & FileName, int StartEvent, int numEvents);

private:
    std::vector<ATrackingHistory*> * History = 0;
    QVector<TrackHolderClass*> * Tracks = 0;
    const ATrackBuildOptions & TrackBuildOptions;

    QString currentLine;

    void processNewEvent();
    void processNewParticle();
    void processNewStep();

    enum Status {Init, ExpectingTrack, TrackStarted, TrackOngoing};

    Status CurrentStatus = Init;
    int ExpectedEvent;

    QString Error;

private:
    TrackHolderClass * CurrentTrack = nullptr;
};

#endif // ATRACKINGDATAIMPORTER_H
