/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "SimpleFBP.h"
#include <limits>
#include "Duff.h"
#include "FBP45.h"
#include "FireWeather.h"
#include "FuelLookup.h"
#include "FuelType.h"
#include "FWI.h"
#include "Log.h"
#include "unstable.h"
namespace fs::simplefbp
{
MathSize SimpleFuelD1::isfD1(
  const SpreadInfo& spread,
  const MathSize ros_multiplier,
  const MathSize isi
) const noexcept
{
  return limitIsf(
    ros_multiplier,
    spread.slopeFactor() * (ros_multiplier * a()) * pow(1.0 - exp(negB() * isi), c())
  );
}
/**
 * \brief Surface SimpleFuel Consumption (SFC) (kg/m^2) [GLC-X-10 eq 9a/9b]
 * \param ffmc Fine SimpleFuel Moisture Code
 * \return Surface SimpleFuel Consumption (SFC) (kg/m^2) [GLC-X-10 eq 9a/9b]
 */
[[nodiscard]] static constexpr MathSize calculate_surface_fuel_consumption_c1(const MathSize ffmc
) noexcept
{
  return max(0.0, 0.75 + ((ffmc > 84) ? 0.75 : -0.75) * sqrt(1 - exp(-0.23 * (ffmc - 84))));
}
/**
 * \brief Surface SimpleFuel Consumption (SFC) (kg/m^2) [GLC-X-10 eq 9a/9b]
 * \return Surface SimpleFuel Consumption (SFC) (kg/m^2) [GLC-X-10 eq 9a/9b]
 */
static LookupTable<&calculate_surface_fuel_consumption_c1> SURFACE_FUEL_CONSUMPTION_C1{};
MathSize SimpleFuelC1::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return SURFACE_FUEL_CONSUMPTION_C1(spread.weather->ffmc.value);
}
MathSize SimpleFuelC2::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return SURFACE_FUEL_CONSUMPTION_MIXED_OR_C2(spread.weather->bui.value);
}
MathSize SimpleFuelC6::finalRos(
  const SpreadInfo& spread,
  const MathSize isi,
  const MathSize cfb,
  const MathSize rss
) const noexcept
{
  const auto rsc = crownRateOfSpread(isi, spread.foliarMoisture());
  // using max with 0 is the same as ensuring rsc > rss
  return rss + cfb * max(0.0, rsc - rss);
}
/**
 * \brief Forest Floor Consumption (FFC) (kg/m^2) [ST-X-3 eq 13]
 * \param ffmc Fine SimpleFuel Moisture Code
 * \return Forest Floor Consumption (FFC) (kg/m^2) [ST-X-3 eq 13]
 */
[[nodiscard]] static constexpr MathSize calculate_surface_fuel_consumption_c7_ffmc(
  const MathSize ffmc
) noexcept
{
  return (ffmc > 70) ? 2.0 * (1.0 - exp(-0.104 * (ffmc - 70.0))) : 0.0;
}
/**
 * \brief Forest Floor Consumption (FFC) (kg/m^2) [ST-X-3 eq 13]
 * \return Forest Floor Consumption (FFC) (kg/m^2) [ST-X-3 eq 13]
 */
static LookupTable<&calculate_surface_fuel_consumption_c7_ffmc> SURFACE_FUEL_CONSUMPTION_C7_FFMC{};
/**
 * \brief Woody SimpleFuel Consumption (WFC) (kg/m^2) [ST-X-3 eq 14]
 * \return Woody SimpleFuel Consumption (WFC) (kg/m^2) [ST-X-3 eq 14]
 */
[[nodiscard]] static constexpr MathSize calculate_surface_fuel_consumption_c7_bui(const MathSize bui
) noexcept
{
  return 1.5 * (1.0 - exp(-0.0201 * bui));
}
/**
 * \brief Woody SimpleFuel Consumption (WFC) (kg/m^2) [ST-X-3 eq 14]
 * \return Woody SimpleFuel Consumption (WFC) (kg/m^2) [ST-X-3 eq 14]
 */
static LookupTable<&calculate_surface_fuel_consumption_c7_bui> SURFACE_FUEL_CONSUMPTION_C7_BUI{};
MathSize SimpleFuelC7::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return SURFACE_FUEL_CONSUMPTION_C7_FFMC(spread.weather->ffmc.value)
       + SURFACE_FUEL_CONSUMPTION_C7_BUI(spread.weather->bui.value);
}
[[nodiscard]] static constexpr MathSize calculate_surface_fuel_consumption_d2(const MathSize bui
) noexcept
{
  return bui >= 80 ? 1.5 * (1.0 - exp(-0.0183 * bui)) : 0.0;
}
static LookupTable<&calculate_surface_fuel_consumption_d2> SURFACE_FUEL_CONSUMPTION_D2{};
MathSize SimpleFuelD2::surfaceFuelConsumption(const SpreadInfo& spread) const noexcept
{
  return SURFACE_FUEL_CONSUMPTION_D2(spread.weather->bui.value);
}
MathSize SimpleFuelD2::calculateRos(const int, const FwiWeather& wx, const MathSize isi)
  const noexcept
{
  return (wx.bui.value >= 80) ? rosBasic(isi) : 0.0;
}
// FIX: ensure actual code use in compilation doesn't matter and don't need to be speicified
// manually in sequence
static_assert(0 == INVALID_FUEL_CODE);
static fs::InvalidFuel NULL_FUEL{INVALID_FUEL_CODE, "Non-fuel"};
static fs::InvalidFuel INVALID{1, "Invalid"};
static SimpleFuelC1 C1{2};
static SimpleFuelC2 C2{3};
static SimpleFuelC3 C3{4};
static SimpleFuelC4 C4{5};
static SimpleFuelC5 C5{6};
static SimpleFuelC6 C6{7};
static SimpleFuelC7 C7{8};
static SimpleFuelD1 D1{9};
static SimpleFuelD2 D2{10};
static SimpleFuelO1A O1_A{11};
static SimpleFuelO1B O1_B{12};
static SimpleFuelS1 S1{13};
static SimpleFuelS2 S2{14};
static SimpleFuelS3 S3{15};
static SimpleFuelD1D2 D1_D2{16, &D1, &D2};
static SimpleFuelM1<5> M1_05{17, "M-1 (05 PC)"};
static SimpleFuelM1<10> M1_10{18, "M-1 (10 PC)"};
static SimpleFuelM1<15> M1_15{19, "M-1 (15 PC)"};
static SimpleFuelM1<20> M1_20{20, "M-1 (20 PC)"};
static SimpleFuelM1<25> M1_25{21, "M-1 (25 PC)"};
static SimpleFuelM1<30> M1_30{22, "M-1 (30 PC)"};
static SimpleFuelM1<35> M1_35{23, "M-1 (35 PC)"};
static SimpleFuelM1<40> M1_40{24, "M-1 (40 PC)"};
static SimpleFuelM1<45> M1_45{25, "M-1 (45 PC)"};
static SimpleFuelM1<50> M1_50{26, "M-1 (50 PC)"};
static SimpleFuelM1<55> M1_55{27, "M-1 (55 PC)"};
static SimpleFuelM1<60> M1_60{28, "M-1 (60 PC)"};
static SimpleFuelM1<65> M1_65{29, "M-1 (65 PC)"};
static SimpleFuelM1<70> M1_70{30, "M-1 (70 PC)"};
static SimpleFuelM1<75> M1_75{31, "M-1 (75 PC)"};
static SimpleFuelM1<80> M1_80{32, "M-1 (80 PC)"};
static SimpleFuelM1<85> M1_85{33, "M-1 (85 PC)"};
static SimpleFuelM1<90> M1_90{34, "M-1 (90 PC)"};
static SimpleFuelM1<95> M1_95{35, "M-1 (95 PC)"};
static SimpleFuelM2<5> M2_05{36, "M-2 (05 PC)"};
static SimpleFuelM2<10> M2_10{37, "M-2 (10 PC)"};
static SimpleFuelM2<15> M2_15{38, "M-2 (15 PC)"};
static SimpleFuelM2<20> M2_20{39, "M-2 (20 PC)"};
static SimpleFuelM2<25> M2_25{40, "M-2 (25 PC)"};
static SimpleFuelM2<30> M2_30{41, "M-2 (30 PC)"};
static SimpleFuelM2<35> M2_35{42, "M-2 (35 PC)"};
static SimpleFuelM2<40> M2_40{43, "M-2 (40 PC)"};
static SimpleFuelM2<45> M2_45{44, "M-2 (45 PC)"};
static SimpleFuelM2<50> M2_50{45, "M-2 (50 PC)"};
static SimpleFuelM2<55> M2_55{46, "M-2 (55 PC)"};
static SimpleFuelM2<60> M2_60{47, "M-2 (60 PC)"};
static SimpleFuelM2<65> M2_65{48, "M-2 (65 PC)"};
static SimpleFuelM2<70> M2_70{49, "M-2 (70 PC)"};
static SimpleFuelM2<75> M2_75{50, "M-2 (75 PC)"};
static SimpleFuelM2<80> M2_80{51, "M-2 (80 PC)"};
static SimpleFuelM2<85> M2_85{52, "M-2 (85 PC)"};
static SimpleFuelM2<90> M2_90{53, "M-2 (90 PC)"};
static SimpleFuelM2<95> M2_95{54, "M-2 (95 PC)"};
static SimpleFuelM1M2<5> M1_M2_05{55, "M-1/M-2 (05 PC)", &M1_05, &M2_05};
static SimpleFuelM1M2<10> M1_M2_10{56, "M-1/M-2 (10 PC)", &M1_10, &M2_10};
static SimpleFuelM1M2<15> M1_M2_15{57, "M-1/M-2 (15 PC)", &M1_15, &M2_15};
static SimpleFuelM1M2<20> M1_M2_20{58, "M-1/M-2 (20 PC)", &M1_20, &M2_20};
static SimpleFuelM1M2<25> M1_M2_25{59, "M-1/M-2 (25 PC)", &M1_25, &M2_25};
static SimpleFuelM1M2<30> M1_M2_30{60, "M-1/M-2 (30 PC)", &M1_30, &M2_30};
static SimpleFuelM1M2<35> M1_M2_35{61, "M-1/M-2 (35 PC)", &M1_35, &M2_35};
static SimpleFuelM1M2<40> M1_M2_40{62, "M-1/M-2 (40 PC)", &M1_40, &M2_40};
static SimpleFuelM1M2<45> M1_M2_45{63, "M-1/M-2 (45 PC)", &M1_45, &M2_45};
static SimpleFuelM1M2<50> M1_M2_50{64, "M-1/M-2 (50 PC)", &M1_50, &M2_50};
static SimpleFuelM1M2<55> M1_M2_55{65, "M-1/M-2 (55 PC)", &M1_55, &M2_55};
static SimpleFuelM1M2<60> M1_M2_60{66, "M-1/M-2 (60 PC)", &M1_60, &M2_60};
static SimpleFuelM1M2<65> M1_M2_65{67, "M-1/M-2 (65 PC)", &M1_65, &M2_65};
static SimpleFuelM1M2<70> M1_M2_70{68, "M-1/M-2 (70 PC)", &M1_70, &M2_70};
static SimpleFuelM1M2<75> M1_M2_75{69, "M-1/M-2 (75 PC)", &M1_75, &M2_75};
static SimpleFuelM1M2<80> M1_M2_80{70, "M-1/M-2 (80 PC)", &M1_80, &M2_80};
static SimpleFuelM1M2<85> M1_M2_85{71, "M-1/M-2 (85 PC)", &M1_85, &M2_85};
static SimpleFuelM1M2<90> M1_M2_90{72, "M-1/M-2 (90 PC)", &M1_90, &M2_90};
static SimpleFuelM1M2<95> M1_M2_95{73, "M-1/M-2 (95 PC)", &M1_95, &M2_95};
static SimpleFuelM3<5> M3_05{74, "M-3 (05 PDF)"};
static SimpleFuelM3<10> M3_10{75, "M-3 (10 PDF)"};
static SimpleFuelM3<15> M3_15{76, "M-3 (15 PDF)"};
static SimpleFuelM3<20> M3_20{77, "M-3 (20 PDF)"};
static SimpleFuelM3<25> M3_25{78, "M-3 (25 PDF)"};
static SimpleFuelM3<30> M3_30{79, "M-3 (30 PDF)"};
static SimpleFuelM3<35> M3_35{80, "M-3 (35 PDF)"};
static SimpleFuelM3<40> M3_40{81, "M-3 (40 PDF)"};
static SimpleFuelM3<45> M3_45{82, "M-3 (45 PDF)"};
static SimpleFuelM3<50> M3_50{83, "M-3 (50 PDF)"};
static SimpleFuelM3<55> M3_55{84, "M-3 (55 PDF)"};
static SimpleFuelM3<60> M3_60{85, "M-3 (60 PDF)"};
static SimpleFuelM3<65> M3_65{86, "M-3 (65 PDF)"};
static SimpleFuelM3<70> M3_70{87, "M-3 (70 PDF)"};
static SimpleFuelM3<75> M3_75{88, "M-3 (75 PDF)"};
static SimpleFuelM3<80> M3_80{89, "M-3 (80 PDF)"};
static SimpleFuelM3<85> M3_85{90, "M-3 (85 PDF)"};
static SimpleFuelM3<90> M3_90{91, "M-3 (90 PDF)"};
static SimpleFuelM3<95> M3_95{92, "M-3 (95 PDF)"};
static SimpleFuelM3<100> M3_100{93, "M-3 (100 PDF)"};
static SimpleFuelM4<5> M4_05{94, "M-4 (05 PDF)"};
static SimpleFuelM4<10> M4_10{95, "M-4 (10 PDF)"};
static SimpleFuelM4<15> M4_15{96, "M-4 (15 PDF)"};
static SimpleFuelM4<20> M4_20{97, "M-4 (20 PDF)"};
static SimpleFuelM4<25> M4_25{98, "M-4 (25 PDF)"};
static SimpleFuelM4<30> M4_30{99, "M-4 (30 PDF)"};
static SimpleFuelM4<35> M4_35{100, "M-4 (35 PDF)"};
static SimpleFuelM4<40> M4_40{101, "M-4 (40 PDF)"};
static SimpleFuelM4<45> M4_45{102, "M-4 (45 PDF)"};
static SimpleFuelM4<50> M4_50{103, "M-4 (50 PDF)"};
static SimpleFuelM4<55> M4_55{104, "M-4 (55 PDF)"};
static SimpleFuelM4<60> M4_60{105, "M-4 (60 PDF)"};
static SimpleFuelM4<65> M4_65{106, "M-4 (65 PDF)"};
static SimpleFuelM4<70> M4_70{107, "M-4 (70 PDF)"};
static SimpleFuelM4<75> M4_75{108, "M-4 (75 PDF)"};
static SimpleFuelM4<80> M4_80{109, "M-4 (80 PDF)"};
static SimpleFuelM4<85> M4_85{110, "M-4 (85 PDF)"};
static SimpleFuelM4<90> M4_90{111, "M-4 (90 PDF)"};
static SimpleFuelM4<95> M4_95{112, "M-4 (95 PDF)"};
static SimpleFuelM4<100> M4_100{113, "M-4 (100 PDF)"};
static SimpleFuelM3M4<5> M3_M4_05{114, "M-3/M-4 (05 PDF)", &M3_05, &M4_05};
static SimpleFuelM3M4<10> M3_M4_10{115, "M-3/M-4 (10 PDF)", &M3_10, &M4_10};
static SimpleFuelM3M4<15> M3_M4_15{116, "M-3/M-4 (15 PDF)", &M3_15, &M4_15};
static SimpleFuelM3M4<20> M3_M4_20{117, "M-3/M-4 (20 PDF)", &M3_20, &M4_20};
static SimpleFuelM3M4<25> M3_M4_25{118, "M-3/M-4 (25 PDF)", &M3_25, &M4_25};
static SimpleFuelM3M4<30> M3_M4_30{119, "M-3/M-4 (30 PDF)", &M3_30, &M4_30};
static SimpleFuelM3M4<35> M3_M4_35{120, "M-3/M-4 (35 PDF)", &M3_35, &M4_35};
static SimpleFuelM3M4<40> M3_M4_40{121, "M-3/M-4 (40 PDF)", &M3_40, &M4_40};
static SimpleFuelM3M4<45> M3_M4_45{122, "M-3/M-4 (45 PDF)", &M3_45, &M4_45};
static SimpleFuelM3M4<50> M3_M4_50{123, "M-3/M-4 (50 PDF)", &M3_50, &M4_50};
static SimpleFuelM3M4<55> M3_M4_55{124, "M-3/M-4 (55 PDF)", &M3_55, &M4_55};
static SimpleFuelM3M4<60> M3_M4_60{125, "M-3/M-4 (60 PDF)", &M3_60, &M4_60};
static SimpleFuelM3M4<65> M3_M4_65{126, "M-3/M-4 (65 PDF)", &M3_65, &M4_65};
static SimpleFuelM3M4<70> M3_M4_70{127, "M-3/M-4 (70 PDF)", &M3_70, &M4_70};
static SimpleFuelM3M4<75> M3_M4_75{128, "M-3/M-4 (75 PDF)", &M3_75, &M4_75};
static SimpleFuelM3M4<80> M3_M4_80{129, "M-3/M-4 (80 PDF)", &M3_80, &M4_80};
static SimpleFuelM3M4<85> M3_M4_85{130, "M-3/M-4 (85 PDF)", &M3_85, &M4_85};
static SimpleFuelM3M4<90> M3_M4_90{131, "M-3/M-4 (90 PDF)", &M3_90, &M4_90};
static SimpleFuelM3M4<95> M3_M4_95{132, "M-3/M-4 (95 PDF)", &M3_95, &M4_95};
static SimpleFuelM3M4<100> M3_M4_100{133, "M-3/M-4 (100 PDF)", &M3_100, &M4_100};
static SimpleFuelM1<0> M1_00{134, "M-1 (00 PC)"};
static SimpleFuelM2<0> M2_00{135, "M-2 (00 PC)"};
static SimpleFuelM1M2<0> M1_M2_00{136, "M-1/M-2 (00 PC)", &M1_00, &M2_00};
static SimpleFuelM3<0> M3_00{137, "M-3 (00 PDF)"};
static SimpleFuelM4<0> M4_00{138, "M-4 (00 PDF)"};
static SimpleFuelM3M4<0> M3_M4_00{139, "M-3/M-4 (00 PDF)", &M3_00, &M4_00};
static SimpleFuelO1 O1{140, "O-1", &O1_A, &O1_B};
const array<const FuelType*, NUMBER_OF_FUELS> SimpleFuels{
  &NULL_FUEL, &INVALID,  &C1,       &C2,        &C3,       &C4,       &C5,       &C6,
  &C7,        &D1,       &D2,       &O1_A,      &O1_B,     &S1,       &S2,       &S3,
  &D1_D2,     &M1_05,    &M1_10,    &M1_15,     &M1_20,    &M1_25,    &M1_30,    &M1_35,
  &M1_40,     &M1_45,    &M1_50,    &M1_55,     &M1_60,    &M1_65,    &M1_70,    &M1_75,
  &M1_80,     &M1_85,    &M1_90,    &M1_95,     &M2_05,    &M2_10,    &M2_15,    &M2_20,
  &M2_25,     &M2_30,    &M2_35,    &M2_40,     &M2_45,    &M2_50,    &M2_55,    &M2_60,
  &M2_65,     &M2_70,    &M2_75,    &M2_80,     &M2_85,    &M2_90,    &M2_95,    &M1_M2_05,
  &M1_M2_10,  &M1_M2_15, &M1_M2_20, &M1_M2_25,  &M1_M2_30, &M1_M2_35, &M1_M2_40, &M1_M2_45,
  &M1_M2_50,  &M1_M2_55, &M1_M2_60, &M1_M2_65,  &M1_M2_70, &M1_M2_75, &M1_M2_80, &M1_M2_85,
  &M1_M2_90,  &M1_M2_95, &M3_05,    &M3_10,     &M3_15,    &M3_20,    &M3_25,    &M3_30,
  &M3_35,     &M3_40,    &M3_45,    &M3_50,     &M3_55,    &M3_60,    &M3_65,    &M3_70,
  &M3_75,     &M3_80,    &M3_85,    &M3_90,     &M3_95,    &M3_100,   &M4_05,    &M4_10,
  &M4_15,     &M4_20,    &M4_25,    &M4_30,     &M4_35,    &M4_40,    &M4_45,    &M4_50,
  &M4_55,     &M4_60,    &M4_65,    &M4_70,     &M4_75,    &M4_80,    &M4_85,    &M4_90,
  &M4_95,     &M4_100,   &M3_M4_00, &M3_M4_05,  &M3_M4_10, &M3_M4_15, &M3_M4_20, &M3_M4_25,
  &M3_M4_30,  &M3_M4_35, &M3_M4_40, &M3_M4_45,  &M3_M4_50, &M3_M4_55, &M3_M4_60, &M3_M4_65,
  &M3_M4_70,  &M3_M4_75, &M3_M4_80, &M3_M4_85,  &M3_M4_90, &M3_M4_95, &M1_00,    &M2_00,
  &M1_M2_00,  &M3_00,    &M4_00,    &M3_M4_100, &O1,
};
}
#ifdef TEST_FBP
namespace fs::testing
{
#include <iterator>
template <class T = MathSize>
class RangeIterator
{
  friend class iterable;

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = int;
  using value_type = T;
  RangeIterator(
    const value_type start,
    const value_type end,
    const value_type increment,
    const bool inclusive = true
  )
    : start_(start), end_(end), increment_(increment), inclusive_(inclusive)
  {
    logging::verbose(
      "Range is from %f to %f with step %f %s",
      start_,
      end_,
      increment_,
      inclusive_ ? "inclusive" : "exclusive"
    );
  }
  RangeIterator() = default;
  RangeIterator(const RangeIterator& rhs) = default;
  RangeIterator(RangeIterator&& rhs) = default;
  RangeIterator& operator=(const RangeIterator& rhs) = default;
  RangeIterator& operator=(RangeIterator&& rhs) = default;

public:
  inline value_type operator*() const { return start_ + step_ * increment_; }
  inline RangeIterator& operator++()
  {
    logging::check_fatal(
      *(*this) < start_,
      "operator++() %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      (*(*this) - start_),
      step_
    );
    step_++;
    logging::check_fatal(
      *this > end(),
      "operator++() %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      (*(*this) - *end()),
      step_
    );
    return *this;
  }
  inline RangeIterator operator++(int)
  {
    logging::check_fatal(
      *(*this) < start_,
      "operator++(int) %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      step_
    );
    RangeIterator oTmp = *(*this);
    *(*this) += increment_;
    logging::check_fatal(
      *this > end(),
      "operator++(int) %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      step_
    );
    return oTmp;
  }
  inline RangeIterator& operator--()
  {
    logging::check_fatal(
      *this > end(),
      "operator--() %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      step_
    );
    *(*this) -= increment_;
    logging::check_fatal(
      *(*this) < start_,
      "operator--() %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      step_
    );
    return *this;
  }
  inline RangeIterator operator--(int)
  {
    RangeIterator oTmp = *(*this);
    logging::check_fatal(
      *this > end(),
      "operator--(int) %g more than end value %g (%+g) for step %d",
      *(*this),
      *end(),
      step_
    );
    *(*this) -= increment_;
    logging::check_fatal(
      *(*this) < start_,
      "operator--(int) %g less than start value %g (+%g) for step %d",
      *(*this),
      start_,
      step_
    );
    return oTmp;
  }
  inline difference_type operator-(const RangeIterator& rhs) const
  {
    return static_cast<difference_type>(step_ - rhs.step_);
  }
  inline auto operator<=>(const RangeIterator<value_type>& rhs) const = default;
  // inline bool operator==(const RangeIterator& rhs) const { return *(*this) == rhs; }
  // inline bool operator!=(const RangeIterator& rhs) const { return !(*(*this) == rhs); }
  // inline bool operator>(const RangeIterator& rhs) const { return *(*this) < *rhs; }
  // inline bool operator<(const RangeIterator& rhs) const { return *(*this) > *rhs; }
  // inline bool operator>=(const RangeIterator& rhs) const { return *(*this) <= *rhs; }
  // inline bool operator<=(const RangeIterator& rhs) const { return *(*this) >= *rhs; }
  auto begin() { return RangeIterator<value_type>(this, start_); }
  auto end()
  {
    // if inclusive then end is slightly past end value so end is included
    return RangeIterator<value_type>(
      this, end_ + (inclusive_ ? increment_ : static_cast<value_type>(0))
    );
  }

private:
  RangeIterator(const RangeIterator* rhs, const T value)
    : start_(rhs->start_), end_(rhs->end_), increment_(rhs->increment_),
      step_(static_cast<difference_type>((value - start_) / increment_)),
      inclusive_(rhs->inclusive_)
  { }
  value_type start_{};
  value_type end_{};
  value_type increment_{};
  size_t step_{0};
  bool inclusive_{};
};
static_assert(
  std::bidirectional_iterator<RangeIterator<MathSize>>,
  "iterator must be an iterator!"
);
static_assert(std::bidirectional_iterator<RangeIterator<int>>, "iterator must be an iterator!");
auto range(
  const MathSize start,
  const MathSize end,
  const MathSize step,
  const bool inclusive = true
)
{
  std::ignore = inclusive;
  return RangeIterator(start, end, step);
  // // convert into integer range and then back
  // const auto steps = static_cast<int>(floor((end - start) / step));
  // // const auto max_value = start + steps * step;
  // logging::info("Range is from %f to %f with step %f (%d steps)", start, end, step, steps);
  // // +1 to include end
  // auto it = std::views::iota(0, steps + (inclusive ? 1 : 0));
  // return std::views::transform(it, [=](const int v) {
  //   // static int cur_step = 0;
  //   // const auto r = start + v / static_cast<MathSize>(steps);
  //   const auto r = start + v * step;
  //   // printf("%f\n", r);
  //   // FIX: logging doesn't work within this?
  //   logging::check_fatal(r < start, "%f less than start value %f for step %d", r, start, v);
  //   logging::check_fatal(r > end, "%f more than end value %f for step %d", r, end, v);
  //   // logging::check_fatal(cur_step > steps, "%ld more than steps value %ld for step %d",
  //   cur_step,
  //   // steps, v);
  //   // ++cur_step;
  //   const auto diff = r - start;
  //   const auto epsilon = step * 1E-5;
  //   if (1 == v)
  //   {
  //     logging::check_fatal(
  //       abs(diff - step) > epsilon, "%f different than step increment %f for step %d", diff,
  //       step, v
  //     );
  //   }
  //   else if (0 < v)
  //   {
  //     logging::check_fatal(
  //       step > diff, "%f smaller than step increment %f for step %d", diff, step, v
  //     );
  //     const int v0 = static_cast<int>((r - start) / step);
  //     logging::check_equal(
  //       v, v0, std::format("current step for {} with start {} and step {}", r, start,
  //       step).c_str()
  //     );
  //   }
  //   return r;
  // });
}
auto range_int(const int start, const int end, const int step, const bool inclusive = true)
{
  std::ignore = inclusive;
  return RangeIterator<int>(start, end, step);
  // // convert into integer range and then back
  // const auto steps = static_cast<int>(floor((end - start) / step));
  // // const auto max_value = start + steps * step;
  // logging::debug("Range is from %d to %d with step %d (%d steps)", start, end, step, steps);
  // // +1 to include end
  // auto it = std::views::iota(0, steps + (inclusive ? 1 : 0));
  // return std::views::transform(it, [=](const int v) {
  //   // static int cur_step = 0;
  //   // const auto r = start + v / static_cast<MathSize>(steps);
  //   const int r = start + v * step;
  //   // printf("%f\n", r);
  //   logging::check_fatal(r < start, "%g less than start value %g (+%g) for step %d", r, start,
  //   v); logging::check_fatal(r > end, "%g more than end value %g (%+g) for step %d", r, end, v);
  //   // logging::check_fatal(cur_step > steps, "%ld more than steps value %ld for step %d",
  //   cur_step,
  //   // steps, v);
  //   // ++cur_step;
  //   const auto epsilon = step * 1E-5;
  //   const auto diff = r - start;
  //   if (1 == v)
  //   {
  //     logging::check_fatal(
  //       abs(diff - step) > epsilon, "%d different than step increment %d for step %d", diff,
  //       step, v
  //     );
  //   }
  //   else if (0 < v)
  //   {
  //     logging::check_fatal(
  //       step > diff, "%d smaller than step increment %d for step %d", diff, step, v
  //     );
  //     const int v0 = static_cast<int>((r - start) / step);
  //     logging::check_equal(
  //       v, v0, std::format("current step for {} with start {} and step {}", r, start,
  //       step).c_str()
  //     );
  //   }
  //   return r;
  // });
}
auto check_range(
  const char* name_fct,
  const char* name_param,
  const auto fct_a,
  const auto fct_b,
  const MathSize epsilon,
  const MathSize start,
  const MathSize end,
  const MathSize step,
  const bool inclusive = true
)
{
  logging::debug("Checking %s", name_fct);
  for (auto v : range(start, end, step, inclusive))
  {
    const auto msg = std::format("{} ({} = {})", name_fct, name_param, v);
    logging::check_tolerance(epsilon, fct_a(v), fct_b(v), msg.c_str());
    logging::verbose(msg.c_str());
  }
}
// FIX: this was used to compare to the old template version, but doesn't work now
//      left for reference for now so idea could be used for more tests
using fs::FuelBase;
using fs::FuelType;
// check %, so 1 decimal is fine
static constexpr MathSize EPSILON = 1e-1f;
auto check_equal(const auto& lhs, const auto& rhs, const char* name)
{
  logging::check_equal_verbose(logging::LOG_DEBUG, lhs, rhs, name);
}
template <class TypeA, class TypeB>
int compare_fuel_valid(
  const string name,
  // const simplefbp::SimpleFuelBase<BulkDensity, InorganicPercent, DuffDepth>& a,
  // const fs::FuelBase<BulkDensity, InorganicPercent, DuffDepth>& b
  const TypeA& a,
  const TypeB& b
)
{
  logging::info("Checking %s", name.c_str());
  //
  // FuelType
  //
  const auto a0 = a.isValid();
  const auto b0 = b.isValid();
  check_equal(a0, b0, "isValid");
  check_equal(a.name(), b.name(), "name");
  check_equal(a.code(), b.code(), "code");
  return 0;
}
template <class TypeA, class TypeB>
int compare_fuel_basic(
  const string name,
  // const simplefbp::SimpleFuelBase<BulkDensity, InorganicPercent, DuffDepth>& a,
  // const fs::FuelBase<BulkDensity, InorganicPercent, DuffDepth>& b
  const TypeA& a,
  const TypeB& b
)
{
  if (const auto cmp = compare_fuel_valid(name, a, b); 0 != cmp)
  {
    return cmp;
  }
  //
  // FuelType
  //
  // check_equal(a.isValid(), b.isValid(), "isValid");
  check_equal(FuelType::safeCode(&a), FuelType::safeCode(&b), "safeCode");
  check_equal(FuelType::safeName(&a), FuelType::safeName(&b), "safeName");
  // static constexpr MathSize criticalRos(const MathSize sfc, const MathSize csi)
  // static constexpr bool isCrown(const MathSize csi, const MathSize sfi)
  check_equal(a.cfl(), b.cfl(), "cfl");
  check_equal(a.canCrown(), b.canCrown(), "canCrown");
  // MathSize grass_curing(const int, const FwiWeather&) const
  check_equal(a.cbh(), b.cbh(), "cbh");
  // MathSize crownFractionBurned(MathSize rss, MathSize rso) const noexcept
  check_range(
    "probabilityPeat()",
    "mc_fraction",
    [&](const auto& v) { return a.probabilityPeat(v); },
    [&](const auto& v) { return b.probabilityPeat(v); },
    EPSILON,
    -1,
    3,
    0.0001
  );
  // ThresholdSize survivalProbability(const FwiWeather& wx) const noexcept
  check_range(
    "buiEffect()",
    "bui",
    [&](const auto& v) { return a.buiEffect(v); },
    [&](const auto& v) { return b.buiEffect(v); },
    EPSILON,
    -1,
    300,
    0.01
  );
  check_range(
    "crownConsumption()",
    "cfb",
    [&](const auto& v) { return a.crownConsumption(v); },
    [&](const auto& v) { return b.crownConsumption(v); },
    EPSILON,
    0,
    100,
    0.01
  );
  // MathSize calculateRos(int nd, const FwiWeather& wx, MathSize isi) const
  // need to check breakpoints
  // - BUI 80 (D2)
  // - DC 500 (O1)
  // - nd for different latitudes
  //   - elevation 0
  // FIX: use some weird increments to do less but not always have __0.0
  size_t cur{0};
  static constexpr size_t CHECK_EVERY_NTH_TEST{1'000'000'000};
  static constexpr MathSize BOUNDS_CANADA_LAT_MIN = 41;
  static constexpr MathSize BOUNDS_CANADA_LAT_MAX = 84;
  static constexpr MathSize BOUNDS_CANADA_LON_MIN = -141;
  static constexpr MathSize BOUNDS_CANADA_LON_MAX = -52;
  // FIX: use some weird increments to do less but not always have __.0
  static constexpr MathSize DEGREE_INCREMENT = 0.3;
  static constexpr MathSize ELEVATION_EARTH_MIN = -418;
  static constexpr MathSize ELEVATION_EARTH_MAX = 8848;
  static constexpr MathSize ELEVATION_INCREMENT = 1000;
  for (auto bui : range(0.0, 300.0, 7.0))
  {
    logging::verbose("bui %f", bui);
    for (auto dc : range(0.0, 2000.0, 7.0))
    {
      logging::verbose("dc %f", dc);
      const FwiWeather wx{
        Weather::Zero(), Ffmc::Zero(), Dmc::Zero(), Dc{dc}, Isi::Zero(), Bui{bui}, Fwi::Zero()
      };
      for (int jd : range_int(0, 366, 1))
      {
        logging::verbose("jd %d", jd);
        // for (auto latitude : range(-90.0, 90.0, DEGREE_INCREMENT))
        for (auto latitude : range(BOUNDS_CANADA_LAT_MIN, BOUNDS_CANADA_LAT_MAX, DEGREE_INCREMENT))
        {
          // for (auto longitude : range(-180.0, 180.0, DEGREE_INCREMENT))
          for (auto longitude :
               range(BOUNDS_CANADA_LON_MIN, BOUNDS_CANADA_LON_MAX, DEGREE_INCREMENT))
          {
            for (auto elevation :
                 range(ELEVATION_EARTH_MIN, ELEVATION_EARTH_MAX, ELEVATION_INCREMENT))
            {
              cur %= CHECK_EVERY_NTH_TEST;
              if (0 == cur)
              {
                const Point pt{latitude, longitude};
                const auto nd = calculate_nd_for_point(jd, elevation, pt);
                const auto msg = std::format(
                  "calculateRos(jd={}, bui={}, dc={}, elevation={}, latitude={}, longitude={})",
                  jd,
                  bui,
                  dc,
                  elevation,
                  latitude,
                  longitude
                );
                check_range(
                  msg.c_str(),
                  "isi",
                  [&](const auto& v) { return a.calculateRos(nd, wx, v); },
                  [&](const auto& v) { return b.calculateRos(nd, wx, v); },
                  EPSILON,
                  0,
                  250,
                  0.1
                );
              }
              ++cur;
            }
          }
        }
      }
    }
  }
  // MathSize calculateIsf(const SpreadInfo& spread, MathSize isi)
  // MathSize surfaceFuelConsumption(const SpreadInfo& spread) const
  check_range(
    "lengthToBreadth()",
    "ws",
    [&](const auto& v) { return a.lengthToBreadth(v); },
    [&](const auto& v) { return b.lengthToBreadth(v); },
    EPSILON,
    0,
    200,
    0.01
  );
  // MathSize finalRos(const SpreadInfo& spread, MathSize isi, MathSize cfb, MathSize rss) const
  // MathSize criticalSurfaceIntensity(const SpreadInfo& spread) const
  // check_equal(a.name(), b.name(), "name");
  // check_equal(a.code(), b.code(), "code");
  return 0;
}
template <class TypeA, class TypeB>
int compare_fuel(
  const string name,
  // const simplefbp::SimpleFuelBase<BulkDensity, InorganicPercent, DuffDepth>& a,
  // const fs::FuelBase<BulkDensity, InorganicPercent, DuffDepth>& b
  const TypeA& a,
  const TypeB& b
)
{
  if (const auto cmp = compare_fuel_basic(name, a, b); 0 != cmp)
  {
    return cmp;
  }
  //
  // FuelBase
  //
  check_equal(a.bulkDensity(), b.bulkDensity(), "bulkDensity");
  check_equal(a.inorganicPercent(), b.inorganicPercent(), "inorganicPercent");
  check_equal(a.duffDepth(), b.duffDepth(), "duffDepth");
  testing::compare_duff("duffDmcType", *a.duffDmcType(), *b.duffDmcType(), logging::LOG_DEBUG);
  testing::compare_duff("duffFfmcType", *a.duffFfmcType(), *b.duffFfmcType(), logging::LOG_DEBUG);
  check_equal(a.ffmcRatio(), b.ffmcRatio(), "ffmcRatio");
  check_equal(a.dmcRatio(), b.dmcRatio(), "dmcRatio");
  //
  // StandardFuel
  //
  check_range(
    "rosBasic()",
    "isi",
    [&](const auto& v) { return a.rosBasic(v); },
    [&](const auto& v) { return b.rosBasic(v); },
    EPSILON,
    0,
    250,
    0.01
  );
  // MathSize limitIsf(const MathSize mu, const MathSize rsf) const noexcept
  check_equal(a.bui0(), b.bui0(), "bui0");
  check_equal(a.a(), b.a(), "a");
  check_equal(a.negB(), b.negB(), "negB");
  check_equal(a.c(), b.c(), "c");
  // static constexpr MathSize crownRateOfSpread(const MathSize isi, const MathSize fmc) noexcept
  check_equal(a.logQ(), b.logQ(), "logQ");
  return 0;
}
int test_fbp(const int argc, const char* const argv[])
{
  std::ignore = argc;
  std::ignore = argv;
  logging::info("Testing FBP");
  // for (size_t i = 0; i < FuelLookup::Fuels.size(); ++i)
  // {
  //   auto& a = *simplefbp::SimpleFuels[i];
  //   auto& b = *dynamic_cast<fs::FuelBase*>(FuelLookup::Fuels[i]);
  //   compare(a.name(), a, b);
  //   // compare("", *simplefbp::SimpleFuels[i], *FuelLookup::Fuels[i]);
  // }
  auto i = 0;
  compare_fuel_valid("Non-fuel", simplefbp::NULL_FUEL, *FuelLookup::Fuels[i++]);
  compare_fuel_valid("Invalid", simplefbp::INVALID, *FuelLookup::Fuels[i++]);
  compare_fuel("C1", simplefbp::C1, *dynamic_cast<const fs::FuelC1*>(FuelLookup::Fuels[i++]));
  compare_fuel("C2", simplefbp::C2, *dynamic_cast<const fs::FuelC2*>(FuelLookup::Fuels[i++]));
  compare_fuel("C3", simplefbp::C3, *dynamic_cast<const fs::FuelC3*>(FuelLookup::Fuels[i++]));
  compare_fuel("C4", simplefbp::C4, *dynamic_cast<const fs::FuelC4*>(FuelLookup::Fuels[i++]));
  compare_fuel("C5", simplefbp::C5, *dynamic_cast<const fs::FuelC5*>(FuelLookup::Fuels[i++]));
  compare_fuel("C6", simplefbp::C6, *dynamic_cast<const fs::FuelC6*>(FuelLookup::Fuels[i++]));
  compare_fuel("C7", simplefbp::C7, *dynamic_cast<const fs::FuelC7*>(FuelLookup::Fuels[i++]));
  compare_fuel("D1", simplefbp::D1, *dynamic_cast<const fs::FuelD1*>(FuelLookup::Fuels[i++]));
  compare_fuel("D2", simplefbp::D2, *dynamic_cast<const fs::FuelD2*>(FuelLookup::Fuels[i++]));
  compare_fuel("O1_A", simplefbp::O1_A, *dynamic_cast<const fs::FuelO1A*>(FuelLookup::Fuels[i++]));
  compare_fuel("O1_B", simplefbp::O1_B, *dynamic_cast<const fs::FuelO1B*>(FuelLookup::Fuels[i++]));
  compare_fuel("S1", simplefbp::S1, *dynamic_cast<const fs::FuelS1*>(FuelLookup::Fuels[i++]));
  compare_fuel("S2", simplefbp::S2, *dynamic_cast<const fs::FuelS2*>(FuelLookup::Fuels[i++]));
  compare_fuel("S3", simplefbp::S3, *dynamic_cast<const fs::FuelS3*>(FuelLookup::Fuels[i++]));
  compare_fuel_basic(
    "D1_D2", simplefbp::D1_D2, *dynamic_cast<const fs::FuelD1D2*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_05", simplefbp::M1_05, *dynamic_cast<const fs::FuelM1<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_10", simplefbp::M1_10, *dynamic_cast<const fs::FuelM1<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_15", simplefbp::M1_15, *dynamic_cast<const fs::FuelM1<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_20", simplefbp::M1_20, *dynamic_cast<const fs::FuelM1<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_25", simplefbp::M1_25, *dynamic_cast<const fs::FuelM1<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_30", simplefbp::M1_30, *dynamic_cast<const fs::FuelM1<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_35", simplefbp::M1_35, *dynamic_cast<const fs::FuelM1<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_40", simplefbp::M1_40, *dynamic_cast<const fs::FuelM1<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_45", simplefbp::M1_45, *dynamic_cast<const fs::FuelM1<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_50", simplefbp::M1_50, *dynamic_cast<const fs::FuelM1<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_55", simplefbp::M1_55, *dynamic_cast<const fs::FuelM1<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_60", simplefbp::M1_60, *dynamic_cast<const fs::FuelM1<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_65", simplefbp::M1_65, *dynamic_cast<const fs::FuelM1<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_70", simplefbp::M1_70, *dynamic_cast<const fs::FuelM1<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_75", simplefbp::M1_75, *dynamic_cast<const fs::FuelM1<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_80", simplefbp::M1_80, *dynamic_cast<const fs::FuelM1<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_85", simplefbp::M1_85, *dynamic_cast<const fs::FuelM1<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_90", simplefbp::M1_90, *dynamic_cast<const fs::FuelM1<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_95", simplefbp::M1_95, *dynamic_cast<const fs::FuelM1<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_05", simplefbp::M2_05, *dynamic_cast<const fs::FuelM2<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_10", simplefbp::M2_10, *dynamic_cast<const fs::FuelM2<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_15", simplefbp::M2_15, *dynamic_cast<const fs::FuelM2<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_20", simplefbp::M2_20, *dynamic_cast<const fs::FuelM2<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_25", simplefbp::M2_25, *dynamic_cast<const fs::FuelM2<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_30", simplefbp::M2_30, *dynamic_cast<const fs::FuelM2<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_35", simplefbp::M2_35, *dynamic_cast<const fs::FuelM2<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_40", simplefbp::M2_40, *dynamic_cast<const fs::FuelM2<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_45", simplefbp::M2_45, *dynamic_cast<const fs::FuelM2<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_50", simplefbp::M2_50, *dynamic_cast<const fs::FuelM2<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_55", simplefbp::M2_55, *dynamic_cast<const fs::FuelM2<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_60", simplefbp::M2_60, *dynamic_cast<const fs::FuelM2<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_65", simplefbp::M2_65, *dynamic_cast<const fs::FuelM2<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_70", simplefbp::M2_70, *dynamic_cast<const fs::FuelM2<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_75", simplefbp::M2_75, *dynamic_cast<const fs::FuelM2<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_80", simplefbp::M2_80, *dynamic_cast<const fs::FuelM2<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_85", simplefbp::M2_85, *dynamic_cast<const fs::FuelM2<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_90", simplefbp::M2_90, *dynamic_cast<const fs::FuelM2<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_95", simplefbp::M2_95, *dynamic_cast<const fs::FuelM2<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_05", simplefbp::M1_M2_05, *dynamic_cast<const fs::FuelM1M2<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_10", simplefbp::M1_M2_10, *dynamic_cast<const fs::FuelM1M2<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_15", simplefbp::M1_M2_15, *dynamic_cast<const fs::FuelM1M2<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_20", simplefbp::M1_M2_20, *dynamic_cast<const fs::FuelM1M2<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_25", simplefbp::M1_M2_25, *dynamic_cast<const fs::FuelM1M2<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_30", simplefbp::M1_M2_30, *dynamic_cast<const fs::FuelM1M2<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_35", simplefbp::M1_M2_35, *dynamic_cast<const fs::FuelM1M2<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_40", simplefbp::M1_M2_40, *dynamic_cast<const fs::FuelM1M2<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_45", simplefbp::M1_M2_45, *dynamic_cast<const fs::FuelM1M2<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_50", simplefbp::M1_M2_50, *dynamic_cast<const fs::FuelM1M2<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_55", simplefbp::M1_M2_55, *dynamic_cast<const fs::FuelM1M2<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_60", simplefbp::M1_M2_60, *dynamic_cast<const fs::FuelM1M2<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_65", simplefbp::M1_M2_65, *dynamic_cast<const fs::FuelM1M2<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_70", simplefbp::M1_M2_70, *dynamic_cast<const fs::FuelM1M2<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_75", simplefbp::M1_M2_75, *dynamic_cast<const fs::FuelM1M2<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_80", simplefbp::M1_M2_80, *dynamic_cast<const fs::FuelM1M2<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_85", simplefbp::M1_M2_85, *dynamic_cast<const fs::FuelM1M2<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_90", simplefbp::M1_M2_90, *dynamic_cast<const fs::FuelM1M2<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_95", simplefbp::M1_M2_95, *dynamic_cast<const fs::FuelM1M2<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_05", simplefbp::M3_05, *dynamic_cast<const fs::FuelM3<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_10", simplefbp::M3_10, *dynamic_cast<const fs::FuelM3<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_15", simplefbp::M3_15, *dynamic_cast<const fs::FuelM3<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_20", simplefbp::M3_20, *dynamic_cast<const fs::FuelM3<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_25", simplefbp::M3_25, *dynamic_cast<const fs::FuelM3<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_30", simplefbp::M3_30, *dynamic_cast<const fs::FuelM3<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_35", simplefbp::M3_35, *dynamic_cast<const fs::FuelM3<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_40", simplefbp::M3_40, *dynamic_cast<const fs::FuelM3<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_45", simplefbp::M3_45, *dynamic_cast<const fs::FuelM3<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_50", simplefbp::M3_50, *dynamic_cast<const fs::FuelM3<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_55", simplefbp::M3_55, *dynamic_cast<const fs::FuelM3<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_60", simplefbp::M3_60, *dynamic_cast<const fs::FuelM3<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_65", simplefbp::M3_65, *dynamic_cast<const fs::FuelM3<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_70", simplefbp::M3_70, *dynamic_cast<const fs::FuelM3<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_75", simplefbp::M3_75, *dynamic_cast<const fs::FuelM3<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_80", simplefbp::M3_80, *dynamic_cast<const fs::FuelM3<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_85", simplefbp::M3_85, *dynamic_cast<const fs::FuelM3<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_90", simplefbp::M3_90, *dynamic_cast<const fs::FuelM3<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_95", simplefbp::M3_95, *dynamic_cast<const fs::FuelM3<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_100", simplefbp::M3_100, *dynamic_cast<const fs::FuelM3<100>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_05", simplefbp::M4_05, *dynamic_cast<const fs::FuelM4<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_10", simplefbp::M4_10, *dynamic_cast<const fs::FuelM4<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_15", simplefbp::M4_15, *dynamic_cast<const fs::FuelM4<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_20", simplefbp::M4_20, *dynamic_cast<const fs::FuelM4<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_25", simplefbp::M4_25, *dynamic_cast<const fs::FuelM4<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_30", simplefbp::M4_30, *dynamic_cast<const fs::FuelM4<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_35", simplefbp::M4_35, *dynamic_cast<const fs::FuelM4<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_40", simplefbp::M4_40, *dynamic_cast<const fs::FuelM4<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_45", simplefbp::M4_45, *dynamic_cast<const fs::FuelM4<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_50", simplefbp::M4_50, *dynamic_cast<const fs::FuelM4<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_55", simplefbp::M4_55, *dynamic_cast<const fs::FuelM4<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_60", simplefbp::M4_60, *dynamic_cast<const fs::FuelM4<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_65", simplefbp::M4_65, *dynamic_cast<const fs::FuelM4<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_70", simplefbp::M4_70, *dynamic_cast<const fs::FuelM4<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_75", simplefbp::M4_75, *dynamic_cast<const fs::FuelM4<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_80", simplefbp::M4_80, *dynamic_cast<const fs::FuelM4<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_85", simplefbp::M4_85, *dynamic_cast<const fs::FuelM4<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_90", simplefbp::M4_90, *dynamic_cast<const fs::FuelM4<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_95", simplefbp::M4_95, *dynamic_cast<const fs::FuelM4<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_100", simplefbp::M4_100, *dynamic_cast<const fs::FuelM4<100>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_00", simplefbp::M3_M4_00, *dynamic_cast<const fs::FuelM3M4<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_05", simplefbp::M3_M4_05, *dynamic_cast<const fs::FuelM3M4<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_10", simplefbp::M3_M4_10, *dynamic_cast<const fs::FuelM3M4<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_15", simplefbp::M3_M4_15, *dynamic_cast<const fs::FuelM3M4<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_20", simplefbp::M3_M4_20, *dynamic_cast<const fs::FuelM3M4<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_25", simplefbp::M3_M4_25, *dynamic_cast<const fs::FuelM3M4<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_30", simplefbp::M3_M4_30, *dynamic_cast<const fs::FuelM3M4<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_35", simplefbp::M3_M4_35, *dynamic_cast<const fs::FuelM3M4<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_40", simplefbp::M3_M4_40, *dynamic_cast<const fs::FuelM3M4<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_45", simplefbp::M3_M4_45, *dynamic_cast<const fs::FuelM3M4<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_50", simplefbp::M3_M4_50, *dynamic_cast<const fs::FuelM3M4<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_55", simplefbp::M3_M4_55, *dynamic_cast<const fs::FuelM3M4<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_60", simplefbp::M3_M4_60, *dynamic_cast<const fs::FuelM3M4<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_65", simplefbp::M3_M4_65, *dynamic_cast<const fs::FuelM3M4<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_70", simplefbp::M3_M4_70, *dynamic_cast<const fs::FuelM3M4<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_75", simplefbp::M3_M4_75, *dynamic_cast<const fs::FuelM3M4<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_80", simplefbp::M3_M4_80, *dynamic_cast<const fs::FuelM3M4<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_85", simplefbp::M3_M4_85, *dynamic_cast<const fs::FuelM3M4<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_90", simplefbp::M3_M4_90, *dynamic_cast<const fs::FuelM3M4<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_95", simplefbp::M3_M4_95, *dynamic_cast<const fs::FuelM3M4<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_00", simplefbp::M1_00, *dynamic_cast<const fs::FuelM1<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_00", simplefbp::M2_00, *dynamic_cast<const fs::FuelM2<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M1_M2_00", simplefbp::M1_M2_00, *dynamic_cast<const fs::FuelM1M2<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_00", simplefbp::M3_00, *dynamic_cast<const fs::FuelM3<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_00", simplefbp::M4_00, *dynamic_cast<const fs::FuelM4<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic(
    "M3_M4_100",
    simplefbp::M3_M4_100,
    *dynamic_cast<const fs::FuelM3M4<100>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_basic("O1", simplefbp::O1, *dynamic_cast<const fs::FuelO1*>(FuelLookup::Fuels[i++]));
  check_equal(NUMBER_OF_FUELS, i, "Number of fuels");
  return 0;
}
}
#endif
