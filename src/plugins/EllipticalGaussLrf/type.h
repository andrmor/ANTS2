#ifndef TYPE_H
#define TYPE_H

#include "alrftype.h"

class Type : public LRF::ALrfType
{
protected:
  virtual QJsonObject lrfToJson(const LRF::ALrf *lrf) const;

  virtual LRF::ALrf *lrfFromJson(const QJsonObject &json) const;

public:
  Type() : LRF::ALrfType("Elliptic Gaussian") {}

  bool isCudaCapable() const override { return true; }
  void getCudaParameters(std::shared_ptr<const LRF::ALrf> /*lrf*/) override;

#ifdef GUI
  QWidget *newSettingsWidget(QWidget *parent = 0) const override;

  QWidget *newInternalsWidget(QWidget *parent = 0) const override;
#endif

  LRF::ALrf *lrfFromData(const QJsonObject &settings, bool fit_error,
                         const std::vector<APoint> &event_pos,
                         const std::vector<double> &event_signal) const override;
};

#endif // TYPE_H
