#ifndef ASCRIPTVALUECONVERTER_H
#define ASCRIPTVALUECONVERTER_H

#include <QJsonValue>
#include <QScriptValue>

//The purpose of making this a class instead of functions is because this is a
//highly optimizable procedure in terms of resulting space, due to possible
//repeating objects. Check AScriptValueCopier to see how to reuse objects which
//refer to the same data. There is also room for to/fromVariant methods.

class AScriptValueConverter
{
public:
  AScriptValueConverter() {}
  QJsonValue toJson(const QScriptValue &obj);
  QScriptValue fromJson(const QJsonValue &json, QScriptEngine *engine);
  //void fromJson(const QJsonValue &json, QScriptValue &dest);
};

#endif // ASCRIPTVALUECONVERTER_H
