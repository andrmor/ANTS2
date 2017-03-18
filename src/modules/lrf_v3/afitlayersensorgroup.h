#ifndef AFITLAYERSENSORGROUP_H
#define AFITLAYERSENSORGROUP_H

#include <vector>

struct APoint;
class ATransform;

namespace LRF {

class AFitLayerSensorGroup
{
  std::vector<int> sensor_indices;
  std::vector<ATransform> to_sensor_origin;

  void push_back(int ipm, const ATransform &t);

public:
  void print();

  bool empty() const { return to_sensor_origin.empty(); }

  std::size_t size() const { return to_sensor_origin.size(); }

  std::pair<int, const ATransform*> operator[](std::size_t i) const
  { return std::make_pair(sensor_indices[i], &to_sensor_origin[i]); }

  std::pair<int, ATransform*> operator[](std::size_t i)
  { return std::make_pair(sensor_indices[i], &to_sensor_origin[i]); }

  static std::vector<AFitLayerSensorGroup> mkIndividual(const std::vector<std::pair<int,APoint>> &sensor_positions);

  static std::vector<AFitLayerSensorGroup> mkCommon(const std::vector<std::pair<int,APoint>> &sensor_positions);

  static std::vector<AFitLayerSensorGroup> mkRadial(const std::vector<std::pair<int,APoint>> &sensor_positions, double r_tolerance);

  static std::vector<AFitLayerSensorGroup> mkLattice(const std::vector<std::pair<int,APoint>> &sensor_positions, int N_gon, double r_tolerance);
};

} //namespace LRF

#endif // AFITLAYERSENSORGROUP_H
