#ifndef AMATERIALLIBRARY_H
#define AMATERIALLIBRARY_H

class AMaterialParticleCollection;

#ifdef GUI
class QWidget;
#endif

class AMaterialLoader
{
public:
    AMaterialLoader(AMaterialParticleCollection & MpCollection);

#ifdef GUI
    bool LoadTmpMatFromGui(QWidget * parentWidget);  // true if loaded
#endif

private:
     AMaterialParticleCollection & MpCollection;
};

#endif // AMATERIALLIBRARY_H
