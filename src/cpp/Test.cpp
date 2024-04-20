/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Test.h"
#include "FireSpread.h"
#include "Model.h"
#include "Observer.h"
#include "Util.h"

namespace tbd::sim
{
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
                           data::ConstantGrid<topo::Cell>* cells) noexcept
    : Environment(dir_out, cells, 0)
  {
  }
};
static const wx::Temperature TEMP(20.0);
static const wx::RelativeHumidity RH(30.0);
static const wx::Precipitation PREC(0.0);
static vector<const wx::FwiWeather*>* make_weather(const wx::Dc& dc,
                                                   const wx::Bui& bui,
                                                   const wx::Dmc& dmc,
                                                   const wx::Ffmc& ffmc,
                                                   const wx::Wind& wind)
{
  auto wx = new vector<const wx::FwiWeather*>{static_cast<size_t>(YEAR_HOURS)};
  std::generate(wx->begin(),
                wx->end(),
                [&wind, &ffmc, &dmc, &dc, &bui]() {
                  return make_unique<wx::FwiWeather>(
                           TEMP,
                           RH,
                           wind,
                           PREC,
                           ffmc,
                           dmc,
                           dc,
                           wx::Isi(wind.speed(), ffmc),
                           bui,
                           wx::Fwi(wx::Isi(wind.speed(), ffmc), bui))
                    .release();
                });
  return wx;
}
/**
 * \brief A FireWeather stream with the same value for every date and time.
 */
class TestWeather final
  : public wx::FireWeather
{
public:
  ~TestWeather() override = default;
  TestWeather(const TestWeather& rhs) = delete;
  TestWeather(TestWeather&& rhs) = delete;
  TestWeather& operator=(const TestWeather& rhs) = delete;
  TestWeather& operator=(TestWeather&& rhs) = delete;
  /**
   * \brief A Constant weather stream with only one possible fuel
   * \param fuel Fuel to use
   * \param start_date Start date for stream
   * \param dc Drought Code
   * \param bui Build-up Index
   * \param dmc Duff Moisture Code
   * \param ffmc Fine Fuel Moisture Code
   * \param wind Wind
   */
  TestWeather(const fuel::FuelType* fuel,
              const Day start_date,
              const wx::Dc& dc,
              const wx::Bui& bui,
              const wx::Dmc& dmc,
              const wx::Ffmc& ffmc,
              const wx::Wind& wind)
    : FireWeather(set<const fuel::FuelType*>{fuel},
                  start_date,
                  MAX_DAYS - 1,
                  make_weather(dc, bui, dmc, ffmc, wind))
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
               const double end_date,
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
  constexpr auto FMT_FBP_OUT = "%8.2f%8.1f%8g%8.1f%8g%8.1f%8.1f%8g%8.1f%8.1f%8.1f%8s%8.3f%8.3f%8c%8ld%8g%8.4g%8.4g%8.4g%s";
  static const vector<const char*> HEADERS{"PREC", "TEMP", "RH", "WS", "WD", "FFMC", "DMC", "DC", "ISI", "BUI", "FWI", "FUEL", "CFB", "CFC", "FD", "HFI", "RAZ", "ROS", "SFC", "TFC"};
  printf("Calculated spread is:\n");
  for (auto h : HEADERS)
  {
    printf("%8s", h);
  }
  printf("\n");
  printf(FMT_FBP_OUT,
         w->prec().asDouble(),
         w->temp().asDouble(),
         w->rh().asDouble(),
         w->wind().speed().asDouble(),
         w->wind().direction().asDouble(),
         w->ffmc().asDouble(),
         w->dmc().asDouble(),
         w->dc().asDouble(),
         w->isi().asDouble(),
         w->bui().asDouble(),
         w->fwi().asDouble(),
         fuel->name(),
         spread.crownFractionBurned(),
         spread.crownFuelConsumption(),
         spread.fireDescription(),
         static_cast<size_t>(spread.maxIntensity()),
         spread.headDirection().asDegrees(),
         spread.headRos(),
         spread.surfaceFuelConsumption(),
         spread.totalFuelConsumption(),
         "\r\n");
}
static Semaphore num_concurrent{static_cast<int>(std::thread::hardware_concurrency())};
int run_test(const string output_directory,
             const string& fuel_name,
             const SlopeSize slope,
             const AspectSize aspect,
             const double num_hours,
             const wx::Dc& dc,
             const wx::Bui& bui,
             const wx::Dmc& dmc,
             const wx::Ffmc& ffmc,
             const wx::Wind& wind)
{
  // delay instantiation so things only get made when executed
  CriticalSection _(num_concurrent);
  const auto year = 2020;
  const auto month = 6;
  const auto day = 15;
  const auto hour = 12;
  const auto minute = 0;
  const auto t = util::to_tm(year, month, day, hour, minute);
  printf("DJ = %d\n", t.tm_yday);
  static const auto Latitude = 49.3911;
  static const auto Longitude = -84.7395;
  static const topo::StartPoint ForPoint(Latitude, Longitude);
  const auto start_date = t.tm_yday;
  const auto end_date = start_date + static_cast<double>(num_hours) / DAY_HOURS;
  util::make_directory_recursive(output_directory.c_str());
  const auto fuel = Settings::fuelLookup().byName(fuel_name);
  auto values = vector<topo::Cell>();
  //  values.reserve(static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS);
  for (Idx r = 0; r < MAX_ROWS; ++r)
  {
    for (Idx c = 0; c < MAX_COLUMNS; ++c)
    {
      values.emplace_back(r, c, slope, aspect, fuel::FuelType::safeCode(fuel));
    }
  }
  const auto cells = new data::ConstantGrid<topo::Cell>{
    TEST_GRID_SIZE,
    MAX_ROWS,
    MAX_COLUMNS,
    topo::Cell{},
    topo::Cell{},
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
  TestWeather weather(fuel, start_date, dc, bui, dmc, ffmc, wind);
  TestScenario scenario(&model, start_cell, ForPoint, start_date, end_date, &weather);
  const auto w = weather.at(start_date);
  auto info = SpreadInfo(scenario,
                         start_date,
                         start_cell->key(),
                         model.nd(start_date),
                         w);
  showSpread(info, w, fuel);
  map<double, ProbabilityMap*> probabilities{};
  logging::debug("Starting simulation");
  // NOTE: don't want to reset first because TestScenario handles what that does
  scenario.run(&probabilities);
  scenario.saveObservers("");
  logging::note("Final Size: %0.0f, ROS: %0.2f",
                scenario.currentFireSize(),
                info.headRos());
  return 0;
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
const double DEFAULT_HOURS = 10.0;
const vector<string> FUEL_NAMES{"C-2", "O-1a", "M-1/M-2 (25 PC)", "S-1", "C-3"};
int test(const int argc, const char* const argv[])
{
  // FIX: I think this does a lot of the same things as the test code is doing because it was
  // derived from this code
  Settings::setDeterministic(true);
  Settings::setMinimumRos(0.0);
  Settings::setSavePoints(false);
  // make sure all tests run regardless of how long it takes
  Settings::setMaximumTimeSeconds(numeric_limits<size_t>::max());
  const wx::Dc dc(275);
  const wx::Dmc dmc(35.5);
  const wx::Bui bui(dmc, dc);
  const wx::Ffmc ffmc(90);
  static const wx::Temperature TEMP(20.0);
  static const wx::RelativeHumidity RH(30.0);
  static const wx::Precipitation PREC(0.0);
  assert(argc > 1 && 0 == strcmp(argv[1], "test"));
  try
  {
    // increase logging level because there's no way to on command line right now
    logging::Log::increaseLogLevel();
    // logging::Log::increaseLogLevel();
    // logging::Log::increaseLogLevel();
    auto result = 0;
    // HACK: use a variable and ++ so in case arg indices change
    auto i = 1;
    // start at 2 because first arg is "test"
    ++i;
    string output_directory(argv[i++]);
    replace(output_directory.begin(), output_directory.end(), '\\', '/');
    if ('/' != output_directory[output_directory.length() - 1])
    {
      output_directory += '/';
    }
    logging::debug("Output directory is %s", output_directory.c_str());
    util::make_directory_recursive(output_directory.c_str());
    if (i == argc - 1 && 0 == strcmp(argv[i], "all"))
    {
      const auto num_hours = DEFAULT_HOURS;
      constexpr auto mask = "%s%s_S%03d_A%03d_WD%03d_WS%03d/";
      // generate all options first so we can say how many there are at start
      auto slopes = vector<SlopeSize>();
      for (SlopeSize slope = 0; slope <= 100; slope += SLOPE_INCREMENT)
      {
        slopes.emplace_back(slope);
      }
      auto aspects = vector<AspectSize>();
      for (AspectSize aspect = 0; aspect < 360; aspect += ASPECT_INCREMENT)
      {
        aspects.emplace_back(aspect);
      }
      auto wind_directions = vector<int>();
      for (auto wind_direction = 0; wind_direction < 360; wind_direction += WD_INCREMENT)
      {
        wind_directions.emplace_back(wind_direction);
      }
      auto wind_speeds = vector<int>();
      for (auto wind_speed = 0; wind_speed <= MAX_WIND; wind_speed += WS_INCREMENT)
      {
        wind_speeds.emplace_back(wind_speed);
      }
      size_t values = 1;
      values *= FUEL_NAMES.size();
      values *= slopes.size();
      values *= aspects.size();
      values *= wind_directions.size();
      values *= wind_speeds.size();
      printf("There are %ld options to try based on:\n", values);
      show_options("fuels", FUEL_NAMES);
      show_options("slopes", slopes);
      show_options("aspects", aspects);
      show_options("wind directions", wind_directions);
      show_options("wind speeds", wind_speeds);
      for (const auto& fuel : FUEL_NAMES)
      {
        auto simple_fuel_name{fuel};
        simple_fuel_name.erase(
          std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), '-'),
          simple_fuel_name.end());
        simple_fuel_name.erase(
          std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), ' '),
          simple_fuel_name.end());
        simple_fuel_name.erase(
          std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), '('),
          simple_fuel_name.end());
        simple_fuel_name.erase(
          std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), ')'),
          simple_fuel_name.end());
        simple_fuel_name.erase(
          std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), '/'),
          simple_fuel_name.end());
        const size_t out_length = output_directory.length() + 28 + simple_fuel_name.length() + 1;
        vector<char> out{};
        out.resize(out_length);
        // do everything in parallel but not all at once because it uses too much memory for most computers
        vector<std::future<int>> results{};
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
                logging::note("Queueing test for %s", out);
                // need to make string now because it'll be another value if we wait
                results.push_back(async(launch::async,
                                        run_test,
                                        string(&(out[0])),
                                        fuel,
                                        slope,
                                        aspect,
                                        num_hours,
                                        dc,
                                        bui,
                                        dmc,
                                        ffmc,
                                        wind));
              }
            }
          }
        }
        for (auto& r : results)
        {
          r.wait();
          result += r.get();
        }
      }
    }
    else
    {
      const auto num_hours = argc > i ? stod(argv[i++]) : DEFAULT_HOURS;
      const auto slope = static_cast<SlopeSize>(argc > i ? stoi(argv[i++]) : 0);
      const auto aspect = static_cast<AspectSize>(argc > i ? stoi(argv[i++]) : 0);
      const wx::Speed wind_speed(argc > i ? stoi(argv[i++]) : 20);
      const wx::Direction wind_direction(argc > i ? stoi(argv[i++]) : 180, false);
      const wx::Wind wind(wind_direction, wind_speed);
      assert(i == argc);
      logging::note(
        "Running tests with constant inputs for %d:\n"
        "\tSlope:\t\t\t%d\n"
        "\tAspect:\t\t\t%d\n"
        "\tWind Speed:\t\t%f\n"
        "\tWind Direction:\t\t%f\n",
        num_hours,
        slope,
        aspect,
        wind_speed,
        wind_direction);
      result = run_test(output_directory.c_str(),
                        "C-2",
                        slope,
                        aspect,
                        num_hours,
                        dc,
                        bui,
                        dmc,
                        ffmc,
                        wind);
    }
    return result;
  }
  catch (const runtime_error& err)
  {
    logging::fatal(err);
  }
  return 0;
}
}
