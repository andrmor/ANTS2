#include "atrackingdataimporter.h"
#include "aeventtrackingrecord.h"
#include "atrackrecords.h"
#include "atrackbuildoptions.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

ATrackingDataImporter::ATrackingDataImporter(const ATrackBuildOptions & TrackBuildOptions, std::vector<AEventTrackingRecord *> * History, QVector<TrackHolderClass *> * Tracks) :
TrackBuildOptions(TrackBuildOptions), History(History), Tracks(Tracks) {}

const QString ATrackingDataImporter::processFile(const QString &FileName, int StartEvent)
{
    Error.clear();

    QFile file( FileName );
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
        return "Failed to open file " + FileName;

    ExpectedEvent = StartEvent;
    CurrentStatus = ExpectingEvent;
    CurrentEventRecord = AEventTrackingRecord::create();
    CurrentTrack = nullptr;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        currentLine = in.readLine();
        if (currentLine.isEmpty()) continue;

        qDebug() << currentLine;

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
    if (isPromisesFailed()) return Error;

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

    if (isPromisesFailed()) return; // container of promises should be empty at the end of event

    if (Tracks && CurrentTrack)
    {
        //qDebug() << "  Sending previous track to Track container";
        *Tracks << CurrentTrack;
        CurrentTrack = nullptr;
    }

    if (History)
    {
        CurrentEventRecord = AEventTrackingRecord::create();
        History->push_back(CurrentEventRecord);
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
        Error = "Unexpected start of track - waiting new event";
        return;
    }
    if (CurrentStatus == ExpectingStep)
    {
        Error = "Unexpected start of track - waiting for 1st step";
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
        if (!CurrentEventRecord)
        {
            Error = "Attempt to start new track when event record does not exist";
            return;
        }

        // if parent track == 0 create new primary record in this event
        // else pointer to empty an record should be in the list of promised secondaries -> update the record
        int trIndex = f.at(0).toInt();
        int parTrIndex = f.at(1).toInt();
        if (parTrIndex == 0)
        {
            AParticleTrackingRecord * r = AParticleTrackingRecord::create(f.at(2).toInt(),
                                                                          f.at(6).toFloat(),
                                                                          f.at(3).toFloat(),
                                                                          f.at(4).toFloat(),
                                                                          f.at(5).toFloat(),
                                                                          0); // time!!!
            CurrentParticleRecord = r;
            CurrentEventRecord->addPrimaryRecord(r);
        }
        else
        {
            AParticleTrackingRecord * secrec = PromisedSecondaries[trIndex];
            if (!secrec)
            {
                Error = "Promised secondary not found!";
                return;
            }
            secrec->update(f.at(2).toInt(),
                           f.at(6).toFloat(),
                           f.at(3).toFloat(),
                           f.at(4).toFloat(),
                           f.at(5).toFloat(),
                           0); // time!!!
            CurrentParticleRecord = secrec;
            PromisedSecondaries.remove(trIndex);
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

    if (Tracks)
    {
        if (!CurrentTrack)
        {
            Error = "Track not started while attempting to add step";
            return;
        }
        //qDebug() << "  Adding node";
        CurrentTrack->Nodes << TrackNodeStruct(f.at(0).toDouble(), f.at(1).toDouble(), f.at(2).toDouble(), 0);  // time? ***!!!
    }

    if (History)
    {
        if (!CurrentParticleRecord)
        {
            Error = "Attempt to add step when particle record does not exist";
            return;
        }

        ATrackingStepData * step = new ATrackingStepData(f.at(0).toFloat(),
                                                         f.at(1).toFloat(),
                                                         f.at(2).toFloat(),
                                                         0,
                                                         f.at(3).toFloat(),
                                                         f.at(4));
        CurrentParticleRecord->addStep(step);

        if (f.size() > 5) // time!!!
        {
            for (int i=5; i<f.size(); i++)
            {
                int index = f.at(i).toInt();
                if (PromisedSecondaries.contains(index))
                {
                    Error = QString("Error: secondary with index %1 was already promised").arg(index);
                    return;
                }
                AParticleTrackingRecord * sr = AParticleTrackingRecord::create(); //empty!
                step->addSecondary(sr);
                CurrentParticleRecord->addSecondary(sr);
                PromisedSecondaries.insert(index, sr);
            }
        }
    }

    CurrentStatus = TrackOngoing;
}

bool ATrackingDataImporter::isPromisesFailed()
{
    if (!PromisedSecondaries.isEmpty())
    {
        Error = "Untreated promises of secondaries remained on event finish!";
        return true;
    }
    return false;
}
