#include "atrackingdataimporter.h"
#include "atrackinghistory.h"
#include "atrackrecords.h"
#include "atrackbuildoptions.h"

#include <QFile>
#include <QTextStream>

ATrackingDataImporter::ATrackingDataImporter(const ATrackBuildOptions & TrackBuildOptions, std::vector<ATrackingHistory*> * History, QVector<TrackHolderClass*> * Tracks) :
    TrackBuildOptions(TrackBuildOptions), History(History), Tracks(Tracks) {}

const QString ATrackingDataImporter::processFile(const QString &FileName, int StartEvent, int numEvents)
{
    Error.clear();

    QFile file( FileName );
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
        return "Failed to open file " + FileName;

    ExpectedEvent = StartEvent;
    CurrentStatus = Init;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        currentLine = in.readLine();

        if (currentLine.startsWith('#')) processNewEvent();
        else if (currentLine.startsWith('>')) processNewParticle();
        else processNewStep();

        if (!Error.isEmpty()) return Error;
    }
    return "";
}

void ATrackingDataImporter::processNewEvent()
{
    currentLine.remove(0, 1);

    int evId = currentLine.toInt();

    switch (CurrentStatus)
    {
    case Init:
        if (evId != ExpectedEvent)
        {
            Error = QString("Expected event #%1, but received #%2").arg(ExpectedEvent).arg(evId);
            return;
        }
        ExpectedEvent++;
        CurrentStatus = ExpectingTrack;
    break;

    }
}

void ATrackingDataImporter::processNewParticle()
{
    currentLine.remove(0, 1);
    //Id ParentId PartId x y z E
    //0      1      2    3 4 5 6
    QStringList f = currentLine.split(' ', QString::SkipEmptyParts);
    if (f.size() != 7)
    {
        Error = "Bad format in new track line";
        return;
    }

    if (History)
    {

    }

    if (Tracks)
    {
        if (CurrentTrack) *Tracks << CurrentTrack;

        CurrentTrack = new TrackHolderClass();
        CurrentTrack->UserIndex = 22;
        TrackBuildOptions.applyToParticleTrack(CurrentTrack, f.at(2).toInt());

        CurrentTrack->Nodes.append( TrackNodeStruct(f.at(3).toDouble(), f.at(4).toDouble(), f.at(5).toDouble(), 0) );  // time? ***!!!
    }

}

void ATrackingDataImporter::processNewStep()
{
    //x y z dE proc {secondaries}
    //0 1 2 3    4    5...

    QStringList f = currentLine.split(' ', QString::SkipEmptyParts);
    if (f.size() < 5)
    {
        Error = "Bad format in track line";
        return;
    }

    if (History)
    {

    }

    if (Tracks)
    {
        if (!CurrentTrack)
        {
            Error = "Track not started!";
            return;
        }

        CurrentTrack->Nodes << TrackNodeStruct(f.at(0).toDouble(), f.at(1).toDouble(), f.at(2).toDouble(), 0);  // time? ***!!!
    }
}


