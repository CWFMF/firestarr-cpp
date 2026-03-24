/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Settings.h"
#include <exception>
#include <filesystem>
#include <mutex>
#include "FuelLookup.h"
#include "Log.h"
#include "Trim.h"
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
[[nodiscard]] int Settings::staticCuring() const noexcept { return static_curing_; }
void Settings::setStaticCuring(const int value) noexcept
{
  logging::check_fatal(
    0 > value || 100 < value, "Grass curing (%) must be in range [0-100] but got %d", value
  );
  static_curing_ = value;
  force_static_curing = true;
}
[[nodiscard]] vector<int> Settings::outputDateOffsets() const { return output_date_offsets_; }
void Settings::setOutputDateOffsets(const string value)
{
  output_date_offsets_ = parse_list<int>(value, [](const string s) { return stoi(s); });
  max_date_offset_ = *std::max_element(output_date_offsets_.begin(), output_date_offsets_.end());
}
[[nodiscard]] int Settings::maxDateOffset() const noexcept { return max_date_offset_; }
string get_value(string_map<string>& settings, const string_view key)
{
  const auto found = settings.find(key);
  if (found != settings.end())
  {
    auto result = found->second;
    settings.erase(found);
    return result;
  }
  logging::fatal("Missing setting for %s", string(key).c_str());
  // HACK: use return to avoid compiler warning
  static const auto Invalid = "INVALID";
  return Invalid;
}
const string Settings::getRoot() const noexcept { return dir_root_; }
void Settings::setRoot(const string dirname) noexcept
{
  std::lock_guard<mutex> lock{MUTEX};
  logging::check_fatal(nullptr != INSTANCE, "Settings already initialized from file");
  INSTANCE = make_unique<Settings>(dirname);
}
Settings::Settings(const string dirname)
{
  try
  {
    dir_root_ = dirname;
    const auto filename = dir_root_ + "settings.ini";
    string_map<string> settings{};
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
          settings.emplace(key, value);
          logging::debug("%s: %s", key.c_str(), value.c_str());
        }
      }
      in.close();
    }
    // HACK: resolve path when used to avoid failure when invalid but unused
    raster_root = LazyPath(dir_root_, get_value(settings, "RASTER_ROOT"));
    fuel_lookup = LazyFuelLookup(dir_root_, get_value(settings, "FUEL_LOOKUP_TABLE"));
    // HACK: run into fuel consumption being too low if we don't have a minimum ros
    static const auto MinRos = 0.05;
    // HACK: make sure this is always > 0 so that we don't have to check
    // specifically for 0 to avoid div error
    minimum_ros = max(stod(get_value(settings, "MINIMUM_ROS")), MinRos);
    maximum_spread_distance = stod(get_value(settings, "MAX_SPREAD_DISTANCE"));
    minimum_ffmc = stod(get_value(settings, "MINIMUM_FFMC"));
    minimum_ffmc_at_night = stod(get_value(settings, "MINIMUM_FFMC_AT_NIGHT"));
    offset_sunrise = stod(get_value(settings, "OFFSET_SUNRISE"));
    offset_sunset = stod(get_value(settings, "OFFSET_SUNSET"));
    confidence_level = stod(get_value(settings, "CONFIDENCE_LEVEL"));
    maximum_time_seconds = stol(get_value(settings, "MAXIMUM_TIME"));
    interim_output_interval_seconds = stol(get_value(settings, "INTERIM_OUTPUT_INTERVAL"));
    minimum_simulation_count = stol(get_value(settings, "MINIMUM_SIMULATIONS"));
    minimum_active_simulation_count = stol(get_value(settings, "MINIMUM_ACTIVE_SIMULATIONS"));
    maximum_simulation_count = stol(get_value(settings, "MAXIMUM_SIMULATIONS"));
    threshold_scenario_weight = stod(get_value(settings, "THRESHOLD_SCENARIO_WEIGHT"));
    threshold_daily_weight = stod(get_value(settings, "THRESHOLD_DAILY_WEIGHT"));
    threshold_hourly_weight = stod(get_value(settings, "THRESHOLD_HOURLY_WEIGHT"));
    setOutputDateOffsets(get_value(settings, "OUTPUT_DATE_OFFSETS").c_str());
    default_percent_conifer = stoi(get_value(settings, "DEFAULT_PERCENT_CONIFER"));
    default_percent_dead_fir = stoi(get_value(settings, "DEFAULT_PERCENT_DEAD_FIR"));
    intensity_max_low = stoi(get_value(settings, "INTENSITY_MAX_LOW"));
    intensity_max_moderate = stoi(get_value(settings, "INTENSITY_MAX_MODERATE"));
    if (!settings.empty())
    {
      logging::warning("Unused settings in settings file %s", filename.c_str());
      for (const auto& kv : settings)
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
}
