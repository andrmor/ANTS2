#include "afileparticlegenerator.h"
#include "aparticlesimsettings.h"
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
#include <istream>
#include <iostream>

AFileParticleGenerator::AFileParticleGenerator(const AFileGenSettings & Settings, const AMaterialParticleCollection & MpCollection) :
    ConstSettings(Settings), MpCollection(MpCollection) {}

AFileParticleGenerator::~AFileParticleGenerator()
{
    ReleaseResources();
}

bool AFileParticleGenerator::Init()
{
    ReleaseResources();

    switch (ConstSettings.FileFormat)
    {
    case AFileGenSettings::Simplistic:
        Engine = new AFilePGEngineSimplistic(this);
        break;
    case AFileGenSettings::G4Binary:
        Engine = new AFilePGEngineG4antsBin(this);
        break;
    case AFileGenSettings::G4Ascii:
        Engine = new AFilePGEngineG4antsTxt(this);
        break;
    default:
        ErrorString = "Invalid file format";
        return false;
    }

    return Engine->doInit();
}

#include <algorithm>
bool AFileParticleGenerator::InitWithCheck(AFileGenSettings & settings, bool bExpanded)
{
    ReleaseResources();

    if (settings.FileName.isEmpty())
    {
        ErrorString = "File name is not defined";
        return false;
    }

    bool bNeedInspect = !settings.isValidated() || bExpanded;
    //qDebug() << "Init called, requires inspect?" << bNeedInspect;
    if (bNeedInspect) settings.clearStatistics();

    bool bOK = determineFileFormat(settings);
    if (!bOK) return false;

    switch (settings.FileFormat)
    {
    case AFileGenSettings::Simplistic:
        Engine = new AFilePGEngineSimplistic(this);
        break;
    case AFileGenSettings::G4Binary:
        Engine = new AFilePGEngineG4antsBin(this);
        break;
    case AFileGenSettings::G4Ascii:
        Engine = new AFilePGEngineG4antsTxt(this);
        break;
    default:
        ErrorString = "Invalid file format";
        return false;
    }

    if (bNeedInspect)
        bOK = Engine->doInitAndInspect(settings, bExpanded);
    else
        bOK = Engine->doInit();

    if (bOK && bNeedInspect)
    {
        settings.LastValidationMode = settings.ValidationMode;
        settings.ValidatedWithParticles = MpCollection.getListOfParticleNames();
        settings.FileLastModified = QFileInfo(settings.FileName).lastModified();
    }

    if (bOK && settings.ParticleStat.size() > 0)
    {
        std::sort(settings.ParticleStat.begin(), settings.ParticleStat.end(),
                  [](const AParticleInFileStatRecord & l, const AParticleInFileStatRecord & r)
                  {
                    return (l.Entries > r.Entries);
                  });
    }
    return bOK;
}

bool AFileParticleGenerator::determineFileFormat(AFileGenSettings & settings)
{
    if (isFileG4Binary(settings))
    {
        //qDebug() << "Assuming it is G4 bin format";
        settings.FileFormat = AFileGenSettings::G4Binary;
        return true;
    }

    if (isFileG4Ascii(settings))
    {
        //qDebug() << "Assuming it is G4 ascii format";
        settings.FileFormat = AFileGenSettings::G4Ascii;
        return true;
    }

    if (isFileSimpleAscii(settings))
    {
        //qDebug() << "Assuming it is simplistic format";
        settings.FileFormat = AFileGenSettings::Simplistic;
        return true;
    }

    settings.FileFormat = AFileGenSettings::BadFormat;
    if (!ErrorString.isEmpty()) return false;

    ErrorString = QString("Unexpected format of the file %1").arg(settings.FileName);
    return false;
}

bool AFileParticleGenerator::isFileG4Binary(AFileGenSettings & settings)
{
    std::ifstream inB(settings.FileName.toLatin1().data(), std::ios::in | std::ios::binary);
    if (!inB.is_open())
    {
        ErrorString = QString("Cannot open file %1").arg(settings.FileName);
        return false;
    }
    char ch;
    inB.get(ch);
    if (ch == (char)0xEE)
    {
        int iEvent = -1;
        inB.read((char*)&iEvent, sizeof(int));
        if (iEvent == 0) return true;
    }
    return false;
}

bool AFileParticleGenerator::isFileG4Ascii(AFileGenSettings & settings)
{
    std::ifstream inT(settings.FileName.toLatin1().data());
    if (!inT.is_open())
    {
        ErrorString = QString("Cannot open file %1").arg(settings.FileName);
        return false;
    }

    std::string str;
    getline(inT, str);
    inT.close();
    if (str.size() > 1 && str[0] == '#')
    {
        QString line = QString(str.data()).mid(1);
        bool bOK;
        int iEvent = line.toInt(&bOK);
        if (bOK && iEvent == 0) return true;
    }
    return false;
}

bool AFileParticleGenerator::isFileSimpleAscii(AFileGenSettings & settings)
{
    QFile file(settings.FileName);
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
        ErrorString = QString("Cannot open file %1").arg(settings.FileName);
        return false;
    }

    const QRegularExpression rx = QRegularExpression("(\\ |\\,|\\:|\\t)");  // separators are: ' ' or ',' or ':' or '\t'
    QTextStream Stream(&file);
    while (!Stream.atEnd())
    {
        const QString line = Stream.readLine().simplified();
        if (line.isEmpty()) continue;
        if (line.startsWith('#')) continue;
        const QStringList f = line.split(rx, QString::SkipEmptyParts);
        if ( f.size() == 9 ||
            (f.size() == 10 && f.at(9) == '*'))
        {
            bool bOK;
            int    pId = f.at(0).toInt(&bOK);
            if (!bOK) return false;
            if (pId < 0 || pId >= MpCollection.countParticles()) return false;
            for (int i = 1; i < 9; i++)
            {
                f.at(i).toDouble(&bOK);
                if (!bOK) return false;
            }
            return true;
        }
    }
    return false;
}

void AFileParticleGenerator::ReleaseResources()
{
    delete Engine; Engine = nullptr;
}

bool AFileParticleGenerator::GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles, int /*iEvent*/)
{
    if (!Engine)
    {
        ErrorString = "Generator is not configured!";
        return false;
    }

    return Engine->doGenerateEvent(GeneratedParticles);
}

void AFileParticleGenerator::SetStartEvent(int startEvent)
{
    if (Engine) Engine->doSetStartEvent(startEvent);
}

bool AFileParticleGenerator::generateG4File(int eventBegin, int eventEnd, const QString & FileName)
{
    if (Engine)
        return Engine->doGenerateG4File(eventBegin, eventEnd, FileName);
    else
        return false;
}

// ***************************************************************************

AFilePGEngineSimplistic::~AFilePGEngineSimplistic()
{
    delete Stream; Stream = nullptr;
    if (File.isOpen()) File.close();
}

bool AFilePGEngineSimplistic::doInit()
{
    File.setFileName(FPG->ConstSettings.FileName);
    if(!File.open(QIODevice::ReadOnly | QFile::Text))
    {
        FPG->SetErrorString( QString("Failed to open file: %1").arg(FPG->ConstSettings.FileName) );
        return false;
    }

    Stream = new QTextStream(&File);
    return true;
}

bool AFilePGEngineSimplistic::doInitAndInspect(AFileGenSettings & settings, bool bDetailedInspection)
{
    File.setFileName(settings.FileName);
    if (!File.open(QIODevice::ReadOnly | QFile::Text))
    {
        FPG->SetErrorString( QString("Failed to open file: %1").arg(settings.FileName) );
        return false;
    }

    Stream = new QTextStream(&File);

    //qDebug() << "Inspecting file (standard format):" << FileName;
    bool bContinueEvent = false;
    while (!Stream->atEnd())
    {
        const QString line = Stream->readLine();
        const QStringList f = line.split(rx, QString::SkipEmptyParts);
        if (f.size() < 9) continue;

        bool bOK;
        int  pId = f.at(0).toInt(&bOK);
        if (!bOK) continue; //assuming this is a comment line
        if (!settings.isValidParticle(pId))
        {
            FPG->SetErrorString( QString("Invalid particle index %1 in file %2").arg(pId).arg(settings.FileName) );
            return false;
        }

        if (bDetailedInspection)
        {
            const QString & name = f.first(); // I know it is int, reusing G4ants-ish infrastructure
            bool bNotFound = true;
            for (AParticleInFileStatRecord & rec : settings.ParticleStat)
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
                settings.ParticleStat.push_back(AParticleInFileStatRecord(name, f.at(1).toDouble()));
        }

        if (!bContinueEvent) settings.NumEventsInFile++;

        if (f.size() > 9 && f.at(9) == '*')
        {
            if (!bContinueEvent) settings.statNumMultipleEvents++;
            bContinueEvent = true;
        }
        else bContinueEvent = false;
    }

    if (bDetailedInspection)
        for (AParticleInFileStatRecord & rec : settings.ParticleStat)
            rec.NameQt = FPG->MpCollection.getParticleName(rec.NameQt.toInt()); //index is valid

    return true;
}

bool AFilePGEngineSimplistic::doGenerateEvent(QVector<AParticleRecord *> &GeneratedParticles)
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

bool AFilePGEngineSimplistic::doSetStartEvent(int startEvent)
{
    if (Stream)
    {
        Stream->seek(0);
        if (startEvent == 0) return true;

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
                if (event == startEvent-1) return true;
            }
        }
    }
    return false;
}

bool AFilePGEngineSimplistic::doGenerateG4File(int /*eventBegin*/, int /*eventEnd*/, const QString & /*FileName*/)
{
    /*
    QFile file(FileName);
    if(!file.open(QIODevice::WriteOnly | QFile::Text))
    {
        FPG->SetErrorString("Cannot open file to export primaries for G4ants sim");
        return false;
    }
    QTextStream out(&file);

    Stream->seek(0);
    bool bContinueEvent = false;
    int iEvent = -1;
    while (!Stream->atEnd())
    {
        const QString line = Stream->readLine();
        const QStringList f = line.split(rx, QString::SkipEmptyParts);
        if (f.size() < 9 || f.at(0) == '#') continue;

        bool bOK;
        int pId = f.at(0).toInt(&bOK);
        if (!bOK) continue; //assuming this is a comment line
        if (pId < 0 || pId >= FPG->RegisteredParticleCount)
        {
            FPG->SetErrorString( QString("Invalid particle index %1 in file %2").arg(pId).arg(FileName) );
            return false;
        }

        //assuming it is a valid particle line
        if (!bContinueEvent)
        {
            iEvent++;
            if (iEvent >= eventEnd) return true;
            if (iEvent < eventBegin)
            {
                if (f.size() > 9 && f.at(9) == '*') bContinueEvent = true;
                continue;
            }
        }

        // todo:
        // if new event, save event marker
        // save particle record

        if (f.size() > 9 && f.at(9) == '*') bContinueEvent = true;
    }
    */
    return false;
}

AFilePGEngineG4antsTxt::~AFilePGEngineG4antsTxt()
{
    if (inStream) inStream->close();
    delete inStream; inStream = nullptr;
}

bool AFilePGEngineG4antsTxt::doInit()
{
    inStream = new std::ifstream(FPG->ConstSettings.FileName.toLatin1().data());
    if (!inStream->is_open())
    {
        FPG->SetErrorString( QString("Cannot open file %1").arg(FPG->ConstSettings.FileName) );
        return false;
    }
    return true;
}

bool AFilePGEngineG4antsTxt::doInitAndInspect(AFileGenSettings & settings, bool bDetailedInspection)
{
    inStream = new std::ifstream(settings.FileName.toLatin1().data());

    if (!inStream->is_open())
    {
        FPG->SetErrorString( QString("Cannot open file %1").arg(settings.FileName) );
        return false;
    }

        //qDebug() << "Inspecting G4ants-generated txt file:" << FileName;
        std::string str;
        bool bWasParticle = true;
        bool bWasMulty    = false;
        while (!inStream->eof())
        {
            getline( *inStream, str );
            if (str.empty())
            {
                if (inStream->eof())
                {
                    inStream->clear();  // will reuse the stream
                    break;
                }

                FPG->SetErrorString("Found empty line!");
                return false;
            }

            if (str[0] == '#')
            {
                //new event
                settings.NumEventsInFile++;
                if (bWasMulty)     settings.statNumMultipleEvents++;
                if (!bWasParticle) settings.statNumEmptyEventsInFile++;
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

            QString name = f.first();
            //kill [***] appearing in ion names
            int iBracket = name.indexOf('[');
            if (iBracket != -1) name = name.left(iBracket);
            if (!settings.isValidParticle(name))
            {
                FPG->SetErrorString("Found particle not defined in this ANTS2 configuration: " + name);
                return false;
            }

            if (bDetailedInspection)
            {
                bool bNotFound = true;
                for (AParticleInFileStatRecord & rec : settings.ParticleStat)
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
                    settings.ParticleStat.push_back(AParticleInFileStatRecord(name, f.at(1).toDouble()));
            }

            if (bWasParticle) bWasMulty = true;
            bWasParticle = true;
        }
        if (bWasMulty)     settings.statNumMultipleEvents++;
        if (!bWasParticle) settings.statNumEmptyEventsInFile++;

        return true;
}

bool AFilePGEngineG4antsTxt::doGenerateEvent(QVector<AParticleRecord *> &GeneratedParticles)
{
    std::string str;
    while ( getline(*inStream, str) )
    {
        qDebug() << str.data();
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

        QString name = f.first();
        //kill [***] appearing in ion names
        int iBracket = name.indexOf('[');
        if (iBracket != -1) name = name.left(iBracket);

        AParticleRecord * p = new AParticleRecord();
        p->Id     = FPG->MpCollection.getParticleId(name);  // invalid particles will be found during init phase
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

    if (inStream->eof()) return true;
    return false;
}

bool AFilePGEngineG4antsTxt::doSetStartEvent(int startEvent)
{
    if (!inStream) return false;

    inStream->seekg(0);

    std::string str;

    while ( getline(*inStream, str) )
    {
        if (str[0] == '#') //new event
        {
            const int iEvent = std::stoi( str.substr(1) );  // kill the leading '#' - the file was already verified at this stage!
            if (iEvent == startEvent)
                return true;
        }
    }
    return false;
}

bool AFilePGEngineG4antsTxt::doGenerateG4File(int eventBegin, int eventEnd, const QString &FileName)
{
    std::ofstream outStream;
    outStream.open(FileName.toLatin1().data());
    if (!outStream.is_open())
    {
        FPG->SetErrorString("Cannot open file to export primaries for G4ants sim");
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

bool AFilePGEngineG4antsBin::doInit()
{
    inStream = new std::ifstream(FPG->ConstSettings.FileName.toLatin1().data(), std::ios::in | std::ios::binary);
    if (!inStream->is_open()) //paranoic
    {
        FPG->SetErrorString( QString("Cannot open file %1").arg(FPG->ConstSettings.FileName) );
        return false;
    }
    inStream->clear();
    return true;
}

bool AFilePGEngineG4antsBin::doInitAndInspect(AFileGenSettings & settings, bool bDetailedInspection)
{
    inStream = new std::ifstream(settings.FileName.toLatin1().data(), std::ios::in | std::ios::binary);

    if (!inStream->is_open()) //paranoic
    {
        FPG->SetErrorString( QString("Cannot open file %1").arg(settings.FileName) );
        return false;
    }

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
                settings.NumEventsInFile++;
                if (bWasMulty)     settings.statNumMultipleEvents++;
                if (!bWasParticle) settings.statNumEmptyEventsInFile++;
                bWasMulty = false;
                bWasParticle = false;
            }
            else if (h == char(0xFF))
            {
                //data line
                ParticleName.clear();
                bool bCopyToName = true;
                while (*inStream >> h)
                {
                    if (h == (char)0x00) break;
                    if (bCopyToName)
                    {
                        if (h == '[') bCopyToName = false;
                        else ParticleName += h;
                    }
                }

                if (!settings.isValidParticle(ParticleName.data()))
                {
                    FPG->SetErrorString( QString("Found particle not defined in this ANTS2 configuration: %1").arg(ParticleName.data()) );
                    return false;
                }

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
                    bool bNotFound = true;
                    for (AParticleInFileStatRecord & rec : settings.ParticleStat)
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
                        settings.ParticleStat.push_back(AParticleInFileStatRecord(ParticleName, energy));
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
        if (bWasMulty)     settings.statNumMultipleEvents++;
        if (!bWasParticle) settings.statNumEmptyEventsInFile++;

    inStream->clear();
    return true;
}

bool AFilePGEngineG4antsBin::doGenerateEvent(QVector<AParticleRecord *> & GeneratedParticles)
{
    char h;
    int eventId;
    std::string pn;
    while (inStream->get(h))
    {
        if (h == (char)0xEE)
        {
            //next event starts here!
            inStream->read((char*)&eventId, sizeof(int));
            return true;
        }
        else if (h == (char)0xFF)
        {
            //data line
            pn.clear();
            bool bCopyToName = true;
            while (*inStream >> h)
            {
                if (h == (char)0x00) break;

                if (bCopyToName)
                {
                    if (h == '[') bCopyToName = false;
                    else pn += h;
                }
            }

            AParticleRecord * p = new AParticleRecord();
            p->Id = FPG->MpCollection.getParticleId(pn.data());
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

    if (inStream->eof()) return true;
    return false;
}

bool AFilePGEngineG4antsBin::doSetStartEvent(int startEvent)
{
    inStream->seekg(0);

    char h;
    int eventId;
    double buf[8];
    while (inStream->get(h))
    {
        if (h == (char)0xEE)
        {
            //next event starts here
            inStream->read((char*)&eventId, sizeof(int));
            //qDebug() << "Event Id in seaching for" << startEvent << " found:" << eventId;
            if (eventId == startEvent)
                return true;
        }
        else if (h == (char)0xFF)
        {
            //data line
            while (*inStream >> h)
            {
                if (h == (char)0x00) break;
            }
            inStream->read((char*)buf, 8*sizeof(double));
        }
        else
        {
            qWarning() << "Unexpected format of a line in the binary file with the input particles";
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
        FPG->SetErrorString("CCannot open file to export primaries for G4ants sim");
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
