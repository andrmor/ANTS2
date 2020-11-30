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
    ADrawObject(TObject * pointer, const QString & options, bool enabled = true);
    ADrawObject(TObject * pointer, const QString & options, bool enabled, bool bLogScaleX, bool bLogScaleY);

    TObject * Pointer = nullptr;
    QString Name;
    QString Options;
    bool    bEnabled  = true;
    bool    bLogScaleX = false;
    bool    bLogScaleY = false;

private:
    void extractName();
};

#endif // ADRAWOBJECT_H
