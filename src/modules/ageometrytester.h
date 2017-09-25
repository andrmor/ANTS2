#ifndef AGEOMETRYTESTER_H
#define AGEOMETRYTESTER_H

#include <QString>
#include <QVector>

class AGeometryTesterReportRecord;
class TGeoManager;

class AGeometryTester
{
public:
    AGeometryTester(TGeoManager* GeoManager) : GeoManager(GeoManager) {}
    void Test(double* start, double* direction);

    QVector<AGeometryTesterReportRecord> Record;
    double escapeX, escapeY, escapeZ;

private:
   TGeoManager* GeoManager;
};

class AGeometryTesterReportRecord
{
public:
   QString volName;
   int nodeIndex;
   int matIndex;
   double startX, startY, startZ;
};

#endif // AGEOMETRYTESTER_H
