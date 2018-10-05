#ifndef AJSONTOOLS_H
#define AJSONTOOLS_H

#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

class QCheckBox;
class QSpinBox;
class QLineEdit;
class QComboBox;

class TH1I;
class TH1D;

bool parseJson(const QJsonObject &json, const QString &key, bool &var);
bool parseJson(const QJsonObject &json, const QString &key, int &var);   //can convert double content of the key to int - uses std::round
bool parseJson(const QJsonObject &json, const QString &key, double &var);
bool parseJson(const QJsonObject &json, const QString &key, float &var);
bool parseJson(const QJsonObject &json, const QString &key, QString &var);
bool parseJson(const QJsonObject &json, const QString &key, QJsonArray &var);
bool parseJson(const QJsonObject &json, const QString &key, QJsonObject &obj);

void JsonToCheckbox(QJsonObject& json, QString key, QCheckBox* cb);
void JsonToSpinBox(QJsonObject& json, QString key, QSpinBox* sb);
void JsonToLineEditDouble(QJsonObject& json, QString key, QLineEdit* le);
void JsonToLineEditInt(QJsonObject& json, QString key, QLineEdit* le);
void JsonToLineEditText(QJsonObject& json, QString key, QLineEdit* le);
void JsonToComboBox(QJsonObject& json, QString key, QComboBox* qb);

bool writeTwoQVectorsToJArray(const QVector<double> &x, const QVector<double> &y, QJsonArray &ar);
bool readTwoQVectorsFromJArray(QJsonArray &ar, QVector<double> &x, QVector<double> &y);
bool write2DQVectorToJArray(const QVector<QVector<double> > &xy, QJsonArray &ar);
void read2DQVectorFromJArray(QJsonArray &ar, QVector<QVector<double> > &xy);

bool LoadJsonFromFile(QJsonObject &json, const QString& fileName);
bool SaveJsonToFile(const QJsonObject &json, const QString& fileName);

bool writeTH1ItoJsonArr(TH1I *hist, QJsonArray &ja);
bool writeTH1DtoJsonArr(TH1D* hist, QJsonArray &ja);

bool isContainAllKeys(QJsonObject json, QStringList keys);

const QJsonObject strToObject(const QString& s);
const QString jsonToString(const QJsonObject& json);

#endif // AJSONTOOLS_H
