#include "atrackingdataimporter.h"
#include "aeventtrackingrecord.h"
#include "atrackrecords.h"
#include "atrackbuildoptions.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

ATrackingDataImporter::ATrackingDataImporter(const ATrackBuildOptions & TrackBuildOptions, std::vector<AEventTrackingRecord *> * History, QVector<TrackHolderClass *> * Tracks) :
TrackBuildOptions(TrackBuildOptions), History(History), Tracks(Tracks) {}

const QString ATrackingDataImporter::processFile(const QString &FileName, int StartEvent, int numEvents)
{
    Error.clear();

    QFile file( FileName );
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
        return "Failed to open file " + FileName;

    ExpectedEvent = StartEvent;
    CurrentStatus = ExpectingEvent;
    CurrentHistoryRecord = AEventTrackingRecord::create();
    CurrentTrack = nullptr;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        currentLine = in.readLine();
        if (currentLine.isEmpty()) continue;

        if (currentLine.startsWith('#')) processNewEvent();
        else if (currentLine.startsWith('>')) processNewTrack();
        else processNewStep();

        if (!Error.isEmpty()) return Error;
    }
    if (Tracks && CurrentTrack)
    {
        //qDebug() << "Sending last track - file at end";
        *Tracks << CurrentTrack;
        CurrentTrack = nullptr;
    }
    //if (History) ...
    return "";
}

void ATrackingDataImporter::processNewEvent()
{
    //qDebug() << "EV-->"<<currentLine;
    currentLine.remove(0, 1);
    int evId = currentLine.toInt();

    if (CurrentStatus == ExpectingStep)
    {
        Error = "Unexpected start of event - single step in one record";
        return;
    }

    if (evId != ExpectedEvent)
    {
        Error = QString("Expected event #%1, but received #%2").arg(ExpectedEvent).arg(evId);
        return;
    }

    if (Tracks && CurrentTrack)
    {
        //qDebug() << "  Sending previous track to Track container";
        *Tracks << CurrentTrack;
        CurrentTrack = nullptr;
    }

    if (History)
    {
        if (CurrentStatus != ExpectingEvent) // enough CurrHist exists?
            History->push_back(CurrentHistoryRecord);
        CurrentHistoryRecord = AEventTrackingRecord::create();
    }
    ExpectedEvent++;
    CurrentStatus = ExpectingTrack;
}


void ATrackingDataImporter::processNewTrack()
{
    //qDebug() << "NT:"<<currentLine;
    currentLine.remove(0, 1);
    //Id ParentId PartId x y z E
    //0      1      2    3 4 5 6
    QStringList f = currentLine.split(' ', QString::SkipEmptyParts);
    if (f.size() != 7)
    {
        Error = "Bad format in new track line";
        return;
    }

    if (CurrentStatus == ExpectingEvent)
    {
        Error = "Unexpected start of track - was expecting event";
        return;
    }
    if (CurrentStatus == ExpectingStep)
    {
        Error = "Unexpected start of track - was expecting 1st step";
        return;
    }

    if (Tracks)
    {
        if (CurrentTrack)
        {
            //qDebug() << "  Sending previous track to Track container";
            *Tracks << CurrentTrack;
        }

        //qDebug() << "  Creating new track and its firt node";
        CurrentTrack = new TrackHolderClass();
        CurrentTrack->UserIndex = 22;
        TrackBuildOptions.applyToParticleTrack(CurrentTrack, f.at(2).toInt());

        CurrentTrack->Nodes.append( TrackNodeStruct(f.at(3).toDouble(), f.at(4).toDouble(), f.at(5).toDouble(), 0) );  // time? ***!!!
    }

    if (History)
    {
        if (CurrentStatus == ExpectingTrack)
        {
            // start new record
        }
        else // TrackOngoing
        {
            //add to previous record
        }
    }

    CurrentStatus = ExpectingStep;
}

void ATrackingDataImporter::processNewStep()
{
    //qDebug() << "PS:"<<currentLine;
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

        //qDebug() << "  Adding node";
        CurrentTrack->Nodes << TrackNodeStruct(f.at(0).toDouble(), f.at(1).toDouble(), f.at(2).toDouble(), 0);  // time? ***!!!
    }

    CurrentStatus = TrackOngoing;
}
