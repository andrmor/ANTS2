#include "arootobjcollection.h"
#include "arootobjbase.h"

ARootObjCollection::~ARootObjCollection()
{
    clear();
}

bool ARootObjCollection::append(const QString &name, ARootObjBase* record)
{
    QMutexLocker locker(&Mutex);

    if (Collection.contains(name)) return false;

    Collection.insert(name, record);
    return true;
}

ARootObjBase *ARootObjCollection::getRecord(const QString &name)
{
    QMutexLocker locker(&Mutex);
    return Collection.value(name, 0);
}

bool ARootObjCollection::remove(const QString &name)
{
    QMutexLocker locker(&Mutex);

    if (Collection.contains(name)) return false;

    delete Collection[name];
    Collection.remove(name);

    return true;
}

void ARootObjCollection::clear()
{
    QMutexLocker locker(&Mutex);
    QMapIterator<QString, ARootObjBase*> iter(Collection);
    while (iter.hasNext())
    {
        iter.next();
        delete iter.value();
    }
    Collection.clear();
}
