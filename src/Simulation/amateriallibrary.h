#ifndef AMATERIALLIBRARY_H
#define AMATERIALLIBRARY_H

#include <QString>

class AMaterialParticleCollection;
class QWidget;

class AMaterialLibrary
{
public:
    AMaterialLibrary(AMaterialParticleCollection & MpCollection);

    QString LoadFile(const QString & fileName, QWidget * parentWidget);

private:
     AMaterialParticleCollection & MpCollection;
};

#endif // AMATERIALLIBRARY_H
