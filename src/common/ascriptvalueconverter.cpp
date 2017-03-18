#include "ascriptvalueconverter.h"

#include <QScriptValueIterator>
#include <QScriptEngine>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QDateTime>
#include <QDebug>


//I have a feeling these will change soon...
static const QString tag_type = "type";
static const QString tag_value = "value";

QJsonValue AScriptValueConverter::toJson(const QScriptValue &obj)
{
  if (obj.isUndefined())
    return QJsonValue::Undefined;
  if (obj.isNull())
    return QJsonValue::Null;

  if (obj.isQObject())
    //How to truly support these!? It actually doesn't matter for now...
    return QJsonValue::Null;

  else if (obj.isQMetaObject())
    //How to truly support these!? It actually doesn't matter for now...
    return QJsonValue::Null;

  else if (obj.isBool())
    return QJsonValue(obj.toBool());

  else if (obj.isNumber())
    //Doesn't matter which type of number, JSON always stores double
    return QJsonValue(obj.toNumber());

  else if (obj.isString())
    return QJsonValue(obj.toString());

  else if (obj.isDate()) {
    QJsonObject json;
    json[tag_type] = "DateTime";
    json[tag_value] = obj.toDateTime().toString();
    return json;
  }

  else if (obj.isRegExp()) {
    QJsonObject json;
    json[tag_type] = "RegExp";
    json[tag_value] = obj.toRegExp().pattern();
    return json;
  }

  //Please make up your mind Qt people... In AScriptValueCopier I had to put
  //core types manually because they're not variant. Here core types are only
  //seen as variant. What the hell?
  else if (obj.isVariant()) {
    QVariant var = obj.toVariant();
    switch(var.type()) {
      case QVariant::Invalid: return QJsonValue::Undefined;
      case QVariant::Bool: return QJsonValue(var.toBool());
      case QVariant::Int: return QJsonValue(var.toInt());
      case QVariant::UInt: return QJsonValue((long long)var.toUInt());
      case QVariant::LongLong: return QJsonValue(var.toLongLong());
      case QVariant::ULongLong: return QJsonValue((double)var.toULongLong());
      case QVariant::Double: return QJsonValue(var.toDouble());
      case QVariant::Char: return QJsonValue(var.toChar());
      case QVariant::Map:
      case QVariant::List:
      case QVariant::String:
      case QVariant::StringList:
      case QVariant::ByteArray:
      default:
         qDebug()<<"WARNING: Unimplemented Variant type requested to be converted!";
    }
  }

  else if (obj.isFunction()) {
    //Calling .toString() on a pure JS function returns the function's source
    //code, but on a native function toString() returns something like
    //"function() { [native code] }". That's why there's a syntax check.
    QString code = obj.toString();
    auto syntaxCheck = obj.engine()->checkSyntax(code);
    if (syntaxCheck.state() == syntaxCheck.Valid) {
      QJsonObject json;
      json[tag_type] = "Function";
      json[tag_value] = QString('(') + code + ')';
      return json;
    }
  }
  else if(obj.isObject() || obj.isArray()) {
    QJsonObject val;
    //We should take into account script class for isObject() case (somehow?)
    //if (obj.scriptClass())
    //  v = m_toEngine.newObject(obj.scriptClass(), copy(obj.data()));
    QScriptValueIterator it(obj);
    while(it.hasNext()) {
      it.next();
      val[it.name()] = toJson(it.value());
    }
    QJsonObject json;
    json[tag_type] = obj.isObject() ? "Object" : "Array";
    json[tag_value] = val;
    return json;
  }
  return QJsonValue();
}

QScriptValue AScriptValueConverter::fromJson(const QJsonValue &json, QScriptEngine *engine)
{
  switch(json.type()) {
    case QJsonValue::Null: return QScriptValue::NullValue;
    case QJsonValue::Bool: return engine->newVariant(json.toBool());
    case QJsonValue::Double: return engine->newVariant(json.toDouble());
    case QJsonValue::String: return engine->newVariant(json.toString());
    case QJsonValue::Array: {
      QScriptValue dest = engine->newArray();
      QJsonArray array = json.toArray();
      for(int i = 0; i < array.count(); i++)
        dest.setProperty(QString::number(i), fromJson(array[i], dest.engine()));
      return dest;
    } break;
    case QJsonValue::Object: {
      QJsonObject obj = json.toObject();
      QString type = obj[tag_type].toString();
      if(type == "Function")
        return engine->evaluate(obj[tag_value].toString());
      else if(type == "Object") {
        QScriptValue dest = engine->newObject();
        obj = obj[tag_value].toObject();
        for(const QString &name : obj.keys())
          dest.setProperty(name, fromJson(obj[name], dest.engine()));
        return dest;
      }
      else if (type == "Array") {
        QScriptValue dest = engine->newArray();
        obj = obj[tag_value].toObject();
        for(const QString &name : obj.keys())
          dest.setProperty(name, fromJson(obj[name], dest.engine()));
        return dest;
      }
      else if(type == "DateTime")
        return engine->newDate(QDateTime::fromString(obj[tag_value].toString()));
      else if(type == "RegExp")
        return engine->newRegExp(QRegExp(obj[tag_value].toString()));
      else { //Consider it a regular object, not previously saved by AScriptValueConverter
        QScriptValue dest = engine->newObject();
        for(const QString &name : obj.keys())
          dest.setProperty(name, fromJson(obj[name], dest.engine()));
        return dest;
      }
    } break;
    case QJsonValue::Undefined: return QScriptValue::UndefinedValue;
  }
}

/*void AScriptValueConverter::fromJson(const QJsonValue &json, QScriptValue &dest)
{
  switch(json.type()) {
    case QJsonValue::Null: dest = QScriptValue::NullValue;
    case QJsonValue::Bool: dest = dest.engine()->newVariant(json.toBool());
    case QJsonValue::Double: dest = dest.engine()->newVariant(json.toDouble());
    case QJsonValue::String: dest = dest.engine()->newVariant(json.toString());
    case QJsonValue::Array: {
      dest = dest.engine()->newArray();
      QJsonArray array = json.toArray();
      for(int i = 0; i < array.count(); i++)
        dest.setProperty(QString::number(i), fromJson(array[i], dest.engine()));
    } break;
    case QJsonValue::Object: {
      QJsonObject obj = json.toObject();
      QString type = obj[tag_type].toString();
      if(type == "Function")
        dest = dest.engine()->evaluate(obj[tag_value].toString());
      else if(type == "Object") {
        dest = dest.engine()->newObject();
        obj = obj[tag_value].toObject();
        for(const QString &name : obj.keys())
          dest.setProperty(name, fromJson(obj[name], dest.engine()));
      }
      else if (type == "Array") {
        dest = dest.engine()->newArray();
        obj = obj[tag_value].toObject();
        for(const QString &name : obj.keys())
          dest.setProperty(name, fromJson(obj[name], dest.engine()));
      }
      else if(type == "DateTime") {
      }
      else if(type == "RegExp") {
      }
      else {
      }
    } break;
    case QJsonValue::Undefined: dest = QScriptValue::UndefinedValue;
  }
}*/
