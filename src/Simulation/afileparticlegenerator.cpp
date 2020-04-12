#include "afileparticlegenerator.h"
#include "amaterialparticlecolection.h"
#include "aparticlerecord.h"
#include "ajsontools.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QFileInfo>

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>

AFileParticleGenerator::AFileParticleGenerator(const AMaterialParticleCollection & MpCollection) :
    MpCollection(MpCollection) {}

AFileParticleGenerator::~AFileParticleGenerator()
{
    ReleaseResources();
}

void AFileParticleGenerator::SetFileName(const QString &fileName)
{
    if (FileName == fileName) return;

    FileName = fileName;
    InvalidateFile();
}

void AFileParticleGenerator::SetFileFormat(AFileMode mode)
{
    if (Mode == mode) return;

    Mode = mode;
    InvalidateFile();
}

#include <algorithm>
bool AFileParticleGenerator::Init()
{
    ReleaseResources();

    bool bExpanded = bCollectExpandedStatistics; bCollectExpandedStatistics = false; // single trigger flag!

    if (FileName.isEmpty())
    {
        ErrorString = "File name is not defined";
        return false;
    }

    bool bNeedInspect = isRequireInspection() || bExpanded;
    //qDebug() << "Init called, requires inspect?" << bNeedInspect;
    if (bNeedInspect)
    {
        RegisteredParticleCount = MpCollection.countParticles();
        clearFileStat();  // resizes statParticleQuantity container according to RegisteredParticleCount
        FileLastModified = QFileInfo(FileName).lastModified();
    }

    bool bOK;
    if (Mode == AFileMode::Standard)
    {
        Engine = new AFilePGEngineStandard(this);
        bOK = Engine->doInit(bNeedInspect, bExpanded);
    }
    else
    {
        bOK = testG4mode();
        if (bOK)
        {
            if (bG4binary)
            {
                Engine = new AFilePGEngineG4antsBin(this);
                bOK = Engine->doInit(bNeedInspect, bExpanded);
            }
            else
            {
                Engine = new AFilePGEngineG4antsTxt(this);
                bOK = Engine->doInit(bNeedInspect, bExpanded);
            }
        }
    }

    if (bOK && ParticleStat.size() > 0)
    {
        std::sort(ParticleStat.begin(), ParticleStat.end(), [](const AParticleInFileStatRecord & l, const AParticleInFileStatRecord & r)
        {
            return (l.Entries > r.Entries);
        });
    }

    return bOK;
}

bool AFileParticleGenerator::testG4mode()
{
    //is it ascii mode?
    std::ifstream inT(FileName.toLatin1().data());
    if (!inT.is_open())
    {
        ErrorString = QString("Cannot open file %1").arg(FileName);
        return false;
    }

    std::string str;
    getline(inT, str);
    inT.close();
    if (str.size() > 0 && str[0] == '#')
    {
        //qDebug() << "It seems it is a valid ascii G4ants file";
        bG4binary = false;
        return true;
    }

    std::ifstream inB(FileName.toLatin1().data(), std::ios::in | std::ios::binary);
    if (!inB.is_open())
    {
        ErrorString = QString("Cannot open file %1").arg(FileName);
        return false;
    }

    char ch;
    inB.get(ch);
    if (ch == (char)0xEE)
    {
        //qDebug() << "It seems it is a valid binary G4ants file";
        bG4binary = true;
        return true;
    }

    ErrorString = QString("Unexpected format of the file %1").arg(FileName);
    return false;
}

bool AFileParticleGenerator::isRequireInspection() const
{
    QFileInfo fi(FileName);

    if (FileLastModified != fi.lastModified()) return true;

    if (Mode == AFileMode::Standard &&
        RegisteredParticleCount != MpCollection.countParticles()) return true;

    return false;
}

void AFileParticleGenerator::ReleaseResources()
{
    delete Engine; Engine = nullptr;
}

bool AFileParticleGenerator::GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles, int iEvent)
{
    if (!Engine)
    {
        ErrorString = "Generator is not configured!";
        return false;
    }

    return Engine->doGenerateEvent(GeneratedParticles);
}

bool AFileParticleGenerator::IsParticleInUse(int /*particleId*/, QString & /*SourceNames*/) const
{
    return false; //TODO
}

void AFileParticleGenerator::writeToJson(QJsonObject &json) const
{
    json["FileName"] = FileName;
    json["Mode"] = static_cast<int>(Mode);

    json["NumEventsInFile"] = NumEventsInFile;
    json["RegisteredParticleCount"] = RegisteredParticleCount;
    json["FileLastModified"] = FileLastModified.toMSecsSinceEpoch();
}

bool AFileParticleGenerator::readFromJson(const QJsonObject &json)
{
    parseJson(json, "NumEventsInFile", NumEventsInFile);
    parseJson(json, "RegisteredParticleCount", RegisteredParticleCount);
    qint64 lastMod;
    parseJson(json, "FileLastModified", lastMod);
    FileLastModified = QDateTime::fromMSecsSinceEpoch(lastMod);

    Mode = AFileMode::Standard;
    if (json.contains("Mode"))
    {
        int im;
        parseJson(json, "Mode", im);
        if (im > -1 && im < 2)
            Mode = static_cast<AFileMode>(im);
    }

    return parseJson(json, "FileName", FileName);
}

void AFileParticleGenerator::SetStartEvent(int startEvent)
{
    if (Engine) Engine->doSetStartEvent(startEvent);
}

void AFileParticleGenerator::InvalidateFile()
{
    FileLastModified = QDateTime(); //will force to inspect file on next use
    clearFileStat();
}

bool AFileParticleGenerator::IsValidated() const
{
    QFileInfo fi(FileName);

    if (!fi.exists()) return false;
    if (FileLastModified != fi.lastModified()) return false;
    if (Mode == AFileMode::Standard && RegisteredParticleCount != MpCollection.countParticles()) return false;

    return true;
}

bool AFileParticleGenerator::generateG4File(int eventBegin, int eventEnd, const QString & FileName)
{
    if (Engine)
        return Engine->doGenerateG4File(eventBegin, eventEnd, FileName);
    else
        return false;
}

void AFileParticleGenerator::clearFileStat()
{
    NumEventsInFile = 0;
    statNumEmptyEventsInFile = 0;
    statNumMultipleEvents = 0;
    ParticleStat.clear();
}

// ***************************************************************************

AFilePGEngineStandard::~AFilePGEngineStandard()
{
    delete Stream; Stream = nullptr;
    if (File.isOpen()) File.close();
}

bool AFilePGEngineStandard::doInit(bool bNeedInspect, bool bDetailedInspection)
{
    File.setFileName(FileName);
    if(!File.open(QIODevice::ReadOnly | QFile::Text))
    {
        FPG->SetErrorString( QString("Failed to open file: %1").arg(FileName) );
        return false;
    }

    Stream = new QTextStream(&File);

    if (bNeedInspect)
    {
        //qDebug() << "Inspecting file (standard format):" << FileName;
        bool bContinueEvent = false;
        while (!Stream->atEnd())
        {
            const QString line = Stream->readLine();
            const QStringList f = line.split(rx, QString::SkipEmptyParts);
            if (f.size() < 9) continue;

            bool bOK;
            int    pId = f.at(0).toInt(&bOK);
            if (!bOK) continue; //assuming this is a comment line
            if (pId < 0 || pId >= FPG->RegisteredParticleCount)
            {
                FPG->SetErrorString( QString("Invalid particle index %1 in file %2").arg(pId).arg(FileName) );
                return false;
            }

            //FPG->statParticleQuantity[pId]++;

            if (bDetailedInspection)
            {
                const QString & name = f.first(); // I know it is int, reusing G4ants-ish infrastructure
                bool bNotFound = true;
                for (AParticleInFileStatRecord & rec : FPG->ParticleStat)
                {
                    if (rec.NameQt == name)
                    {
                        rec.Entries++;
                        rec.Energy += f.at(1).toDouble();
                        bNotFound = false;
                        break;
                    }
                }

                if (bNotFound)
                    FPG->ParticleStat.push_back(AParticleInFileStatRecord(name, f.at(1).toDouble()));
            }

            if (!bContinueEvent) FPG->NumEventsInFile++;

            if (f.size() > 9 && f.at(9) == '*')
            {
                if (!bContinueEvent) FPG->statNumMultipleEvents++;
                bContinueEvent = true;
            }
            else bContinueEvent = false;
        }

        if (bDetailedInspection)
            for (AParticleInFileStatRecord & rec : FPG->ParticleStat)
                rec.NameQt = FPG->MpCollection.getParticleName(rec.NameQt.toInt()); //index is valid
    }

    return true;
}

bool AFilePGEngineStandard::doGenerateEvent(QVector<AParticleRecord *> &GeneratedParticles)
{
    while (!Stream->atEnd())
    {
        if (FPG->IsAbortRequested()) return false;

        const QString line = Stream->readLine();
        QStringList f = line.split(rx, QString::SkipEmptyParts);
        //format: ParticleId Energy X Y Z VX VY VZ Time *  //'*' is optional - indicates event not finished yet
        if (f.size() < 9) continue;    // skip bad lines!
        if (f.at(0) == '#') continue;  // comment line

        bool bOK;
        int  pId    = f.at(0).toInt(&bOK);
        if (!bOK) continue;
        //TODO protection: wrong index, either test on start

        double energy = f.at(1).toDouble();
        double x =      f.at(2).toDouble();
        double y =      f.at(3).toDouble();
        double z =      f.at(4).toDouble();
        double vx =     f.at(5).toDouble();
        double vy =     f.at(6).toDouble();
        double vz =     f.at(7).toDouble();
        double t  =     f.at(8).toDouble();

        AParticleRecord* p = new AParticleRecord(pId,
                                                 x, y, z,
                                                 vx, vy, vz,
                                                 t, energy);
        p->ensureUnitaryLength();
        GeneratedParticles << p;

        if (f.size() > 9 && f.at(9) == '*') continue; //this is multiple event!
        return true; //normal termination
    }
    return false; //could not read particle record in file!
}

void AFilePGEngineStandard::doSetStartEvent(int startEvent)
{
    if (Stream)
    {
        Stream->seek(0);
        if (startEvent == 0) return;

        int event = -1;
        bool bContinueEvent = false;
        while (!Stream->atEnd())
        {
            const QString line = Stream->readLine();
            QStringList f = line.split(rx, QString::SkipEmptyParts);
            if (f.size() < 9) continue;
            if (f.at(0) == '#') continue;
            bool bOK;
            f.at(0).toInt(&bOK);
            if (!bOK) continue; //assuming this is a comment line

            if (!bContinueEvent) event++;

            if (f.size() > 9 && f.at(9) == '*')
                bContinueEvent = true;
            else
            {
                bContinueEvent = false;
                if (event == startEvent-1) return;
            }
        }
    }
}

AFilePGEngineG4antsTxt::~AFilePGEngineG4antsTxt()
{
    if (inStream) inStream->close();
    delete inStream; inStream = nullptr;
}

bool AFilePGEngineG4antsTxt::doInit(bool bNeedInspect, bool bDetailedInspection)
{
    inStream = new std::ifstream(FileName.toLatin1().data());

    if (!inStream->is_open())
    {
        FPG->SetErrorString( QString("Cannot open file %1").arg(FileName) );
        return false;
    }

    if (bNeedInspect)
    {
        //qDebug() << "Inspecting G4ants-generated txt file:" << FileName;
        std::string str;
        bool bWasParticle = true;
        bool bWasMulty    = false;
        while (!inStream->eof())
        {
            getline( *inStream, str );
            if (str.empty())
            {
                if (inStream->eof()) break;

                FPG->SetErrorString("Found empty line!");
                return false;
            }

            if (str[0] == '#')
            {
                //new event
                FPG->NumEventsInFile++;
                if (bWasMulty)     FPG->statNumMultipleEvents++;
                if (!bWasParticle) FPG->statNumEmptyEventsInFile++;
                bWasMulty = false;
                bWasParticle = false;
                continue;
            }

            QStringList f = QString(str.data()).split(rx, QString::SkipEmptyParts);
            //pname en x y z i j k time

            if (f.size() != 9)
            {
                FPG->SetErrorString("Bad format of particle record!");
                return false;
            }

            if (bDetailedInspection)
            {
                const QString & name = f.first();
                //TODO kill [***] of ions
                bool bNotFound = true;
                for (AParticleInFileStatRecord & rec : FPG->ParticleStat)
                {
                    if (rec.NameQt == name)
                    {
                        rec.Entries++;
                        rec.Energy += f.at(1).toDouble();
                        bNotFound = false;
                        break;
                    }
                }

                if (bNotFound)
                    FPG->ParticleStat.push_back(AParticleInFileStatRecord(name, f.at(1).toDouble()));
            }

            if (bWasParticle) bWasMulty = true;
            bWasParticle = true;
        }
        if (bWasMulty)     FPG->statNumMultipleEvents++;
        if (!bWasParticle) FPG->statNumEmptyEventsInFile++;
    }
    return true;
}

bool AFilePGEngineG4antsTxt::doGenerateEvent(QVector<AParticleRecord *> &GeneratedParticles)
{
    std::string str;
    while ( getline(*inStream, str) )
    {
        if (str.empty())
        {
            if (inStream->eof()) return false;
            FPG->SetErrorString("Found empty line!");
            return false;
        }

        if (str[0] == '#') return true; //new event

        QStringList f = QString(str.data()).split(rx, QString::SkipEmptyParts); //pname en time x y z i j k
        if (f.size() != 9)
        {
            FPG->SetErrorString("Bad format of particle record!");
            return false;
        }

        AParticleRecord * p = new AParticleRecord();

        p->Id     = -1;
        p->energy = f.at(1).toDouble();
        p->r[0]   = f.at(2).toDouble();
        p->r[1]   = f.at(3).toDouble();
        p->r[2]   = f.at(4).toDouble();
        p->v[0]   = f.at(5).toDouble();
        p->v[1]   = f.at(6).toDouble();
        p->v[2]   = f.at(7).toDouble();
        p->time   = f.at(8).toDouble();

        GeneratedParticles << p;
    }
    return false;
}

bool AFilePGEngineG4antsTxt::doGenerateG4File(int eventBegin, int eventEnd, const QString &FileName)
{
    std::ofstream outStream;
    outStream.open(FileName.toLatin1().data());
    if (!outStream.is_open())
    {
        FPG->SetErrorString("Cannot open file to export G4 pareticle data");
        return false;
    }

    int currentEvent = -1;
    int eventsToDo = eventEnd - eventBegin;
    bool bSkippingEvents = (eventBegin != 0);
    inStream->seekg(0);
    std::string str;
    while ( getline(*inStream, str) )
    {
        if (inStream->eof())
        {
            if (eventsToDo == 0) return true;
            else
            {
                FPG->SetErrorString("Unexpected end of file");
                return false;
            }
        }
        if (str.empty())
        {
            FPG->SetErrorString("Found empty line!");
            return false;
        }

        if (str[0] == '#')
        {
            if (eventsToDo == 0) return true;
            currentEvent++;

            if (bSkippingEvents && currentEvent == eventBegin) bSkippingEvents = false;

            if (!bSkippingEvents)
            {
                outStream << str << std::endl;
                eventsToDo--;
            }
            continue;
        }

        if (!bSkippingEvents) outStream << str << std::endl;
    }

    if (eventsToDo == 0) return true;

    FPG->SetErrorString("Unexpected end of file");
    return false;
}

AFilePGEngineG4antsBin::~AFilePGEngineG4antsBin()
{
    if (inStream) inStream->close();
    delete inStream; inStream = nullptr;
}

bool AFilePGEngineG4antsBin::doInit(bool bNeedInspect, bool bDetailedInspection)
{
    inStream = new std::ifstream(FileName.toLatin1().data(), std::ios::in | std::ios::binary);

    if (!inStream->is_open()) //paranoic
    {
        FPG->SetErrorString( QString("Cannot open file %1").arg(FileName) );
        return false;
    }

    if (bNeedInspect)
    {
        //qDebug() << "Inspecting G4ants-generated bin file:" << FileName;
        bool bWasParticle = true;
        bool bWasMulty    = false;
        std::string ParticleName;
        double energy, time;
        double PosDir[6];
        char h;
        int eventId;
        while (inStream->get(h))
        {
            if (h == char(0xEE)) //new event
            {
                inStream->read((char*)&eventId, sizeof(int));
                FPG->NumEventsInFile++;
                if (bWasMulty)     FPG->statNumMultipleEvents++;
                if (!bWasParticle) FPG->statNumEmptyEventsInFile++;
                bWasMulty = false;
                bWasParticle = false;
            }
            else if (h == char(0xFF))
            {
                //data line
                ParticleName.clear();
                while (*inStream >> h)
                {
                    if (h == (char)0x00) break;
                    ParticleName += h;
                }
                //qDebug() << pn.data();
                inStream->read((char*)&energy,       sizeof(double));
                inStream->read((char*)&PosDir,     6*sizeof(double));
                inStream->read((char*)&time,         sizeof(double));
                if (inStream->fail())
                {
                    FPG->SetErrorString("Unexpected format of a line in the binary file with the input particles");
                    return false;
                }

                if (bDetailedInspection)
                {
                    //TODO kill [***] of ions
                    bool bNotFound = true;
                    for (AParticleInFileStatRecord & rec : FPG->ParticleStat)
                    {
                        if (rec.NameStd == ParticleName)
                        {
                            rec.Entries++;
                            rec.Energy += energy;
                            bNotFound = false;
                            break;
                        }
                    }

                    if (bNotFound)
                        FPG->ParticleStat.push_back(AParticleInFileStatRecord(ParticleName, energy));
                }

                if (bWasParticle) bWasMulty = true;
                bWasParticle = true;
            }
            else
            {
                FPG->SetErrorString("Format error in binary file!");
                return false;
            }
        }
        if (!inStream->eof())
        {
            FPG->SetErrorString("Format error in binary file!");
            return false;
        }
        if (bWasMulty)     FPG->statNumMultipleEvents++;
        if (!bWasParticle) FPG->statNumEmptyEventsInFile++;
    }
    return true;
}

bool AFilePGEngineG4antsBin::doGenerateEvent(QVector<AParticleRecord *> &GeneratedParticles)
{
    char h;
    int eventId;
    std::string pn;
    while (inStream->get(h))
    {
        if (h == (char)0xEE)
        {
            //next event starts here
            inStream->read((char*)&eventId, sizeof(int));
            return true;
        }
        else if (h == (char)0xFF)
        {
            //data line
            AParticleRecord * p = new AParticleRecord();
            pn.clear();
            while (*inStream >> h)
            {
                if (h == (char)0x00) break;
                pn += h;
            }
            //qDebug() << pn.data();
            p->Id = -1;
            inStream->read((char*)&p->energy,   sizeof(double));
            inStream->read((char*)&p->r,      3*sizeof(double));
            inStream->read((char*)&p->v,      3*sizeof(double));
            inStream->read((char*)&p->time,     sizeof(double));
            if (inStream->fail())
            {
                FPG->SetErrorString("Unexpected format of a line in the binary file with the input particles");
                return false;
            }
            GeneratedParticles << p;
        }
        else
        {
            FPG->SetErrorString("Unexpected format of a line in the binary file with the input particles");
            return false;
        }
    }
    return false;
}

bool AFilePGEngineG4antsBin::doGenerateG4File(int eventBegin, int eventEnd, const QString &FileName)
{
    std::ofstream  outStream;
    outStream.open(FileName.toLatin1().data(), std::ios::out | std::ios::binary);
    if (!outStream.is_open())
    {
        FPG->SetErrorString("Cannot open file to export G4 pareticle data");
        return false;
    }

    int currentEvent = -1;
    int eventsToDo = eventEnd - eventBegin;
    bool bSkippingEvents = (eventBegin != 0);
    int eventId;
    std::string pn;
    double energy, time;
    double posDir[6];
    char ch;
    inStream->seekg(0);
    while (inStream->get(ch))
    {
        if (inStream->eof())
        {
            if (eventsToDo == 0) return true;
            else
            {
                FPG->SetErrorString("Unexpected end of file");
                return false;
            }
        }
        if (inStream->fail())
        {
            FPG->SetErrorString("Unexpected error during reading of a header char in the G4ants binary file");
            return false;
        }

        if (ch == (char)0xEE)
        {
            inStream->read((char*)&eventId, sizeof(int));
            if (eventsToDo == 0) return true;
            currentEvent++;

            if (bSkippingEvents && currentEvent == eventBegin) bSkippingEvents = false;

            if (!bSkippingEvents)
            {
                outStream << ch;
                outStream.write((char*)&eventId, sizeof(int));
                eventsToDo--;
            }

            continue;
        }
        else if (ch == (char)0xFF)
        {
            //data line
            pn.clear();
            while (*inStream >> ch)
            {
                if (ch == (char)0x00) break;
                pn += ch;
            }
            //qDebug() << pn.data();
            inStream->read((char*)&energy,   sizeof(double));
            inStream->read((char*)&posDir, 6*sizeof(double));
            inStream->read((char*)&time,     sizeof(double));
            if (inStream->fail())
            {
                FPG->SetErrorString("Unexpected format of a line in the G4ants binary file");
                return false;
            }
            if (!bSkippingEvents)
            {
                outStream << (char)0xFF;
                outStream << pn << (char)0x00;
                outStream.write((char*)&energy,  sizeof(double));
                outStream.write((char*)posDir, 6*sizeof(double));
                outStream.write((char*)&time,    sizeof(double));
            }
        }
        else
        {
            FPG->SetErrorString("Unexpected format of a header char in the binary file with the input particles");
            return false;
        }
    }

    if (eventsToDo == 0) return true;

    FPG->SetErrorString("Unexpected end of file");
    return false;
}

AParticleInFileStatRecord::AParticleInFileStatRecord(const std::string & NameStd, double Energy)
    : NameStd(NameStd), NameQt(NameStd.data()), Entries(1), Energy(Energy) {}

AParticleInFileStatRecord::AParticleInFileStatRecord(const QString & NameQt, double Energy)
    : NameStd(NameQt.toLatin1().data()), NameQt(NameQt), Entries(1), Energy(Energy) {}
