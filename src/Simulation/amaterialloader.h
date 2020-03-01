#ifndef AMATERIALLIBRARY_H
#define AMATERIALLIBRARY_H

#include <QString>

class AMaterialParticleCollection;
class QWidget;

class AMaterialLoader
{
public:
    AMaterialLoader(AMaterialParticleCollection & MpCollection);

    bool LoadTmpMatFromGui(QWidget * parentWidget);  // true if loaded

    QString LoadFile(const QString & fileName, QWidget * parentWidget); //obsolete

private:
     AMaterialParticleCollection & MpCollection;
};

#endif // AMATERIALLIBRARY_H
