#ifndef ADRAWOBJECT_H
#define ADRAWOBJECT_H

#include <QString>

class TObject;

class ADrawObject
{
public:
    ADrawObject() {}
    ~ADrawObject() {} //do not own TObject!
    ADrawObject(TObject * pointer, const char* options)     {Pointer = pointer; Options = options;}
    ADrawObject(TObject * pointer, const QString & options) {Pointer = pointer; Options = options;}

    TObject * Pointer = nullptr;
    QString Options;
};

#endif // ADRAWOBJECT_H
