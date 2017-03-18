#include "asensor.h"

#include <QDebug>

#include "alrftypemanager.h"

namespace LRF {

ASensor ASensor::fromJson(const QJsonObject &state, bool *ok)
{
  ASensor output;
  if(ok) *ok = false;
  if(!state["ipm"].isDouble() || !state["deck"].isArray())
    return output;

  output.ipm = state["ipm"].toInt();

  for(const QJsonValue &parcel : state["deck"].toArray()) {
    QJsonObject json = parcel.toObject();
    if(!json["lrf"].isObject() || !json["coeff"].isDouble())
      return output;

    ALrf *lrf = ALrfTypeManager::instance().lrfFromJson(json["lrf"].toObject());
    if(lrf == nullptr)
      return output;

    Parcel p;
    p.coeff = json["coeff"].toDouble();
    p.lrf = std::shared_ptr<const ALrf>(lrf);
    output.deck.push_back(std::move(p));
  }

  if(ok) *ok = true;
  return output;
}

QJsonObject ASensor::toJson() const
{
  QJsonArray deck;
  for(const Parcel &p : this->deck) {
    QJsonObject parcel;
    parcel["coeff"] = p.coeff;
    parcel["lrf"] = ALrfTypeManager::instance().lrfToJson(p.lrf.get());
    deck.append(std::move(parcel));
  }

  QJsonObject state;
  state["ipm"] = ipm;
  state["deck"] = deck;
  return state;
}

ASensor ASensor::copyToCurrentThread() const
{
  ASensor sens_copy;
  sens_copy.ipm = this->ipm;
  for(const ASensor::Parcel &parcel : this->deck) {
    const ALrfType &lrf_type = ALrfTypeManager::instance().getTypeFast(parcel.lrf->type());
    ASensor::Parcel parcel_copy;
    parcel_copy.coeff = parcel.coeff;
    parcel_copy.lrf = lrf_type.copyToCurrentThread(parcel.lrf);
    sens_copy.deck.push_back(parcel_copy);
  }
  return sens_copy;
}

bool ASensor::isCudaCapable() const
{
  for(const ASensor::Parcel &parcel : this->deck) {
    const ALrfType &type = ALrfTypeManager::instance().getTypeFast(parcel.lrf->type());
    if(!type.isCudaCapable())
      return false;
  }
  return true;
}

ASensorGroup::ASensorGroup(const QJsonValue &state)
{
  bool ok;
  for(const QJsonValue &json_sensor : state.toArray()) {
    ASensor sensor = ASensor::fromJson(json_sensor.toObject(), &ok);
    if(ok)
      sensors.push_back(sensor);
    else {
      qDebug()<<"Failed to load ASensor from json:"<<json_sensor;
      qDebug()<<"ASensorGroup will continue to attempt to load the other sensors.";
    }
  }
}

QJsonValue ASensorGroup::toJson() const
{
  QJsonArray sensors;
  for(const ASensor &s : this->sensors)
    sensors.append(s.toJson());
  return sensors;
}

bool ASensorGroup::isCudaCapable() const
{
  for(const ASensor &sensor : sensors)
    if(!sensor.isCudaCapable())
      return false;
  return true;
}

ASensor *ASensorGroup::getSensor(int ipm)
{
  auto it = std::lower_bound(sensors.begin(), sensors.end(), ASensor(ipm));
  return (it != sensors.end() && ipm == it->ipm) ? &*it : nullptr;
}

const ASensor *ASensorGroup::getSensor(int ipm) const
{
  auto it = std::lower_bound(sensors.begin(), sensors.end(), ASensor(ipm));
  return (it != sensors.end() && ipm == it->ipm) ? &*it : nullptr;
}

const ASensorGroup *ASensorGroup::copyToCurrentThread() const
{
  ASensorGroup *group_copy = new ASensorGroup;
  for(const ASensor &sensor : sensors)
    group_copy->sensors.push_back(sensor.copyToCurrentThread());
  return group_copy;
}

void ASensorGroup::stack(const ASensor &sensor)
{
  ASensor *old = getSensor(sensor.ipm);
  if(old == nullptr) {
    sensors.push_back(sensor);
    std::sort(sensors.begin(), sensors.end());
  }
  else old->deck.insert(old->deck.end(), sensor.deck.begin(), sensor.deck.end());
}

void ASensorGroup::stack(const ASensorGroup &group)
{
  for(auto &sensor : group.sensors)
    stack(sensor);
}

void ASensorGroup::overwrite(const ASensor &sensor)
{
  ASensor *old = getSensor(sensor.ipm);
  if(old == nullptr) {
    sensors.push_back(sensor);
    std::sort(sensors.begin(), sensors.end());
  }
  else *old = sensor;
}

void ASensorGroup::overwrite(const ASensorGroup &group)
{
  for(auto &sensor : group.sensors)
    overwrite(sensor);
}

} //namespace LRF
