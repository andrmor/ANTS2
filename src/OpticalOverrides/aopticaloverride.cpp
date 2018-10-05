#include "aopticaloverride.h"
#include "amaterialparticlecolection.h"
#include "aphoton.h"
#include "acommonfunctions.h"
#include "ajsontools.h"
#include "afiletools.h"
#include "asimulationstatistics.h"
#include "atracerstateful.h"

#include <QDebug>
#include <QJsonObject>

#include "TMath.h"
#include "TRandom2.h"
#include "TH1.h"

#ifdef GUI
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QDoubleValidator>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include "amessage.h"
#include "TGraph.h"
#include "graphwindowclass.h"
#endif

void AOpticalOverride::writeToJson(QJsonObject &json) const
{
    json["Model"] = getType();
    json["MatFrom"] = MatFrom;
    json["MatTo"] = MatTo;
}

bool AOpticalOverride::readFromJson(const QJsonObject &json)
{
    QString type = json["Model"].toString();
    if (type != getType()) return false; //file for wrong model!
    return true;
}

#ifdef GUI
QWidget *AOpticalOverride::getEditWidget(QWidget *, GraphWindowClass *)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);
    f->setMinimumHeight(100);
    return f;
}
#endif

#include "abasicopticaloverride.h"
#include "spectralbasicopticaloverride.h"
#include "fsnpopticaloverride.h"
#include "awaveshifteroverride.h"
#include "phscatclaudiomodel.h"
#include "scatteronmetal.h"
#include "ascriptopticaloverride.h"

AOpticalOverride *OpticalOverrideFactory(QString model, AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo)
{
   if (model == "Simplistic" || model == "Simplistic_model")
     return new ABasicOpticalOverride(MatCollection, MatFrom, MatTo);
   if (model == "SimplisticSpectral" || model == "SimplisticSpectral_model")
     return new SpectralBasicOpticalOverride(MatCollection, MatFrom, MatTo);
   else if (model == "ClaudioModel" || model == "Claudio_Model_V2d2")
     return new PhScatClaudioModelV2d2(MatCollection, MatFrom, MatTo);
   else if (model == "DielectricToMetal")
     return new ScatterOnMetal(MatCollection, MatFrom, MatTo);
   else if (model == "FSNP" || model == "FS_NP" || model=="Neves_model")
     return new FSNPOpticalOverride(MatCollection, MatFrom, MatTo);
   else if (model == "SurfaceWLS")
     return new AWaveshifterOverride(MatCollection, MatFrom, MatTo);
   else if (model == "CustomScript")
     return new AScriptOpticalOverride(MatCollection, MatFrom, MatTo);

   return NULL; //undefined override type!
}

const QStringList ListOvAllOpticalOverrideTypes()
{
    QStringList l;

    l << "Simplistic"
      << "SimplisticSpectral"
      << "FSNP"
      << "DielectricToMetal"
      << "ClaudioModel"
      << "SurfaceWLS"
      << "CustomScript";

    return l;
}
