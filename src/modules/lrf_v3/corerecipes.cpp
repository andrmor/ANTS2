#include "corerecipes.h"

#include <algorithm>
#include <functional>

#include <TProfile2D.h>
#include <QDebug>

#include "arecipeinput.h"
#include "asensor.h"
#include "alrftypemanager.h"

namespace LRF { namespace CoreRecipes {

static const double M_PI = 3.14159265358979323846;

class Group {
  std::vector<int> sensor_indices;
  std::vector<ATransform> to_sensor_origin;

  template<class... Args>
  void emplace_back(int ipm, Args&&... args)
  {
    sensor_indices.push_back(ipm);
    to_sensor_origin.emplace_back(args...);
  }

public:
  void print()
  {
    for(size_t i = 0; i < to_sensor_origin.size(); i++) {
      const ATransform &t = to_sensor_origin[i];
      qDebug()<<"\tPM#"<<sensor_indices[i]<<"; phi"
             <<t.getPhi()<<(t.getFlip() ? "flipped" : "non-flipped")
            <<"; r"<<t.getShift().normxy();
    }
  }

  bool empty() const { return to_sensor_origin.empty(); }
  size_t size() const { return to_sensor_origin.size(); }
  std::pair<int, const ATransform&> operator[](size_t i) const
  {
    return std::make_pair(sensor_indices[i], to_sensor_origin[i]);
  }

  static std::vector<Group> mkIndividual(const ARecipeInput &input)
  {
    std::vector<Group> groups;
    for(size_t ipm = 0; ipm < input.sensorsPos().size(); ipm++) {
      const APoint &pos = input.sensorsPos()[ipm];
      Group g; g.emplace_back(ipm, -pos);
      groups.push_back(g);
    }
    return groups;
  }

  static std::vector<Group> mkCommon(const ARecipeInput &input)
  {
    Group group;
    for(size_t ipm = 0; ipm < input.sensorsPos().size(); ipm++) {
      const APoint &pos = input.sensorsPos()[ipm];
      group.emplace_back(ipm, -pos, -pos.angleToOrigin());
    }
    return std::vector<Group>(1, group);
  }

  static std::vector<Group> mkRadial(const ARecipeInput &input, double r_tolerance)
  {
    std::vector<std::tuple<double, int>> dist_ipm(input.sensorsPos().size());
    for(size_t i = 0; i < dist_ipm.size(); i++)
      dist_ipm[i] = std::make_tuple(input.sensorsPos()[i].normxy(), i);
    std::sort(dist_ipm.begin(), dist_ipm.end());

    std::vector<Group> groups;
    Group group;
    double r0 = std::get<0>(dist_ipm[0]);
    for(size_t i = 0; i < dist_ipm.size(); i++) {
      if(std::get<0>(dist_ipm[i]) - r0 > r_tolerance) {
        groups.push_back(group);
        group = Group();
        r0 = std::get<0>(dist_ipm[i]);
      }
      int ipm = std::get<1>(dist_ipm[i]);
      const APoint &pos = input.sensorsPos()[ipm];
      group.emplace_back(ipm, -pos, -pos.angleToOrigin());
    }
    groups.shrink_to_fit();
    return groups;
  }

  static std::vector<Group> mkLattice(const ARecipeInput &input, int N_gon, double r_tolerance)
  {
    //Sort sensors by distance to sensor
    std::vector<std::tuple<double, int>> dist_ipm(input.sensorsPos().size());
    for(size_t i = 0; i < dist_ipm.size(); i++)
      dist_ipm[i] = std::make_tuple(input.sensorsPos()[i].normxy(), i);
    std::sort(dist_ipm.begin(), dist_ipm.end());

    const double symmetry_factor = N_gon/(2*M_PI);
    std::vector<Group> groups;
    auto end = dist_ipm.end();
    for(auto it = dist_ipm.begin(), next_it = it; it != end; it = next_it) {
      //Determine next radial group index
      double r0 = std::get<0>(*it);
      while(++next_it != end && std::get<0>(*next_it) - r0 < r_tolerance);

      //Make r group, and cache phi and flipped phi values
      auto sensors_pos = input.sensorsPos();
      std::vector<std::tuple<double, int, double>> phi_ipm_fphi(next_it - it);
      for(size_t i = 0; i < phi_ipm_fphi.size(); i++) {
        int ipm = std::get<1>(it[i]);
        const APoint &pos = sensors_pos[ipm];
        double flip_phi = APoint::angleToOrigin(pos.x(), -pos.y());
        phi_ipm_fphi[i] = std::tuple<double, int, double>(pos.angleToOrigin(), ipm, flip_phi);
      }

      //Make the phi groups
#if 0 //Vladimir's version
      std::sort(phi_ipm_fphi.begin(), phi_ipm_fphi.end());
      while(phi_ipm_fphi.size() > 0) {
        double phi0 = std::get<0>(phi_ipm_fphi[0]);
        std::sort(phi_ipm_fphi.begin(), phi_ipm_fphi.end(), std::greater<>());
#else //Raimundo's version
      //Somehow not sorting by phi messes with the results of non-regular arrays
      //and we can't preemptively sort along with radius because of r_tolerance.
      //The (un)sorting problem should be understood before any further changes!
      std::sort(phi_ipm_fphi.begin(), phi_ipm_fphi.end(), std::greater<>());
      while(phi_ipm_fphi.size() > 0) {
        //Somehow not choosing the smallest phi as phi0, messes the results
        double phi0 = std::get<0>(*phi_ipm_fphi.rbegin());
#endif
        Group group;
        for(int i = phi_ipm_fphi.size()-1; i >= 0; --i) {
          int ipm = std::get<1>(phi_ipm_fphi[i]);
          const APoint &pos = sensors_pos[ipm];
          double crap;
          double phi = std::get<0>(phi_ipm_fphi[i]) - phi0;
          if(modf(phi*symmetry_factor+0.00005, &crap) < 0.0001) {
            group.emplace_back(ipm, -pos, -phi, false);
            phi_ipm_fphi.erase(phi_ipm_fphi.begin()+i);
          } else {
            phi = std::get<2>(phi_ipm_fphi[i]) - phi0;
            if(modf(phi*symmetry_factor+0.00005, &crap) < 0.0001) {
              group.emplace_back(ipm, -pos, phi, true);
              phi_ipm_fphi.erase(phi_ipm_fphi.begin()+i);
            }
          }
        }
        //qDebug()<<"New group:";
        //group.print();
        groups.push_back(group);
      }
    }
    groups.shrink_to_fit();
    return groups;
  }
};

FitLayer::FitLayer(const QJsonObject &settings) : settings(settings)
{
  QString lrf_type_name = settings["LrfType"].toString();
  std::string lrf_type_name_std = std::string(lrf_type_name.toLocal8Bit().data());
  lrf_type = ALrfTypeManager::instance().getType(lrf_type_name_std);
  if(lrf_type == nullptr)
    qDebug()<<"Unknown lrf type:"<<lrf_type_name<<"."
              "This should never happen (will crash soon)!";

  lrf_settings = settings["LrfSettings"].toObject();

  QString str_stack_op = settings["EffectOnPrevious"].toString();
  if(str_stack_op == "append") stack_op = 0;
  else if(str_stack_op == "overwrite") stack_op = 1;

  QJsonObject groupping = settings["Groupping"].toObject();
  group_type = groupping["Type"].toInt();
  adjust_gains = groupping["AdjustGains"].toBool();
}

bool FitLayer::apply(ASensorGroup &group, const ARecipeInput &input) const
{
  std::vector<Group> symmetry_groups;
  std::vector<ASensorGroup> resulting_groups;
  switch(group_type) {
    default: qDebug()<<"Unimplemented groupping type "<<group_type<<"."
                       "This should never happen (not groupping sensors)!";
    case 0: symmetry_groups = Group::mkIndividual(input); break;
    case 1: symmetry_groups = Group::mkCommon(input); break;
    case 2: symmetry_groups = Group::mkRadial(input, 0.1); break;
    case 3: symmetry_groups = Group::mkLattice(input, 4, 0.1); break;
    case 4: symmetry_groups = Group::mkLattice(input, 6, 0.1); break;
  }

  const int num_events = input.size();
  for(auto &symmetry : symmetry_groups) {
    const int sensor_count = symmetry.size();
    std::vector<double> group_signals(num_events*sensor_count);
    std::vector<APoint> group_pos(num_events*sensor_count);
    double minx = 0, miny = 0, maxx = 0, maxy = 0;

    //TODO: optimize: better array access, save memory on duplicate event signals
    for(int i = 0; i < sensor_count; i++) {
      auto ipm_transf = symmetry[i];
      for(int iev = 0; iev < num_events; iev++) {
        auto &pos = input.eventPos(iev);

        //Copy-transform event positions
        auto trans_pos = ipm_transf.second.transform(pos);
        group_pos[i*num_events+iev] = trans_pos;

        if(adjust_gains) {
          minx = std::min(minx, trans_pos.x());
          maxx = std::max(maxx, trans_pos.x());
          miny = std::min(miny, trans_pos.y());
          maxy = std::max(maxy, trans_pos.y());
        }

        //Copy event signals
        group_signals[i*num_events+iev] = input.eventSignal(iev, ipm_transf.first);
        if(stack_op == 0) { //append
          const ASensor *sensor = group.getSensor(ipm_transf.first);
          if(sensor != nullptr)
            group_signals[i*num_events+iev] -= sensor->eval(pos);
        }
      }
    }

    ALrf *lrf = lrf_type->lrfFromData(lrf_settings, APoint(), group_pos, group_signals);
    if(!lrf) return false;

    std::vector<double> gains(sensor_count, 1.0);
    //TODO: put in separate rule
    if(adjust_gains) {
      const int ngrid = sqrt(num_events/7.);
      TProfile2D hp0("hpgains0", "hpgains0", ngrid, minx, maxx, ngrid, miny, maxy);
      for (int i = 0; i < num_events; i++)
        hp0.Fill(group_pos[i].x(), group_pos[i].y(), group_signals[i]);

      for(int ipm = 1; ipm < sensor_count; ipm++)
      {
        const APoint *pos = &group_pos[num_events*ipm];
        const double *pmsig = &group_signals[num_events*ipm];

        TProfile2D hp1("hpgains", "hpgains", ngrid, minx, maxx, ngrid, miny, maxy);
        for (int i = 0; i < num_events; i++)
          hp1.Fill(pos[i].x(), pos[i].y(), pmsig[i]);

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

        if (sumxx == 0) gains[ipm] = 666.0;
        else gains[ipm] = sumxy/sumxx;
      }
    }

    ASensorGroup current_symmetry_group;
    for(int i = 0; i < sensor_count; i++) {
      auto ipm_transf = symmetry[i];
      ATransform inverse = ipm_transf.second;
      inverse.invert();
      auto tranfs_lrf = std::shared_ptr<ALrf>(lrf->transform(inverse));
      qDebug()<<"gain"<<ipm_transf.first<<"="<<gains[i];
      current_symmetry_group.overwrite(ASensor(ipm_transf.first, gains[i], tranfs_lrf));
    }
    resulting_groups.push_back(current_symmetry_group);
    delete lrf;
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

std::vector<ADetectorID> FitLayer::detectorDepends() const
{
  return std::vector<ADetectorID>();
}

QJsonObject FitLayer::toJson() const
{
  return QJsonObject();
}

} } //namespace LRF::Recipes
