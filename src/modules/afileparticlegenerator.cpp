#include "afileparticlegenerator.h"
#include "amaterialparticlecolection.h"
#include "aparticlerecord.h"
#include "ajsontools.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QFileInfo>

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
    File.setFileName(FileName);
    if(!File.open(QIODevice::ReadOnly | QFile::Text))
    {
        ErrorString = QString("Failed to open file: %1").arg(FileName);
        qWarning() << ErrorString;
        return false;
    }

    Stream = new QTextStream(&File);

    QFileInfo fi(File);
    if (FileLastModified != fi.lastModified() || RegisteredParticleCount != MpCollection.countParticles()) //requires inspection
    {
        qDebug() << "Inspecting file:" << FileName;
        RegisteredParticleCount = MpCollection.countParticles();
        clearFileStat();  // resizes statParticleQuantity container according to RegisteredParticleCount

        FileLastModified = fi.lastModified();

        bool bContinueEvent = false;
        while (!Stream->atEnd())
        {
            const QString line = Stream->readLine();
            QStringList f = line.split(rx, QString::SkipEmptyParts);

            if (f.size() < 8) continue;

            bool bOK;
            int    pId = f.at(0).toInt(&bOK);
            if (!bOK) continue; //assuming this is a comment line
            if (pId < 0 || pId >= RegisteredParticleCount)
            {
                ErrorString = QString("Invalid particle index %1 in file %2").arg(pId).arg(FileName);
                return false;
            }
            statParticleQuantity[pId]++;

            /*
            double energy = f.at(1).toDouble();
            double x =      f.at(2).toDouble();
            double y =      f.at(3).toDouble();
            double z =      f.at(4).toDouble();
            double vx =     f.at(5).toDouble();
            double vy =     f.at(6).toDouble();
            double vz =     f.at(7).toDouble();
            */

            if (!bContinueEvent) NumEventsInFile++;

            if (f.size() > 8 && f.at(8) == '*')
            {
                if (!bContinueEvent) statNumMultipleEvents++;
                bContinueEvent = true;
            }
            else bContinueEvent = false;
        }
    }
    return true;
}

void AFileParticleGenerator::ReleaseResources()
{
    delete Stream; Stream = 0;
    if (File.isOpen()) File.close();
}

bool AFileParticleGenerator::GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles)
{
    while (!Stream->atEnd())
    {
        if (bAbortRequested) return false;
        const QString line = Stream->readLine();
        QStringList f = line.split(rx, QString::SkipEmptyParts);
        //format: ParticleId Energy X Y Z VX VY VZ *  //'*' is optional - indicates event not finished yet

        if (f.size() < 8) continue;

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

        AParticleRecord* p = new AParticleRecord(pId,
                                                 x, y, z,
                                                 vx, vy, vz,
                                                 0, energy);
        p->ensureUnitaryLength();
        GeneratedParticles << p;

        if (f.size() > 8 && f.at(8) == '*') continue; //this is multiple event!
        return true; //normal termination
    }

    return false; //could not read particle record in file!
}

bool AFileParticleGenerator::IsParticleInUse(int particleId, QString &SourceNames) const
{
    return false; //TODO
}

void AFileParticleGenerator::writeToJson(QJsonObject &json) const
{
    json["FileName"] = FileName;

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
            if (f.size() < 8) continue;
            bool bOK;
            f.at(0).toInt(&bOK);
            if (!bOK) continue; //assuming this is a comment line

            if (!bContinueEvent) event++;

            if (f.size() > 8 && f.at(8) == '*')
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
    return (fi.exists() && FileLastModified == fi.lastModified() && RegisteredParticleCount == MpCollection.countParticles());
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
            if (f.size() < 8) continue;
            bool bOK;
            f.at(0).toInt(&bOK);
            if (!bOK) continue; //assuming this is a comment line

            if (!bContinueEvent) event++;

            if (event >= fromEvent && event < toEvent) s += line;

            if (f.size() > 8 && f.at(8) == '*')
                bContinueEvent = true;
            else
                bContinueEvent = false;
        }

    }
    return s;
}

void AFileParticleGenerator::clearFileStat()
{
    FileLastModified = QDateTime(); //will force to inspect file on next use

    NumEventsInFile = 0;
    statNumMultipleEvents = 0;
    statParticleQuantity = QVector<int>(RegisteredParticleCount, 0);
}
