#ifndef LRFARECIPEINPUT_H
#define LRFARECIPEINPUT_H

#include <vector>

#include "apoint.h"
#include "apositionenergyrecords.h"

#include "ajsontools.h"

namespace LRF {

class ASensorGroup;

///
/// \brief A struct to be passed to ARecipe::apply().
/// \details This is one of the bridges between Ants2 and the Lrf Module.
///  It knows all (actual) sensor positions, and caches pointers for good
///  events.
///  Maybe later we can find a way for the recipe/lrf to ask for specific data
///  on demand. But for now this is it, or Andrey will kill me for the time this
///  is taking to make!
///
class ARecipeInput
{
  const std::vector<APoint> *sensor_positions;
  std::vector<const QVector<float> *> events;
  std::vector<const ABaseScanAndReconRecord *> records;
  bool fUsedScanData;
public:
  //Designed to be compatible with EventsDataClass. Change at will if exporting.
  ARecipeInput(const std::vector<APoint> &sensor_positions,
               const QVector<QVector<float>> &events,
               const QVector<AReconRecord*> &reconData,
               const QVector<AScanRecord*> &scanData)
  {
    this->sensor_positions = &sensor_positions;
    fUsedScanData = !scanData.isEmpty();

    const int isize = events.size();
    for (int i = 0; i < isize; i++)
    {
      if (!reconData[i]->GoodEvent)
        continue;

      if (fUsedScanData)  records.push_back(scanData[i]);
      else                records.push_back(reconData[i]);
      this->events.push_back(&events[i]);
    }

    this->events.shrink_to_fit();
    records.shrink_to_fit();
  }

  const std::vector<APoint> &sensorsPos() const { return *sensor_positions; }

  int size() const { return events.size(); }
  APoint eventPos(int ievent) const { return APoint(records[ievent]->Points[0].r); }
  double eventSignal(int iev, int ipm) const { return (*events[iev])[ipm]; }
};

} //namespace LRF

#endif // LRFARECIPEINPUT_H
