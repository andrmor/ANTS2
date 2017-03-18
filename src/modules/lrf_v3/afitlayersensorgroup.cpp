#include "afitlayersensorgroup.h"

#include <tuple>
#include <algorithm>
#include <functional>

#include <QDebug>

#include "apoint.h"
#include "atransform.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846264338327	/* pi */
#endif

namespace LRF {

void AFitLayerSensorGroup::push_back(int ipm, const ATransform &t)
{
  sensor_indices.push_back(ipm);
  to_sensor_origin.push_back(t);
}

void AFitLayerSensorGroup::print()
{
  for(std::size_t i = 0; i < to_sensor_origin.size(); i++) {
    const ATransform &t = to_sensor_origin[i];
    qDebug()<<"\tPM#"<<sensor_indices[i]<<"; phi"
           <<t.getPhi()<<(t.getFlip() ? "flipped" : "non-flipped")
          <<"; r"<<t.getShift().normxy();
  }
}

std::vector<AFitLayerSensorGroup> AFitLayerSensorGroup::mkIndividual(const std::vector<std::pair<int, APoint> > &sensor_positions)
{
  std::vector<AFitLayerSensorGroup> groups;
  for(std::size_t i = 0; i < sensor_positions.size(); i++) {
    const APoint &pos = sensor_positions[i].second;
    AFitLayerSensorGroup g; g.push_back(sensor_positions[i].first, -pos);
    groups.push_back(g);
  }
  return groups;
}

std::vector<AFitLayerSensorGroup> AFitLayerSensorGroup::mkCommon(const std::vector<std::pair<int, APoint> > &sensor_positions)
{
  AFitLayerSensorGroup group;
  for(std::size_t i = 0; i < sensor_positions.size(); i++) {
    const APoint &pos = sensor_positions[i].second;
    group.push_back(sensor_positions[i].first, ATransform(-pos, -pos.angleToOrigin()));
  }
  return std::vector<AFitLayerSensorGroup>(1, group);
}

std::vector<AFitLayerSensorGroup> AFitLayerSensorGroup::mkRadial(const std::vector<std::pair<int, APoint> > &sensor_positions, double r_tolerance)
{
  std::vector<std::tuple<double, int>> dist_ipm(sensor_positions.size());
  for(std::size_t i = 0; i < dist_ipm.size(); i++)
    dist_ipm[i] = std::make_tuple(sensor_positions[i].second.normxy(), sensor_positions[i].first);
  std::sort(dist_ipm.begin(), dist_ipm.end());

  std::vector<AFitLayerSensorGroup> groups;
  AFitLayerSensorGroup group;
  double r0 = std::get<0>(dist_ipm[0]);
  for(size_t i = 0; i < dist_ipm.size(); i++) {
    if(std::get<0>(dist_ipm[i]) - r0 > r_tolerance) {
      groups.push_back(group);
      group = AFitLayerSensorGroup();
      r0 = std::get<0>(dist_ipm[i]);
    }
    int ipm = std::get<1>(dist_ipm[i]);
    const APoint &pos = sensor_positions[ipm].second;
    group.push_back(ipm, ATransform(-pos, -pos.angleToOrigin()));
  }
  groups.push_back(group);
  groups.shrink_to_fit();
  return groups;
}

std::vector<AFitLayerSensorGroup> AFitLayerSensorGroup::mkLattice(const std::vector<std::pair<int, APoint> > &sensor_positions, int N_gon, double r_tolerance)
{
  //Sort sensors by distance to sensor
  std::vector<std::tuple<double, int>> dist_ipm(sensor_positions.size());
  for(size_t i = 0; i < dist_ipm.size(); i++)
    dist_ipm[i] = std::make_tuple(sensor_positions[i].second.normxy(), sensor_positions[i].first);
  std::sort(dist_ipm.begin(), dist_ipm.end());

  const double symmetry_factor = N_gon/(2*M_PI);
  std::vector<AFitLayerSensorGroup> groups;
  auto end = dist_ipm.end();
  for(auto it = dist_ipm.begin(), next_it = it; it != end; it = next_it) {
    //Determine next radial group index
    double r0 = std::get<0>(*it);
    while(++next_it != end && std::get<0>(*next_it) - r0 < r_tolerance);

    //Make r group, and cache phi and flipped phi values
    std::vector<std::tuple<double, int, double>> phi_ipm_fphi(next_it - it);
    for(size_t i = 0; i < phi_ipm_fphi.size(); i++) {
      int ipm = std::get<1>(it[i]);
      const APoint &pos = sensor_positions[ipm].second;
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
    std::sort(phi_ipm_fphi.begin(), phi_ipm_fphi.end(), std::greater<std::tuple<double, int, double>>());
    while(phi_ipm_fphi.size() > 0) {
      //Somehow not choosing the smallest phi as phi0, messes the results
      double phi0 = std::get<0>(*phi_ipm_fphi.rbegin());
#endif
      AFitLayerSensorGroup group;
      for(int i = phi_ipm_fphi.size()-1; i >= 0; --i) {
        int ipm = std::get<1>(phi_ipm_fphi[i]);
        const APoint &pos = sensor_positions[ipm].second;
        double crap;
        double phi = std::get<0>(phi_ipm_fphi[i]) - phi0;
        if(modf(phi*symmetry_factor+0.00005, &crap) < 0.0001) {
          group.push_back(ipm, ATransform(-pos, -phi, false));
          phi_ipm_fphi.erase(phi_ipm_fphi.begin()+i);
        } else {
          phi = std::get<2>(phi_ipm_fphi[i]) - phi0;
          if(modf(phi*symmetry_factor+0.00005, &crap) < 0.0001) {
            group.push_back(ipm, ATransform(-pos, phi, true));
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

} //namespace LRF
