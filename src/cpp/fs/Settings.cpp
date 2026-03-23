/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Settings.h"
#include <exception>
#include <filesystem>
#include "Log.h"
#include "Trim.h"
#include "Util.h"
namespace fs
{
namespace settings
{
atomic<bool> save_individual{false};
atomic<bool> run_async{true};
atomic<bool> deterministic{false};
atomic<bool> surface{false};
atomic<bool> save_as_ascii{false};
atomic<bool> save_as_tiff{true};
atomic<bool> save_points{false};
atomic<bool> save_intensity{true};
atomic<bool> save_probability{true};
atomic<bool> save_occurrence{false};
atomic<bool> save_simulation_area{false};
atomic<bool> force_greenup{false};
atomic<bool> force_no_greenup{false};
atomic<bool> force_static_curing{false};
atomic<MathSize> minimum_ros{0.0};
atomic<MathSize> maximum_spread_distance{0.0};
atomic<MathSize> minimum_ffmc{0.0};
atomic<MathSize> minimum_ffmc_at_night{0.0};
atomic<DurationSize> utc_offset{0.0};
atomic<DurationSize> offset_sunrise{0.0};
atomic<DurationSize> offset_sunset{0.0};
atomic<int> default_percent_conifer{0};
atomic<int> default_percent_dead_fir{0};
atomic<int> intensity_max_low{0};
atomic<int> intensity_max_moderate{0};
atomic<ThresholdSize> confidence_level{0.0};
atomic<size_t> salt;
atomic<size_t> maximum_time_seconds{0};
atomic<size_t> interim_output_interval_seconds{0};
atomic<size_t> minimum_simulation_count{0};
atomic<size_t> minimum_active_simulation_count{0};
atomic<size_t> maximum_simulation_count{0};
atomic<ThresholdSize> threshold_scenario_weight{0.0};
atomic<ThresholdSize> threshold_daily_weight{0.0};
atomic<ThresholdSize> threshold_hourly_weight{0.0};
atomic<bool> was_initialized{false};
}
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
/**
 * \brief Settings implementation class
 */
class SettingsImplementation
{
public:
  ~SettingsImplementation() = default;
  SettingsImplementation(const SettingsImplementation& rhs) = delete;
  SettingsImplementation(SettingsImplementation&& rhs) = delete;
  SettingsImplementation& operator=(const SettingsImplementation& rhs) = delete;
  SettingsImplementation& operator=(SettingsImplementation&& rhs) = delete;
  static SettingsImplementation& instance() noexcept;
  static SettingsImplementation& instance(bool check_loaded) noexcept;
  const string getRoot() noexcept;
  /**
   * \brief Set root directory and read settings from file
   * \param dirname Directory to use for settings and relative paths
   */
  void setRoot(const string dirname) noexcept;
  /**
   * \brief Root directory that raster inputs are stored in
   * \return Root directory that raster inputs are stored in
   */
  [[nodiscard]] const char* rasterRoot() const noexcept
  {
    // HACK: resolve path when used to avoid failure when invalid but unused
    static auto canonical_path = get_canonical_path(dir_root_.c_str(), raster_root_);
    return canonical_path.c_str();
  }
  /**
   * \brief Fuel lookup table
   * \return Fuel lookup table
   */
  [[nodiscard]] const FuelLookup& fuelLookup() noexcept
  {
    if (nullptr == fuel_lookup_)
    {
      // HACK: resolve path when used to avoid failure when invalid but unused
      static auto canonical_path = get_canonical_path(dir_root_.c_str(), fuel_lookup_table_file_);
      // do this here because it relies on instance being created already
      fuel_lookup_ = std::make_unique<FuelLookup>(canonical_path.c_str());
      logging::check_fatal(nullptr == fuel_lookup_, "Fuel lookup table has not been loaded");
    }
    return *fuel_lookup_;
  }
  void setRasterRoot(const string dirname) noexcept { raster_root_ = dirname; }
  void setFuelLookupTable(const string filename) noexcept { fuel_lookup_table_file_ = filename; }
  /**
   * \brief Static curing value
   * \return Static curing value
   */
  [[nodiscard]] int staticCuring() const noexcept { return static_curing_; }
  /**
   * \brief Set static curing value
   * \return Set static curing value
   */
  void setStaticCuring(const int value) noexcept
  {
    logging::check_fatal(
      0 > value || 100 < value, "Grass curing (%) must be in range [0-100] but got %d", value
    );
    static_curing_ = value;
    settings::force_static_curing = true;
  }
  /**
   * \brief Days to output probability contours for (1 is start date, 2 is day after, etc.)
   * \return Days to output probability contours for (1 is start date, 2 is day after, etc.)
   */
  [[nodiscard]] vector<int> outputDateOffsets() const { return output_date_offsets_; }
  /**
   * \brief Set days to output probability contours for (1 is start date, 2 is day after, etc.)
   * \return None
   */
  void setOutputDateOffsets(const string value)
  {
    output_date_offsets_ = parse_list<int>(value, [](const string s) { return stoi(s); });
    max_date_offset_ = *std::max_element(output_date_offsets_.begin(), output_date_offsets_.end());
  }
  /**
   * \brief Whatever the maximum value in the date offsets is
   * \return Whatever the maximum value in the date offsets is
   */
  [[nodiscard]] constexpr int maxDateOffset() const noexcept { return max_date_offset_; }

private:
  /**
   * \brief Initialize object but don't load settings from file
   */
  explicit SettingsImplementation() noexcept = default;
  /**
   * \brief Directory used for settings and relative paths
   */
  string dir_root_;
  /**
   * \brief Mutex for parallel access
   */
  mutex mutex_;
  /**
   * \brief Root directory that raster inputs are stored in
   */
  string raster_root_;
  /**
   * \brief Name of file that defines fuel lookup table
   */
  string fuel_lookup_table_file_;
  /**
   * \brief fuel lookup table
   */
  unique_ptr<FuelLookup> fuel_lookup_ = nullptr;
  /**
   * \brief Static curing value
   */
  atomic<int> static_curing_ = 75;
  /**
   * \brief Days to output probability contours for (1 is start date, 2 is day after, etc.)
   */
  vector<int> output_date_offsets_;
  /**
   * \brief Whatever the maximum value in the date offsets is
   */
  int max_date_offset_;
};
/**
 * \brief The singleton instance for this class
 * \param check_loaded Whether to ensure a file has been loaded already
 * \return The singleton instance for this class
 */
SettingsImplementation& SettingsImplementation::instance(bool check_loaded) noexcept
{
  static SettingsImplementation instance_{};
  if (check_loaded)
  {
    logging::check_fatal(
      instance_.dir_root_.empty(),
      "Expected settings to be loaded, but no root directory specified yet"
    );
  }
  return instance_;
}
/**
 * \brief The singleton instance for this class
 * \return The singleton instance for this class
 */
SettingsImplementation& SettingsImplementation::instance() noexcept { return instance(true); }
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
const string SettingsImplementation::getRoot() noexcept { return dir_root_; }
void SettingsImplementation::setRoot(const string dirname) noexcept
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
    raster_root_ = get_value(settings, "RASTER_ROOT");
    fuel_lookup_table_file_ = get_value(settings, "FUEL_LOOKUP_TABLE");
    // HACK: run into fuel consumption being too low if we don't have a minimum ros
    static const auto MinRos = 0.05;
    // HACK: make sure this is always > 0 so that we don't have to check
    // specifically for 0 to avoid div error
    settings::minimum_ros = max(stod(get_value(settings, "MINIMUM_ROS")), MinRos);
    settings::maximum_spread_distance = stod(get_value(settings, "MAX_SPREAD_DISTANCE"));
    settings::minimum_ffmc = stod(get_value(settings, "MINIMUM_FFMC"));
    settings::minimum_ffmc_at_night = stod(get_value(settings, "MINIMUM_FFMC_AT_NIGHT"));
    settings::offset_sunrise = stod(get_value(settings, "OFFSET_SUNRISE"));
    settings::offset_sunset = stod(get_value(settings, "OFFSET_SUNSET"));
    settings::confidence_level = stod(get_value(settings, "CONFIDENCE_LEVEL"));
    settings::maximum_time_seconds = stol(get_value(settings, "MAXIMUM_TIME"));
    settings::interim_output_interval_seconds =
      stol(get_value(settings, "INTERIM_OUTPUT_INTERVAL"));
    settings::minimum_simulation_count = stol(get_value(settings, "MINIMUM_SIMULATIONS"));
    settings::minimum_active_simulation_count =
      stol(get_value(settings, "MINIMUM_ACTIVE_SIMULATIONS"));
    settings::maximum_simulation_count = stol(get_value(settings, "MAXIMUM_SIMULATIONS"));
    settings::threshold_scenario_weight = stod(get_value(settings, "THRESHOLD_SCENARIO_WEIGHT"));
    settings::threshold_daily_weight = stod(get_value(settings, "THRESHOLD_DAILY_WEIGHT"));
    settings::threshold_hourly_weight = stod(get_value(settings, "THRESHOLD_HOURLY_WEIGHT"));
    setOutputDateOffsets(get_value(settings, "OUTPUT_DATE_OFFSETS").c_str());
    settings::default_percent_conifer = stoi(get_value(settings, "DEFAULT_PERCENT_CONIFER"));
    settings::default_percent_dead_fir = stoi(get_value(settings, "DEFAULT_PERCENT_DEAD_FIR"));
    settings::intensity_max_low = stoi(get_value(settings, "INTENSITY_MAX_LOW"));
    settings::intensity_max_moderate = stoi(get_value(settings, "INTENSITY_MAX_MODERATE"));
    if (!settings.empty())
    {
      logging::warning("Unused settings in settings file %s", filename.c_str());
      for (const auto& kv : settings)
      {
        logging::warning("%s = %s", kv.first.c_str(), kv.second.c_str());
      }
    }
    settings::was_initialized = true;
    logging::check_fatal(
      !settings::was_initialized, "Settings read from file, but was_initialized is false"
    );
  }
  catch (const std::exception& ex)
  {
    logging::fatal(ex);
    std::terminate();
  }
}
const string Settings::getRoot() noexcept
{
  return SettingsImplementation::instance(false).getRoot();
}
void Settings::setRoot(const string dirname) noexcept
{
  return SettingsImplementation::instance(false).setRoot(dirname);
}
void Settings::setRasterRoot(const string dirname) noexcept
{
  return SettingsImplementation::instance().setRasterRoot(dirname);
}
const char* Settings::rasterRoot() noexcept
{
  return SettingsImplementation::instance().rasterRoot();
}
void Settings::setFuelLookupTable(const string filename) noexcept
{
  return SettingsImplementation::instance().setFuelLookupTable(filename);
}
const FuelLookup& Settings::fuelLookup() noexcept
{
  return SettingsImplementation::instance().fuelLookup();
}
int Settings::staticCuring() noexcept { return SettingsImplementation::instance().staticCuring(); }
void Settings::setStaticCuring(const int value) noexcept
{
  SettingsImplementation::instance().setStaticCuring(value);
}
vector<int> Settings::outputDateOffsets()
{
  return SettingsImplementation::instance().outputDateOffsets();
}
void Settings::setOutputDateOffsets(const string value)
{
  SettingsImplementation::instance().setOutputDateOffsets(value);
}
int Settings::maxDateOffset() noexcept
{
  return SettingsImplementation::instance().maxDateOffset();
}
}
