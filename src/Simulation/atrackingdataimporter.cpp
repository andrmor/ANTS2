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
        if (bBinaryInput)
        {
            if (inStream->eof()) break;
        }
        else
        {
            if (currentLine.isEmpty()) continue;
        }

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
    {
        // EE - new event, F0 - new track, F8 - trasnportation step, FF - non-transport step
        *inStream >> binHeader;
        if (inStream->eof()) return; //this is the proper way to reach end of file
        if (inStream->fail())
            Error = "Error in header char input";
    }
    else
        currentLine = inTextStream->readLine();
}

bool ATrackingDataImporter::isNewEvent()
{
    if (!Error.isEmpty()) return false;

    if (bBinaryInput)
    {
        // EE - new event, F0 - new track, F8 - trasnportation step, FF - non-transport step
        return (binHeader == char(0xEE));
    }
    else
        return currentLine.startsWith('#');
}

bool ATrackingDataImporter::isNewTrack()
{
    if (!Error.isEmpty()) return false;

    if (bBinaryInput)
    {
        // EE - new event, F0 - new track, F8 - trasnportation step, FF - non-transport step
        return (binHeader == char(0xF0));
    }
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

int ATrackingDataImporter::extractEventId()
{
    if (bBinaryInput)
    {
        int evId;
        inStream->read((char*)&evId, sizeof(int));
        if (inStream->fail()) Error = "Error in header char input";
        //qDebug() << "Event id:" << evId << "  error?" << !Error.isEmpty();
        return evId;
    }
    else
    {
        //qDebug() << "EV-->"<<currentLine;
        currentLine.remove(0, 1);
        bool bOK = false;
        int evId = currentLine.toInt(&bOK);
        if (!bOK) Error = "Error in conversion of event number to integer";
        return evId;
    }
}

void ATrackingDataImporter::readNewTrack()
{
    if (bBinaryInput)
    {
        //format:
        //trackId(int) parentTrackId(int) PartName(string0) X(double) Y(double) Z(double) time(double) kinEnergy(double) NextMat(int) NextVolNmae(string0) NextVolIndex(int)

        inStream->read((char*)&BtrackId,        sizeof(int));
        inStream->read((char*)&BparentTrackId,  sizeof(int));
        readString(BparticleName);
        inStream->read((char*)Bpos,           3*sizeof(double));
        inStream->read((char*)&Btime,           sizeof(double));
        inStream->read((char*)&BkinEnergy,      sizeof(double));
        inStream->read((char*)&BnextMat,        sizeof(int));
        readString(BnextVolName);
        inStream->read((char*)&BnextVolIndex,   sizeof(int));

        if (inStream->fail())
        {
            Error = "Unexpected format of a new track binary record";
            return;
        }
    }
    else
    {
        //qDebug() << "NT:"<<currentLine;
        currentLine.remove(0, 1);
        //TrackID ParentTrackID ParticleName   X Y Z Time E iMat VolName VolIndex
        //   0           1           2         3 4 5   6  7   8     9       10
        inputSL = currentLine.split(' ', QString::SkipEmptyParts);
        if (inputSL.size() != 11)
            Error = "Bad format in new track line";
    }
}

void ATrackingDataImporter::initNewTrackRecord()
{
    int ParticleID;

    if (bBinaryInput)
    {
        ParticleID = ParticleNames.indexOf(BparticleName.data());
        CurrentTrack->Nodes.append( TrackNodeStruct(Bpos[0], Bpos[1], Bpos[2]) ); //need time?
    }
    else
    {
        ParticleID = ParticleNames.indexOf(inputSL.at(2));
        CurrentTrack->Nodes.append( TrackNodeStruct(inputSL.at(3).toDouble(), inputSL.at(4).toDouble(), inputSL.at(5).toDouble()) ); //need time?
    }

    TrackBuildOptions.applyToParticleTrack(CurrentTrack, ParticleID);
}

bool ATrackingDataImporter::isPrimaryRecord() const
{
    if (bBinaryInput)
        return (BparentTrackId == 0);
    else
    {
        int parTrIndex = inputSL.at(1).toInt();
        return (parTrIndex == 0);
    }
}

AParticleTrackingRecord * ATrackingDataImporter::createAndInitParticleTrackingRecord() const
{
    AParticleTrackingRecord * r = nullptr;

    if (bBinaryInput)
    {
        r = AParticleTrackingRecord::create(BparticleName.data());

        ATransportationStepData * step = new ATransportationStepData(Bpos,          // pos
                                                                     Btime,         // time
                                                                     BkinEnergy,    // E
                                                                     0,             // depoE
                                                                     "C");          // process = 'C' which is "Creation"
        step->setVolumeInfo(BnextVolName.data(), BnextVolIndex, BnextMat);
        r->addStep(step);
    }
    else
    {
        //TrackID ParentTrackID ParticleName   X Y Z Time E iMat VolName VolIndex
        //   0           1           2         3 4 5   6  7   8     9       10
        r = AParticleTrackingRecord::create(inputSL.at(2));

        ATransportationStepData * step = new ATransportationStepData(inputSL.at(3).toFloat(), // X
                                                                     inputSL.at(4).toFloat(), // Y
                                                                     inputSL.at(5).toFloat(), // Z
                                                                     inputSL.at(6).toFloat(), // time
                                                                     inputSL.at(7).toFloat(), // E
                                                                     0,                       // depoE
                                                                     "C");                    // process = 'C' which is "Creation"
        step->setVolumeInfo(inputSL.at(9), inputSL.at(10).toInt(), inputSL.at(8).toInt());
        r->addStep(step);
    }

    return r;
}

int ATrackingDataImporter::getNewTrackIndex() const
{
    if (bBinaryInput)
        return BtrackId;
    else
        return inputSL.at(0).toInt();
}

void ATrackingDataImporter::updatePromisedSecondary(AParticleTrackingRecord *secrec)
{
    if (bBinaryInput)
    {
        secrec->updatePromisedSecondary(BparticleName.data(),  // p_name
                                        BkinEnergy,     // E
                                        Bpos[0],        // X
                                        Bpos[1],        // Y
                                        Bpos[2],        // Z
                                        Btime,          // time
                                        BnextVolName.data(),   //VolName
                                        BnextVolIndex,  //VolIndex
                                        BnextMat        //MatIndex
                                        );
    }
    else
    {
        //TrackID ParentTrackID ParticleName   X Y Z Time E iMat VolName VolIndex
        //   0           1           2         3 4 5   6  7   8     9       10
        secrec->updatePromisedSecondary(inputSL.at(2),            // p_name
                                        inputSL.at(7).toFloat(),  // E
                                        inputSL.at(3).toFloat(),  // X
                                        inputSL.at(4).toFloat(),  // Y
                                        inputSL.at(5).toFloat(),  // Z
                                        inputSL.at(6).toFloat(),  // time
                                        inputSL.at(9),            //VolName
                                        inputSL.at(10).toInt(),   //VolIndex
                                        inputSL.at(8).toInt()     //MatIndex
                                        );
    }
}

void ATrackingDataImporter::readNewStep()
{
    if (bBinaryInput)
    {
        // format for "T" processes:
        // bin:   [FF or F8] ProcName0 X Y Z Time KinE DirectDepoE iMatTo VolNameTo0 VolIndexTo numSec [secondaries]
        // for non-"T" process, iMatTo VolNameTo  VolIndexTo are absent
        if (binHeader == char(0xF8) || binHeader == char(0xFF))  //transport or non-transport
        {
            readString(BprocessName);
            inStream->read((char*)Bpos,           3*sizeof(double));
            inStream->read((char*)&Btime,           sizeof(double));
            inStream->read((char*)&BkinEnergy,      sizeof(double));
            inStream->read((char*)&BdepoEnergy,     sizeof(double));
            if (binHeader == char(0xF8))
            {
                inStream->read((char*)&BnextMat,      sizeof(int));
                readString(BnextVolName);
                inStream->read((char*)&BnextVolIndex, sizeof(int));
            }
            int numSec = 0;
            inStream->read((char*)&numSec,            sizeof(int));
            BsecVec.resize(numSec);
            for (int i=0; i<numSec; i++)
                inStream->read((char*)&BsecVec[i],    sizeof(int));

            if (inStream->fail())
                if (!inStream->eof())
                {
                    Error = "Unexpected format of a step in history/track binary file";
                    return;
                }
        }
        else Error = "Unexpected header char for a step in history/track binary file";
    }
    else
    {
        //qDebug() << "PS:"<<currentLine;

        // format for "T" processes:
        // ProcName X Y Z Time KinE DirectDepoE iMatTo VolNameTo VolIndexTo [secondaries]
        //     0    1 2 3   4    5      6          7       8           9             ...
        // not that if energy depo is present on T step, it is in the previous volume!

        // format for all other processes:
        // ProcName X Y Z Time KinE DirectDepoE [secondaries]
        //     0    1 2 3   4    5      6           ...

        inputSL = currentLine.split(' ', QString::SkipEmptyParts);
        if (inputSL.size() < 7)
        {
            Error = "Bad format in step line";
            return;
        }
        if (inputSL.first() == "T" && inputSL.size() < 10)
        {
            Error = "Bad format in tracking line (transportation step)";
            return;
        }
    }
}

void ATrackingDataImporter::addTrackStep()
{
    // skip Transportation step (escape out of world is marked with "O")

    if (bBinaryInput)
    {
        if (binHeader == char(0xFF))
            CurrentTrack->Nodes << TrackNodeStruct(Bpos[0], Bpos[1], Bpos[2]);  // need time?
    }
    else
    {
        if (inputSL.at(0) != "T")
        {
            //qDebug() << "  Adding node";
            // ProcName X Y Z Time KinE DirectDepoE [secondaries]
            //     0    1 2 3   4    5      6           ...
            CurrentTrack->Nodes << TrackNodeStruct(inputSL.at(1).toDouble(), inputSL.at(2).toDouble(), inputSL.at(3).toDouble());  // need time?
        }
    }
}

void ATrackingDataImporter::addHistoryStep()
{
    ATrackingStepData * step = ( isTransportationStep() ? createHistoryTransportationStep()
                                                        : createHistoryStep() );
    CurrentParticleRecord->addStep(step);

    readSecondaries();
    for (const int & index : BsecVec)
    {
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

ATrackingStepData *ATrackingDataImporter::createHistoryTransportationStep() const
{
    ATransportationStepData * step;

    if (bBinaryInput)
    {
        step = new ATransportationStepData(Bpos[0],         // X
                                           Bpos[1],         // Y
                                           Bpos[2],         // Z
                                           Btime,           // time
                                           BkinEnergy,      // energy
                                           BdepoEnergy,     // depoE
                                           BprocessName.data());   // pr

        step->setVolumeInfo(BnextVolName.data(), BnextVolIndex, BnextMat);
    }
    else
    {
        // ProcName X Y Z Time KinE DirectDepoE iMatTo VolNameTo VolIndexTo [secondaries]
        //     0    1 2 3   4    5      6          7       8           9         ...

        step = new ATransportationStepData(inputSL.at(1).toFloat(), // X
                                           inputSL.at(2).toFloat(), // Y
                                           inputSL.at(3).toFloat(), // Z
                                           inputSL.at(4).toFloat(), // time
                                           inputSL.at(5).toFloat(), // energy
                                           inputSL.at(6).toFloat(), // depoE
                                           inputSL.at(0));          // pr

        step->setVolumeInfo(inputSL.at(8), inputSL.at(9).toInt(), inputSL.at(7).toInt());
    }

    return step;
}

ATrackingStepData * ATrackingDataImporter::createHistoryStep() const
{
    ATrackingStepData * step;

    if (bBinaryInput)
    {
        step = new ATrackingStepData(Bpos[0],         // X
                                     Bpos[1],         // Y
                                     Bpos[2],         // Z
                                     Btime,           // time
                                     BkinEnergy,      // energy
                                     BdepoEnergy,     // depoE
                                     BprocessName.data());   // pr
    }
    else
    {
        // ProcName X Y Z Time KinE DirectDepoE [secondaries]
        //     0    1 2 3   4    5      6           ...

        step = new ATrackingStepData(inputSL.at(1).toFloat(), // X
                                     inputSL.at(2).toFloat(), // Y
                                     inputSL.at(3).toFloat(), // Z
                                     inputSL.at(4).toFloat(), // time
                                     inputSL.at(5).toFloat(), // energy
                                     inputSL.at(6).toFloat(), // depoE
                                     inputSL.at(0));          // pr
    }

    return step;
}

void ATrackingDataImporter::readSecondaries()
{
    if (bBinaryInput)
    {
        //already done;
    }
    else
    {
        BsecVec.clear();
        const int secIndex = (inputSL.at(0) == "T" ? 10 : 7);
        for (int i = secIndex; i < inputSL.size(); i++)
            BsecVec << inputSL.at(i).toInt();
    }
}

bool ATrackingDataImporter::isTransportationStep() const
{
    if (bBinaryInput)
        return (binHeader == char(0xF8));
    else
    {
        return (inputSL.at(0) == "T");
    }
}

void ATrackingDataImporter::readString(std::string & str) const
{
    char ch;
    str.clear();
    while (*inStream >> ch)
    {
        if (ch == char(0x00)) break;
        str += ch;
    }
}

void ATrackingDataImporter::processNewEvent()
{
    if (!Error.isEmpty()) return;

    if (CurrentStatus == ExpectingStep)
    {
        Error = "Unexpected start of event - single step in one record";
        return;
    }
    if (isErrorInPromises()) return; // container of promises should be empty at the end of event

    int evId = extractEventId();
    if (!Error.isEmpty()) return;

    if (evId != ExpectedEvent)
    {
        Error = QString("Expected event #%1, but received #%2").arg(ExpectedEvent).arg(evId);
        return;
    }

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
    if (!Error.isEmpty()) return;

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

    readNewTrack();
    if (!Error.isEmpty()) return;

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
            //qDebug() << "  Creating new track and its first node";
            CurrentTrack = new TrackHolderClass();
            CurrentTrack->UserIndex = 22;
            initNewTrackRecord();
        }
    }

    if (History)
    {
        if (!CurrentEventRecord)
        {
            Error = "Attempt to start new track when event record does not exist";
            return;
        }

        // if primary (parent track == 0), create a new primary record in this event
        // else a pointer to empty record should be in the list of promised secondaries -> update the record
        if (isPrimaryRecord())
        {
            AParticleTrackingRecord * r = createAndInitParticleTrackingRecord();
            CurrentEventRecord->addPrimaryRecord(r);
            CurrentParticleRecord = r;
        }
        else
        {
            int trIndex = getNewTrackIndex();
            AParticleTrackingRecord * secrec = PromisedSecondaries[trIndex];
            if (!secrec)
            {
                Error = "Promised secondary not found!";
                return;
            }

            updatePromisedSecondary(secrec);
            CurrentParticleRecord = secrec;
            PromisedSecondaries.remove(trIndex);
        }
    }
    CurrentStatus = ExpectingStep;
}

void ATrackingDataImporter::processNewStep()
{
    readNewStep();
    if (!Error.isEmpty()) return;

    if (Tracks)
    {
        if (!CurrentTrack)
        {
            Error = "Track not started while attempting to add a node to it";
            return;
        }
        addTrackStep();
    }

    if (History)
    {
        if (!CurrentParticleRecord)
        {
            Error = "Attempt to add step when particle record does not exist";
            return;
        }
        addHistoryStep();
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
