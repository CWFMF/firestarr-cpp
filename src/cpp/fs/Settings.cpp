/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Settings.h"
#include <exception>
#include <filesystem>
#include <mutex>
#include "FuelLookup.h"
#include "Log.h"
#include "Trim.h"
#include "unstable.h"
#include "Util.h"
namespace fs::settings
{
static mutex MUTEX{};
unique_ptr<Settings> INSTANCE{nullptr};
Settings& instance() noexcept
{
  std::lock_guard<mutex> lock{MUTEX};
  static const bool not_initialized = nullptr == INSTANCE;
  if (not_initialized)
  {
    logging::fatal("Expected settings to be loaded, but no root directory specified yet");
  }
  return *INSTANCE;
};
template <class T>
static vector<T> parse_list(string str, T (*convert)(const string s))
{
  vector<int> result{};
  // want format without spaces to work
  // OUTPUT_DATE_OFFSETS = [1,2,3,7,14]
  logging::check_fatal(str[0] != '[', "Expected list starting with '[");
  istringstream iss(str.substr(1));
  while (getline(iss, str, ','))
  {
    // need to make sure this isn't an empty list
    if (0 != strcmp("]", str.c_str()))
    {
      result.push_back(convert(str));
    }
  }
  return result;
}
OutputDateOffsets::OutputDateOffsets(const string value) noexcept
  : text_(value),
    output_date_offsets_{parse_list<int>(value, [](const string s) { return stoi(s); })},
    max_date_offset_{*std::max_element(output_date_offsets_.begin(), output_date_offsets_.end())}
{ }
string get_value(
  std::pair<string_map<string>, string_map<string>>& settings,
  const string_view key,
  const bool required = true
)
{
  auto& settings_unparsed = settings.first;
  auto& settings_found = settings.second;
  const auto found = settings_unparsed.find(key);
  if (found != settings_unparsed.end())
  {
    auto result = found->second;
    settings_unparsed.erase(found);
    // if empty then same as no value
    if ("" != result)
    {
      settings_found.emplace(key, result);
      return result;
    }
  }
  if (required)
  {
    logging::fatal("Missing setting for %s", string(key).c_str());
  }
  // HACK: use return to avoid compiler warning
  static const auto Invalid = "INVALID";
  return Invalid;
}
bool get_flag(
  const bool default_value,
  std::pair<string_map<string>, string_map<string>>& settings,
  const string key
)
{
  constexpr auto required = false;
  auto value = get_value(settings, key, required);
  if ("INVALID" == value)
  {
    return default_value;
  }
  if ("1" == value)
  {
    return true;
  }
  if ("0" == value)
  {
    return false;
  }
  return logging::fatal<bool>(
    "Only valid values for %s are 0 (false) or 1 (true) but got %s", key.c_str(), value.c_str()
  );
}
constexpr string to_string(const Mode mode)
{
  switch (mode)
  {
    case Mode::Simulation:
      return "SIMULATION";
    case Mode::Test:
      return "TEST";
    case Mode::Surface:
      return "SURFACE";
  }
  return logging::fatal<string>("Mode %s not handled", mode);
};
string ModeOptions()
{
  static const auto MODE_OPTIONS{std::format(
    "[{}, {}, {}]", to_string(Mode::Simulation), to_string(Mode::Test), to_string(Mode::Surface)
  )};
  return MODE_OPTIONS;
}
Mode get_mode(
  const Mode default_value,
  std::pair<string_map<string>, string_map<string>>& settings,
  const string key
)
{
  constexpr auto required = false;
  auto value = tolower(get_value(settings, key, required));
  if ("invalid" == value)
  {
    return default_value;
  }
  if ("simulation" == value)
  {
    return Mode::Simulation;
  }
  if ("surface" == value)
  {
    return Mode::Surface;
  }
  if ("test" == value)
  {
    return Mode::Test;
  }
  return logging::fatal<Mode>(
    "Only valid value for %s is one of %s but got %s",
    key.c_str(),
    ModeOptions().c_str(),
    value.c_str()
  );
}
const string Settings::getRoot() const noexcept { return dir_root_; }
const string Settings::getBinaryDirectory() const noexcept { return dir_binary_; }
void Settings::setRoot(const string dir_binary, const string dir_out) noexcept
{
  std::lock_guard<mutex> lock{MUTEX};
  logging::check_fatal(nullptr != INSTANCE, "Settings already initialized from file");
  INSTANCE = make_unique<Settings>(dir_binary, dir_out);
}
Settings::Settings(const string dir_binary, const string dir_settings)
{
  try
  {
    dir_binary_ = dir_binary;
    dir_root_ = dir_settings;
    const auto filename = dir_root_ + "settings.ini";
    logging::note("Initializing settings from %s", filename.c_str());
    settings_ =
      make_pair<string_map<string>, string_map<string>>(string_map<string>{}, string_map<string>{});
    ;
    ifstream in;
    in.open(filename.c_str());
    if (in.is_open())
    {
      string str;
      logging::info("Reading settings from '%s'", filename.c_str());
      while (getline(in, str))
      {
        istringstream iss(str);
        if (getline(iss, str, '#'))
        {
          iss = istringstream(str);
        }
        if (getline(iss, str, '='))
        {
          const auto key = trim_copy(str);
          getline(iss, str, '\n');
          const auto value = trim_copy(str);
          settings_.first.emplace(key, value);
          logging::debug("%s: %s", key.c_str(), value.c_str());
        }
      }
      in.close();
    }
    // HACK: resolve path when used to avoid failure when invalid but unused
    raster_root = LazyPath(dir_root_, get_value(settings_, "RASTER_ROOT"));
    fuel_lookup = LazyFuelLookup(dir_root_, get_value(settings_, "FUEL_LOOKUP_TABLE"));
    // HACK: run into fuel consumption being too low if we don't have a minimum ros
    static const auto MinRos = 0.05;
    // HACK: make sure this is always > 0 so that we don't have to check
    // specifically for 0 to avoid div error
    minimum_ros = max(stod(get_value(settings_, "MINIMUM_ROS")), MinRos);
    maximum_spread_distance = stod(get_value(settings_, "MAX_SPREAD_DISTANCE"));
    minimum_ffmc = stod(get_value(settings_, "MINIMUM_FFMC"));
    minimum_ffmc_at_night = stod(get_value(settings_, "MINIMUM_FFMC_AT_NIGHT"));
    offset_sunrise = stod(get_value(settings_, "OFFSET_SUNRISE"));
    offset_sunset = stod(get_value(settings_, "OFFSET_SUNSET"));
    confidence_level = stod(get_value(settings_, "CONFIDENCE_LEVEL"));
    maximum_time_seconds = stol(get_value(settings_, "MAXIMUM_TIME"));
    interim_output_interval_seconds = stol(get_value(settings_, "INTERIM_OUTPUT_INTERVAL"));
    minimum_simulation_count = stol(get_value(settings_, "MINIMUM_SIMULATIONS"));
    minimum_active_simulation_count = stol(get_value(settings_, "MINIMUM_ACTIVE_SIMULATIONS"));
    maximum_simulation_count = stol(get_value(settings_, "MAXIMUM_SIMULATIONS"));
    threshold_scenario_weight = stod(get_value(settings_, "THRESHOLD_SCENARIO_WEIGHT"));
    threshold_daily_weight = stod(get_value(settings_, "THRESHOLD_DAILY_WEIGHT"));
    threshold_hourly_weight = stod(get_value(settings_, "THRESHOLD_HOURLY_WEIGHT"));
    output_date_offsets = OutputDateOffsets{get_value(settings_, "OUTPUT_DATE_OFFSETS")};
    default_percent_conifer = stoi(get_value(settings_, "DEFAULT_PERCENT_CONIFER"));
    default_percent_dead_fir = stoi(get_value(settings_, "DEFAULT_PERCENT_DEAD_FIR"));
    intensity_max_low = stoi(get_value(settings_, "INTENSITY_MAX_LOW"));
    intensity_max_moderate = stoi(get_value(settings_, "INTENSITY_MAX_MODERATE"));
    save_individual = get_flag(false, settings_, "SAVE_INDIVIDUAL");
    run_async = get_flag(true, settings_, "RUN_ASYNC");
    deterministic = get_flag(false, settings_, "DETERMINISTIC");
    mode = get_mode(Mode::Simulation, settings_, "MODE");
    save_as_ascii = get_flag(false, settings_, "SAVE_AS_ASCII");
    save_as_tiff = get_flag(true, settings_, "SAVE_AS_TIFF");
    save_points = get_flag(false, settings_, "SAVE_POINTS");
    save_intensity = get_flag(true, settings_, "SAVE_INTENSITY");
    save_probability = get_flag(true, settings_, "SAVE_PROBABILITY");
    save_occurrence = get_flag(false, settings_, "SAVE_OCCURRENCE");
    save_simulation_area = get_flag(false, settings_, "SAVE_SIMULATION_AREA");
    // FIX: have single GREENUP = setting ?
    force_greenup = get_flag(false, settings_, "FORCE_GREENUP");
    force_no_greenup = get_flag(false, settings_, "FORCE_NO_GREENUP");
    if (const auto value = get_value(settings_, "CURING", false); "INVALID" != value)
    {
      static_curing = stoi(value);
    }
    if (const auto value = get_value(settings_, "TZ", false); "INVALID" != value)
    {
      utc_offset = stod(value);
    }
    if (const auto value = get_value(settings_, "SALT", false); "INVALID" != value)
    {
      const int v = stoi(value);
      salt = static_cast<size_t>(abs(v));
      if (v < 0)
      {
        logging::warning("Negative salt value '%d' converted to positive value %zu", v, salt);
      }
    }
    // if (const auto value = get_value(settings_, "OUTPUT_DIRECTORY", false); "INVALID" != value)
    // {
    //   output_directory = value;
    // }
    if (const auto value = get_value(settings_, "WX", false); "INVALID" != value)
    {
      wx_file_name = value;
    }
    if (const auto value = get_value(settings_, "LOG_FILE_NAME", false); "INVALID" != value)
    {
      log_file_name = value;
    }
    if (const auto value = get_value(settings_, "PERIMETER", false); "INVALID" != value)
    {
      perimeter = value;
    }
    if (const auto value = get_value(settings_, "FFMC", false); "INVALID" != value)
    {
      ffmc = Ffmc{stod(value)};
    }
    if (const auto value = get_value(settings_, "DMC", false); "INVALID" != value)
    {
      dmc = Dmc{stod(value)};
    }
    if (const auto value = get_value(settings_, "DC", false); "INVALID" != value)
    {
      dc = Dc{stod(value)};
    }
    if (const auto value = get_value(settings_, "TZ", false); "INVALID" != value)
    {
      utc_offset = stod(value);
    }
    if (const auto value = get_value(settings_, "WD", false); "INVALID" != value)
    {
      wind_direction = stod(value);
    }
    if (const auto value = get_value(settings_, "WS", false); "INVALID" != value)
    {
      wind_speed = stod(value);
    }
    // HACK: check for value that should only exist for specific mode
    auto get_mode_value = [&](const char* key, const Mode mode_required) {
      const auto value = get_value(settings_, key, false);
      if ("INVALID" != value)
      {
        logging::check_fatal(
          mode_required != mode,
          "Value for %s found but mode is %s not %s",
          key,
          to_string(mode).c_str(),
          to_string(mode_required).c_str()
        );
      }
      return value;
    };
    if (const auto value = get_mode_value("SIZE", Mode::Simulation); "INVALID" != value)
    {
      initial_size = stoi(value);
    }
    if (const auto value = get_mode_value("FUEL", Mode::Test); "INVALID" != value)
    {
      fuel_name = value;
    }
    if (const auto value = get_mode_value("TEST_ALL", Mode::Test); "INVALID" != value)
    {
      // FIX: duplicates effort
      test_all = get_flag(true, settings_, "TEST_ALL");
    }
    if (const auto value = get_mode_value("HOURS", Mode::Simulation); "INVALID" != value)
    {
      hours = stod(value);
    }
    if (const auto value = get_mode_value("START_DATE", Mode::Simulation); "INVALID" != value)
    {
      start_date = parse_date(value);
    }
    if (const auto value = get_mode_value("START_TIME", Mode::Simulation); "INVALID" != value)
    {
      // NOTE: required START_DATE to be parsed previously
      add_time(start_date.value(), value);
    }
    if (const auto value = get_mode_value("LATITUDE", Mode::Simulation); "INVALID" != value)
    {
      latitude = stod(value);
    }
    if (const auto value = get_mode_value("LONGITUDE", Mode::Simulation); "INVALID" != value)
    {
      longitude = stod(value);
    }
    if (!settings_.first.empty())
    {
      logging::warning("Unused settings in settings file %s", filename.c_str());
      for (const auto& kv : settings_.first)
      {
        logging::warning("%s = %s", kv.first.c_str(), kv.second.c_str());
      }
    }
  }
  catch (const std::exception& ex)
  {
    logging::fatal(ex);
    std::terminate();
  }
}
void Settings::saveTo(const string& output_directory) const noexcept
{
  const auto filename = string(output_directory) + "settings.ini";
  ofstream out{filename};
  auto put = [&](const string& key, const string& comment, const auto& value) {
    out << "# " << comment << "\n";
    out << key << " = " << value << "\n";
  };
  // HACK: just hardcode how this works since it's the opposite of parsing
  // // FIX: this should always just be whatever folder the settings file is in?
  // auto abs = std::filesystem::absolute(output_directory);
  // put("OUTPUT_DIRECTORY", "output directory", abs.c_str());
  put("WX", "weather file path", wx_file_name.c_str());
  put("LOG_FILE_NAME", "log file name", log_file_name.c_str());
  put("PERIMETER", "perimeter to use for ignition", perimeter.c_str());
  put("RASTER_ROOT", "root directory to read rasters from", raster_root.canonical());
  put(
    "MINIMUM_ROS",
    "minimum rate of spread before fire is considered to actually be spreading",
    minimum_ros
  );
  put(
    "MAX_SPREAD_DISTANCE",
    "maximum distance that the fire is allowed to spread in one step (# of cells)",
    maximum_spread_distance
  );
  put("OUTPUT_DATE_OFFSETS", "days to output probability contours for", output_date_offsets.text());
  put("FUEL_LOOKUP_TABLE", "lookup table for fuels (prometheus format)", fuel_lookup.canonical());
  put("MINIMUM_FFMC", "minimum ffmc for fire to spread", minimum_ffmc);
  put("MINIMUM_FFMC_AT_NIGHT", "minimum ffmc for fire to spread at night", minimum_ffmc_at_night);
  put(
    "OFFSET_SUNRISE",
    "offset from sunrise at which the day is considered to start (hours)",
    offset_sunrise
  );
  put(
    "OFFSET_SUNSET",
    "offset from sunrise at which the day is considered to end (hours)",
    offset_sunset
  );
  put(
    "THRESHOLD_SCENARIO_WEIGHT",
    "weight given to scenario value when generating random thresholds",
    threshold_scenario_weight
  );
  put(
    "THRESHOLD_DAILY_WEIGHT",
    "weight given to daily value when generating random thresholds",
    threshold_daily_weight
  );
  put(
    "THRESHOLD_HOURLY_WEIGHT",
    "weight given to hourly value when generating random thresholds",
    threshold_hourly_weight
  );
  put(
    "DEFAULT_PERCENT_CONIFER",
    "default Percent Conifer to use for M1/M2 fuels where none is specified (%)",
    default_percent_conifer
  );
  put(
    "DEFAULT_PERCENT_DEAD_FIR",
    "default Percent Dead Fir to use for M3/M4 fuels where none is specified (%)",
    default_percent_dead_fir
  );
  put(
    "MAXIMUM_TIME",
    "maximum amount of time to take for simulation (seconds) (0 is unlimited)",
    maximum_time_seconds
  );
  put(
    "INTERIM_OUTPUT_INTERVAL",
    "amount of time between generating interim outputs (seconds) (0 is no interim outputs)",
    interim_output_interval_seconds
  );
  put(
    "MINIMUM_SIMULATIONS",
    "minimum number of simulations to do (0 is exactly 1 simulation per scenario)",
    minimum_simulation_count
  );
  put(
    "MINIMUM_ACTIVE_SIMULATIONS",
    "minimum number of simulations where any spread occurs to do (0 is exactly 1 simulation per scenario)",
    minimum_active_simulation_count
  );
  put(
    "MAXIMUM_SIMULATIONS",
    "maximum number of simulations to do (0 is exactly 1 simulation per scenario)",
    maximum_simulation_count
  );
  put(
    "CONFIDENCE_LEVEL", "confidence required before simulation stops (% / 100)", confidence_level
  );
  put(
    "INTENSITY_MAX_LOW",
    "maximum fire intensity for the 'low' range of intensity (kW/m)",
    intensity_max_low
  );
  put(
    "INTENSITY_MAX_MODERATE",
    "maximum fire intensity for the 'moderate' range of intensity (kW/m)",
    intensity_max_moderate
  );
  put("SAVE_INDIVIDUAL", "whether or not to save individual grids", save_individual);
  put("RUN_ASYNC", "whether or not to run things asynchronously where possible", run_async);
  put(
    "DETERMINISTIC",
    "whether or not to run deterministically (100% chance of spread & survival)",
    deterministic
  );
  static const auto mode_help = std::format("simulation mode (one of {})", ModeOptions());
  put("MODE", mode_help, to_string(mode));
  put("SAVE_AS_ASCII", "whether or not to save grids as .asc", save_as_ascii);
  put("SAVE_AS_TIFF", "whether or not to save grids as .tif", save_as_tiff);
  put("SAVE_POINTS", "whether or not to save points used for spread", save_points);
  put("SAVE_INTENSITY", "whether or not to save intensity grids", save_intensity);
  put("SAVE_PROBABILITY", "whether or not to save probability grids", save_probability);
  put("SAVE_OCCURRENCE", "whether or not to save occurrence grids", save_occurrence);
  put("SAVE_SIMULATION_AREA", "whether or not to save simulation area grids", save_simulation_area);
  put("FORCE_GREENUP", "whether or not to force greenup for all fires", force_greenup);
  put("FORCE_NO_GREENUP", "whether or not to force no greenup for all fires", force_no_greenup);
  put("CURING", "static curing value to force for all fires", static_curing.as_string());
  put("TZ", "offset from UTC to use for entire simulation (hours)", utc_offset);
  put("SALT", "salt to use for random seeds", salt);
  if (Mode::Simulation == mode)
  {
    put("SIZE", "initial fire size", initial_size);
  }
  if (Mode::Surface != mode)
  {
    put("START_DATE", "ignition start time", format_date(start_date.value()));
    put("START_TIME", "ignition start time", format_time(start_date.value()));
    put("LATITUDE", "weather file path", latitude);
    put("LONGITUDE", "weather file path", longitude);
  }
  if (Mode::Test == mode)
  {
    put("FUEL", "fbp fuel type", fuel_name);
    put("TEST_ALL", "test every combination of settings for tests", test_all);
    put("HOURS", "number of hours to run tests for", hours);
  }
  // always need these for startup or constant indices
  put("FFMC", "fine fuel moisture code", ffmc);
  put("DMC", "duff moisture code", dmc);
  put("DC", "drought code", dc);
  if (Mode::Simulation != mode)
  {
    put("WD", "wind direction (degrees)", wind_direction);
    put("WS", "wind speed (km/h)", wind_speed);
  }
  if (Mode::Test == mode)
  {
    put("SLOPE", "ground slope (%)", slope);
    put("ASPECT", "ground aspect (degrees)", aspect);
  }
  // FIX: don't have output for set/getRoot()
  // put("", "", );
}
FwiWeather Settings::get_weather() const
{
  return {
    Weather{
      Temperature::Zero(),
      RelativeHumidity::Zero(),
      Wind{
        Speed{wind_speed.value_or(Speed::Invalid().value)},
        Direction{Degrees{wind_direction.value_or(Direction::Invalid().value)}}
      },
      {apcp_prev}
    },
    ffmc.value_or(Ffmc::Invalid()),
    dmc.value_or(Dmc::Invalid()),
    dc.value_or(Dc::Invalid())
  };
}
}
