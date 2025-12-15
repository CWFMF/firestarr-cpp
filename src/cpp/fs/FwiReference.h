// https://publications.gc.ca/collections/collection_2016/rncan-nrcan/Fo133-1-424-eng.pdf
#ifndef FS_FWIREFERENCE_H
#define FS_FWIREFERENCE_H
#include "unstable.h"
namespace fs::fwireference
{
constexpr auto DEFAULT_LATITUDE = 46.0;
MathSize FFMCcalc(
  MathSize temperature,
  MathSize relative_humidity,
  MathSize wind_speed,
  MathSize rain_24hr,
  MathSize ffmc_previous
);
MathSize DMCcalc(
  MathSize temperature,
  MathSize relative_humidity,
  MathSize rain_24hr,
  MathSize dmc_previous,
  int month,
  const MathSize latitude = DEFAULT_LATITUDE
);
MathSize DCcalc(
  MathSize temperature,
  MathSize rain_24hr,
  MathSize dc_previous,
  int month,
  const MathSize latitude = DEFAULT_LATITUDE
);
MathSize ISIcalc(MathSize ffmc, MathSize wind_speed);
MathSize BUIcalc(MathSize dmc, MathSize dc);
MathSize FWIcalc(MathSize isi, MathSize bui);
}
#endif
