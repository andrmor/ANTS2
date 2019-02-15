#include "fsnpopticaloverride.h"
#include "aphoton.h"
#include "amaterial.h"
#include "amaterialparticlecolection.h"
#include "atracerstateful.h"
#include "asimulationstatistics.h"
#include "ajsontools.h"

#include <QJsonObject>

#include "TMath.h"
#include "TRandom2.h"

#ifdef GUI
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleValidator>
#endif

AOpticalOverride::OpticalOverrideResultEnum FSNPOpticalOverride::calculate(ATracerStateful &Resources, APhoton *Photon, const double *NormalVector)
{
  // Angular reflectance: fraction of light reflected at the interface bewteen
  // medium 1 and medium 2 assuming non-polarized incident light:
  // ==> Incident light goes from medium 1 (n1) into medium 2 (n2).
  // ==> cos(0): perpendicular to the surface.
  // ==> cos(Pi/2): grazing/parallel to the surface.
  // NOTE that even if the incident light is not polarized (equal amounts of s-
  // and p-), the reflected light may be polarized because different percentages
  // of s-(perpendicular) and p-(parallel) polarized waves are/may be reflected.

  //refractive indexes of materials before and after the interface
  double n1 = (*MatCollection)[MatFrom]->getRefractiveIndex(Photon->waveIndex);
  double n2 = (*MatCollection)[MatTo]->getRefractiveIndex(Photon->waveIndex);

  //angle of incidence
  double cos1 = 0;
  for (int i=0; i<3; i++)
     cos1 += NormalVector[i]*Photon->v[i];

  //Calculating reflection probability
  double fresnelUnpolarR;
  double sin1sqr = 1.0 - cos1*cos1;
  double nsqr = n2/n1; nsqr *= nsqr;
  if (sin1sqr > nsqr)
    fresnelUnpolarR = 1.0;
  else
    {
      double f1 = nsqr * cos1;
      double f2 = TMath::Sqrt( nsqr - sin1sqr );
      // p-polarized
      double Rp12 = (f1-f2) / (f1+f2); Rp12 *= Rp12;
      // s-polarized
      double Rs12 = (cos1-f2) / (cos1+f2); Rs12 *= Rs12;

      fresnelUnpolarR = 0.5 * (Rp12+Rs12);
    }

//  if random[0,1]<fresnelUnpolarR do specular reflection
  if (Resources.RandGen->Rndm() < fresnelUnpolarR)
    {
      //qDebug()<<"Override: specular reflection";
        //rotating the vector: K = K - 2*(NK)*N
      Photon->v[0] -= 2.0*cos1*NormalVector[0]; Photon->v[1] -= 2.0*cos1*NormalVector[1]; Photon->v[2] -= 2.0*cos1*NormalVector[2];
      Status = SpikeReflection;
      return Back;
    }

// if random[0,1]>albedo kill photon else do diffuse reflection
  if (Resources.RandGen->Rndm() > Albedo)
    {
      //qDebug()<<"Override: absorption";
      Status = Absorption;
      return Absorbed;
    }

  //qDebug() << "Override: Lambertian scattering";
  double norm2;
  do
    {
      Photon->RandomDir(Resources.RandGen);
      Photon->v[0] -= NormalVector[0]; Photon->v[1] -= NormalVector[1]; Photon->v[2] -= NormalVector[2];
      norm2 = Photon->v[0]*Photon->v[0] + Photon->v[1]*Photon->v[1] + Photon->v[2]*Photon->v[2];
    }
  while (norm2 < 0.000001);
  double normInverted = 1.0/TMath::Sqrt(norm2);
  Photon->v[0] *= normInverted; Photon->v[1] *= normInverted; Photon->v[2] *= normInverted;
  Status = LambertianReflection;
  return Back;
}

const QString FSNPOpticalOverride::getReportLine() const
{
    return QString("Albedo %1").arg(Albedo);
}

const QString FSNPOpticalOverride::getLongReportLine() const
{
    QString s = "--> FSNP <--\n";
    s += QString("Albedo: %1").arg(Albedo);
    return s;
}

void FSNPOpticalOverride::writeToJson(QJsonObject &json) const
{
  AOpticalOverride::writeToJson(json);

  json["Albedo"] = Albedo;
}

bool FSNPOpticalOverride::readFromJson(const QJsonObject &json)
{
    return parseJson(json, "Albedo", Albedo);
}

#ifdef GUI
QWidget *FSNPOpticalOverride::getEditWidget(QWidget *, GraphWindowClass *)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QHBoxLayout* l = new QHBoxLayout(f);
        QLabel* lab = new QLabel("Albedo:");
    l->addWidget(lab);
        QLineEdit* le = new QLineEdit(QString::number(Albedo));
        QDoubleValidator* val = new QDoubleValidator(f);
        val->setNotation(QDoubleValidator::StandardNotation);
        val->setBottom(0);
        //val->setTop(1.0); //Qt(5.8.0) BUG: check does not work
        val->setDecimals(6);
        le->setValidator(val);
        QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->Albedo = le->text().toDouble(); } );
    l->addWidget(le);

    return f;
}
#endif

const QString FSNPOpticalOverride::checkOverrideData()
{
    if (Albedo < 0 || Albedo > 1.0) return "Albedo should be within [0, 1.0]";
    return "";
}
