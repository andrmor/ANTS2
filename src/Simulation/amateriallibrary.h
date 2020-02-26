#ifndef AMATERIALLIBRARY_H
#define AMATERIALLIBRARY_H

#include <QString>

class AMaterialParticleCollection;
class QWidget;

class AMaterialLibrary
{
public:
    AMaterialLibrary(AMaterialParticleCollection * MpCollection, const QString & LibDir);

    //temporary!
    QString LoadFile(QWidget *parentWidget);

private:
     AMaterialParticleCollection * MpCollection = nullptr;
     QString LibDir;
};

#endif // AMATERIALLIBRARY_H
