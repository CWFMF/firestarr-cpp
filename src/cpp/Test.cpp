/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Test.h"
#include "FireSpread.h"
#include "Model.h"
#include "Observer.h"
#include "Util.h"
#include "ConstantWeather.h"

namespace tbd::sim
{
using tbd::fuel::simplify_fuel_name;
/**
 * \brief An Environment with no elevation and the same value in every Cell.
 */
class TestEnvironment
  : public topo::Environment
{
public:
  /**
   * \brief Environment with the same data in every cell
   * \param dir_out Folder to save outputs to
   * \param cells Constant cells
   */
  explicit TestEnvironment(const string dir_out,
                           topo::CellGrid* cells) noexcept
    : Environment(dir_out, cells, 0)
  {
  }
};
/**
 * \brief A Scenario run with constant fuel, weather, and topography.
 */
class TestScenario final
  : public Scenario
{
public:
  ~TestScenario() override = default;
  TestScenario(const TestScenario& rhs) = delete;
  TestScenario(TestScenario&& rhs) = delete;
  TestScenario& operator=(const TestScenario& rhs) = delete;
  TestScenario& operator=(TestScenario&& rhs) = delete;
  /**
   * \brief Constructor
   * \param model Model running this Scenario
   * \param start_cell Cell to start ignition in
   * \param start_point StartPoint represented by start_cell
   * \param start_date Start date of simulation
   * \param end_date End data of simulation
   * \param weather Constant weather to use for duration of simulation
   */
  TestScenario(Model* model,
               const shared_ptr<topo::Cell>& start_cell,
               const topo::StartPoint& start_point,
               const int start_date,
               const DurationSize end_date,
               wx::FireWeather* weather)
    : Scenario(model,
               1,
               weather,
               weather,
               start_date,
               start_cell,
               start_point,
               static_cast<Day>(start_date),
               static_cast<Day>(end_date))
  {
    registerObserver(new IntensityObserver(*this, "intensity"));
    registerObserver(new ArrivalObserver(*this));
    registerObserver(new SourceObserver(*this));
    addEvent(Event::makeEnd(end_date));
    last_save_ = end_date;
    final_sizes_ = {};
    // cast to avoid warning
    static_cast<void*>(reset(nullptr, nullptr, reinterpret_cast<util::SafeVector*>(&final_sizes_)));
  }
};
void showSpread(const SpreadInfo& spread, const wx::FwiWeather* w, const fuel::FuelType* fuel)
{
  static const map<const char* const, const char* const> FMT{
    {"PREC", " %6.2f"},
    {"TEMP", " %6.1f"},
    {"RH", " %6g"},
    {"WS", " %6.1f"},
    {"WD", " %6g"},
    {"FFMC", " %6.1f"},
    {"DMC", " %6.1f"},
    {"DC", " %6g"},
    {"ISI", " %6.1f"},
    {"BUI", " %6.1f"},
    {"FWI", " %6.1f"},
    {"GS", " %6d"},
    {"SAZ", " %6d"},
    {"FUEL", "%20s"},
    {"GC", " %6g"},
    {"L:B", " %6.2f"},
    {"CBH", " %6.1f"},
    {"CFB", " %6.3f"},
    {"CFC", " %6.3f"},
    {"FD", " %6c"},
    {"HFI", " %6ld"},
    {"RAZ", " %6d"},
    {"ROS", " %6.4g"},
    {"SFC", " %6.4g"},
    {"TFC", " %6.4g"},
  };
  printf("Calculated spread is:\n");
  // print header row
  for (const auto& h_f : FMT)
  {
    // HACK: need to format string of the same length
    const auto h = h_f.first;
    printf(
      0 == strcmp(h, "FUEL") ? "%20s" : "%7s",
      h);
  }
  printf("\n");
  // HACK: just do individual calls for now
  // can we assign them to a lookup table if they're not all numbers?
  printf(FMT.at("PREC"), w->prec().asValue());
  printf(FMT.at("TEMP"), w->temp().asValue());
  printf(FMT.at("RH"), w->rh().asValue());
  printf(FMT.at("WS"), w->wind().speed().asValue());
  printf(FMT.at("WD"), w->wind().direction().asValue());
  printf(FMT.at("FFMC"), w->ffmc().asValue());
  printf(FMT.at("DMC"), w->dmc().asValue());
  printf(FMT.at("DC"), w->dc().asValue());
  printf(FMT.at("ISI"), w->isi().asValue());
  printf(FMT.at("BUI"), w->bui().asValue());
  printf(FMT.at("FWI"), w->fwi().asValue());
  printf(FMT.at("GS"), spread.percentSlope());
  printf(FMT.at("SAZ"), spread.slopeAzimuth());
  printf(FMT.at("FUEL"), fuel->name());
  printf(FMT.at("GC"), fuel->grass_curing(spread.nd(), *w));
  printf(FMT.at("L:B"), spread.lengthToBreadth());
  printf(FMT.at("CBH"), fuel->cbh());
  printf(FMT.at("CFB"), spread.crownFractionBurned());
  printf(FMT.at("CFC"), spread.crownFuelConsumption());
  printf(FMT.at("FD"), spread.fireDescription());
  printf(FMT.at("HFI"), static_cast<size_t>(spread.maxIntensity()));
  printf(FMT.at("RAZ"), static_cast<DirectionSize>(spread.headDirection().asDegrees()));
  printf(FMT.at("ROS"), spread.headRos());
  printf(FMT.at("SFC"), spread.surfaceFuelConsumption());
  printf(FMT.at("TFC"), spread.totalFuelConsumption());
  printf("\r\n");
}
static Semaphore num_concurrent{static_cast<int>(std::thread::hardware_concurrency())};
string run_test(const string output_directory,
                const string& fuel_name,
                const SlopeSize slope,
                const AspectSize aspect,
                const DurationSize num_hours,
                const wx::Dc& dc,
                const wx::Dmc& dmc,
                const wx::Ffmc& ffmc,
                const wx::Wind& wind,
                const bool ignore_existing)
{
  if (ignore_existing && util::directory_exists(output_directory.c_str()))
  {
    // skip if directory exists
    logging::warning("Skipping existing directory %s", output_directory.c_str());
    return output_directory;
  }
  // delay instantiation so things only get made when executed
  CriticalSection _(num_concurrent);
  logging::debug("Concurrent test limit is %d", num_concurrent.limit());
  logging::note("Running test for %s", output_directory.c_str());
  const auto year = 2020;
  const auto month = 6;
  const auto day = 15;
  const auto hour = 12;
  const auto minute = 0;
  const auto t = util::to_tm(year, month, day, hour, minute);
  logging::verbose("DJ = %d\n", t.tm_yday);
  static const auto Latitude = 49.3911;
  static const auto Longitude = -84.7395;
  static const topo::StartPoint ForPoint(Latitude, Longitude);
  const auto start_date = t.tm_yday;
  const auto end_date = start_date + static_cast<DurationSize>(num_hours) / DAY_HOURS;
  util::make_directory_recursive(output_directory.c_str());
  const auto fuel = Settings::fuelLookup().bySimplifiedName(simplify_fuel_name(fuel_name));
  auto values = vector<topo::Cell>();
  //  values.reserve(static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS);
  for (Idx r = 0; r < MAX_ROWS; ++r)
  {
    for (Idx c = 0; c < MAX_COLUMNS; ++c)
    {
      values.emplace_back(r, c, slope, aspect, fuel::FuelType::safeCode(fuel));
    }
  }
  const topo::Cell cell_nodata{};
  const auto cells = new topo::CellGrid{
    TEST_GRID_SIZE,
    MAX_ROWS,
    MAX_COLUMNS,
    cell_nodata.fullHash(),
    cell_nodata,
    TEST_XLLCORNER,
    TEST_YLLCORNER,
    TEST_XLLCORNER + TEST_GRID_SIZE * MAX_COLUMNS,
    TEST_YLLCORNER + TEST_GRID_SIZE * MAX_ROWS,
    TEST_PROJ4,
    std::move(values)};
  TestEnvironment env(output_directory, cells);
  const Location start_location(static_cast<Idx>(MAX_ROWS / 2),
                                static_cast<Idx>(MAX_COLUMNS / 2));
  Model model(output_directory, ForPoint, &env);
  const auto start_cell = make_shared<topo::Cell>(model.cell(start_location));
  ConstantWeather weather(fuel, start_date, dc, dmc, ffmc, wind);
  TestScenario scenario(&model, start_cell, ForPoint, start_date, end_date, &weather);
  const auto w = weather.at(start_date);
  auto info = SpreadInfo(scenario,
                         start_date,
                         start_cell->key(),
                         model.nd(start_date),
                         w);
  showSpread(info, w, fuel);
  map<DurationSize, ProbabilityMap*> probabilities{};
  logging::debug("Starting simulation");
  // NOTE: don't want to reset first because TestScenabuirio handles what that does
  scenario.run(&probabilities);
  scenario.saveObservers("");
  logging::note("Final Size: %0.0f, ROS: %0.2f",
                scenario.currentFireSize(),
                info.headRos());
  return output_directory;
}
string run_test_ignore_existing(
  const string output_directory,
  const string& fuel_name,
  const SlopeSize slope,
  const AspectSize aspect,
  const DurationSize num_hours,
  const wx::Dc& dc,
  const wx::Dmc& dmc,
  const wx::Ffmc& ffmc,
  const wx::Wind& wind)
{
  return run_test(output_directory, fuel_name, slope, aspect, num_hours, dc, dmc, ffmc, wind, true);
}
template <class V, class T = V>
void show_options(const char* name,
                  const vector<V>& values,
                  const char* fmt,
                  std::function<T(V&)> convert)
{
  printf("\t%ld %s: ", values.size(), name);
  // HACK: always print something before but avoid extra comma
  const char* prefix_open = "[";
  const char* prefix_comma = ", ";
  const char** p = &prefix_open;
  for (auto v : values)
  {
    printf(*p);
    printf(fmt, convert(v));
    p = &prefix_comma;
  }
  printf("]\n");
};
template <class V>
void show_options(const char* name, const vector<V>& values)
{
  return show_options<V, V>(name,
                            values,
                            "%d",
                            [](V& value) { return value; });
};
void show_options(const char* name, const vector<string>& values)
{
  return show_options<string, const char*>(name,
                                           values,
                                           "%s",
                                           [](string& value) {
                                             return value.c_str();
                                           });
};

const AspectSize ASPECT_INCREMENT = 90;
const SlopeSize SLOPE_INCREMENT = 60;
const int WS_INCREMENT = 5;
const int WD_INCREMENT = 45;
const int MAX_WIND = 50;
const DurationSize DEFAULT_HOURS = 10.0;
const SlopeSize DEFAULT_SLOPE = 0;
const AspectSize DEFAULT_ASPECT = 0;
const wx::Speed DEFAULT_WIND_SPEED(20);
const wx::Direction DEFAULT_WIND_DIRECTION(180, false);
const wx::Wind DEFAULT_WIND(DEFAULT_WIND_DIRECTION, DEFAULT_WIND_SPEED);
const wx::Ffmc DEFAULT_FFMC(90);
const wx::Dmc DEFAULT_DMC(35.5);
const wx::Dc DEFAULT_DC(275);
// these weather variables change nothing?
static const wx::Temperature TEMP(20.0);
static const wx::RelativeHumidity RH(30.0);
static const wx::Precipitation PREC(0.0);
const vector<string> FUEL_NAMES{"C-2", "O-1a", "M-1/M-2 (25 PC)", "S-1", "C-3"};
const auto DEFAULT_FUEL_NAME = simplify_fuel_name(FUEL_NAMES[0]);

int test(
  const string& output_directory,
  const DurationSize num_hours,
  const tbd::wx::FwiWeather* wx,
  const string& constant_fuel_name,
  const SlopeSize constant_slope,
  const AspectSize constant_aspect,
  const bool test_all)
{
  // FIX: I think this does a lot of the same things as the test code is doing because it was
  // derived from this code
  Settings::setDeterministic(true);
  Settings::setMinimumRos(0.0);
  Settings::setSavePoints(false);
  // make sure all tests run regardless of how long it takes
  Settings::setMaximumTimeSeconds(numeric_limits<size_t>::max());
  const auto hours = INVALID_TIME == num_hours ? DEFAULT_HOURS : num_hours;
  const auto ffmc = (tbd::wx::Ffmc::Invalid == wx->ffmc()) ? DEFAULT_FFMC : wx->ffmc();
  const auto dmc = (tbd::wx::Dmc::Invalid == wx->dmc()) ? DEFAULT_DMC : wx->dmc();
  const auto dc = (tbd::wx::Dc::Invalid == wx->dc()) ? DEFAULT_DC : wx->dc();
  // HACK: need to compare value and not object
  const auto wind_direction = (tbd::wx::Direction::Invalid.asValue() == wx->wind().direction().asValue()) ? DEFAULT_WIND_DIRECTION : wx->wind().direction();
  const auto wind_speed = (tbd::wx::Speed::Invalid.asValue() == wx->wind().speed().asValue()) ? DEFAULT_WIND_SPEED : wx->wind().speed();
  const auto wind = tbd::wx::Wind(wind_direction, wind_speed);
  static const wx::Temperature TEMP(20.0);
  static const wx::RelativeHumidity RH(30.0);
  static const wx::Precipitation PREC(0.0);
  const auto slope = (INVALID_SLOPE == constant_slope) ? DEFAULT_SLOPE : constant_slope;
  const auto aspect = (INVALID_ASPECT == constant_aspect) ? DEFAULT_ASPECT : constant_aspect;
  const auto fixed_fuel_name = simplify_fuel_name(constant_fuel_name);
  const auto fuel = (fixed_fuel_name.empty() ? DEFAULT_FUEL_NAME : fixed_fuel_name);
  try
  {
    if (test_all)
    {
      size_t result = 0;
      constexpr auto mask = "%s%s_S%03d_A%03d_WD%03d_WS%03d/";
      // generate all options first so we can say how many there are at start
      auto fuel_names = vector<string>();
      if (fixed_fuel_name.empty())
      {
        for (auto f : FUEL_NAMES)
        {
          fuel_names.emplace_back(f);
        }
      }
      else
      {
        fuel_names.emplace_back(fuel);
      }
      auto slopes = vector<SlopeSize>();
      if (INVALID_SLOPE == constant_slope)
      {
        for (SlopeSize slope = 0; slope <= 100; slope += SLOPE_INCREMENT)
        {
          slopes.emplace_back(slope);
        }
      }
      else
      {
        slopes.emplace_back(constant_slope);
      }
      auto aspects = vector<AspectSize>();
      if (INVALID_ASPECT == constant_aspect)
      {
        for (AspectSize aspect = 0; aspect < 360; aspect += ASPECT_INCREMENT)
        {
          aspects.emplace_back(aspect);
        }
      }
      else
      {
        aspects.emplace_back(constant_aspect);
      }
      auto wind_directions = vector<int>();
      if (tbd::wx::Direction::Invalid == wx->wind().direction())
      {
        for (auto wind_direction = 0; wind_direction < 360; wind_direction += WD_INCREMENT)
        {
          wind_directions.emplace_back(wind_direction);
        }
      }
      else
      {
        wind_directions.emplace_back(static_cast<int>(wx->wind().direction().asDegrees()));
      }
      auto wind_speeds = vector<int>();
      if (tbd::wx::Speed::Invalid == wx->wind().speed())
      {
        for (auto wind_speed = 0; wind_speed <= MAX_WIND; wind_speed += WS_INCREMENT)
        {
          wind_speeds.emplace_back(wind_speed);
        }
      }
      else
      {
        wind_speeds.emplace_back(static_cast<int>(wx->wind().speed().asValue()));
      }
      size_t values = 1;
      values *= fuel_names.size();
      values *= slopes.size();
      values *= aspects.size();
      values *= wind_directions.size();
      values *= wind_speeds.size();
      printf("There are %ld options to try based on:\n", values);
      show_options("fuels", fuel_names);
      show_options("slopes", slopes);
      show_options("aspects", aspects);
      show_options("wind directions", wind_directions);
      show_options("wind speeds", wind_speeds);
      // do everything in parallel but not all at once because it uses too much memory for most computers
      vector<std::future<string>> results{};
      for (const auto& fuel : fuel_names)
      {
        auto simple_fuel_name = simplify_fuel_name(fuel);
        const size_t out_length = output_directory.length() + 28 + simple_fuel_name.length() + 1;
        vector<char> out{};
        out.resize(out_length);
        // do everything in parallel but not all at once because it uses too much memory for most computers
        for (auto slope : slopes)
        {
          for (auto aspect : aspects)
          {
            for (auto wind_direction : wind_directions)
            {
              const wx::Direction direction(wind_direction, false);
              for (auto wind_speed : wind_speeds)
              {
                const wx::Wind wind(direction, wx::Speed(wind_speed));
                sxprintf(&(out[0]),
                         out_length,
                         mask,
                         output_directory.c_str(),
                         simple_fuel_name.c_str(),
                         slope,
                         aspect,
                         wind_direction,
                         wind_speed);
                logging::verbose("Queueing test for %s", out);
                // need to make string now because it'll be another value if we wait
                results.push_back(async(launch::async,
                                        run_test_ignore_existing,
                                        string(&(out[0])),
                                        fuel,
                                        slope,
                                        aspect,
                                        hours,
                                        dc,
                                        dmc,
                                        ffmc,
                                        wind));
              }
            }
          }
        }
      }
      for (auto& r : results)
      {
        r.wait();
        auto dir_out = r.get();
        logging::check_fatal(!util::directory_exists(dir_out.c_str()),
                             "Directory for test is missing: %s\n",
                             dir_out.c_str());
        ++result;
      }
      vector<string> directories{};
      util::read_directory(false, output_directory, &directories);
      logging::check_fatal(directories.size() != result,
                           "Expected %ld directories but have %ld",
                           result,
                           directories.size());
      logging::note("Successfully ran %ld tests", result);
    }
    else
    {
      logging::note(
        "Running tests with constant inputs for %f hours:\n"
        "\tFFMC:\t\t\t%f\n"
        "\tDMC:\t\t\t%f\n"
        "\tDC:\t\t\t%f\n"
        "\tWind Speed:\t\t%f\n"
        "\tWind Direction:\t\t%f\n"
        "\tSlope:\t\t\t%d\n"
        "\tAspect:\t\t\t%d\n",
        hours,
        ffmc.asValue(),
        dmc.asValue(),
        dc.asValue(),
        wind_speed,
        wind_direction,
        slope,
        aspect);
      auto dir_out = run_test(output_directory.c_str(),
                              fuel,
                              slope,
                              aspect,
                              hours,
                              dc,
                              dmc,
                              ffmc,
                              wind,
                              false);
      logging::check_fatal(!util::directory_exists(dir_out.c_str()),
                           "Directory for test is missing: %s\n",
                           dir_out.c_str());
    }
  }
  catch (const runtime_error& err)
  {
    logging::fatal(err);
  }
  return 0;
}
}
