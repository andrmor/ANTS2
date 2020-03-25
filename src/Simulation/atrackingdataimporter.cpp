#include "atrackingdataimporter.h"
#include "aeventtrackingrecord.h"
#include "atrackrecords.h"
#include "atrackbuildoptions.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <fstream>

#include "TGeoManager.h"
#include "TGeoNode.h"

ATrackingDataImporter::ATrackingDataImporter(const ATrackBuildOptions & TrackBuildOptions,
                                             const QStringList & ParticleNames,
                                             std::vector<AEventTrackingRecord *> * History,
                                             std::vector<TrackHolderClass *> * Tracks,
                                             int maxTracks) :
    TrackBuildOptions(TrackBuildOptions), ParticleNames(ParticleNames), History(History), Tracks(Tracks), MaxTracks(maxTracks) {}

ATrackingDataImporter::~ATrackingDataImporter()
{
    clearImportResources();
}

const QString ATrackingDataImporter::processFile(const QString & FileName, int StartEvent, bool bBinary)
{
    bBinaryInput = bBinary;
    Error.clear();

    prepareImportResources(FileName);
    if (!Error.isEmpty()) return Error;

    ExpectedEvent = StartEvent;
    CurrentStatus = ExpectingEvent;
    CurrentEventRecord = AEventTrackingRecord::create();
    CurrentTrack = nullptr;

    while (!isEndReached())
    {
        readBuffer();
        if (!bBinaryInput && currentLine.isEmpty()) continue;

        if      (isNewEvent()) processNewEvent();
        else if (isNewTrack()) processNewTrack();
        else                   processNewStep();

        if (!Error.isEmpty())
        {
            clearImportResources();
            return Error;
        }
    }

    if (Tracks && CurrentTrack)
    {
        //qDebug() << "Sending last track - file at end";
        Tracks->push_back(CurrentTrack);
        CurrentTrack = nullptr;
    }

    if (isErrorInPromises())
    {
        clearImportResources();
        return Error;
    }

    return "";
}

bool ATrackingDataImporter::isEndReached() const
{
    if (bBinaryInput)
        return inStream->eof();
    else
        return inTextStream->atEnd();
}

void ATrackingDataImporter::readBuffer()
{
    if (bBinaryInput)
        ; // ***
    else
        currentLine = inTextStream->readLine();
}

bool ATrackingDataImporter::isNewEvent()
{
    if (!Error.isEmpty()) return false;

    if (bBinaryInput)
        return false; // ***
    else
        return currentLine.startsWith('#');
}

bool ATrackingDataImporter::isNewTrack()
{
    if (!Error.isEmpty()) return false;

    if (bBinaryInput)
        return false; // ***
    else
        return currentLine.startsWith('>');
}

void ATrackingDataImporter::prepareImportResources(const QString & FileName)
{
    if (bBinaryInput)
    {
        inStream = new std::ifstream(FileName.toLatin1().data(), std::ios::in | std::ios::binary);

        if (!inStream->is_open())
        {
            Error = "Failed to open file " + FileName;
            return;
        }
    }
    else
    {
        inTextFile = new QFile(FileName);
        if (!inTextFile->open(QIODevice::ReadOnly | QFile::Text))
        {
            Error = "Failed to open file " + FileName;
            return;
        }
        inTextStream = new QTextStream(inTextFile);
    }
}

void ATrackingDataImporter::clearImportResources()
{
    delete inTextStream; inTextStream = nullptr;
    delete inTextFile;   inTextFile = nullptr;
    delete inStream;     inStream = nullptr;
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
    //TrackID ParentTrackID ParticleId X Y Z Time E iMat VolName VolIndex
    //   0           1           2     3 4 5   6  7   8     9       10
    QStringList f = currentLine.split(' ', QString::SkipEmptyParts);
    if (f.size() != 11)
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
            AParticleTrackingRecord * r = AParticleTrackingRecord::create(f.at(2)); // p_name

            //float x, float y, float z, float time, float energy, float depositedEnergy, const QString & process
            ATransportationStepData * step = new ATransportationStepData(f.at(3).toFloat(), // X
                                                                         f.at(4).toFloat(), // Y
                                                                         f.at(5).toFloat(), // Z
                                                                         f.at(6).toFloat(), // time
                                                                         f.at(7).toFloat(), // E
                                                                         0,                 // depoE
                                                                         "C");              // pr = 'C' which is "Creation"
            step->setVolumeInfo(f.at(9), f.at(10).toInt(), f.at(8).toInt());

            r->addStep(step);
            CurrentEventRecord->addPrimaryRecord(r);
            CurrentParticleRecord = r;
        }
        else
        {
            AParticleTrackingRecord * secrec = PromisedSecondaries[trIndex];
            if (!secrec)
            {
                Error = "Promised secondary not found!";
                return;
            }
            secrec->updatePromisedSecondary(f.at(2),            // p_name
                                            f.at(7).toFloat(),  // E
                                            f.at(3).toFloat(),  // X
                                            f.at(4).toFloat(),  // Y
                                            f.at(5).toFloat(),  // Z
                                            f.at(6).toFloat(),  // time
                                            f.at(9),            //VolName
                                            f.at(10).toInt(),   //VolIndex
                                            f.at(8).toInt()     //MatIndex
                                            );
            CurrentParticleRecord = secrec;
            PromisedSecondaries.remove(trIndex);
        }
    }

    CurrentStatus = ExpectingStep;
}

void ATrackingDataImporter::processNewStep()
{
    //qDebug() << "PS:"<<currentLine;

    // format for "T" processes:
    // ProcName X Y Z Time KinE DirectDepoE iMatTo VolNameTo VolIndexTo [secondaries]
    //     0    1 2 3   4    5      6          7       8           9             ...
    // not that if energy depo is present on T step, it is in the previous volume!

    // format for all other processes:
    // ProcName X Y Z Time KinE DirectDepoE [secondaries]
    //     0    1 2 3   4    5      6           ...

    QStringList f = currentLine.split(' ', QString::SkipEmptyParts);
    if (f.size() < 7)
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

        if (f.at(0) != "T") // skip Transportation (escape out of world is marked with "O")
        {
            //qDebug() << "  Adding node";
            CurrentTrack->Nodes << TrackNodeStruct(f.at(1).toDouble(), f.at(2).toDouble(), f.at(3).toDouble());  // need time?
        }
    }

    if (History)
    {
        if (!CurrentParticleRecord)
        {
            Error = "Attempt to add step when particle record does not exist";
            return;
        }

        const QString & Process = f.at(0);
        ATrackingStepData * step;
        int secIndex;

        if (Process == "T")
        {
            if (f.size() < 10)
            {
                Error = "Bad format in tracking line (transportation step)";
                return;
            }

            step = new ATransportationStepData(f.at(1).toFloat(), // X
                                               f.at(2).toFloat(), // Y
                                               f.at(3).toFloat(), // Z
                                               f.at(4).toFloat(), // time
                                               f.at(5).toFloat(), // energy
                                               f.at(6).toFloat(), // depoE
                                               f.at(0));          // pr
            (static_cast<ATransportationStepData*>(step))->setVolumeInfo(f.at(8), f.at(9).toInt(), f.at(7).toInt());
            secIndex = 10;
        }
        else
        {
            step = new ATrackingStepData(f.at(1).toFloat(), // X
                                         f.at(2).toFloat(), // Y
                                         f.at(3).toFloat(), // Z
                                         f.at(4).toFloat(), // time
                                         f.at(5).toFloat(), // energy
                                         f.at(6).toFloat(), // depoE
                                         f.at(0));          // pr
            secIndex = 7;
        }

        CurrentParticleRecord->addStep(step);

        if (f.size() > secIndex)
        {
            for (int i = secIndex; i < f.size(); i++)
            {
                int index = f.at(i).toInt();
                if (PromisedSecondaries.contains(index))
                {
                    Error = QString("Error: secondary with index %1 was already promised").arg(index);
                    return;
                }
                AParticleTrackingRecord * sr = AParticleTrackingRecord::create(); //empty!
                step->Secondaries.push_back(CurrentParticleRecord->countSecondaries());
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
