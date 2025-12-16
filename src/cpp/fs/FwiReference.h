// https://publications.gc.ca/collections/collection_2016/rncan-nrcan/Fo133-1-424-eng.pdf
#ifndef FS_FWIREFERENCE_H
#define FS_FWIREFERENCE_H
#include "stdafx.h"
#include "FWI.h"
#include "Weather.h"
namespace fs::fwireference
{
// months as array indexes
class Month
{
public:
  enum class Value
  {
    January,
    February,
    March,
    April,
    May,
    June,
    July,
    August,
    September,
    October,
    November,
    December
  };
  static Month from_index(const int value) { return {static_cast<Value>(value)}; }
  static Month from_ordinal(const int value) { return {static_cast<Value>(value - 1)}; }
  Month(const Value value) : value{value} { }
  int ordinal() const { return static_cast<int>(value) + 1; }
  size_t index() const { return static_cast<size_t>(value); }
  string name() const
  {
    static constexpr string NAMES[]{
      "January",
      "February",
      "March",
      "April",
      "May",
      "June",
      "July",
      "August",
      "September",
      "October",
      "November",
      "December"
    };
    return NAMES[index()];
  }

private:
  Value value;
};
struct Latitude
{
  MathSize value{};
  auto operator<=>(const Latitude& rhs) const = default;
  Latitude operator-(const Latitude& rhs) const { return {value - rhs.value}; }
  Latitude operator-() const { return {-value}; }
};
static inline Latitude abs(const Latitude& rhs) { return {std::abs(rhs.value)}; }
struct Moisture
{
  MathSize value{};
};
constexpr Latitude DEFAULT_LATITUDE{46.0};
Ffmc FFMCcalc(
  Temperature temperature,
  RelativeHumidity relative_humidity,
  Speed wind_speed,
  Precipitation rain_24hr,
  Ffmc ffmc_previous
);
Dmc DMCcalc(
  Temperature temperature,
  RelativeHumidity relative_humidity,
  Precipitation rain_24hr,
  const Dmc dmc_previous,
  const Month month,
  const Latitude latitude = DEFAULT_LATITUDE
);
Dc DCcalc(
  Temperature temperature,
  Precipitation rain_24hr,
  const Dc dc_previous,
  const Month month,
  const Latitude latitude = DEFAULT_LATITUDE
);
Isi ISIcalc(Ffmc ffmc, Speed wind_speed);
Bui BUIcalc(Dmc dmc, Dc dc);
Fwi FWIcalc(Isi isi, Bui bui);
}
#endif
