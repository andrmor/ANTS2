#include "ainstructioninput.h"

#include "apmgroupsmanager.h"
#include "eventsdataclass.h"

namespace LRF {

AInstructionInput::AInstructionInput(const ARepository *repo,
                                     const std::vector<APoint> &sensor_positions,
                                     const APmGroupsManager *sensor_groups,
                                     const EventsDataClass *events_data_hub,
                                     bool fUseScanData, bool fit_error, bool scale_by_energy)
{
  this->repo = repo;
  this->sensor_groups = sensor_groups;
  this->sensor_positions = sensor_positions;
  this->current_group = sensor_groups->getCurrentGroup();
  this->used_scan_data = fUseScanData;
  this->fit_error = fit_error;

  scale_by_energy = scale_by_energy && !fUseScanData;

  for(int igrp = 0; igrp < sensor_groups->countPMgroups(); igrp++) {
#if 0
    std::vector<APoint> positions;
    for(size_t i = 0; i < sensor_positions.size(); i++)
      if(sensor_groups->isPmBelongsToGroupFast(i, igrp))
        positions.push_back(sensor_positions[i]);
    positions.shrink_to_fit();
    this->sensor_positions.push_back(std::move(sensor_positions));
#endif

    if(events_data_hub) {
      const int isize = events_data_hub->Events.size();
      const QVector<AReconRecord*> &reconData = events_data_hub->ReconstructionData[igrp];
      std::vector<const QVector<float> *> grp_events;
      std::vector<const ABaseScanAndReconRecord *> grp_records;
      std::vector<double> grp_energy_factors;
      for(int i = 0; i < isize; i++) {
        if (!reconData[i]->GoodEvent)
          continue;

        if (fUseScanData)  grp_records.push_back(events_data_hub->Scan[i]);
        else               grp_records.push_back(reconData[i]);
        grp_events.push_back(&events_data_hub->Events[i]);
        grp_energy_factors.push_back(scale_by_energy ? 1./reconData[i]->Points[0].energy : 1.);
      }
      grp_events.shrink_to_fit();
      events.push_back(std::move(grp_events));
      grp_records.shrink_to_fit();
      records.push_back(std::move(grp_records));
      grp_energy_factors.shrink_to_fit();
      energy_factors.push_back(std::move(grp_energy_factors));
    }
  }
}

bool AInstructionInput::isPmBelongsToGroup(int ipm, int group) const
{
  return sensor_groups->isPmBelongsToGroup(ipm, group);
}

size_t AInstructionInput::sensorCount(int sensor_group) const
{
  return sensor_groups->countPMsInGroup(sensor_group);
}

} //namespace LRF
