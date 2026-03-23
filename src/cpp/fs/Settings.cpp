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
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the day
   * \return Minimum Fine Fuel Moisture Code required for spread during the day
   */
  [[nodiscard]] constexpr MathSize minimumFfmc() const noexcept { return minimum_ffmc_; }
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the night
   * \return Minimum Fine Fuel Moisture Code required for spread during the night
   */
  [[nodiscard]] constexpr MathSize minimumFfmcAtNight() const noexcept
  {
    return minimum_ffmc_at_night_;
  }
  /**
   * \brief Set offset from UTC to use for entire simulation (hours)
   * \param v Offset from UTC to use for entire simulation (hours)
   */
  void setUtcOffset(const DurationSize v) noexcept { utc_offset_ = v; }
  /**
   * \brief Offset from UTC to use for entire simulation (hours)
   * \return Offset from UTC to use for entire simulation (hours)
   */
  [[nodiscard]] constexpr DurationSize utcOffset() const noexcept { return utc_offset_; }
  /**
   * \brief Offset from sunrise at which the day is considered to start (hours)
   * \return Offset from sunrise at which the day is considered to start (hours)
   */
  [[nodiscard]] constexpr DurationSize offsetSunrise() const noexcept { return offset_sunrise_; }
  /**
   * \brief Offset from sunrise at which the day is considered to end (hours)
   * \return Offset from sunrise at which the day is considered to end (hours)
   */
  [[nodiscard]] constexpr DurationSize offsetSunset() const noexcept { return offset_sunset_; }
  /**
   * \brief Default Percent Conifer to use for M1/M2 fuels where none is specified (%)
   * \return Percent of the stand that is composed of conifer (%)
   */
  [[nodiscard]] constexpr int defaultPercentConifer() const noexcept
  {
    return default_percent_conifer_;
  }
  /**
   * \brief Default Percent Dead Fir to use for M3/M4 fuels where none is specified (%)
   * \return Percent of the stand that is composed of dead fir (NOT percent of the fir that is dead)
   * (%)
   */
  [[nodiscard]] constexpr int defaultPercentDeadFir() const noexcept
  {
    return default_percent_dead_fir_;
  }
  /**
   * \brief The maximum fire intensity for the 'low' range of intensity (kW/m)
   * \return The maximum fire intensity for the 'low' range of intensity (kW/m)
   */
  [[nodiscard]] constexpr int intensityMaxLow() const noexcept { return intensity_max_low_; }
  /**
   * \brief The maximum fire intensity for the 'moderate' range of intensity (kW/m)
   * \return The maximum fire intensity for the 'moderate' range of intensity (kW/m)
   */
  [[nodiscard]] constexpr int intensityMaxModerate() const noexcept
  {
    return intensity_max_moderate_;
  }
  /**
   * \brief Confidence required before simulation stops (% / 100)
   * \return Confidence required before simulation stops (% / 100)
   */
  [[nodiscard]] ThresholdSize confidenceLevel() const noexcept { return confidence_level_; }
  /**
   * \brief Set confidence required before simulation stops (% / 100)
   * \return Set confidence required before simulation stops (% / 100)
   */
  void setConfidenceLevel(const ThresholdSize value) noexcept { confidence_level_ = value; }
  size_t salt() noexcept { return salt_; }
  void setSalt(const size_t value) noexcept { salt_ = value; }
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
   * \brief Minimum Fine Fuel Moisture Code required for spread during the day
   */
  MathSize minimum_ffmc_;
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the night
   */
  MathSize minimum_ffmc_at_night_;
  /**
   * \brief Offset from UTC to use for entire simulation (hours)
   */
  DurationSize utc_offset_;
  /**
   * \brief Offset from sunrise at which the day is considered to start (hours)
   */
  DurationSize offset_sunrise_;
  /**
   * \brief Offset from sunrise at which the day is considered to end (hours)
   */
  DurationSize offset_sunset_;
  /**
   * \brief Confidence required before simulation stops (% / 100)
   */
  atomic<ThresholdSize> confidence_level_;
  /**
   * \brief Salt to use for random seeds
   */
  atomic<size_t> salt_{0};
  /**
   * \brief Static curing value
   */
  atomic<int> static_curing_ = 75;
  /**
   * \brief Days to output probability contours for (1 is start date, 2 is day after, etc.)
   */
  vector<int> output_date_offsets_;
  /**
   * \brief Default Percent Conifer to use for M1/M2 fuels where none is specified (%)
   */
  int default_percent_conifer_;
  /**
   * \brief Default Percent Dead Fir to use for M3/M4 fuels where none is specified (%)
   */
  int default_percent_dead_fir_;
  /**
   * \brief Whatever the maximum value in the date offsets is
   */
  int max_date_offset_;
  /**
   * \brief The maximum fire intensity for the 'low' range of intensity (kW/m)
   */
  int intensity_max_low_;
  /**
   * \brief The maximum fire intensity for the 'moderate' range of intensity (kW/m)
   */
  int intensity_max_moderate_;
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
    minimum_ffmc_ = stod(get_value(settings, "MINIMUM_FFMC"));
    minimum_ffmc_at_night_ = stod(get_value(settings, "MINIMUM_FFMC_AT_NIGHT"));
    offset_sunrise_ = stod(get_value(settings, "OFFSET_SUNRISE"));
    offset_sunset_ = stod(get_value(settings, "OFFSET_SUNSET"));
    confidence_level_ = stod(get_value(settings, "CONFIDENCE_LEVEL"));
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
    default_percent_conifer_ = stoi(get_value(settings, "DEFAULT_PERCENT_CONIFER"));
    default_percent_dead_fir_ = stoi(get_value(settings, "DEFAULT_PERCENT_DEAD_FIR"));
    intensity_max_low_ = stoi(get_value(settings, "INTENSITY_MAX_LOW"));
    intensity_max_moderate_ = stoi(get_value(settings, "INTENSITY_MAX_MODERATE"));
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
MathSize Settings::minimumFfmc() noexcept
{
  return SettingsImplementation::instance().minimumFfmc();
}
MathSize Settings::minimumFfmcAtNight() noexcept
{
  return SettingsImplementation::instance().minimumFfmcAtNight();
}
void Settings::setUtcOffset(const DurationSize v) noexcept
{
  SettingsImplementation::instance().setUtcOffset(v);
}
DurationSize Settings::utcOffset() noexcept
{
  return SettingsImplementation::instance().utcOffset();
}
DurationSize Settings::offsetSunrise() noexcept
{
  return SettingsImplementation::instance().offsetSunrise();
}
DurationSize Settings::offsetSunset() noexcept
{
  return SettingsImplementation::instance().offsetSunset();
}
int Settings::defaultPercentConifer() noexcept
{
  return SettingsImplementation::instance().defaultPercentConifer();
}
int Settings::defaultPercentDeadFir() noexcept
{
  return SettingsImplementation::instance().defaultPercentDeadFir();
}
int Settings::intensityMaxLow() noexcept
{
  return SettingsImplementation::instance().intensityMaxLow();
}
int Settings::intensityMaxModerate() noexcept
{
  return SettingsImplementation::instance().intensityMaxModerate();
}
ThresholdSize Settings::confidenceLevel() noexcept
{
  return SettingsImplementation::instance().confidenceLevel();
}
void Settings::setConfidenceLevel(const ThresholdSize value) noexcept
{
  SettingsImplementation::instance().setConfidenceLevel(value);
}
size_t Settings::salt() noexcept { return SettingsImplementation::instance().salt(); }
void Settings::setSalt(const size_t value) noexcept
{
  SettingsImplementation::instance().setSalt(value);
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
