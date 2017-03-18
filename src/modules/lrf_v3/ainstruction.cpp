#include "ainstruction.h"

#include <QJsonObject>
#include <QDebug>

#include <TProfile2D.h>

#include "acommonfunctions.h"
#include "ainstructioninput.h"
#include "asensor.h"
#include "arepository.h"
#include "alrftypemanager.h"
#include "atransform.h"
#include "afitlayersensorgroup.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace LRF {

namespace Instructions {

//Simpler than a globally accessible central manager, which might also be
//error-prone, since registration would still be manual. Don't forget to match
//the names with the ones in AInstructionListWidgetItem constructor and every
//subclass implementation of AInstruction::typeName()
typedef std::function<AInstruction*(const QJsonObject &settings)> InstructionFactory;
typedef std::tuple<std::string, InstructionFactory> InstructionInfo;
static const InstructionInfo instruction_list[] =
{
  InstructionInfo("",          [](const QJsonObject &) -> AInstruction* { return nullptr; }),
  InstructionInfo("Fit Layer", [](const QJsonObject &settings) { return new FitLayer(settings); }),
  InstructionInfo("Inherit", [](const QJsonObject &settings) { return new Inherit(settings); }),
};

} //namespace Instructions

AInstruction *AInstruction::createInstruction(const std::string &type_name, const std::string &name, const QJsonObject &settings)
{
  for(const Instructions::InstructionInfo &ii : Instructions::instruction_list) {
    if(type_name != std::get<0>(ii))
      continue;
    AInstruction *instr = std::get<1>(ii)(settings);
    if(!instr) {
      qDebug()<<"Failed to load instruction of type:"<<QString::fromStdString(type_name);
      return nullptr;
    }
    instr->name_ = name;
    return instr;
  }
  qDebug()<<"Unknown instruction type:"<<QString::fromStdString(type_name);
  return nullptr;
}

bool AInstruction::operator==(const AInstruction &other) const
{
  return toJson() == other.toJson();
}

AInstruction *AInstruction::clone() const {
  AInstruction *cl = createInstruction(typeName(), name(), toJsonImpl());
  if(!cl)
    qDebug()<<"Failed to recreate existing instruction!";
  return cl;
}

QJsonObject AInstruction::toJson() const {
  QJsonObject instr;
  instr["type"] = QString::fromStdString(typeName());
  instr["name"] = QString::fromStdString(name_);
  instr["data"] = toJsonImpl();
  return instr;
}

AInstruction *AInstruction::fromJson(const QJsonObject &json) {
  std::string type = json["type"].toString().toLocal8Bit().data();
  std::string name = json["name"].toString().toLocal8Bit().data();
  return createInstruction(type, name, json["data"].toObject());
}

namespace Instructions {

QJsonObject FitLayer::toJsonImpl() const
{
  return settings;
}

FitLayer::FitLayer(const QJsonObject &settings) : settings(settings)
{
  QJsonObject lrf_selection = settings["Lrf"].toObject();
  QString lrf_type_name = lrf_selection["Type"].toString();
  lrf_settings = lrf_selection["Settings"].toObject();
  std::string lrf_type_name_std = std::string(lrf_type_name.toLocal8Bit().data());
  lrf_type = ALrfTypeManager::instance().getType(lrf_type_name_std);
  if(lrf_type == nullptr) {
    qDebug()<<"Unknown lrf type:"<<lrf_type_name<<". Maybe some plugin was not loaded.";
    lrf_type = ALrfTypeManager::instance().getType(ALrfTypeID(0));
    if(lrf_type == nullptr) {
      qDebug()<<"No lrf types exist! Something very serious going on... crash may be imminent.";
      return;
    }
  }

  stack_op = settings["EffectOnPrevious"].toInt();
  reconstruction_group = settings["SensorGroup"].toInt();
  if(settings["SensorComposition"].toInt() != 0)
    sensor_list_txt = settings["SensorList"].toString();
  else sensor_list_txt = "";

  QJsonObject groupping = settings["Groupping"].toObject();
  group_enabled = groupping["Enabled"].toBool();
  group_type = groupping["Type"].toInt();
  adjust_gains = groupping["AdjustGains"].toBool();
}

std::vector<AFitLayerSensorGroup> FitLayer::getSymmetryGroups(const AInstructionInput &input) const
{
  std::vector<std::pair<int, APoint>> sensor_pos;

  if(sensor_list_txt != "") {
    QList<int> sensor_list;
    ExtractNumbersFromQString(sensor_list_txt, &sensor_list);
    for(int ipm : sensor_list)
      if(input.isPmBelongsToGroup(ipm, reconstruction_group))
        sensor_pos.emplace_back(ipm, input.getSensorsPosition()[ipm]);
  } else {
    auto &isp = input.getSensorsPosition();
    for(unsigned int ipm = 0; ipm < isp.size(); ipm++)
      if(input.isPmBelongsToGroup(ipm, reconstruction_group))
        sensor_pos.emplace_back(ipm, isp[ipm]);
  }

  std::vector<AFitLayerSensorGroup> symmetry_groups;
  if(group_enabled) {
    switch(group_type) {
      case 0: symmetry_groups = AFitLayerSensorGroup::mkCommon(sensor_pos); break;
      case 1: symmetry_groups = AFitLayerSensorGroup::mkRadial(sensor_pos, 0.001); break;
      case 2: symmetry_groups = AFitLayerSensorGroup::mkLattice(sensor_pos, 4, 0.001); break;
      case 3: symmetry_groups = AFitLayerSensorGroup::mkLattice(sensor_pos, 6, 0.001); break;
      default: qDebug()<<"Unimplemented groupping type "<<group_type<<"."
                         "This should never happen (failing)!";
    }
  } else {
    symmetry_groups = AFitLayerSensorGroup::mkIndividual(sensor_pos);
  }
  return symmetry_groups;
}

bool FitLayer::apply(ASensorGroup &group, const AInstructionInput &input) const
{
  std::vector<AFitLayerSensorGroup> symmetry_groups = getSymmetryGroups(input);

  std::vector<ASensorGroup> resulting_groups;
  const int num_events = input.eventCount(reconstruction_group);
  if(num_events < 100) return false;
  int symmetry_i = 0;
  for(auto &symmetry : symmetry_groups) {
    const int sensor_count = symmetry.size();
    std::vector<double> group_signals;
    std::vector<APoint> group_pos;
    try {
      group_signals.resize(num_events*sensor_count);
      group_pos.resize(num_events*sensor_count);
    } catch(std::bad_alloc) {
      qDebug()<<"Failed to allocate memory to group event signals and positions.";
      return false;
    }
    double minx = 0, miny = 0, maxx = 0, maxy = 0;


    for(int i = 0; i < sensor_count; i++) {
      auto ipm_transf = symmetry[i];

      //Do not shift events if we aren't to transform lrfs later
      //if(global_lrf)
      //  ipm_transf.second->setShift(APoint());

      for(int iev = 0; iev < num_events; iev++) {
        //Copy-transform event positions
        auto trans_pos = ipm_transf.second->transform(input.eventPos(iev, reconstruction_group));
        group_pos[i*num_events+iev] = trans_pos;

        if(adjust_gains) {
          minx = std::min(minx, trans_pos.x());
          maxx = std::max(maxx, trans_pos.x());
          miny = std::min(miny, trans_pos.y());
          maxy = std::max(maxy, trans_pos.y());
        }
      }

      if(!input.reportProgress((symmetry_i+(i+1)*0.1f*(adjust_gains?1:2)/sensor_count)/symmetry_groups.size()))
        return false;
    }

    std::vector<double> gains(sensor_count, 1.0);
    if(adjust_gains) {
      const int ngrid = sqrt(num_events/7.);
      int ipm = symmetry[0].first;
      TProfile2D hp0("hpgains0", "hpgains0", ngrid, minx, maxx, ngrid, miny, maxy);
      for (int iev = 0; iev < num_events; iev++)
        hp0.Fill(group_pos[iev].x(), group_pos[iev].y(), input.eventSignal(iev, ipm, reconstruction_group));

      for(int i = 1; i < sensor_count; i++)
      {
        const APoint *pos = &group_pos[num_events*i];
        ipm = symmetry[i].first;

        TProfile2D hp1("hpgains", "hpgains", ngrid, minx, maxx, ngrid, miny, maxy);
        for (int iev = 0; iev < num_events; iev++)
          hp1.Fill(pos[iev].x(), pos[iev].y(), input.eventSignal(iev, ipm, reconstruction_group));

        double sumxy = 0., sumxx = 0.;
        for (int ix = 0; ix < ngrid; ix++)
          for (int iy = 0; iy < ngrid; iy++) {
            int bin0 = hp0.GetBin(ix, iy);
            int bin1 = hp1.GetBin(ix, iy);
            //must have something in both bins
            if (hp0.GetBinEntries(bin0) && hp1.GetBinEntries(bin1))  {
              double z0 = hp0.GetBinContent(bin0);
              sumxy += z0*hp1.GetBinContent(bin1);
              sumxx += z0*z0;
            }
          }

        gains[i] = sumxx == 0 ? 666.0 : sumxy/sumxx;
        //qDebug()<<"gain"<<symmetry[i].first<<"="<<gains[i];

        if(!input.reportProgress((symmetry_i+0.1f+(i+1)*0.1f/sensor_count)/symmetry_groups.size()))
          return false;
      }
    }

    for(int i = 0; i < sensor_count; i++) {
      int ipm = symmetry[i].first;
      double inv_gain = 1./gains[i];
      for(int iev = 0; iev < num_events; iev++) {
        //Copy event signals with gain adjustment
        group_signals[i*num_events+iev] = inv_gain * input.eventSignal(iev, ipm, reconstruction_group);
        if(stack_op == 0) { //append
          const ASensor *sensor = group.getSensor(ipm);
          if(sensor != nullptr)
            group_signals[i*num_events+iev] -= sensor->eval(input.eventPos(iev, reconstruction_group));
        }
      }

      if(!input.reportProgress((symmetry_i+0.2f+(i+1)*0.2f/sensor_count)/symmetry_groups.size()))
        return false;
    }

    ALrf *lrf = lrf_type->lrfFromData(lrf_settings, input.fitError(), group_pos, group_signals);
    if(!lrf) return false;

    if(!input.reportProgress((symmetry_i+0.98f)/symmetry_groups.size()))
      return false;

    ASensorGroup current_symmetry_group;
    for(int i = 0; i < sensor_count; i++) {
      auto ipm_transf = symmetry[i];
      ATransform inverse = *ipm_transf.second;
      inverse.invert();
      std::shared_ptr<ALrf> tranfs_lrf(lrf->clone());
      tranfs_lrf->transform(inverse);
      current_symmetry_group.overwrite(ASensor(ipm_transf.first, gains[i], tranfs_lrf));
    }
    delete lrf;
    resulting_groups.push_back(current_symmetry_group);

    symmetry_i++;
  }

  switch(stack_op) {
    case 0: //append
      for(auto &symmetry_group : resulting_groups)
        group.stack(symmetry_group);
    break;
    default: case 1: //overwrite
      for(auto &symmetry_group : resulting_groups)
        group.overwrite(symmetry_group);
    break;
  }

  return true;
}



QJsonObject Inherit::toJsonImpl() const
{
  QJsonObject settings;
  settings["recipe"] = (int)rid.val();
  settings["version"] = (int)vid.val();
  settings["sensors"] = sensors;
  return settings;
}

Inherit::Inherit(ARecipeID rid, ARecipeVersionID vid, const QString &sensors)
  : rid(rid), vid(vid), sensors(sensors)
{ }

Inherit::Inherit(const QJsonObject &settings)
  : rid(settings["recipe"].toInt()),
    vid(settings["version"].toInt()),
    sensors(settings["sensors"].toString())
{ }

bool Inherit::apply(ASensorGroup &group, const AInstructionInput &input) const
{
  QList<int> sensor_selection;
  if(sensors.isEmpty()) {
    size_t size = input.getSensorsPosition().size();
    sensor_selection.reserve(size);
    for(size_t i = 0; i < size; i++)
      sensor_selection.append(i);
  } else {
    ExtractNumbersFromQString(sensors, &sensor_selection);
  }

  if(!input.repository().hasVersion(rid, vid))
    return false;
  const ASensorGroup &source = input.repository().getVersion(rid, vid).sensors;

  for(int ipm : sensor_selection) {
    const ASensor *sensor = source.getSensor(ipm);
    if(!sensor) return false;
    group.overwrite(*sensor);
  }

  return true;
}

std::vector<std::pair<ARecipeID, ARecipeVersionID>> Inherit::getDependencies() const
{
  return std::vector<std::pair<ARecipeID, ARecipeVersionID>>(1, std::make_pair(rid, vid));
}

void Inherit::remapRecipeIDs(const std::map<ARecipeID, ARecipeID> &map)
{
  auto key_val = map.find(rid);
  if(key_val != map.end())
    rid = key_val->second;
}

void Inherit::remapVersionIDs(const std::map<ARecipeVersionID, ARecipeVersionID> &map)
{
  auto key_val = map.find(vid);
  if(key_val != map.end())
    vid = key_val->second;
}

}

} //namespace LRF::CoreInstructions
