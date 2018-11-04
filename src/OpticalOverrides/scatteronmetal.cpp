#include "scatteronmetal.h"
#include "amaterialparticlecolection.h"
#include "aphoton.h"
#include "asimulationstatistics.h"
#include "atracerstateful.h"
#include "ajsontools.h"

#include <QDebug>
#include <QJsonObject>

#include "TComplex.h"
#include "TRandom2.h"

#ifdef GUI
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleValidator>
#endif

const QString ScatterOnMetal::getReportLine() const
{
  QString s;
  s += "n = " + QString::number(RealN) + "  ";
  s += "k = " + QString::number(ImaginaryN);
  return s;
}

const QString ScatterOnMetal::getLongReportLine() const
{
    QString s = "--> Dielectric to metal <--\n";
    s += "Refractive index of metal:\n";
    s += QString("  real: %1\n").arg(RealN);
    s += QString("  imaginary: %1").arg(ImaginaryN);
    return s;
}

void ScatterOnMetal::writeToJson(QJsonObject &json) const
{
  AOpticalOverride::writeToJson(json);

  json["RealN"]  = RealN;
  json["ImaginaryN"]  = ImaginaryN;
}

bool ScatterOnMetal::readFromJson(const QJsonObject &json)
{
    if ( !parseJson(json, "RealN", RealN) ) return false;
    if ( !parseJson(json, "ImaginaryN", ImaginaryN) ) return false;
    return true;
}

#ifdef GUI
QWidget *ScatterOnMetal::getEditWidget(QWidget *, GraphWindowClass *)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QHBoxLayout* hl = new QHBoxLayout(f);
        QVBoxLayout* l = new QVBoxLayout();
            QLabel* lab = new QLabel("Refractive index, real:");
        l->addWidget(lab);
            lab = new QLabel("Refractive index, imaginary:");
        l->addWidget(lab);
    hl->addLayout(l);
        l = new QVBoxLayout();
            QLineEdit* le = new QLineEdit(QString::number(RealN));
            QDoubleValidator* val = new QDoubleValidator(f);
            val->setNotation(QDoubleValidator::StandardNotation);
            //val->setBottom(0);
            val->setDecimals(6);
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->RealN = le->text().toDouble(); } );
        l->addWidget(le);
            le = new QLineEdit(QString::number(ImaginaryN));
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->ImaginaryN = le->text().toDouble(); } );
        l->addWidget(le);
    hl->addLayout(l);

    return f;
}
#endif

const QString ScatterOnMetal::checkOverrideData()
{
    return "";
}

AOpticalOverride::OpticalOverrideResultEnum ScatterOnMetal::calculate(ATracerStateful &Resources, APhoton *Photon, const double *NormalVector)
{
  double CosTheta = Photon->v[0]*NormalVector[0] + Photon->v[1]*NormalVector[1] + Photon->v[2]*NormalVector[2];

//  double Rindex1, Rindex2;
//  if (iWave == -1)
//    {
//      Rindex1 = (*MatCollection)[MatFrom]->n;
//      //Rindex2 = (*MatCollection)[MatTo]->n;
//      Rindex2 = RealN;
//    }
//  else
//    {
//      Rindex1 = (*MatCollection)[MatFrom]->nWaveBinned.at(iWave);
//      //Rindex2 = (*MatCollection)[MatTo]->nWaveBinned.at(iWave);
//      Rindex2 = RealN;
//    }
//  double Refl = calculateReflectivity(CosTheta, RealN/Rindex1, ImaginaryN/Rindex1);

  double Refl = calculateReflectivity(CosTheta, RealN, ImaginaryN, Photon->waveIndex);
  //qDebug() << "Dielectric-metal override: Cos theta="<<CosTheta<<" Reflectivity:"<<Refl;

  if ( Resources.RandGen->Rndm() > Refl )
    {
      //Absorption
      //qDebug() << "Override: Loss on metal";
      Status = Absorption;
      return Absorbed;
    }

  //else specular reflection
  //qDebug()<<"Override: Specular reflection from metal";
    //rotating the vector: K = K - 2*(NK)*N
  double NK = NormalVector[0]*Photon->v[0]; NK += NormalVector[1]*Photon->v[1];  NK += NormalVector[2]*Photon->v[2];
  Photon->v[0] -= 2.0*NK*NormalVector[0]; Photon->v[1] -= 2.0*NK*NormalVector[1]; Photon->v[2] -= 2.0*NK*NormalVector[2];
  Status = SpikeReflection;
  return Back;
}

double ScatterOnMetal::calculateReflectivity(double CosTheta, double RealN, double ImaginaryN, int waveIndex)
{
  //qDebug() << "cosTheta, n, k: "<< CosTheta << RealN << ImaginaryN;

  TComplex N(RealN, ImaginaryN);
  TComplex U(1,0);

  double SinTheta = (CosTheta < 0.9999999) ? sqrt(1.0 - CosTheta*CosTheta) : 0;
//  TComplex CosPhi = TMath::Sqrt( U - SinTheta*SinTheta/(N*N));
  double nFrom = (*MatCollection)[MatFrom]->getRefractiveIndex(waveIndex);
  TComplex CosPhi = TMath::Sqrt( U - SinTheta*SinTheta/ (N*N/nFrom/nFrom) );

//  TComplex rs = (CosTheta - N*CosPhi) / (CosTheta + N*CosPhi);
  TComplex rs = (nFrom*CosTheta - N*CosPhi) / (nFrom*CosTheta + N*CosPhi);
//  TComplex rp = ( -N*CosTheta + CosPhi) / (N*CosTheta + CosPhi);
  TComplex rp = ( -N*CosTheta + nFrom*CosPhi) / (N*CosTheta + nFrom*CosPhi);

  double RS = rs.Rho2();
  double RP = rp.Rho2();

  double R = 0.5 * (RS+RP);
  //qDebug() << "Refl coeff = "<< R;

  return R;
}

