#include "afileparticlegenerator.h"
#include "amaterialparticlecolection.h"
#include "ajsontools.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QFileInfo>

AFileParticleGenerator::AFileParticleGenerator(const AMaterialParticleCollection & MpCollection) :
    MpCollection(MpCollection) {}

void AFileParticleGenerator::SetFileName(const QString &fileName)
{
    if (FileName == fileName) return;

    FileName = fileName;
    clearFileStat();
}

bool AFileParticleGenerator::Init()
{
    if (File.isOpen()) File.close();
    File.setFileName(FileName);

    if(!File.open(QIODevice::ReadOnly | QFile::Text))
    {
        ErrorString = QString("Failed to open file: %1").arg(FileName);
        qWarning() << ErrorString;
        return false;
    }

    if (Stream) delete Stream; Stream = 0;
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

            if (!bContinueEvent) statNumEvents++;

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
    File.close();
}

QVector<AGeneratedParticle> * AFileParticleGenerator::GenerateEvent()
{
    QVector<AGeneratedParticle>* GeneratedParticles = new QVector<AGeneratedParticle>;

    while (!Stream->atEnd())
    {
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

        (*GeneratedParticles) << AGeneratedParticle(pId, energy, x, y, z, vx, vy, vz);

        if (f.size() > 8 && f.at(8) == '*') continue;
        break;
    }
    return GeneratedParticles;
}

const QString AFileParticleGenerator::CheckConfiguration() const
{
    return ""; //TODO
}

void AFileParticleGenerator::RemoveParticle(int)
{
    qWarning() << "Remove particle has no effect for AFileParticleGenerator";
}

bool AFileParticleGenerator::IsParticleInUse(int particleId, QString &SourceNames) const
{
    return false; //TODO
}

void AFileParticleGenerator::writeToJson(QJsonObject &json) const
{
    json["FileName"] = FileName;
}

bool AFileParticleGenerator::readFromJson(const QJsonObject &json)
{
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
            int  pId = f.at(0).toInt(&bOK);
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
    QFileInfo fi(File);
    return (fi.exists() && FileLastModified == fi.lastModified() && RegisteredParticleCount == MpCollection.countParticles());
}

void AFileParticleGenerator::clearFileStat()
{
    FileLastModified = QDateTime(); //will force to inspect file on next use

    statNumEvents = 0;
    statNumMultipleEvents = 0;
    statParticleQuantity = QVector<int>(RegisteredParticleCount, 0);
}
