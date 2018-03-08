#ifndef AROOTOBJCOLLECTION_H
#define AROOTOBJCOLLECTION_H

#include <QMap>
#include <QString>
#include <QMutex>

class ARootObjBase;

class ARootObjCollection
{
public:
    ~ARootObjCollection();

    bool append(const QString& name, ARootObjBase* record);

    ARootObjBase* getRecord(const QString& name);   // Unlocks mutex on return!

    bool remove(const QString& name);               // Not multithread-safe: graph can be in use by the GUI
    void clear();                                   // Not multithread-safe: graph can be in use by the GUI

private:
    QMap<QString, ARootObjBase*> Collection;
    QMutex        Mutex;

};

#endif // AROOTOBJCOLLECTION_H
