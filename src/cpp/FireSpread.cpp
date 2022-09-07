/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FireSpread.h"
#include "FuelLookup.h"
#include "FuelType.h"
#include "Scenario.h"
#include "Settings.h"
#include "unstable.h"
namespace fs
{
/**
 * \brief Maximum slope that affects ISI - everything after this is the same factor
 */
static constexpr auto MAX_SLOPE_FOR_FACTOR = 69;
SlopeTableArray make_slope_table() noexcept
{
  // HACK: slope can be infinite, but anything > max is the same as max
  // ST-X-3 Eq. 39 - Calculate Spread Factor
  // GLC-X-10 39a/b increase to 70% limit
  SlopeTableArray result{};
  for (size_t i = 0; i <= MAX_SLOPE_FOR_FACTOR; ++i)
  {
    result.at(i) = exp(3.533 * pow(i / 100.0, 1.2));
  }
  constexpr auto MAX_SLOPE = MAX_SLOPE_FOR_FACTOR + 1;
  // anything >=70 is just 10
  std::fill(&(result[MAX_SLOPE]), &(result[MAX_SLOPE_FOR_DISTANCE]), 10.0);
  // if we ask for result of invalid slope it should be invalid
  std::fill(&(result[MAX_SLOPE_FOR_DISTANCE + 1]), &(result[INVALID_SLOPE]), -1);
  static_assert(result.size() == INVALID_SLOPE + 1);
  return result;
}
const SlopeTableArray SpreadInfo::SlopeTable = make_slope_table();
int calculate_nd_ref_for_point(const int elevation, const Point& point) noexcept
{
  // NOTE: cffdrs R package stores longitude West as a positive, so this would be `- long`
  const auto latn = elevation <= 0 ? (46.0 + 23.4 * exp(-0.0360 * (150 + point.longitude())))
                                   : (43.0 + 33.7 * exp(-0.0351 * (150 + point.longitude())));
  // add 0.5 to round by truncating
  return static_cast<int>(truncl(
    0.5
    + (elevation <= 0 ? 151.0 * (point.latitude() / latn) : 142.1 * (point.latitude() / latn) + 0.0172 * elevation)
  ));
}
int calculate_nd_for_point(const Day day, const int elevation, const Point& point)
{
  return static_cast<int>(abs(day - calculate_nd_ref_for_point(elevation, point)));
}
static constexpr MathSize calculate_standard_back_isi_wsv(const MathSize v) noexcept
{
  return 0.208 * exp(-0.05039 * v);
}
static const LookupTable<&calculate_standard_back_isi_wsv> STANDARD_BACK_ISI_WSV{};
static constexpr MathSize calculate_standard_wsv(const MathSize v) noexcept
{
  return v < 40.0 ? exp(0.05039 * v) : 12.0 * (1.0 - exp(-0.0818 * (v - 28)));
}
static const LookupTable<&calculate_standard_wsv> STANDARD_WSV{};
SpreadInfo::SpreadInfo(
  const Scenario& scenario,
  const DurationSize time,
  const SpreadKey& key,
  const int nd,
  const ptr<const FwiWeather> weather
)
  : SpreadInfo(scenario, time, key, nd, weather, scenario.weather_daily(time))
{ }
MathSize SpreadInfo::initial(
  SpreadInfo& spread,
  const FwiWeather& weather,
  MathSize& ffmc_effect,
  MathSize& wsv,
  MathSize& rso,
  MathSize& raz,
  const FuelType* const fuel,
  bool has_no_slope,
  MathSize heading_sin,
  MathSize heading_cos,
  MathSize bui_eff,
  MathSize min_ros,
  MathSize critical_surface_intensity
)
{
  ffmc_effect = spread.ffmcEffect();
  // needs to be non-const so that we can update if slopeEffect changes direction
  raz = spread.wind().heading();
  const auto isz = 0.208 * ffmc_effect;
  wsv = spread.wind().speed().asValue();
  if (!has_no_slope)
  {
    const auto isf1 = fuel->calculateIsf(spread, isz);
    auto wse = 0.0 == isf1 ? 0 : log(isf1 / isz) / 0.05039;
    if (wse > 40)
    {
      wse =
        28.0 - log(1.0 - min(0.999 * 2.496 * ffmc_effect, isf1) / (2.496 * ffmc_effect)) / 0.0818;
    }
    const auto heading = to_heading(to_radians(static_cast<double>(Cell::aspect(spread.key_))));
    const auto wsv_x = spread.wind().wsvX() + wse * cos(heading);
    const auto wsv_y = spread.wind().wsvY() + wse * sin(heading);
    // // we know that at->raz is already set to be the wind heading
    // const auto wsv_x = spread.wind().wsvX() + wse * heading_sin;
    // const auto wsv_y = spread.wind().wsvY() + wse * heading_cos;
    wsv = sqrt(wsv_x * wsv_x + wsv_y * wsv_y);
    raz = (0 == wsv) ? 0 : acos(wsv_y / wsv);
    if (wsv_x < 0)
    {
      raz = RAD_360 - raz;
    }
  }
  spread.raz_ = Direction(raz, true);
  const auto isi = isz * STANDARD_WSV(wsv);
  // FIX: make this a member function so we don't need to preface head_ros_
  spread.head_ros_ = fuel->calculateRos(spread.nd(), weather, isi) * bui_eff;
  if (min_ros > spread.head_ros_)
  {
    spread.head_ros_ = INVALID_ROS;
  }
  else
  {
    spread.sfc_ = fuel->surfaceFuelConsumption(spread);
    rso = FuelType::criticalRos(spread.sfc_, critical_surface_intensity);
    const auto sfi = fire_intensity(spread.sfc_, spread.head_ros_);
    spread.is_crown_ = FuelType::isCrown(critical_surface_intensity, sfi);
    if (spread.is_crown_)
    {
      spread.head_ros_ = fuel->finalRos(
        spread, isi, fuel->crownFractionBurned(spread.head_ros_, rso), spread.head_ros_
      );
    }
  }
  return spread.head_ros_;
}
static MathSize find_min_ros(const Scenario& scenario, const DurationSize time)
{
  return Settings::deterministic()
         ? Settings::minimumRos()
         : std::max(scenario.spreadThresholdByRos(time), Settings::minimumRos());
}
SpreadInfo::SpreadInfo(
  const Scenario& scenario,
  const DurationSize time,
  const SpreadKey& key,
  const int nd,
  const ptr<const FwiWeather> weather,
  const ptr<const FwiWeather> weather_daily
)
  : SpreadInfo(
      time,
      find_min_ros(scenario, time),
      scenario.cellSize(),
      key,
      nd,
      weather,
      weather_daily
    )
{ }
static SpreadKey make_key(const SlopeSize slope, const AspectSize aspect, const char* fuel_name)
{
  const auto lookup = Settings::fuelLookup();
  const auto key =
    Cell::key(Cell::hashCell(slope, aspect, FuelType::safeCode(lookup.byName(fuel_name))));
  const auto a = Cell::aspect(key);
  const auto s = Cell::slope(key);
  const auto fuel = fuel_by_code(Cell::fuelCode(key));
  logging::check_equal(s, slope, "slope");
  logging::check_equal(
    a, (static_cast<SlopeSize>(0) == slope ? static_cast<AspectSize>(0) : aspect), "aspect"
  );
  logging::check_equal(fuel->name(), fuel_name, "fuel");
  return key;
}
SpreadInfo::SpreadInfo(
  const YearSize year,
  const int month,
  const int day,
  const int hour,
  const int minute,
  const Point& start_point,
  const ElevationSize elevation,
  const SlopeSize slope,
  const AspectSize aspect,
  const char* fuel_name,
  const ptr<const FwiWeather> weather
)
  : SpreadInfo(
      to_tm(year, month, day, hour, minute),
      start_point,
      elevation,
      slope,
      aspect,
      fuel_name,
      weather
    )
{ }
SpreadInfo::SpreadInfo(
  const tm& start_date,
  const Point& start_point,
  const ElevationSize elevation,
  const SlopeSize slope,
  const AspectSize aspect,
  const char* fuel_name,
  const ptr<const FwiWeather> weather
)
  : SpreadInfo(
      to_time(start_date),
      0.0,
      100.0,
      slope,
      aspect,
      fuel_name,
      calculate_nd_for_point(start_date.tm_yday, elevation, start_point),
      weather
    )
{ }
SpreadInfo::SpreadInfo(
  const DurationSize time,
  const MathSize min_ros,
  const MathSize cell_size,
  const SlopeSize slope,
  const AspectSize aspect,
  const char* fuel_name,
  const int nd,
  const ptr<const FwiWeather> weather
)
  : SpreadInfo(time, min_ros, cell_size, make_key(slope, aspect, fuel_name), nd, weather, weather)
{ }
SpreadInfo::SpreadInfo(
  const DurationSize time,
  const MathSize min_ros,
  const MathSize cell_size,
  const SpreadKey& key,
  const int nd,
  const ptr<const FwiWeather> weather
)
  : SpreadInfo(time, min_ros, cell_size, key, nd, weather, weather)
{ }
SpreadInfo::SpreadInfo(
  const DurationSize time,
  const MathSize min_ros,
  const MathSize cell_size,
  const SpreadKey& key,
  const int nd,
  const ptr<const FwiWeather> weather,
  const ptr<const FwiWeather> weather_daily
)
  : key_(key), weather_(weather), time_(time), nd_(nd)
{
  // HACK: use weather_daily to figure out probability of spread but hourly for ROS
  const auto slope_azimuth = Cell::aspect(key_);
  const auto fuel = fuel_by_code(Cell::fuelCode(key_));
  if (is_null_fuel(fuel))
  {
    return;
  }
  const auto has_no_slope = 0 == percentSlope();
  MathSize heading_sin = 0;
  MathSize heading_cos = 0;
  if (!has_no_slope)
  {
    const auto heading = to_heading(to_radians(static_cast<MathSize>(slope_azimuth)));
    heading_sin = sin(heading);
    heading_cos = cos(heading);
  }
  // HACK: only use BUI from hourly weather for both calculations
  const auto _bui = bui().asValue();
  const auto bui_eff = fuel->buiEffect(_bui);
  // FIX: gets calculated when not necessary sometimes
  const auto critical_surface_intensity = fuel->criticalSurfaceIntensity(*this);
  MathSize ffmc_effect;
  MathSize wsv;
  MathSize rso;
  MathSize raz;
  if (min_ros > SpreadInfo::initial(
        *this,
        *weather_daily,
        ffmc_effect,
        wsv,
        rso,
        raz,
        fuel,
        has_no_slope,
        heading_sin,
        heading_cos,
        bui_eff,
        min_ros,
        critical_surface_intensity
      )
      || sfc_ < COMPARE_LIMIT)
  {
    return;
  }
  // Now use hourly weather for actual spread calculations
  // don't check again if pointing at same weather
  if (weather != weather_daily)
  {
    if ((min_ros > SpreadInfo::initial(
           *this,
           *weather,
           ffmc_effect,
           wsv,
           rso,
           raz,
           fuel,
           has_no_slope,
           heading_sin,
           heading_cos,
           bui_eff,
           min_ros,
           critical_surface_intensity
         )
         || sfc_ < COMPARE_LIMIT))
    {
      // no spread with hourly weather
      // NOTE: only would happen if FFMC hourly is lower than FFMC daily?
      return;
    }
  }
  const auto back_isi = ffmc_effect * STANDARD_BACK_ISI_WSV(wsv);
  auto back_ros = fuel->calculateRos(nd, *weather, back_isi) * bui_eff;
  if (is_crown_)
  {
    back_ros = fuel->finalRos(*this, back_isi, fuel->crownFractionBurned(back_ros, rso), back_ros);
  }
  // do everything we can to avoid calling trig functions unnecessarily
  const auto b_semi = has_no_slope ? 0 : cos(atan(percentSlope() / 100.0));
  const auto slope_radians = to_radians(slope_azimuth);
  // do check once and make function just return 1.0 if no slope
  const auto no_correction = [](const MathSize) noexcept { return 1.0; };
  const auto do_correction = [b_semi, slope_radians](const MathSize theta) noexcept {
    // never gets called if isInvalid() so don't check
    // figure out how far the ground distance is in map distance horizontally
    auto angle_unrotated = theta - slope_radians;
    // if (to_degrees(angle_unrotated) == 270 || to_degrees(angle_unrotated) == 90)
    // {
    //   // CHECK: if we're going directly across the slope then horizontal distance is same as
    //   spread
    //   // distance
    //   return 1.0;
    // }
    const auto tan_u = tan(angle_unrotated);
    const auto y = b_semi / sqrt(b_semi * tan_u * (b_semi * tan_u) + 1.0);
    const auto x = y * tan_u;
    // CHECK: Pretty sure you can't spread farther horizontally than the spread distance, regardless
    // of angle?
    return min(1.0, sqrt(x * x + y * y));
    // return sqrt(x * x + y * y);
  };
  const auto correction_factor = has_no_slope ? std::function<MathSize(MathSize)>(no_correction)
                                              : std::function<MathSize(MathSize)>(do_correction);
  const auto add_offset = [this, cell_size, min_ros](const MathSize direction, const MathSize ros) {
    if (ros < min_ros)
    {
      return false;
    }
    // spreading, so figure out offset from current point
    const auto ros_cell = ros / cell_size;
    offsets_.emplace_back(ros_cell * sin(direction), ros_cell * cos(direction));
    return true;
  };
  // if not over spread threshold then don't spread
  // HACK: assume there is no fuel where a crown fire's sfc is < COMPARE_LIMIT and its fc is greater
  MathSize ros{};
  // HACK: set ros in boolean if we get that far so that we don't have to repeat the if body
  if (!add_offset(raz, ros = (head_ros_ * correction_factor(raz))))
  {
    // mark as invalid
    head_ros_ = -1;
    return;
  }
  tfc_ = sfc_;
  // don't need to re-evaluate if crown with new head_ros_ because it would only go up if is_crown_
  if (fuel->canCrown() && is_crown_)
  {
    // wouldn't be crowning if ros is 0 so that's why this is in an else
    cfb_ = fuel->crownFractionBurned(head_ros_, rso);
    cfc_ = fuel->crownConsumption(cfb_);
    tfc_ += cfc_;
  }
  // max intensity should always be at the head
  max_intensity_ = fire_intensity(tfc_, ros);
  const auto a = (head_ros_ + back_ros) / 2.0;
  const auto c = a - back_ros;
  const auto l_b = fuel->lengthToBreadth(wsv);
  const auto flank_ros = a / l_b;
  const auto a_sq = a * a;
  const auto flank_ros_sq = flank_ros * flank_ros;
  const auto a_sq_sub_c_sq = a_sq - (c * c);
  const auto ac = a * c;
  const auto calculate_ros = [a, c, ac, flank_ros, a_sq, flank_ros_sq, a_sq_sub_c_sq](
                               const DirectionSize theta
                             ) noexcept {
    const auto cos_t = cos(theta);
    const auto cos_t_sq = cos_t * cos_t;
    const auto f_sq_cos_t_sq = flank_ros_sq * cos_t_sq;
    // 1.0 = cos^2 + sin^2
    const auto sin_t_sq = 1.0 - cos_t_sq;
    // // or could be: 1.0 = cos^2 + sin^2
    // const auto sin_t = sin(theta);
    // const auto sin_t_sq = sin_t * sin_t;
    return abs(
      (a * ((flank_ros * cos_t * sqrt(f_sq_cos_t_sq + a_sq_sub_c_sq * sin_t_sq) - ac * sin_t_sq) / (f_sq_cos_t_sq + a_sq * sin_t_sq))
       + c)
      / cos_t
    );
  };
  const auto add_offsets = [&correction_factor, &add_offset, raz, min_ros](
                             const MathSize angle_radians, const MathSize ros_flat
                           ) {
    if (ros_flat < min_ros)
    {
      return false;
    }
    auto direction = fix_radians(angle_radians + raz);
    // spread is symmetrical across the center axis, but needs to be adjusted if on a slope
    // intentionally don't use || because we want both of these to happen all the time
    auto added = add_offset(direction, ros_flat * correction_factor(direction));
    direction = fix_radians(raz - angle_radians);
    added |= add_offset(direction, ros_flat * correction_factor(direction));
    return added;
  };
  const auto add_offsets_calc_ros = [&add_offsets, &calculate_ros](const MathSize angle_radians) {
    return add_offsets(angle_radians, calculate_ros(angle_radians));
  };
  bool added = true;
  constexpr size_t STEP = 10;
  size_t i = STEP;
  while (added && i < 90)
  {
    added = add_offsets_calc_ros(to_radians(i));
    i += STEP;
  }
  if (added)
  {
    added = add_offsets(to_radians(90), flank_ros * sqrt(a_sq_sub_c_sq) / a);
    i = 90 + STEP;
    while (added && i < 180)
    {
      added = add_offsets_calc_ros(to_radians(i));
      i += STEP;
    }
    if (added)
    {
      // only use back ros if every other angle is spreading since this should be lowest
      //  180
      if (back_ros < min_ros)
      {
        return;
      }
      const auto direction = fix_radians(RAD_180 + raz);
      static_cast<void>(!add_offset(direction, back_ros * correction_factor(direction)));
    }
  }
}
}
