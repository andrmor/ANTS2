#ifndef ADRAWOBJECT_H
#define ADRAWOBJECT_H

#include <QString>

class TObject;

class ADrawObject
{
public:
    ADrawObject() {}
    ~ADrawObject() {} //does not own TObject!
    ADrawObject(TObject * pointer, const char* options);
    ADrawObject(TObject * pointer, const QString & options);

    TObject * Pointer = nullptr;
    QString Name;
    QString Options;
    bool    bEnabled = true;

private:
    void extractName();
};

#endif // ADRAWOBJECT_H
