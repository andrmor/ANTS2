#include "afileparticlegenerator.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStringList>

AFileParticleGenerator::AFileParticleGenerator(const QString &FileName) :
    FileName(FileName) {}

bool AFileParticleGenerator::Init()
{
    File.setFileName(FileName);
    if(!File.open(QIODevice::ReadOnly | QFile::Text))
    {
        qWarning() << "Could not open: " << FileName;
        return false;
    }

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

        int    pId    = f.at(0).toInt(); //TODO index check
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

bool AFileParticleGenerator::CheckConfiguration()
{
    return true; //TODO
}

bool AFileParticleGenerator::IsParticleInUse(int particleId, QString &SourceNames) const
{
    return false; //TODO
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

        int    pId    = f.at(0).toInt();
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
