#ifndef AROOTOBJCOLLECTION_H
#define AROOTOBJCOLLECTION_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QMutex>

class ARootObjBase;

class ARootObjCollection
{
public:
    ~ARootObjCollection();

    bool append(const QString& name, ARootObjBase* record, bool bAbortIfExists = true);

    ARootObjBase* getRecord(const QString& name);   // Unlocks mutex on return!

    bool remove(const QString& name);               // Not multithread-safe: graph can be in use by the GUI
    void clear();                                   // Not multithread-safe: graph can be in use by the GUI

    const QStringList getAllRecordNames() const;

private:
    QMap<QString, ARootObjBase*> Collection;
    mutable QMutex               Mutex;

};

#endif // AROOTOBJCOLLECTION_H
