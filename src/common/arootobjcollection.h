#ifndef AROOTOBJCOLLECTION_H
#define AROOTOBJCOLLECTION_H

#include <QMap>
#include <QString>
#include <QMutex>

#include "arootgraphrecord.h"

class TObject;

template <typename T>
class ARootObjCollection
{
public:
    ~ARootObjCollection() {clear();}

    bool append(TObject* obj, const QString& name, const QString& type)
    {
        if (!obj) return false;
        QMutexLocker locker(&Mutex);
        if (Collection.contains(name)) return false;
        T* rec = new T(obj, name, type);
        Collection.insert(name, rec);
        return true;
    }

    T* getRecord(const QString& name) // Unlocks mutex on return!
    {
        QMutexLocker locker(&Mutex);
        return Collection.value(name, 0);
    }

    bool remove(const QString& name)  // Not multithread-safe: graph can be in use by the GUI
    {
        QMutexLocker locker(&Mutex);
        if (Collection.contains(name)) return false;
        delete Collection[name];
        Collection.remove(name);
        return true;
    }
    void clear() // Not multithread-safe: graph can be in use by the GUI
    {
        QMutexLocker locker(&Mutex);
        QMapIterator<QString, T*> iter(Collection);
        while (iter.hasNext())
        {
            iter.next();
            delete iter.value();
        }
        Collection.clear();
    }

private:
    QMap<QString, T*> Collection;
    QMutex        Mutex;

};

#endif // AROOTOBJCOLLECTION_H
