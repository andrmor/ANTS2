#include "jsonparser.h"
#include <QDebug>

JsonParser::JsonParser(QJsonObject &json) : target(json), err_missing(0), err_type(0)
{   
}

void JsonParser::SetObject(QJsonObject &json)
{
    target = json;
}

bool JsonParser::GetKey(const QString &key)
{
    if (!target.contains(key)) {
//        qDebug() << "no such key:" << key;
        err_missing++;
        return false;
    }

    val = target[key];
//    qDebug() << "found key:"<< key << " with type: " << val.type();
    return true;
}

bool JsonParser::ParseObject(const QString &key, bool &var)
{
    if (!GetKey(key))
        return false;
    if (!val.isBool()) {
        err_type++;
        return false;
    }
    var = val.toBool();
    return true;
}

bool JsonParser::ParseObject(const QString &key, int &var)
{
    if (!GetKey(key))
        return false;
    if (!val.isDouble()) {
        err_type++;
        return false;
    }
    var = val.toInt();
    return true;
}

bool JsonParser::ParseObject(const QString &key, double &var)
{
    if (!GetKey(key))
        return false;
    if (!val.isDouble()) {
        err_type++;
        return false;
    }
    var = val.toDouble();
    return true;
}

bool JsonParser::ParseObject(const QString &key, QString &var)
{
    if (!GetKey(key))
        return false;
    if (!val.isString()) {
        err_type++;
        return false;
    }
    var = val.toString();
    return true;
}

bool JsonParser::ParseObject(const QString &key, QJsonArray &var)
{
    if (!GetKey(key))
        return false;
    if (!val.isArray()) {
        err_type++;
        return false;
    }
    var = val.toArray();
    return true;
}

bool JsonParser::ParseObject(const QString &key, QJsonObject &var)
{
    if (!GetKey(key))
        return false;
    if (!val.isObject()) {
        err_type++;
        return false;
    }
    var = val.toObject();
    return true;
}

bool JsonParser::ParseArray(const QJsonArray &array, QVector <double> &vec)
{
    vec.clear();
    int size = array.size();
    for (int i=0; i<size; i++) {
        val = array[i];
        if (!val.isDouble()) {
            err_type++;
            return false;
        }
        vec.push_back(val.toDouble());
    }
//    qDebug() << vec;
    return true;
}

bool JsonParser::ParseArray(const QJsonArray &array, QVector <float> &vec)
{
    vec.clear();
    int size = array.size();
    for (int i=0; i<size; i++) {
        val = array[i];
        if (!val.isDouble()) {
            err_type++;
            return false;
        }
        vec.push_back(val.toDouble());
    }
//    qDebug() << vec;
    return true;
}

bool JsonParser::ParseArray(const QJsonArray &array, QVector <int> &vec)
{
    vec.clear();
    int size = array.size();
    for (int i=0; i<size; i++) {
        val = array[i];
        if (!val.isDouble()) {
            err_type++;
            return false;
        }
        vec.push_back(val.toInt());
    }
    return true;
}

bool JsonParser::ParseArray(const QJsonArray &array, QVector <QJsonObject> &vec)
{
    vec.clear();
    int size = array.size();
    for (int i=0; i<size; i++) {
        val = array[i];
        if (!val.isObject()) {
            err_type++;
            return false;
        }
        vec.push_back(val.toObject());
    }
    return true;
}

bool JsonParser::ParseArray(const QJsonArray &array, QVector <QJsonArray> &vec)
{
    vec.clear();
    int size = array.size();
    for (int i=0; i<size; i++) {
        val = array[i];
        if (!val.isArray()) {
            err_type++;
            return false;
        }
        vec.push_back(val.toArray());
    }
    return true;
}





