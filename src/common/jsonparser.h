#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QVector>

class JsonParser
{
public:
    JsonParser(QJsonObject &json);
    void SetObject(QJsonObject &json);
    bool GetKey(const QString &key);
    bool ParseObject(QJsonObject &json, const QString &key, bool &var);
    bool ParseObject(QJsonObject &json, const QString &key, int &var);
    bool ParseObject(QJsonObject &json, const QString &key, double &var);
    bool ParseObject(QJsonObject &json, const QString &key, QString &var);
    bool ParseObject(QJsonObject &json, const QString &key, QJsonArray &var);
    bool ParseObject(QJsonObject &json, const QString &key, QJsonObject &var);

    bool ParseObject(const QString &key, bool &var);
    bool ParseObject(const QString &key, int &var);
    bool ParseObject(const QString &key, double &var);
    bool ParseObject(const QString &key, QString &var);
    bool ParseObject(const QString &key, QJsonArray &var);
    bool ParseObject(const QString &key, QJsonObject &var);

    bool ParseArray(const QJsonArray &array, QVector <double> &vec);
    bool ParseArray(const QJsonArray &array, QVector <float> &vec);
    bool ParseArray(const QJsonArray &array, QVector <int> &vec);
    bool ParseArray(const QJsonArray &array, QVector <QJsonObject> &vec);
    bool ParseArray(const QJsonArray &array, QVector <QJsonArray> &vec);
    int GetError() {return err_missing+err_type;}
    int GetErrMissing() {return err_missing;}
    int GetErrType() {return err_type;}
private:
    QJsonObject &target;
    QJsonValue val;
    int err_missing;
    int err_type;
};

#endif // JSONPARSER_H

