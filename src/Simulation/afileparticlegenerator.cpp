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
    clearFileStat();
}

bool AFileParticleGenerator::Init()
{
    ReleaseResources();

    if (FileName.isEmpty())
    {
        ErrorString = "File name is not defined";
        return false;
    }

    bool bOK = (Mode == AParticleFileMode::Standard ? initStandardMode()
                                                    : initG4Mode() );
    return bOK;
}

bool AFileParticleGenerator::initStandardMode()
{
    File.setFileName(FileName);
    if(!File.open(QIODevice::ReadOnly | QFile::Text))
    {
        ErrorString = QString("Failed to open file: %1").arg(FileName);
        qWarning() << ErrorString;
        return false;
    }

    Stream = new QTextStream(&File);

    if (isRequiresInspection())
    {
        qDebug() << "Inspecting file:" << FileName;
        RegisteredParticleCount = MpCollection.countParticles();
        clearFileStat();  // resizes statParticleQuantity container according to RegisteredParticleCount

        FileLastModified = QFileInfo(FileName).lastModified();

        bool bContinueEvent = false;
        while (!Stream->atEnd())
        {
            const QString line = Stream->readLine();
            QStringList f = line.split(rx, QString::SkipEmptyParts);

            if (f.size() < 9) continue;

            bool bOK;
            int    pId = f.at(0).toInt(&bOK);
            if (!bOK) continue; //assuming this is a comment line
            if (pId < 0 || pId >= RegisteredParticleCount)
            {
                ErrorString = QString("Invalid particle index %1 in file %2").arg(pId).arg(FileName);
                return false;
            }
            statParticleQuantity[pId]++;

            if (!bContinueEvent) NumEventsInFile++;

            if (f.size() > 9 && f.at(9) == '*')
            {
                if (!bContinueEvent) statNumMultipleEvents++;
                bContinueEvent = true;
            }
            else bContinueEvent = false;
        }
    }
    return true;
}

bool AFileParticleGenerator::initG4Mode()
{
    bool bOK = testG4mode();
    if (!bOK) return false;

    if (bG4binary)
        inStream = new std::ifstream(FileName.toLatin1().data(), std::ios::in | std::ios::binary);
    else
        inStream = new std::ifstream(FileName.toLatin1().data());

    if (!inStream->is_open()) //paranoic
    {
        ErrorString = QString("Cannot open file %1").arg(FileName);
        return false;
    }

    qDebug() << "Need inspection?" << isRequiresInspection();
    if (isRequiresInspection())
    {
        qDebug() << "Inspecting G4 file:" << FileName;
        clearFileStat();  // resizes statParticleQuantity container according to RegisteredParticleCount


        if (bG4binary)
        {
            bool bWasParticle = true;
            bool bWasMulty    = false;
            std::string pn;
            double energy, time;
            double PosDir[6];
            char h;
            int eventId;
            while (inStream->get(h))
            {
                if (h == char(0xEE)) //new event
                {
                    inStream->read((char*)&eventId, sizeof(int));
                    NumEventsInFile++;
                    if (bWasMulty) statNumMultipleEvents++;
                    if (!bWasParticle) statNumEmptyEventsInFile++;
                    bWasMulty = false;
                    bWasParticle = false;
                }
                else if (h == char(0xFF))
                {
                    //data line
                    pn.clear();
                    while (*inStream >> h)
                    {
                        if (h == (char)0x00) break;
                        pn += h;
                    }
                    //qDebug() << pn.data();
                    inStream->read((char*)&energy,       sizeof(double));
                    inStream->read((char*)&PosDir,     6*sizeof(double));
                    inStream->read((char*)&time,         sizeof(double));
                    if (inStream->fail())
                    {
                        ErrorString = "Unexpected format of a line in the binary file with the input particles";
                        return false;
                    }

                    //statParticleQuantity[pId]++;

                    if (bWasParticle) bWasMulty = true;
                    bWasParticle = true;
                }
                else
                {
                    ErrorString = "Format error in binary file!";
                    return false;
                }
            }
            if (!inStream->eof())
            {
                ErrorString = "Format error in binary file!";
                return false;
            }
            if (bWasMulty) statNumMultipleEvents++;
            if (!bWasParticle) statNumEmptyEventsInFile++;
        }
        else
        {
            std::string str;
            bool bWasParticle = true;
            bool bWasMulty    = false;
            while (!inStream->eof())
            {
                getline( *inStream, str );
                if (str.empty())
                {
                    if (inStream->eof()) break;

                    ErrorString = "Found empty line!";
                    return false;
                }

                if (str[0] == '#')
                {
                    //new event
                    NumEventsInFile++;
                    if (bWasMulty) statNumMultipleEvents++;
                    if (!bWasParticle) statNumEmptyEventsInFile++;
                    bWasMulty = false;
                    bWasParticle = false;
                    continue;
                }

                QStringList f = QString(str.data()).split(rx, QString::SkipEmptyParts);
                //pname en x y z i j k time

                if (f.size() != 9)
                {
                    ErrorString = "Bad format of particle record!";
                    return false;
                }

                //statParticleQuantity[pId]++;

                if (bWasParticle) bWasMulty = true;
                bWasParticle = true;
            }
            if (bWasMulty) statNumMultipleEvents++;
            if (!bWasParticle) statNumEmptyEventsInFile++;
        }

        FileLastModified = QFileInfo(FileName).lastModified();
    }
    return true;
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
        qDebug() << "It seems it is a valid ascii G4ants file";
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
    inB >> ch;
    if (ch == char(0xee))
    {
        qDebug() << "It seems it is a valid binary G4ants file";
        bG4binary = true;
        return true;
    }

    ErrorString = QString("Unexpected format of the file %1").arg(FileName);
    return false;
}

bool AFileParticleGenerator::isRequiresInspection() const
{
    QFileInfo fi(FileName);

    if (FileLastModified != fi.lastModified()) return true;

    if (Mode == AParticleFileMode::Standard &&
        RegisteredParticleCount != MpCollection.countParticles()) return true;

    return false;
}

void AFileParticleGenerator::ReleaseResources()
{
    delete Stream; Stream = nullptr;
    if (File.isOpen()) File.close();

    if (inStream) inStream->close();
    delete inStream; inStream = nullptr;
}

bool AFileParticleGenerator::GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles)
{
    if (Mode == AParticleFileMode::Standard)
    {
        while (!Stream->atEnd())
        {
            if (bAbortRequested) return false;
            const QString line = Stream->readLine();
            QStringList f = line.split(rx, QString::SkipEmptyParts);
            //format: ParticleId Energy X Y Z VX VY VZ Time *  //'*' is optional - indicates event not finished yet

            if (f.size() < 9) continue;
            if (f.at(0) == '#') continue;

            bool bOK;
            int    pId    = f.at(0).toInt(&bOK);
            if (!bOK) continue;
            //TODO protection of wrong index, either test on start

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
    else if (bG4binary)
    {
        char h;
        int eventId;
        std::string pn;
        while (inStream->get(h))
        {
            if (h == char(0xEE))
            {
                //next event starts here
                inStream->read((char*)&eventId,    sizeof(int));
                return true;
            }
            else if (h == char(0xFF))
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
                inStream->read((char*)&p->energy,    sizeof(double));
                inStream->read((char*)&p->r,       3*sizeof(double));
                inStream->read((char*)&p->v,       3*sizeof(double));
                inStream->read((char*)&p->time,      sizeof(double));
                if (inStream->fail())
                {
                    ErrorString = "Unexpected format of a line in the binary file with the input particles";
                    return false;
                }
                GeneratedParticles << p;
            }
            else
            {
                ErrorString = "Unexpected format of a line in the binary file with the input particles";
                return false;
            }
        }
        return false;
    }
    else
    {
        // ascii G4
        std::string str;
        while ( getline(*inStream, str) )
        {
            if (str.empty())
            {
                if (inStream->eof()) return false;
                ErrorString = "Found empty line!";
                return false;
            }

            if (str[0] == '#') return true; //new event

            QStringList f = QString(str.data()).split(rx, QString::SkipEmptyParts); //pname en time x y z i j k
            if (f.size() != 9)
            {
                ErrorString = "Bad format of particle record!";
                return false;
            }
            //qDebug() << "Ascii, "<<f.at(0);

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

    return false;
}

bool AFileParticleGenerator::IsParticleInUse(int particleId, QString &SourceNames) const
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

    Mode = AParticleFileMode::Standard;
    if (json.contains("Mode"))
    {
        int im;
        parseJson(json, "Mode", im);
        if (im > -1 && im < 2)
            Mode = static_cast<AParticleFileMode>(im);
    }

    return parseJson(json, "FileName", FileName);
}

void AFileParticleGenerator::SetStartEvent(int startEvent)
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

void AFileParticleGenerator::InvalidateFile()
{
    clearFileStat();
}

bool AFileParticleGenerator::IsValidated() const
{
    QFileInfo fi(FileName);

    if (!fi.exists()) return false;
    if (FileLastModified != fi.lastModified()) return false;
    if (Mode == AParticleFileMode::Standard && RegisteredParticleCount != MpCollection.countParticles()) return false;

    return true;
}

const QString AFileParticleGenerator::GetEventRecords(int fromEvent, int toEvent) const
{
    QString s;
    if (Stream)
    {
        Stream->seek(0);

        int event = -1;
        bool bContinueEvent = false;
        while (!Stream->atEnd())
        {
            const QString line = Stream->readLine();
            QStringList f = line.split(rx, QString::SkipEmptyParts);
            if (f.size() < 9) continue;
            if (f.at(0) == '#') continue;  //comment line
            bool bOK;
            f.at(0).toInt(&bOK);
            if (!bOK) continue; //assuming this is a comment line

            if (!bContinueEvent) event++;

            if (event >= fromEvent && event < toEvent) s += line;

            if (f.size() > 9 && f.at(9) == '*')
                bContinueEvent = true;
            else
                bContinueEvent = false;
        }

    }
    return s;
}

bool AFileParticleGenerator::generateG4File(int eventBegin, int eventEnd, const QString & FileName)
{
    qDebug() << eventBegin << eventEnd;

    std::ofstream  outStream;
    if (bG4binary) outStream.open(FileName.toLatin1().data(), std::ios::out | std::ios::binary);
    else           outStream.open(FileName.toLatin1().data());

    if (!outStream.is_open())
    {
        ErrorString = "Cannot open file to export G4 pareticle data";
        return false;
    }

    int currentEvent = -1;
    int eventsToDo = eventEnd - eventBegin;
    bool bSkippingEvents = (eventBegin != 0);
    if (bG4binary)
    {
        int eventId;
        std::string pn;
        double energy, time;
        double posDir[6];
        char ch;
        while (inStream->get(ch))
        {
            if (inStream->eof())
            {
                if (eventsToDo == 0) return true;
                else
                {
                    ErrorString = "Unexpected end of file";
                    return false;
                }
            }
            if (inStream->fail())
            {
                ErrorString = "Unexpected error during reading of a header char in the G4ants binary file";
                return false;
            }

            if (ch == (char)0xEE)
            {
                inStream->read((char*)&eventId,    sizeof(int));
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
                inStream->read((char*)&energy,    sizeof(double));
                inStream->read((char*)&posDir,  6*sizeof(double));
                inStream->read((char*)&time,      sizeof(double));
                if (inStream->fail())
                {
                    ErrorString = "Unexpected format of a line in the G4ants binary file";
                    return false;
                }
                if (!bSkippingEvents)
                {
                    outStream << pn << char(0x00);
                    outStream.write((char*)&energy,  sizeof(double));
                    outStream.write((char*)posDir, 6*sizeof(double));
                    outStream.write((char*)&time,    sizeof(double));
                }
            }
            else
            {
                ErrorString = "Unexpected format of a header char in the binary file with the input particles";
                return false;
            }
        }
    }
    else
    {
        std::string str;
        while ( getline(*inStream, str) )
        {
            if (inStream->eof())
            {
                if (eventsToDo == 0) return true;
                else
                {
                    ErrorString = "Unexpected end of file";
                    return false;
                }
            }
            if (str.empty())
            {
                ErrorString = "Found empty line!";
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

            if (!bSkippingEvents)
            {
                outStream << str << std::endl;
            }
        }
    }

    if (eventsToDo == 0) return true;

    ErrorString = "Unexpected end of file";
    return false;
}

void AFileParticleGenerator::clearFileStat()
{
    FileLastModified = QDateTime(); //will force to inspect file on next use

    NumEventsInFile = 0;
    statNumEmptyEventsInFile = 0;
    statNumMultipleEvents = 0;

    statParticleQuantity.clear();
    if (RegisteredParticleCount >= 0)
        statParticleQuantity =  QVector<int>(RegisteredParticleCount, 0);
}
