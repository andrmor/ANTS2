#ifndef AFILETOOLS_H
#define AFILETOOLS_H

#include <QString>
#include <QVector>

// can provide header (pointer to string with beginning symbol(s) of the header lines) -> then the function will return in the header the full header of the file
int LoadDoubleVectorsFromFile(QString FileName, QVector<double>* x);  //cleans previous data
int LoadDoubleVectorsFromFile(QString FileName, QVector<double>* x, QVector<double>* y, QString *header = 0, int numLines = 10);  //cleans previous data
int LoadDoubleVectorsFromFile(QString FileName, QVector<double>* x, QVector<double>* y, QVector<double>* z);  //cleans previous data
const QString LoadDoubleVectorsFromFile(const QString FileName, QVector< QVector<double>* > V);  //cleans previous data, returns error string

int SaveDoubleVectorsToFile(QString FileName, const QVector<double>* x, int count = -1);
int SaveDoubleVectorsToFile(QString FileName, const QVector<double>* x, const QVector<double>* y, int count = -1);
int SaveDoubleVectorsToFile(QString FileName, const QVector<double>* x, const QVector<double>* y, const QVector<double>* z, int count = -1);

int SaveIntVectorsToFile(QString FileName, const QVector<int>* x, int count = -1);
int SaveIntVectorsToFile(QString FileName, const QVector<int>* x, const QVector<int>* y, int count = -1);

int LoadIntVectorsFromFile(QString FileName, QVector<int>* x);
int LoadIntVectorsFromFile(QString FileName, QVector<int>* x, QVector<int>* y);

bool LoadTextFromFile(const QString& FileName, QString& string);
bool SaveTextToFile(const QString& FileName, const QString& text);

#endif // AFILETOOLS_H
