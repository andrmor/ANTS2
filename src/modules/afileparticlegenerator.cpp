#include "afileparticlegenerator.h"
#include "ajsontools.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStringList>

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

    while(!Stream->atEnd())
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

const AParticleFileStat AFileParticleGenerator::InspectFile(const QString &fname, int ParticleCount)
{
    AParticleFileStat stat;
    stat.ParticleStat = QVector<int>(ParticleCount, 0);

    QFile file(fname);
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
        stat.ErrorString = QString("Cannot open file %1").arg(fname);
        return stat;
    }

    QRegularExpression rx("(\\ |\\,|\\:|\\t)"); //Not synchronized with the class!
    QTextStream in(&file);
    bool bContinueEvent = false;
    while(!in.atEnd())
    {
        const QString line = in.readLine();
        QStringList f = line.split(rx, QString::SkipEmptyParts);

        if (f.size() < 8) continue;

        bool bOK;
        int    pId = f.at(0).toInt(&bOK);
        if (!bOK) continue; //assuming this is a comment line
        if (pId < 0 || pId >= ParticleCount)
        {
            stat.ErrorString = QString("Invalid particle index %1 in file %2").arg(pId).arg(fname);
            return stat;
        }
        stat.ParticleStat[pId]++;

        /*
        double energy = f.at(1).toDouble();
        double x =      f.at(2).toDouble();
        double y =      f.at(3).toDouble();
        double z =      f.at(4).toDouble();
        double vx =     f.at(5).toDouble();
        double vy =     f.at(6).toDouble();
        double vz =     f.at(7).toDouble();
        */

        if (!bContinueEvent) stat.numEvents++;

        if (f.size() > 8 && f.at(8) == '*')
        {
            if (!bContinueEvent) stat.numMultipleEvents++;
            bContinueEvent = true;
        }
        else bContinueEvent = false;
    }

    file.close();
    return stat;
}
