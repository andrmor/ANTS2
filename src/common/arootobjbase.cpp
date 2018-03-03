#include "arootobjbase.h"

#include <QDebug>

#include "TObject.h"

ARootObjBase::ARootObjBase(TObject *object, const QString &title, const QString &type) :
    Object(object), Title(title), Type(type) {}

ARootObjBase::~ARootObjBase()
{
    qDebug() << "-------------->Destructor for "<<Title << " Obj:"<<Object;
    delete Object;
}

TObject *ARootObjBase::GetObject()
{
    return Object;
}
