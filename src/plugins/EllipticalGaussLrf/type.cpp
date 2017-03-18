#include "type.h"

#include <TF2.h>
#include <TProfile2D.h>
#include <TFitResult.h>
#include <TFitResultPtr.h>

#include "lrf.h"
#include "internalswidget.h"
#include "settingswidget.h"

QJsonObject Type::lrfToJson(const LRF::ALrf *base_lrf) const
{
  auto lrf = static_cast<const Lrf *>(base_lrf);
  QJsonObject settings;
  settings["transform"] = lrf->getTransform().toJson();
  settings["amp"] = lrf->a;
  settings["sigma2x"] = lrf->s2x;
  settings["sigma2y"] = lrf->s2y;
  settings["tail"] = lrf->tail;
  settings["e_amp"] = lrf->e_a;
  settings["e_sigma2x"] = lrf->e_s2x;
  settings["e_sigma2y"] = lrf->e_s2y;
  settings["e_tail"] = lrf->e_tail;
  settings["rmax"] = lrf->rmax;
  return settings;
}

LRF::ALrf *Type::lrfFromJson(const QJsonObject &json) const
{
  ATransform t = ATransform::fromJson(json["transform"].toObject());
  return new Lrf(id(), json["rmax"].toDouble(),
      json["amp"].toDouble(), json["sigma2x"].toDouble(),
      json["sigma2y"].toDouble(), json["tail"].toDouble(), t,
      json["e_amp"].toDouble(), json["e_sigma2x"].toDouble(),
      json["e_sigma2y"].toDouble(), json["e_tail"].toDouble());
}

void Type::getCudaParameters(std::shared_ptr<const LRF::ALrf>)
{
  //Return coefficients
}

LRF::ALrf *Type::lrfFromData(const QJsonObject &settings, bool fit_error, const std::vector<APoint> &event_pos, const std::vector<double> &event_signal) const
{
  //Auto-detect rmax from fitting data. Needed for root
  double rmax = 0;
  for (size_t i = 0; i < event_pos.size(); i++) {
    double r2 = event_pos[i].normxySq();
    if(r2 > rmax) rmax = r2;
  }
  rmax = sqrt(rmax);

  int nbinsx = 300;
  int nbinsy = 300;

  //Setup fit data
  auto signal_profile = std::unique_ptr<TProfile2D>(new TProfile2D("LrfSignalProf", "", nbinsx, -rmax, rmax, nbinsy, -rmax, rmax));
  signal_profile->SetErrorOption("s");
  signal_profile->Approximate(kTRUE); //Attempt to use low stat bins during fit
  for(size_t i = 0; i < event_pos.size(); i++)
    signal_profile->Fill(event_pos[i].x(), event_pos[i].y(), event_signal[i]);

  //Setup fit function (which calls script eval())
  TF2 func = TF2("LrfFunc", [](double *v, double *p)
  {
    double expo = v[0]*v[0]/p[1] + v[1]*v[1]/p[2];
    return p[0] * std::exp(-0.5*expo) + p[3];
  },
  -rmax, rmax, -rmax, rmax, 4, "");

  func.SetParameter(0, settings["amp"].toDouble(1));
  func.SetParameter(1, settings["sigma2x"].toDouble(1));
  func.SetParameter(2, settings["sigma2y"].toDouble(1));
  func.SetParameter(3, settings["tail"].toDouble(0));

  //Do fit
  TFitResultPtr fit = signal_profile->Fit(&func, "SQ");
  const std::vector<double> &fit_params = fit->Parameters();

  //Check for failed fit
  if(fit_params.size() != 4)
    return nullptr;

  auto result = std::unique_ptr<Lrf>(new Lrf(id(), rmax, fit_params[0], fit_params[1], fit_params[2], fit_params[3]));
  if(fit_error) {
    signal_profile = std::unique_ptr<TProfile2D>(new TProfile2D("LrfSignalSigmaProf", "", nbinsx, -rmax, rmax, nbinsy, -rmax, rmax));
    signal_profile->SetErrorOption("s");
    signal_profile->Approximate(kTRUE); //Attempt to use low stat bins during fit
    for(size_t i = 0; i < event_pos.size(); i++)
      signal_profile->Fill(event_pos[i].x(), event_pos[i].y(), event_signal[i] - result->eval(event_pos[i]));

    func.SetParameter(0, settings["e_amp"].toDouble(1));
    func.SetParameter(1, settings["e_sigma2x"].toDouble(1));
    func.SetParameter(2, settings["e_sigma2y"].toDouble(1));
    func.SetParameter(3, settings["e_tail"].toDouble(0));

    TFitResultPtr e_fit = signal_profile->Fit(&func, "SQ");
    const std::vector<double> &e_fit_params = e_fit->Parameters();
    if(e_fit_params.size() != 4)
      return nullptr;
    result->setSigmaCoeffs(e_fit_params[0], e_fit_params[1], e_fit_params[2], e_fit_params[3]);
  }
  return result.release();
}

QWidget *Type::newInternalsWidget(QWidget *parent) const
{
  return new InternalsWidget(parent);
}

QWidget *Type::newSettingsWidget(QWidget *parent) const
{
  return new SettingsWidget(parent);
}
