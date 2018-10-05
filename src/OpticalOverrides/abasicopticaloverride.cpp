#include "abasicopticaloverride.h"
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
#include <QComboBox>
#include <QDoubleValidator>
#endif

ABasicOpticalOverride::ABasicOpticalOverride(AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo) {}

AOpticalOverride::OpticalOverrideResultEnum ABasicOpticalOverride::calculate(ATracerStateful &Resources, APhoton *Photon, const double *NormalVector)
{
    double rnd = Resources.RandGen->Rndm();

    // surface loss?
    rnd -= probLoss;
    if (rnd<0)
    {
        // qDebug()<<"Override: surface loss!";
        Photon->SimStat->OverrideSimplisticAbsorption++;
        Status = Absorption;
        return Absorbed;
    }

  // specular reflection?
  rnd -= probRef;
  if (rnd<0)
    {
      // qDebug()<<"Override: specular reflection!";
        //rotating the vector: K = K - 2*(NK)*N
      double NK = NormalVector[0]*Photon->v[0]; NK += NormalVector[1]*Photon->v[1];  NK += NormalVector[2]*Photon->v[2];
      Photon->v[0] -= 2.0*NK*NormalVector[0]; Photon->v[1] -= 2.0*NK*NormalVector[1]; Photon->v[2] -= 2.0*NK*NormalVector[2];

      Photon->SimStat->OverrideSimplisticReflection++;
      Status = SpikeReflection;
      return Back;
    }

  // scattering?
  rnd -= probDiff;
  if (rnd<0)
    {
      // qDebug()<<"scattering triggered";
      Photon->SimStat->OverrideSimplisticScatter++;

      switch (scatterModel)
        {
        case 0: //4Pi scattering
          // qDebug()<<"4Pi scatter";
          Photon->RandomDir(Resources.RandGen);
          // qDebug()<<"New direction:"<<K[0]<<K[1]<<K[2];

          //enering new volume or backscattering?
          //normal is in the positive direction in respect to the original direction!
          if (Photon->v[0]*NormalVector[0] + Photon->v[1]*NormalVector[1] + Photon->v[2]*NormalVector[2] < 0)
            {
              // qDebug()<<"   scattering back";
              Status = LambertianReflection;
              return Back;
            }
          // qDebug()<<"   continuing to the next volume";
          Status = Transmission;
          return Forward;

        case 1: //2Pi lambertian, remaining in the same volume (back scattering)
          {
            // qDebug()<<"2Pi lambertian scattering backward";
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
        case 2: //2Pi lambertian, scattering to the next volume
          {
            // qDebug()<<"2Pi lambertian scattering forward";
            double norm2;
            do
              {
                Photon->RandomDir(Resources.RandGen);
                Photon->v[0] += NormalVector[0]; Photon->v[1] += NormalVector[1]; Photon->v[2] += NormalVector[2];
                norm2 = Photon->v[0]*Photon->v[0] + Photon->v[1]*Photon->v[1] + Photon->v[2]*Photon->v[2];
              }
            while (norm2 < 0.000001);

            double normInverted = 1.0/TMath::Sqrt(norm2);
            Photon->v[0] *= normInverted; Photon->v[1] *= normInverted; Photon->v[2] *= normInverted;
            Status = Transmission;
            return Forward;
          }
        }
    }

  // overrides NOT triggered - what is left is covered by Fresnel in the tracker code
  // qDebug()<<"Overrides did not trigger, using fresnel";
  Status = Transmission;
  return NotTriggered;
}

const QString ABasicOpticalOverride::getReportLine() const
{
    double probFresnel = 1.0 - (probRef + probDiff + probLoss);
    QString s;
    if (probLoss > 0) s += QString("Abs %1 +").arg(probLoss);
    if (probRef > 0)  s += QString("Spec %1 +").arg(probRef);
    if (probDiff > 0)
    {
        switch( scatterModel )
        {
        case 0:
            s += "Iso ";
            break;
        case 1:
            s += "Lamb_B ";
            break;
        case 2:
            s += "Lamb_F ";
            break;
        }
        s += QString::number(probDiff);
        s += " +";
    }
    if (probFresnel > 1e-10) s += QString("Fres %1 +").arg(probFresnel);
    s.chop(2);
    return s;
}

const QString ABasicOpticalOverride::getLongReportLine() const
{
    QString s = "--> Simplistic <--\n";
    if (probLoss > 0) s += QString("Absorption: %1%\n").arg(100.0 * probLoss);
    if (probRef > 0)  s += QString("Specular reflection: %1%\n").arg(100.0 * probRef);
    if (probDiff)
    {
        s += QString("Scattering: %1%").arg(100.0 * probDiff);
        switch (scatterModel)
        {
        case 0: s += " (isotropic)\n"; break;
        case 1: s += " (Lambertian, back)\n"; break;
        case 2: s += " (Lambertian, forward)\n"; break;
        }
    }
    double fres = 1.0 - probLoss - probRef - probDiff;
    if (fres > 0) s += QString("Fresnel: %1%").arg(100.0 * fres);
    return s;
}

void ABasicOpticalOverride::writeToJson(QJsonObject &json) const
{
  AOpticalOverride::writeToJson(json);

  json["Abs"]  = probLoss;
  json["Spec"] = probRef;
  json["Scat"] = probDiff;
  json["ScatMode"] = scatterModel;
}

bool ABasicOpticalOverride::readFromJson(const QJsonObject &json)
{
    if ( !parseJson(json, "Abs", probLoss) ) return false;
    if ( !parseJson(json, "Spec", probRef) ) return false;
    if ( !parseJson(json, "Scat", probDiff) ) return false;
    if ( !parseJson(json, "ScatMode", scatterModel) ) return false;
    return true;
}

#ifdef GUI
QWidget *ABasicOpticalOverride::getEditWidget(QWidget*, GraphWindowClass *)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QHBoxLayout* hl = new QHBoxLayout(f);
        QVBoxLayout* l = new QVBoxLayout();
            QLabel* lab = new QLabel("Absorption:");
        l->addWidget(lab);
            lab = new QLabel("Specular reflection:");
        l->addWidget(lab);
            lab = new QLabel("Scattering:");
        l->addWidget(lab);
    hl->addLayout(l);
        l = new QVBoxLayout();
            QLineEdit* le = new QLineEdit(QString::number(probLoss));
            QDoubleValidator* val = new QDoubleValidator(f);
            val->setNotation(QDoubleValidator::StandardNotation);
            val->setBottom(0);
            //val->setTop(1.0); //Qt(5.8.0) BUG: check does not work
            val->setDecimals(6);
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->probLoss = le->text().toDouble(); } );
        l->addWidget(le);
            le = new QLineEdit(QString::number(probRef));
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->probRef = le->text().toDouble(); } );
        l->addWidget(le);
            le = new QLineEdit(QString::number(probDiff));
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->probDiff = le->text().toDouble(); } );
        l->addWidget(le);
    hl->addLayout(l);
        l = new QVBoxLayout();
            lab = new QLabel("");
        l->addWidget(lab);
            lab = new QLabel("");
        l->addWidget(lab);
            QComboBox* com = new QComboBox();
            com->addItem("Isotropic (4Pi)"); com->addItem("Lambertian, 2Pi back"); com->addItem("Lambertian, 2Pi forward");
            com->setCurrentIndex(scatterModel);
            QObject::connect(com, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this](int index) { this->scatterModel = index; } );
        l->addWidget(com);
    hl->addLayout(l);

    return f;
}
#endif

const QString ABasicOpticalOverride::checkOverrideData()
{
    if (scatterModel<0 || scatterModel>2) return "Invalid scatter model";

    if (probLoss<0 || probLoss>1.0) return "Absorption probability should be within [0, 1.0]";
    if (probRef <0 || probRef >1.0) return "Reflection probability should be within [0, 1.0]";
    if (probDiff<0 || probDiff>1.0) return "Scattering probability should be within [0, 1.0]";

    if (probLoss + probRef + probDiff > 1.0) return "Sum of all process probabilities cannot exceed 1.0";

    return "";
}
