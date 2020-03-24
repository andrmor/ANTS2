#ifndef AMATERIALLIBRARY_H
#define AMATERIALLIBRARY_H

class AMaterialParticleCollection;
class QWidget;

class AMaterialLoader
{
public:
    AMaterialLoader(AMaterialParticleCollection & MpCollection);

    bool LoadTmpMatFromGui(QWidget * parentWidget);  // true if loaded

private:
     AMaterialParticleCollection & MpCollection;
};

#endif // AMATERIALLIBRARY_H
