#include "ascriptvaluecopier.h"

#include <QDateTime>
#include <QScriptEngine>
#include <QScriptValueIterator>

void AScriptValueCopier::copyProperties(QScriptValue &dest, QScriptValue obj)
{
  dest.setPrototype(copy(obj.prototype()));
  QScriptValueIterator it(obj);
  while(it.hasNext()) {
    it.next();
    dest.setProperty(it.name(), copy(it.value()));
  }
}

QScriptValue AScriptValueCopier::copy(const QScriptValue &obj)
{
  if (obj.isUndefined())
    return QScriptValue(QScriptValue::UndefinedValue);
  if (obj.isNull())
    return QScriptValue(QScriptValue::NullValue);

  QScriptValue v;
  if (obj.isObject()) {
    //If we've already copied this object, don't copy it again.
    if (copiedObjs.contains(obj.objectId()))
      return copiedObjs.value(obj.objectId());
    copiedObjs.insert(obj.objectId(), v);
  }

  if (obj.isQObject()) {
    v = m_toEngine.newQObject(v, obj.toQObject());
    v.setPrototype(copy(obj.prototype()));
  }
  else if (obj.isQMetaObject())
    v = m_toEngine.newQMetaObject(obj.toQMetaObject());

  else if (obj.isBool())
    v = m_toEngine.newVariant(v, obj.toBool());

  else if (obj.isNumber())
    v = m_toEngine.newVariant(v, obj.toNumber());

  else if (obj.isDate())
    v = m_toEngine.newDate(obj.toDateTime());

  else if (obj.isString())
    v = m_toEngine.newVariant(v, obj.toString());

  else if (obj.isRegExp())
    v = m_toEngine.newRegExp(obj.toRegExp());

  else if (obj.isVariant())
    v = m_toEngine.newVariant(v, obj.toVariant());

  else if (obj.isFunction()) {
    //Calling .toString() on a pure JS function returns the function's source
    //code, but on a native function toString() returns something like
    //"function() { [native code] }". That's why there's a syntax check.
    QString code = obj.toString();

    auto syntaxCheck = m_toEngine.checkSyntax(code);
    if (syntaxCheck.state() == syntaxCheck.Valid)
      v = m_toEngine.evaluate(QString('(') + code + ')');
    else if (code.contains("[native code]"))
      v.setData(obj.data());
    else {
      //Error handling…
    }
  }
  else if(obj.isObject()) {
    if (obj.scriptClass())
      v = m_toEngine.newObject(obj.scriptClass(), copy(obj.data()));
    else
      v = m_toEngine.newObject();
    copyProperties(v, obj);
  }
  else if(obj.isArray()) {
    v = m_toEngine.newArray();
    copyProperties(v, obj);
  }
  else {
    //Error handling…
  }
  return v;
}
