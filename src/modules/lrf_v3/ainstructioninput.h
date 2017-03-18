#ifndef LRFAINSTRUCTIONINPUT_H
#define LRFAINSTRUCTIONINPUT_H

#include <functional>
#include <vector>
#include <QVector>

#include "apoint.h"
#include "apositionenergyrecords.h"

class APmGroupsManager;
class EventsDataClass;

namespace LRF {

class ASensorGroup;
class ARepository;

///
/// \brief A struct to be passed to AInstruction::apply().
/// \details This is one of the bridges between Ants2 and the Lrf Module.
///  It knows all (actual) sensor positions, and caches pointers for good
///  events.
///
class AInstructionInput
{
  const ARepository *repo;
  const APmGroupsManager *sensor_groups;
  std::vector<APoint> sensor_positions;
  std::vector<std::vector<const QVector<float> *>> events;
  std::vector<std::vector<const ABaseScanAndReconRecord *>> records;
  std::vector<std::vector<double>> energy_factors;
  //Values range of [0;1]. Return value of false means stop!
  std::function<bool(float)> progress_reporter;
  int current_group;
  bool used_scan_data, fit_error;
public:
  AInstructionInput(const ARepository *repo,
                    const std::vector<APoint> &sensor_positions,
                    const APmGroupsManager *sensor_groups,
                    const EventsDataClass *events_data_hub,
                    bool fUseScanData, bool fit_error, bool scale_by_energy);

  template<typename T> void setProgressReporter(T reporter) { progress_reporter = reporter; }
  bool reportProgress(float progress) const { return progress_reporter(progress); }

  const ARepository &repository() const { return *repo; }
  bool isPmBelongsToGroup(int ipm, int group) const;
  int getCurrentSensorGroup() const { return current_group; }
  bool fitError() const { return fit_error; }
  const std::vector<APoint> &getSensorsPosition() const { return sensor_positions; }

  size_t sensorCount(int sensor_group) const;
  int eventCount(int sensor_group) const { return events[sensor_group].size(); }
  APoint eventPos(int iev, int sensor_group) const {
    return APoint(records[sensor_group][iev]->Points[0].r);
  }
  float eventSignal(int iev, int ipm, int sensor_group) const {
    return (*events[sensor_group][iev])[ipm] * energy_factors[sensor_group][iev];
  }
};

} //namespace LRF

#endif // LRFAINSTRUCTIONINPUT_H
