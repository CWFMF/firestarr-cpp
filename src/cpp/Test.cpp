/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Test.h"
#include "FireSpread.h"
#include "FireWeather.h"
#include "FuelLookup.h"
#include "Model.h"
#include "Observer.h"
#include "SafeVector.h"
#include "Util.h"
namespace fs
{
/**
 * \brief An Environment with no elevation and the same value in every Cell.
 */
class TestEnvironment : public Environment
{
public:
  /**
   * \brief Environment with the same data in every cell
   * \param cells Constant cells
   */
  explicit TestEnvironment(CellGrid&& cells) noexcept : Environment(std::move(cells), 0) { }
};
/**
 * \brief A Scenario run with constant fuel, weather, and topography.
 */
class TestScenario final : public Scenario
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
  TestScenario(
    Model* model,
    const shared_ptr<Cell>& start_cell,
    const StartPoint& start_point,
    const int start_date,
    const DurationSize end_date,
    const ptr<const FireWeather> weather,
    ptr<SafeVector> final_sizes
  )
    : Scenario(
        model,
        1,
        weather,
        start_date,
        start_cell,
        start_point,
        static_cast<Day>(start_date),
        static_cast<Day>(end_date)
      )
  {
    registerObserver(new IntensityObserver(*this));
    registerObserver(new ArrivalObserver(*this));
    registerObserver(new SourceObserver(*this));
    addEvent(Event::makeEnd(end_date));
    last_save_ = end_date;
    // cast to avoid warning
    std::ignore = reset(nullptr, nullptr, final_sizes);
  }
};
void showSpread(const SpreadInfo& spread, ptr<const FwiWeather> w, const FuelType* fuel)
{
  // column, (width, format)
  static const map<const char* const, std::pair<int, const char* const>> FMT{
    {"PREC", {5, "%*.2f"}}, {"TEMP", {5, "%*.1f"}}, {"RH", {3, "%*g"}},    {"WS", {5, "%*.1f"}},
    {"WD", {3, "%*g"}},     {"FFMC", {5, "%*.1f"}}, {"DMC", {5, "%*.1f"}}, {"DC", {5, "%*g"}},
    {"ISI", {5, "%*.1f"}},  {"BUI", {5, "%*.1f"}},  {"FWI", {5, "%*.1f"}}, {"GS", {3, "%*d"}},
    {"SAZ", {3, "%*d"}},    {"FUEL", {7, "%*s"}},   {"GC", {3, "%*g"}},    {"L:B", {5, "%*.2f"}},
    {"CBH", {4, "%*.1f"}},  {"CFB", {6, "%*.3f"}},  {"CFC", {6, "%*.3f"}}, {"FD", {2, "%*c"}},
    {"HFI", {6, "%*ld"}},   {"RAZ", {3, "%*d"}},    {"ROS", {6, "%*.4g"}}, {"SFC", {6, "%*.4g"}},
    {"TFC", {6, "%*.4g"}},
  };
  printf("Calculated spread is:\n");
  // print header row
  for (const auto& h_f : FMT)
  {
    // HACK: need to format string of the same length
    const auto col = h_f.first;
    const auto width = h_f.second.first;
    // column width + 1 space
    printf("%*s", width + 1, col);
  }
  printf("\n");
  auto print_col = [](const char* col, auto value) {
    const auto width_fmt = FMT.at(col);
    const auto width = width_fmt.first + 1;
    const auto fmt = width_fmt.second;
    printf(fmt, width, value);
  };
  // HACK: just do individual calls for now
  // can we assign them to a lookup table if they're not all numbers?
  print_col("PREC", w->prec().asValue());
  print_col("TEMP", w->temp().asValue());
  print_col("RH", w->rh().asValue());
  print_col("WS", w->wind().speed().asValue());
  print_col("WD", w->wind().direction().asValue());
  print_col("FFMC", w->ffmc().asValue());
  print_col("DMC", w->dmc().asValue());
  print_col("DC", w->dc().asValue());
  print_col("ISI", w->isi().asValue());
  print_col("BUI", w->bui().asValue());
  print_col("FWI", w->fwi().asValue());
  print_col("GS", spread.percentSlope());
  print_col("SAZ", spread.slopeAzimuth());
  const auto simple_fuel = simplify_fuel_name(fuel->name());
  print_col("FUEL", simple_fuel.c_str());
  print_col("GC", fuel->grass_curing(spread.nd(), *w));
  print_col("L:B", spread.lengthToBreadth());
  print_col("CBH", fuel->cbh());
  print_col("CFB", spread.crownFractionBurned());
  print_col("CFC", spread.crownFuelConsumption());
  print_col("FD", spread.fireDescription());
  print_col("HFI", static_cast<size_t>(spread.maxIntensity()));
  print_col("RAZ", static_cast<DirectionSize>(spread.headDirection().asDegrees()));
  print_col("ROS", spread.headRos());
  print_col("SFC", spread.surfaceFuelConsumption());
  print_col("TFC", spread.totalFuelConsumption());
  printf("\r\n");
}
string generate_test_name(
  const auto& fuel,
  const SlopeSize slope,
  const AspectSize aspect,
  const fs::Wind& wind
)
{
  // wind speed & direction can be decimal values, but slope and aspect are int
  constexpr auto mask = "%s_S%03d_A%03d_WD%05.1f_WS%05.1f";
  auto simple_fuel_name = simplify_fuel_name(fuel);
  const size_t out_length = simple_fuel_name.length() + 27 + 1;
  vector<char> out{};
  out.resize(out_length);
  sxprintf(
    &(out[0]),
    out_length,
    mask,
    simple_fuel_name.c_str(),
    slope,
    aspect,
    wind.direction().asDegrees(),
    wind.speed().asValue()
  );
  return string(&(out[0]));
};
string run_test(
  const string_view base_directory,
  const string_view fuel_name,
  const SlopeSize slope,
  const AspectSize aspect,
  const DurationSize num_hours,
  const Dc& dc,
  const Dmc& dmc,
  const Ffmc& ffmc,
  const Wind& wind,
  ptr<SafeVector> final_sizes,
  const bool ignore_existing
)
{
  string test_name = generate_test_name(fuel_name, slope, aspect, wind);
  logging::verbose("Queueing test for %s", &(test_name[0]));
  const string output_directory = string(base_directory) + test_name + "/";
  if (ignore_existing && directory_exists(output_directory.c_str()))
  {
    // skip if directory exists
    logging::warning("Skipping existing directory %s", output_directory.c_str());
    return output_directory;
  }
  // delay instantiation so things only get made when executed
  static Semaphore num_concurrent{10 * static_cast<int>(std::thread::hardware_concurrency())};
  CriticalSection _(num_concurrent);
  // logging::debug("Concurrent test limit is %d", num_concurrent.limit());
  logging::note("Running test for %s", string(output_directory).c_str());
  const auto year = 2020;
  const auto month = 6;
  const auto day = 15;
  const auto hour = 12;
  const auto minute = 0;
  const auto start_time = to_tm(year, month, day, hour, minute);
  static const auto Latitude = 49.3911;
  static const auto Longitude = -84.7395;
  static const StartPoint ForPoint(Latitude, Longitude);
  const auto start_date = start_time.tm_yday;
  const auto end_date = start_date + static_cast<DurationSize>(num_hours) / DAY_HOURS;
  make_directory_recursive(string(output_directory).c_str());
  const auto fuel = Settings::fuelLookup().bySimplifiedName(simplify_fuel_name(fuel_name));
  auto values = vector<Cell>();
  for (Idx r = 0; r < MAX_ROWS; ++r)
  {
    for (Idx c = 0; c < MAX_COLUMNS; ++c)
    {
      values.emplace_back(r, c, slope, aspect, FuelType::safeCode(fuel));
    }
  }
  const Cell cell_nodata{};
  TestEnvironment env{CellGrid{
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
    std::move(values)
  }};
  const Location start_location(static_cast<Idx>(MAX_ROWS / 2), static_cast<Idx>(MAX_COLUMNS / 2));
  Model model(start_time, output_directory, ForPoint, &env);
  const auto start_cell = make_shared<Cell>(model.cell(start_location));
  FireWeather weather(fuel, start_date, dc, dmc, ffmc, wind);
  TestScenario scenario(&model, start_cell, ForPoint, start_date, end_date, &weather, final_sizes);
  const auto w = weather.at(start_date);
  auto info = SpreadInfo(scenario, start_date, start_cell->key(), model.nd(start_date), w);
  showSpread(info, w, fuel);
  vector<shared_ptr<ProbabilityMap>> probabilities{};
  logging::debug("Starting simulation");
  // NOTE: don't want to reset first because TestScenabuirio handles what that does
  scenario.run(&probabilities);
  logging::note("Saving results for %s in %s", test_name.c_str(), output_directory.c_str());
  std::ignore = scenario.saveObservers(output_directory, test_name);
  logging::note("Final Size: %0.0f, ROS: %0.2f", scenario.currentFireSize(), info.headRos());
  return string(output_directory);
}
string run_test_ignore_existing(
  const string_view output_directory,
  const string_view fuel_name,
  const SlopeSize slope,
  const AspectSize aspect,
  const DurationSize num_hours,
  const Dc& dc,
  const Dmc& dmc,
  const Ffmc& ffmc,
  const Wind& wind,
  ptr<SafeVector> final_sizes
)
{
  return run_test(
    output_directory, fuel_name, slope, aspect, num_hours, dc, dmc, ffmc, wind, final_sizes, true
  );
}
template <class V, class T = V>
void show_options(
  const char* name,
  const vector<V>& values,
  const char* fmt,
  std::function<T(V&)> convert
)
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
  return show_options<V, V>(name, values, "%d", [](V& value) { return value; });
};
void show_options(const char* name, const vector<string>& values)
{
  return show_options<string, const char*>(name, values, "%s", [](string& value) {
    return value.c_str();
  });
};
int test(
  const string_view output_directory,
  const DurationSize num_hours,
  const ptr<const FwiWeather> wx,
  const string_view constant_fuel_name,
  const SlopeSize constant_slope,
  const AspectSize constant_aspect,
  const bool test_all
)
{
  static const AspectSize ASPECT_INCREMENT = 90;
  static const SlopeSize SLOPE_INCREMENT = 60;
  static const int WS_INCREMENT = 5;
  static const int WD_INCREMENT = 45;
  static const int MAX_WIND = 50;
  static const DurationSize DEFAULT_HOURS = 10.0;
  static const SlopeSize DEFAULT_SLOPE = 0;
  static const AspectSize DEFAULT_ASPECT = 0;
  static const Speed DEFAULT_WIND_SPEED(20);
  static const Direction DEFAULT_WIND_DIRECTION(180, false);
  static const Wind DEFAULT_WIND(DEFAULT_WIND_DIRECTION, DEFAULT_WIND_SPEED);
  static const Ffmc DEFAULT_FFMC(90);
  static const Dmc DEFAULT_DMC(35.5);
  static const Dc DEFAULT_DC(275);
  // const vector<string> FUEL_NAMES{"C-2", "O-1a", "M-1/M-2 (25 PC)", "S-1", "C-3"};
  // all possible fuel names that aren't invalid
  static auto FUEL_NAMES = []() -> vector<string> {
    auto it = std::views::transform(
      std::views::filter(
        FuelLookup::Fuels, [](const FuelType* f) -> bool { return nullptr != f && f->isValid(); }
      ),
      [](const auto* f) -> string { return simplify_fuel_name(FuelType::safeName(f)); }
    );
    return {it.begin(), it.end()};
  }();
  static const auto DEFAULT_FUEL_NAME = simplify_fuel_name("C-2");
  SafeVector final_sizes{};
  // FIX: I think this does a lot of the same things as the test code is doing because it was
  // derived from this code
  Settings::setDeterministic(true);
  Settings::setMinimumRos(0.0);
  Settings::setSavePoints(false);
  // make sure all tests run regardless of how long it takes
  Settings::setMaximumTimeSeconds(numeric_limits<size_t>::max());
  const auto hours = INVALID_TIME == num_hours ? DEFAULT_HOURS : num_hours;
  const auto ffmc = (fs::Ffmc::Invalid == wx->ffmc()) ? DEFAULT_FFMC : wx->ffmc();
  const auto dmc = (fs::Dmc::Invalid == wx->dmc()) ? DEFAULT_DMC : wx->dmc();
  const auto dc = (fs::Dc::Invalid == wx->dc()) ? DEFAULT_DC : wx->dc();
  // HACK: need to compare value and not object
  const auto wind_direction = (fs::Direction::Invalid.asValue() == wx->wind().direction().asValue())
                              ? DEFAULT_WIND_DIRECTION
                              : wx->wind().direction();
  const auto wind_speed = (fs::Speed::Invalid.asValue() == wx->wind().speed().asValue())
                          ? DEFAULT_WIND_SPEED
                          : wx->wind().speed();
  const auto wind = fs::Wind(wind_direction, wind_speed);
  const auto slope = (INVALID_SLOPE == constant_slope) ? DEFAULT_SLOPE : constant_slope;
  const auto aspect = (INVALID_ASPECT == constant_aspect) ? DEFAULT_ASPECT : constant_aspect;
  const auto fixed_fuel_name = simplify_fuel_name(constant_fuel_name);
  const auto fuel = (fixed_fuel_name.empty() ? DEFAULT_FUEL_NAME : fixed_fuel_name);
  try
  {
    if (test_all)
    {
      size_t result = 0;
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
      if (fs::Direction::Invalid == wx->wind().direction())
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
      if (fs::Speed::Invalid == wx->wind().speed())
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
      // do everything in parallel but not all at once because it uses too much memory for most
      // computers
      vector<std::future<string>> results{};
      for (const auto& fuel : fuel_names)
      {
        // do everything in parallel but not all at once because it uses too much memory for most
        // computers
        for (auto slope : slopes)
        {
          for (auto aspect : aspects)
          {
            for (auto wind_direction : wind_directions)
            {
              const Direction direction(wind_direction, false);
              for (auto wind_speed : wind_speeds)
              {
                const Wind wind(direction, Speed(wind_speed));
                // need to make string now because it'll be another value if we wait
                results.push_back(async(
                  launch::async,
                  run_test_ignore_existing,
                  output_directory,
                  fuel,
                  slope,
                  aspect,
                  hours,
                  dc,
                  dmc,
                  ffmc,
                  wind,
                  &final_sizes
                ));
              }
            }
          }
        }
      }
      for (auto& r : results)
      {
        r.wait();
        auto output_directory = r.get();
        logging::check_fatal(
          !directory_exists(output_directory.c_str()),
          "Directory for test is missing: %s\n",
          output_directory.c_str()
        );
        ++result;
      }
      auto directories = read_directory(output_directory, "*", false);
      logging::check_fatal(
        directories.size() != result,
        "Expected %ld directories but have %ld",
        result,
        directories.size()
      );
      logging::note("Successfully ran %ld tests", result);
    }
    else
    {
      logging::note(
        "Running tests with constant inputs for %f hours:\n"
        "\tFBP Fuel:\t\t%s\n"
        "\tFFMC:\t\t\t%f\n"
        "\tDMC:\t\t\t%f\n"
        "\tDC:\t\t\t%f\n"
        "\tWind Speed:\t\t%f\n"
        "\tWind Direction:\t\t%f\n"
        "\tSlope:\t\t\t%d\n"
        "\tAspect:\t\t\t%d\n",
        fuel.c_str(),
        hours,
        ffmc.asValue(),
        dmc.asValue(),
        dc.asValue(),
        wind_speed,
        wind_direction,
        slope,
        aspect
      );
      auto dir_result = run_test(
        output_directory, fuel, slope, aspect, hours, dc, dmc, ffmc, wind, &final_sizes, false
      );
      logging::check_fatal(
        !directory_exists(dir_result.c_str()),
        "Directory for test is missing: %s\n",
        dir_result.c_str()
      );
    }
  }
  catch (const runtime_error& err)
  {
    logging::fatal(err);
  }
  return 0;
}
}
