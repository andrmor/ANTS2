#ifndef AROOTOBJBASE_H
#define AROOTOBJBASE_H

#include <QString>

class TObject;

class ARootObjBase
{
public:
    ARootObjBase(TObject* object, const QString&  title, const QString& type);
    virtual ~ARootObjBase();

    virtual TObject* GetObject();

    void             externalLock();
    void             externalUnlock();

    const QString&   getType() const;

protected:
    TObject* Object = 0;
    QString  Title;
    QString  Type;                    // object type according to ROOT (e.g. "TH1D")

    QMutex   Mutex;
};

#endif // AROOTOBJBASE_H
