#ifndef AFILETOOLS_H
#define AFILETOOLS_H

#include <QString>
#include <QVector>

int LoadDoubleVectorsFromFile(QString FileName, QVector<double>* x);  //cleans previous data
int LoadDoubleVectorsFromFile(QString FileName, QVector<double>* x, QVector<double>* y);  //cleans previous data
int LoadDoubleVectorsFromFile(QString FileName, QVector<double>* x, QVector<double>* y, QVector<double>* z);  //cleans previous data
const QString LoadDoubleVectorsFromFile(const QString FileName, QVector< QVector<double>* > V);  //cleans previous data, returns error string

int SaveDoubleVectorsToFile(QString FileName, const QVector<double>* x, int count = -1);
int SaveDoubleVectorsToFile(QString FileName, const QVector<double>* x, const QVector<double>* y, int count = -1);
int SaveDoubleVectorsToFile(QString FileName, const QVector<double>* x, const QVector<double>* y, const QVector<double>* z, int count = -1);

int SaveIntVectorsToFile(QString FileName, const QVector<int>* x, int count = -1);
int SaveIntVectorsToFile(QString FileName, const QVector<int>* x, const QVector<int>* y, int count = -1);

int LoadIntVectorsFromFile(QString FileName, QVector<int>* x);
int LoadIntVectorsFromFile(QString FileName, QVector<int>* x, QVector<int>* y);

bool LoadTextFromFile(QString FileName, QString& string);

#endif // AFILETOOLS_H
