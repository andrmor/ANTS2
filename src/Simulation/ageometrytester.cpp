#include "ageometrytester.h"

#include <QDebug>

#include "TGeoManager.h"


void AGeometryTester::Test(double *start, double *direction)
{
    Record.clear();

    TGeoNavigator* navigator = GeoManager->GetCurrentNavigator();
    navigator->SetCurrentPoint(start);
    navigator->SetCurrentDirection(direction);
    navigator->FindNode();

    while (!navigator->IsOutside())
    {
       AGeometryTesterReportRecord r;

       TGeoVolume* vol = navigator->GetCurrentVolume();

       r.volName = vol->GetName();       
       r.nodeIndex = navigator->GetCurrentNode()->GetNumber();
       r.matIndex = vol->GetMaterial()->GetIndex();

       r.startX = navigator->GetCurrentPoint()[0];
       r.startY = navigator->GetCurrentPoint()[1];
       r.startZ = navigator->GetCurrentPoint()[2];

       navigator->FindNextBoundaryAndStep();

       Record.append(r);
    }

    escapeX = navigator->GetCurrentPoint()[0];
    escapeY = navigator->GetCurrentPoint()[1];
    escapeZ = navigator->GetCurrentPoint()[2];
}
