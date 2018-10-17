#include "afileparticlegenerator.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>

AFileParticleGenerator::AFileParticleGenerator(const QString &FileName) :
    FileName(FileName) {}

void AFileParticleGenerator::Init()
{
    File.setFileName(FileName);
    if(!File.open(QIODevice::ReadOnly | QFile::Text))
    {
        qWarning() << "Could not open: " << FileName;
        return false;
    }

    Stream = new QTextStream(&file);
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

    QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'

    /*
    x->resize(0);

    while(!in.atEnd())
         {
            QString line = in.readLine();
            QStringList fields = line.split(rx, QString::SkipEmptyParts);

            bool ok1= false;
            double xx;
            if (fields.size()>0) xx = fields[0].toDouble(&ok1);  //*** potential problem with decimal separator!

            if (ok1)
              {
                x->append(xx);
              }
          }
   */
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
