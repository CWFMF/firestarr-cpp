// Copyright (c) 2020-2021, Queen's Printer for Ontario.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "stdafx.h"
#include "Test.h"
#include "FireSpread.h"
#include "Model.h"
#include "Observer.h"
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
   * \param cells Constant cells 
   */
  explicit TestEnvironment(data::ConstantGrid<topo::Cell>* cells) noexcept
    : Environment(cells, 0)
  {
  }
};
static const wx::Temperature TMP(20.0);
static const wx::RelativeHumidity RH(30.0);
static const wx::AccumulatedPrecipitation APCP(0.0);
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
                           TMP,
                           RH,
                           wind,
                           APCP,
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
int run_test(const char* output_directory,
             const string& fuel_name,
             const SlopeSize slope,
             const AspectSize aspect,
             const int num_hours,
             const wx::Dc& dc,
             const wx::Bui& bui,
             const wx::Dmc& dmc,
             const wx::Ffmc& ffmc,
             const wx::Wind& wind)
{
  static const auto Latitude = 49.3911;
  static const auto Longitude = -84.7395;
  static const topo::StartPoint ForPoint(Latitude, Longitude);
  const auto start_date = 123;
  const auto end_date = start_date + static_cast<double>(num_hours) / DAY_HOURS;
  util::make_directory_recursive(output_directory);
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
    -1,
    TEST_XLLCORNER,
    TEST_YLLCORNER,
    TEST_XLLCORNER + TEST_GRID_SIZE * MAX_COLUMNS,
    TEST_YLLCORNER + TEST_GRID_SIZE * MAX_ROWS,
    TEST_PROJ4,
    std::move(values)};
  TestEnvironment env(cells);
  const Location start_location(static_cast<Idx>(MAX_ROWS / 2),
                                static_cast<Idx>(MAX_COLUMNS / 2));
  Model model(ForPoint, &env);
  const auto start_cell = make_shared<topo::Cell>(model.cell(start_location));
  TestWeather weather(fuel, start_date, dc, bui, dmc, ffmc, wind);
  TestScenario scenario(&model, start_cell, ForPoint, start_date, end_date, &weather);
  auto info = SpreadInfo(scenario,
                         start_date,
                         *start_cell,
                         model.nd(start_date),
                         weather.at(start_date));
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
const AspectSize ASPECT_INCREMENT = 90;
const SlopeSize SLOPE_INCREMENT = 60;
const int WS_INCREMENT = 20;
const int WD_INCREMENT = 45;
const int MAX_WIND = 20;
const int DEFAULT_HOURS = 10;
const vector<string> FUEL_NAMES{"C-2", "O-1a", "M-1/M-2 (25 PC)"};
int test(const int argc, const char* const argv[])
{
  const wx::Dc dc(275);
  const wx::Dmc dmc(35.5);
  const wx::Bui bui(54, dmc, dc);
  const wx::Ffmc ffmc(90);
  assert(argc > 1 && 0 == strcmp(argv[1], "test"));
  try
  {
    // increase logging level because there's no way to on command line right now
    logging::Log::increaseLogLevel();
    logging::Log::increaseLogLevel();
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
    Settings::setOutputDirectory(output_directory);
    if (i == argc - 1 && 0 == strcmp(argv[i], "all"))
    {
      const auto num_hours = DEFAULT_HOURS;
      constexpr auto mask = "%s%s_S%03d_A%03d_WD%03d_WS%03d/";
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
        const auto out_length = output_directory.length() + 28 + simple_fuel_name.length();
        const auto out = new char[out_length];
        for (SlopeSize slope = 0; slope <= 100; slope += SLOPE_INCREMENT)
        {
          for (AspectSize aspect = 0; aspect < 360; aspect += ASPECT_INCREMENT)
          {
            for (auto wind_direction = 0; wind_direction < 360; wind_direction +=
                                                                WD_INCREMENT)
            {
              const wx::Direction direction(wind_direction, false);
              for (auto wind_speed = 0; wind_speed <= MAX_WIND; wind_speed += WS_INCREMENT)
              {
                const wx::Wind wind(direction, wx::Speed(wind_speed));
                logging::note(mask,
                              output_directory.c_str(),
                              simple_fuel_name.c_str(),
                              slope,
                              aspect,
                              wind_direction,
                              wind_speed);
                sprintf(out,
                        mask,
                        output_directory.c_str(),
                        simple_fuel_name.c_str(),
                        slope,
                        aspect,
                        wind_direction,
                        wind_speed);
                Settings::setOutputDirectory(out);
                result += run_test(out,
                                   fuel,
                                   slope,
                                   aspect,
                                   num_hours,
                                   dc,
                                   bui,
                                   dmc,
                                   ffmc,
                                   wind);
              }
            }
          }
        }
        delete[] out;
      }
    }
    else
    {
      const auto num_hours = argc > i ? stoi(argv[i++]) : DEFAULT_HOURS;
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
    logging::fatal(err.what());
  }
  return 0;
}
}
