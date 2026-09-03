/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "fs/ArgumentParser.h"
#include "fs/FuelType.h"
#include "fs/RangeIterator.h"
#include "fs/SimpleFBP.h"
#include "fs/SimpleFuelLookup.h"
#include "fs/SimpleFuelType.h"
#include "test_fbp/FBP45.h"
#include "test_fbp/FuelOldLookup.h"
namespace fs::testing
{
// FIX: this was used to compare to the old template version, but doesn't work now
//      left for reference for now so idea could be used for more tests
using fs::fuel::FuelType;
using fs::fuelold::FuelOldLookup;
using namespace fs::fuelold;
using fs::simplefbp::SimpleFuelLookup;
using fs::simplefbp::SimpleFuelType;
using fs::simplefbp::SimpleFuelVariable;
using fs::simplefbp::SimpleStandardFuel;
// check %, so 1 decimal is fine
static constexpr auto EPSILON = static_cast<MathSize>(1e-1);
auto check_equal(const auto& lhs, const auto& rhs, const char* name)
{
  logging::check_equal_verbose(logging::level::debug, lhs, rhs, name);
}
template <class TypeA, class TypeB>
int compare_fuel_valid(const string name, const TypeA& a, const TypeB& b, const char* msg = "")
{
  logging::info("Checking {:s}: {:s}", name, msg);
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
// use vectors so FuelCompareOptions can assign any of these directly
static const auto BUI_RANGE_DEFAULTS = range(0.0, 300.0, 7.0);
static const auto DC_RANGE_DEFAULTS = range(0.0, 1000.0, 7.0);
static const vector<MathSize> DC_VALUES_GRASS{0, 10, 50, 100, 400, 499, 500, 501, 1000};
static const auto RANGE_MC_FRACTION = range(-1, 3, 0.0001);
static const auto RANGE_WIND_SPEED = range(0, 200, 0.01);
static const auto RANGE_BUI_EFFECT = range(-1, 300, 0.01);
static const auto RANGE_CFB = range(0, 100, 0.01);
static const auto RANGE_ISI = range(0, 250, 0.1);
struct FuelCompareOptions
{
  // HACK: can't figure out how to refer to a range so just use vectors
  // single values for defaults
  const vector<int> nd_values{80};
  const vector<MathSize> bui_values{60};
  const vector<MathSize> dc_values{200};
};
static const FuelCompareOptions FUEL_COMPARE_DEFAULT{};
static const FuelCompareOptions FUEL_COMPARE_DECIDUOUS{.bui_values = BUI_RANGE_DEFAULTS};
int compare_spread(
  const string name,
  const FuelType* a,
  const FuelType* b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  static const DurationSize TIME{INVALID_TIME};
  // HACK: 0.0 is causing offsets to be generated in grass
  static const MathSize MIN_ROS{1E-6};
  static const MathSize CELL_SIZE{100.0};
  static const vector<SlopeSize> slopes{0, 15, 30};
  static const vector<AspectSize> aspects{0, 15, 25, 35, 45, 55};
  // duration is in minutes
  auto show_offset = [=](const ROSOffset& o) {
    const auto intensity{o.intensity};
    const auto ros{o.ros};
    const auto direction{o.raz};
    // offsets are in fraction of a cell per minute
    const auto offset{o.offset};
    cout << std::format(
      " ({:d} kW/m; {:0.6f} m/min @{:03d} == ({:g}, {:g}))\n",
      intensity,
      ros,
      direction.asDegreesSize(),
      offset.x * CELL_SIZE,
      offset.y * CELL_SIZE
    );
  };
  static const auto FFMC_RANGE = range(0.0, 101.0, 1.0);
  static const auto DMC_SMALL_RANGE = range(0.0, 200.0, 47.0);
  static const auto DMC_RANGE = range(0.0, 200.0, 3.0);
  size_t count_comparisons{0};
  logging::debug(
    "compare_spread({:s}, {:s}, {:s})", name.c_str(), FuelType::safeName(a), FuelType::safeName(b)
  );
  // HACK: use less options for things with nd values (just grass?)
  const auto dmc_values = options.nd_values.size() > 1 ? DMC_SMALL_RANGE : DMC_RANGE;
  for (auto nd : options.nd_values)
  {
    logging::extensive("nd {:d}", nd);
    for (auto ffmc : FFMC_RANGE)
    {
      logging::extensive("ffmc {:f}", ffmc);
      for (auto dmc : dmc_values)
      {
        logging::extensive("dmc {:f}", dmc);
        // for (auto bui : options.bui_values)
        {
          for (auto dc : options.dc_values)
          // for (auto dc : range(0.0, std::ranges::max(options.dc_values), 17.0))
          {
            logging::extensive("dc {:f}", dc);
            const FwiWeather weather{Weather::Invalid(), Ffmc{ffmc}, Dmc{dmc}, Dc{dc}};
            for (auto slope : slopes)
            {
              logging::extensive("slope {:d}", slope);
              for (auto aspect : aspects)
              {
                ++count_comparisons;
                logging::extensive("aspect {:d}", aspect);
                // HACK: this constructor ignores fuel part of this
                const auto key = Cell::key(Cell::hashCell(slope, aspect, 0));
                const SpreadInfo spread_a{a, TIME, MIN_ROS, CELL_SIZE, key, nd, &weather, &weather};
                const SpreadInfo spread_b{b, TIME, MIN_ROS, CELL_SIZE, key, nd, &weather, &weather};
                const auto offsets_a = spread_a.offsets();
                const auto offsets_b = spread_b.offsets();
                const auto head_ros = spread_a.headRos();
                static constexpr MathSize ROS_MINIMAL{1.0};
                logging::verbose(
                  "compare_spread() [{:d}] {:s}spreading for ffmc:{:f}; dmc:{:f}; dc:{:f}; nd:{:d}; slope:{:d}; aspect: {:d}",
                  count_comparisons,
                  spread_a.isNotSpreading() ? "not "
                  : head_ros < ROS_MINIMAL  ? "minimal "
                                            : "",
                  ffmc,
                  dmc,
                  dc,
                  nd,
                  slope,
                  aspect
                );
                const auto show_offsets =
                  logging::should_log(logging::level::verbose) && head_ros >= ROS_MINIMAL;
                if (offsets_a.size() != offsets_b.size())
                {
                  logging::error(
                    "compare_spread() size failed for name: {:s}; ffmc:{:f}; dmc:{:f}; dc:{:f}; nd:{:d}; slope:{:d}; aspect: {:d}",
                    name.c_str(),
                    ffmc,
                    dmc,
                    dc,
                    nd,
                    slope,
                    aspect
                  );
                  if (offsets_a.size() < offsets_b.size())
                  {
                    logging::error("compare_spread() size == -1");
                    return -1;
                  }
                  if (offsets_a.size() > offsets_b.size())
                  {
                    logging::error("compare_spread() size == 1");
                    return 1;
                  }
                }
                if (show_offsets)
                {
                  cout << "Offsets are: [";
                }
                for (size_t i = 0; i < offsets_a.size(); ++i)
                {
                  const auto pt_a{offsets_a.at(i)};
                  const auto pt_b{offsets_b.at(i)};
                  if (show_offsets)
                  {
                    show_offset(pt_a);
                  }
                  if (auto cmp_pt = pt_a <=> pt_b; 0 != cmp_pt)
                  {
                    if (!show_offsets)
                    {
                      show_offset(pt_a);
                    }
                    cout << " != ";
                    show_offset(pt_b);
                    cout << "\n";
                    logging::error(
                      "compare_spread() pt failed for name: {:s}; ffmc:{:f}; dmc:{:f}; dc:{:f}; nd:{:d}; slope:{:d}; aspect: {:d}",
                      name.c_str(),
                      ffmc,
                      dmc,
                      dc,
                      nd,
                      slope,
                      aspect
                    );
                    if (std::weak_ordering::less == cmp_pt)
                    {
                      logging::error("compare_spread() pt == -1");
                      return -1;
                    }
                    if (std::weak_ordering::greater == cmp_pt)
                    {
                      logging::error("compare_spread() pt == 1");
                      return 1;
                    }
                  }
                }
                if (show_offsets)
                {
                  cout << "]\n";
                }
              }
            }
          }
        }
      }
    }
  }
  logging::debug("compare_spread() == 0 with {:d} comparisons", count_comparisons);
  return 0;
}
template <class TypeA, class TypeB>
int compare_fuel_basic(
  const string name,
  const TypeA& a,
  const TypeB& b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  const auto n_nd = options.nd_values.size();
  const auto n_bui = options.bui_values.size();
  const auto n_dc = options.dc_values.size();
  const auto n_isi = RANGE_ISI.size();
  static const auto n_mc_fraction = RANGE_MC_FRACTION.size();
  static const auto n_wind_speed = RANGE_WIND_SPEED.size();
  static const auto n_bui_effect = RANGE_BUI_EFFECT.size();
  static const auto n_cfb = RANGE_CFB.size();
  const auto n_loop = n_nd * n_bui * n_dc * n_isi;
  const auto n_total = n_loop + n_mc_fraction + n_wind_speed + n_bui_effect + n_cfb;
  const auto msg = std::format(
    "(({} nds X {} buis X {} dcs X {} isis) + {} mc_fractions + {} wind_speeds' + {} bui_effects + {} cfbs = {} combinations",
    n_nd,
    n_bui,
    n_dc,
    n_isi,
    n_mc_fraction,
    n_wind_speed,
    n_bui_effect,
    n_cfb,
    n_total
  );
  if (const auto cmp = compare_fuel_valid(name, a, b, msg.c_str()); 0 != cmp)
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
    RANGE_MC_FRACTION
  );
  // ThresholdSize survivalProbability(const FwiWeather& wx) const noexcept
  check_range(
    "buiEffect()",
    "bui",
    [&](const auto& v) { return a.buiEffect(v); },
    [&](const auto& v) { return b.buiEffect(v); },
    EPSILON,
    RANGE_BUI_EFFECT
  );
  check_range(
    "crownConsumption()",
    "cfb",
    [&](const auto& v) { return a.crownConsumption(v); },
    [&](const auto& v) { return b.crownConsumption(v); },
    EPSILON,
    RANGE_CFB
  );
  // MathSize calculateRos(int nd, const FwiWeather& wx, MathSize isi) const
  // need to check breakpoints
  // - BUI 80 (D2)
  // - DC 500 (O1)
  // FIX: use some weird increments to do less but not always have __0.0
  for (auto nd : options.nd_values)
  {
    for (auto bui : options.bui_values)
    {
      // logging::verbose("bui {:f}", bui);
      // for (auto dc : range(0.0, 2000.0, 7.0))
      for (auto dc : options.dc_values)
      {
        // logging::verbose("dc {:f}", dc);
        const FwiWeather wx{
          Weather::Zero(), Ffmc::Zero(), Dmc::Zero(), Dc{dc}, Isi::Zero(), Bui{bui}, Fwi::Zero()
        };
        const string msg = logging::should_log(logging::level::verbose)
                           ? std::format("calculateRos(nd={}, bui={}, dc={})", nd, bui, dc)
                           : "calculateRos()";
        check_range(
          msg.c_str(),
          "isi",
          [&](const auto& v) { return a.calculateRos(nd, wx, v); },
          [&](const auto& v) { return b.calculateRos(nd, wx, v); },
          EPSILON,
          RANGE_ISI
        );
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
    RANGE_WIND_SPEED
  );
  // MathSize finalRos(const SpreadInfo& spread, MathSize isi, MathSize cfb, MathSize rss) const
  // MathSize criticalSurfaceIntensity(const SpreadInfo& spread) const
  // check_equal(a.name(), b.name(), "name");
  // check_equal(a.code(), b.code(), "code");
  // return 0;
  return compare_spread(
    name, static_cast<const FuelType*>(&a), static_cast<const FuelType*>(&b), options
  );
}
template <class TypeA, class TypeB>
int compare_fuel(
  const string name,
  const TypeA& a,
  const TypeB& b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  if (const auto cmp = compare_fuel_basic(name, a, b, options); 0 != cmp)
  {
    return cmp;
  }
  //
  // FuelBase
  //
  check_equal(a.bulkDensity(), b.bulkDensity(), "bulkDensity");
  check_equal(a.inorganicPercent(), b.inorganicPercent(), "inorganicPercent");
  check_equal(a.duffDepth(), b.duffDepth(), "duffDepth");
  testing::compare_duff("duffDmcType", *a.duffDmcType(), *b.duffDmcType(), logging::level::debug);
  testing::compare_duff(
    "duffFfmcType", *a.duffFfmcType(), *b.duffFfmcType(), logging::level::debug
  );
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
    RANGE_ISI
  );
  // MathSize limitIsf(const MathSize mu, const MathSize rsf) const noexcept
  check_equal(a.bui0(), b.bui0(), "bui0");
  check_equal(a.a(), b.a(), "a");
  check_equal(a.negB(), b.negB(), "negB");
  check_equal(a.c(), b.c(), "c");
  // static constexpr MathSize crownRateOfSpread(const MathSize isi, const MathSize fmc)
  // noexcept
  check_equal(a.logQ(), b.logQ(), "logQ");
  return 0;
}
template <class TypeA, class TypeB>
int compare_fuel_variable(
  const string name,
  const TypeA& a,
  const TypeB& b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  assert(a.summer() != a.spring());
  // FIX: calling functions of FuelVariable should throw, but don't bother checking that
  if (const auto cmp = compare_fuel_basic(name, *a.summer(), *b.summer(), options); 0 != cmp)
  {
    return cmp;
  }
  return compare_fuel_basic(name, *a.spring(), *b.spring(), options);
}
// static constexpr FuelCompareOptions FUEL_COMPARE_DECIDUOUS{.dc_values = DC_DEFAULT_SINGLE};
vector<int> find_nd_values()
{
  using namespace fs::fuelold;
  // CHECK: FIX: how are nd values 400+?
  set<int> nd_ref_values{};
  set<int> nd_values{};
  static constexpr MathSize BOUNDS_CANADA_LAT_MIN = 41;
  static constexpr MathSize BOUNDS_CANADA_LAT_MAX = 84;
  static constexpr MathSize BOUNDS_CANADA_LON_MIN = -141;
  static constexpr MathSize BOUNDS_CANADA_LON_MAX = -52;
  // FIX: use some weird increments to do less but not always have __.0
  static constexpr MathSize DEGREE_INCREMENT = 0.3;
  // static constexpr MathSize ELEVATION_CANADA_MAX = 5959;
  static constexpr MathSize ELEVATION_EARTH_MIN = -418;
  static constexpr MathSize ELEVATION_EARTH_MAX = 8848;
  static constexpr MathSize ELEVATION_INCREMENT = 100;
  // - nd for different latitudes
  //   - elevation 0
  // const auto latitudes = range(-90.0, 90.0, DEGREE_INCREMENT);
  const auto latitudes = range(BOUNDS_CANADA_LAT_MIN, BOUNDS_CANADA_LAT_MAX, DEGREE_INCREMENT);
  // const auto longitudes = range(-180.0, 180.0, DEGREE_INCREMENT);
  const auto longitudes = range(BOUNDS_CANADA_LON_MIN, BOUNDS_CANADA_LON_MAX, DEGREE_INCREMENT);
  const auto elevations = range(ELEVATION_EARTH_MIN, ELEVATION_EARTH_MAX, ELEVATION_INCREMENT);
  logging::info(
    "Checking ({:d} latitudes X {:d} longitudes X {:d} elevations) = {:d} combinations",
    latitudes.size(),
    longitudes.size(),
    elevations.size(),
    latitudes.size() * longitudes.size() * elevations.size()
  );
  for (auto latitude : latitudes)
  {
    for (auto longitude : longitudes)
    {
      for (auto elevation : elevations)
      {
        const Point pt{latitude, longitude};
        const auto nd_ref = calculate_nd_ref_for_point(elevation, pt);
        nd_ref_values.emplace(nd_ref);
        logging::verbose(
          "now have {:d} values for nd: {}, {:g} gives nd_ref {:d}",
          nd_ref_values.size(),
          pt,
          elevation,
          nd_ref
        );
      }
    }
  }
  for (int day : range_int(0, 366, 1))
  {
    for (auto nd_ref : nd_ref_values)
    {
      logging::verbose("jd {:d}", day);
      // from calculate_nd_for_point(const Day day, const int elevation, const Point& point)
      const auto nd = static_cast<int>(abs(day - nd_ref));
      nd_values.emplace(nd);
    }
  }
  auto min_nd = std::numeric_limits<int>::max();
  auto max_nd = std::numeric_limits<int>::min();
  for (auto nd : nd_values)
  {
    min_nd = min(min_nd, nd);
    max_nd = max(max_nd, nd);
  }
  const bool is_consecutive{static_cast<size_t>(max_nd - min_nd + 1) == nd_values.size()};
  logging::info(
    "Have {:d} nd values between {:d} and {:d} that are {:s}",
    nd_values.size(),
    min_nd,
    max_nd,
    is_consecutive ? "consecutive" : "non-consecutive"
  );
  return {nd_values.begin(), nd_values.end()};
}
int test_fbp(const int argc, const char* const argv[])
{
  std::ignore = argc;
  std::ignore = argv;
  logging::info("Testing FBP");
  const auto nd_all_values = find_nd_values();
  // HACK: initialize here so nd_all_values is set
  static const FuelCompareOptions FUEL_COMPARE_GRASS{
    .nd_values = nd_all_values, .dc_values = DC_VALUES_GRASS
  };
  // for (size_t i = 0; i < FuelOldLookup::Fuels.size(); ++i)
  // {
  //   auto& a = *simplefbp::SimpleFuels[i];
  //   auto& b = *dynamic_cast<fs::FuelBase*>(FuelOldLookup::Fuels[i]);
  //   compare(a.name(), a, b);
  //   // compare("", *simplefbp::SimpleFuels[i], *FuelOldLookup::Fuels[i]);
  // }
  auto i = 0;
  compare_fuel_valid(
    "Non-fuel", *SimpleFuelLookup::Fuels[i], *FuelOldLookup::Fuels[i], "basic test only"
  );
  ++i;
  compare_fuel_valid(
    "Invalid", *SimpleFuelLookup::Fuels[i], *FuelOldLookup::Fuels[i], "basic test only"
  );
  ++i;
  compare_fuel(
    "C1",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldC1*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "C2",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldC2*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "C3",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldC3*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "C4",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldC4*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "C5",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldC5*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "C6",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldC6*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "C7",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldC7*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "D1",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldD1*>(FuelOldLookup::Fuels[i]),
    FUEL_COMPARE_DECIDUOUS
  );
  ++i;
  compare_fuel(
    "D2",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldD2*>(FuelOldLookup::Fuels[i]),
    FUEL_COMPARE_DECIDUOUS
  );
  ++i;
  compare_fuel(
    "O1_A",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldO1A*>(FuelOldLookup::Fuels[i]),
    FUEL_COMPARE_GRASS
  );
  ++i;
  compare_fuel(
    "O1_B",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldO1B*>(FuelOldLookup::Fuels[i]),
    FUEL_COMPARE_GRASS
  );
  ++i;
  compare_fuel(
    "S1",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldS1*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "S2",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldS2*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "S3",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldS3*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "D1_D2",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldD1D2*>(FuelOldLookup::Fuels[i]),
    FUEL_COMPARE_DECIDUOUS
  );
  ++i;
  compare_fuel(
    "M1_05",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<5>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_10",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<10>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_15",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<15>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_20",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<20>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_25",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<25>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_30",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<30>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_35",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<35>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_40",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<40>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_45",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<45>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_50",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<50>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_55",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<55>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_60",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<60>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_65",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<65>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_70",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<70>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_75",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<75>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_80",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<80>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_85",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<85>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_90",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<90>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_95",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<95>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_05",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<5>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_10",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<10>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_15",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<15>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_20",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<20>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_25",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<25>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_30",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<30>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_35",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<35>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_40",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<40>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_45",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<45>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_50",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<50>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_55",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<55>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_60",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<60>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_65",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<65>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_70",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<70>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_75",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<75>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_80",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<80>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_85",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<85>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_90",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<90>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_95",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<95>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_05",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<5>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_10",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<10>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_15",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<15>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_20",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<20>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_25",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<25>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_30",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<30>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_35",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<35>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_40",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<40>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_45",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<45>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_50",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<50>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_55",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<55>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_60",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<60>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_65",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<65>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_70",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<70>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_75",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<75>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_80",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<80>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_85",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<85>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_90",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<90>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_95",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<95>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_05",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<5>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_10",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<10>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_15",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<15>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_20",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<20>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_25",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<25>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_30",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<30>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_35",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<35>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_40",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<40>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_45",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<45>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_50",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<50>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_55",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<55>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_60",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<60>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_65",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<65>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_70",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<70>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_75",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<75>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_80",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<80>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_85",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<85>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_90",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<90>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_95",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<95>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_100",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<100>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_05",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<5>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_10",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<10>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_15",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<15>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_20",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<20>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_25",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<25>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_30",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<30>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_35",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<35>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_40",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<40>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_45",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<45>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_50",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<50>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_55",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<55>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_60",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<60>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_65",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<65>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_70",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<70>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_75",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<75>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_80",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<80>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_85",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<85>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_90",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<90>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_95",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<95>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_100",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<100>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_00",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<0>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_05",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<5>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_10",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<10>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_15",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<15>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_20",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<20>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_25",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<25>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_30",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<30>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_35",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<35>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_40",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<40>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_45",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<45>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_50",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<50>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_55",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<55>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_60",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<60>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_65",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<65>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_70",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<70>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_75",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<75>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_80",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<80>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_85",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<85>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_90",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<90>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_95",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<95>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M1_00",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1<0>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M2_00",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM2<0>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M1_M2_00",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM1M2<0>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M3_00",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3<0>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel(
    "M4_00",
    *dynamic_cast<const SimpleStandardFuel*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM4<0>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "M3_M4_100",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldM3M4<100>*>(FuelOldLookup::Fuels[i])
  );
  ++i;
  compare_fuel_variable(
    "O1",
    *dynamic_cast<const SimpleFuelVariable*>(SimpleFuelLookup::Fuels[i]),
    *dynamic_cast<const FuelOldO1*>(FuelOldLookup::Fuels[i]),
    FUEL_COMPARE_GRASS
  );
  ++i;
  check_equal(NUMBER_OF_FUELS, i, "Number of fuels");
  return 0;
}
}
int main(const int argc, const char* const argv[])
{
  using namespace fs::settings;
  constexpr auto fct_main = fs::testing::test_fbp;
  static const Usage USAGE_TEST{"Run tests and exit", ""};
  SettingsArgumentParser parser{USAGE_TEST, argc, argv, PositionalArgumentsRequired::NotRequired};
  parser.parse_args();
  exit(fct_main(argc, argv));
}
