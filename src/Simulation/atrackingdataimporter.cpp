#include "atrackingdataimporter.h"
#include "aeventtrackingrecord.h"
#include "atrackrecords.h"
#include "atrackbuildoptions.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

ATrackingDataImporter::ATrackingDataImporter(const ATrackBuildOptions & TrackBuildOptions,
                                             const QStringList & ParticleNames,
                                             std::vector<AEventTrackingRecord *> * History,
                                             std::vector<TrackHolderClass *> * Tracks,
                                             int maxTracks) :
TrackBuildOptions(TrackBuildOptions), ParticleNames(ParticleNames), History(History), Tracks(Tracks), MaxTracks(maxTracks) {}

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

        if (currentLine.startsWith('#')) processNewEvent();
        else if (currentLine.startsWith('>')) processNewTrack();
        else processNewStep();

        if (!Error.isEmpty()) return Error;
    }

    if (Tracks && CurrentTrack)
    {
        //qDebug() << "Sending last track - file at end";
        Tracks->push_back( CurrentTrack );
        CurrentTrack = nullptr;
    }
    if (isErrorInPromises()) return Error;

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

    if (isErrorInPromises()) return; // container of promises should be empty at the end of event

    if (Tracks && CurrentTrack)
    {
        //qDebug() << "  Sending previous track to Track container";
        Tracks->push_back( CurrentTrack );
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
    //Id ParentId Part x y z time E
    //0      1     2   3 4 5   6  7
    QStringList f = currentLine.split(' ', QString::SkipEmptyParts);
    if (f.size() != 8)
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
            Tracks->push_back( CurrentTrack );
            CurrentTrack = nullptr;
        }

        if ((int)Tracks->size() > MaxTracks)
        {
            //qDebug() << "Limit reached, not reading new tracks anymore";
            Tracks = nullptr;
        }
        else
        {
            //qDebug() << "  Creating new track and its firt node";
            CurrentTrack = new TrackHolderClass();
            CurrentTrack->UserIndex = 22;
            TrackBuildOptions.applyToParticleTrack( CurrentTrack, ParticleNames.indexOf(f.at(2)) );

            CurrentTrack->Nodes.append( TrackNodeStruct(f.at(3).toDouble(), f.at(4).toDouble(), f.at(5).toDouble()) ); //need time?
        }
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
            AParticleTrackingRecord * r = AParticleTrackingRecord::create(f.at(2),            // p_name
                                                                          f.at(7).toFloat(),  // E
                                                                          f.at(3).toFloat(),  // X
                                                                          f.at(4).toFloat(),  // Y
                                                                          f.at(5).toFloat(),  // Z
                                                                          f.at(6).toFloat()); // time
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
            secrec->update(f.at(2),            // p_name
                           f.at(7).toFloat(),  // E
                           f.at(3).toFloat(),  // X
                           f.at(4).toFloat(),  // Y
                           f.at(5).toFloat(),  // Z
                           f.at(6).toFloat()); // time
            CurrentParticleRecord = secrec;
            PromisedSecondaries.remove(trIndex);
        }
    }

    CurrentStatus = ExpectingStep;
}

void ATrackingDataImporter::processNewStep()
{
    //qDebug() << "PS:"<<currentLine;
    //x y z time dE proc {secondaries}
    //0 1 2   3  4   5       ...

    QStringList f = currentLine.split(' ', QString::SkipEmptyParts);
    if (f.size() < 6)
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

        if (f.at(5) != "T") // skip Transportation (escape out of world is marked with "O")
        {
            //qDebug() << "  Adding node";
            CurrentTrack->Nodes << TrackNodeStruct(f.at(0).toDouble(), f.at(1).toDouble(), f.at(2).toDouble());  // need time?
        }
    }

    if (History)
    {
        if (!CurrentParticleRecord)
        {
            Error = "Attempt to add step when particle record does not exist";
            return;
        }

        ATrackingStepData * step = new ATrackingStepData(f.at(0).toFloat(), // X
                                                         f.at(1).toFloat(), // Y
                                                         f.at(2).toFloat(), // Z
                                                         f.at(3).toFloat(), // time
                                                         f.at(4).toFloat(), // depoE
                                                         f.at(5));          // pr
        CurrentParticleRecord->addStep(step);

        if (f.size() > 6)
        {
            for (int i=6; i<f.size(); i++)
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

bool ATrackingDataImporter::isErrorInPromises()
{
    if (!PromisedSecondaries.isEmpty())
    {
        Error = "Untreated promises of secondaries remained on event finish!";
        return true;
    }
    return false;
}
