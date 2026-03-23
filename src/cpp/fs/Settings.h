/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SETTINGS_H
#define FS_SETTINGS_H
#include "stdafx.h"
#include "FuelLookup.h"
namespace fs
{
namespace settings
{
// Whether or not to save individual grids
extern atomic<bool> save_individual;
// Whether or not to run things asynchronously where possible
extern atomic<bool> run_async;
// Whether or not to run deterministically (100% chance of spread & survival)
extern atomic<bool> deterministic;
// Whether or not to create a probability surface
extern atomic<bool> surface;
// Whether or not to save grids as .asc
extern atomic<bool> save_as_ascii;
// Whether or not to save grids as .tif
extern atomic<bool> save_as_tiff;
// Whether or not to save points used for spread
extern atomic<bool> save_points;
// Whether or not to save intensity grids
extern atomic<bool> save_intensity;
// Whether or not to save probability grids
extern atomic<bool> save_probability;
// Whether or not to save occurrence grids
extern atomic<bool> save_occurrence;
// Whether or not to save simulation area grids
extern atomic<bool> save_simulation_area;
// Whether or not to force greenup for all fires
extern atomic<bool> force_greenup;
// Whether or not to force no greenup for all fires
extern atomic<bool> force_no_greenup;
// Whether or not to force static curing value for all fires
extern atomic<bool> force_static_curing;
// Maximum time simulation can run before it is ended and whatever results it has are used (s)
extern atomic<size_t> maximum_time_seconds;
// Time between generating interim outputs (s)
extern atomic<size_t> interim_output_interval_seconds;
// Minimum number of simulations that must run before stopping
extern atomic<size_t> minimum_simulation_count;
// Minimum number of simulations with any spread that must run before stopping
extern atomic<size_t> minimum_active_simulation_count;
// Maximum number of simulations before stopping and whatever results it has are used
extern atomic<size_t> maximum_simulation_count;
// Weight to give to Scenario part of thresholds=
extern atomic<ThresholdSize> threshold_scenario_weight;
// Weight to give to daily part of thresholds
extern atomic<ThresholdSize> threshold_daily_weight;
// Weight to give to hourly part of thresholds
extern atomic<ThresholdSize> threshold_hourly_weight;
// HACK: provide a way to check we actually initialized
extern atomic<bool> was_initialized;
}
/**
 * \brief Reads and provides access to settings for the simulation.
 */
class Settings
{
public:
  static const string getRoot() noexcept;
  /**
   * \brief Set root directory and read settings from file
   * \param dirname Directory to use for settings and relative paths
   */
  static void setRoot(const string dirname) noexcept;
  /**
   * \brief Set raster root directory
   * \param dirname Directory to use for rasters
   */
  static void setRasterRoot(const string dirname) noexcept;
  /**
   * \brief Root directory that raster inputs are stored in
   * \return Root directory that raster inputs are stored in
   */
  [[nodiscard]] static const char* rasterRoot() noexcept;
  /**
   * \brief Set fuel lookup table file
   * \param dirname Directory to use for rasters
   */
  static void setFuelLookupTable(const string filename) noexcept;
  /**
   * \brief Fuel lookup table
   * \return Fuel lookup table
   */
  [[nodiscard]] static const FuelLookup& fuelLookup() noexcept;
  /**
   * \brief Static curing value
   * \return Static curing value
   */
  [[nodiscard]] static int staticCuring() noexcept;
  /**
   * \brief Set static curing value
   * \return Set static curing value
   */
  static void setStaticCuring(const int value) noexcept;
  /**
   * \brief Minimum rate of spread before fire is considered to be spreading (m/min)
   * \return Minimum rate of spread before fire is considered to be spreading (m/min)
   */
  [[nodiscard]] static MathSize minimumRos() noexcept;
  static void setMinimumRos(MathSize value) noexcept;
  /**
   * \brief Maximum distance that the fire is allowed to spread in one step (# of cells)
   * \return Maximum distance that the fire is allowed to spread in one step (# of cells)
   */
  [[nodiscard]] static MathSize maximumSpreadDistance() noexcept;
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the day
   * \return Minimum Fine Fuel Moisture Code required for spread during the day
   */
  [[nodiscard]] static MathSize minimumFfmc() noexcept;
  /**
   * \brief Minimum Fine Fuel Moisture Code required for spread during the night
   * \return Minimum Fine Fuel Moisture Code required for spread during the night
   */
  [[nodiscard]] static MathSize minimumFfmcAtNight() noexcept;
  /**
   * \brief Set offset from UTC to use for entire simulation (hours)
   * \param v Offset from UTC to use for entire simulation (hours)
   */
  static void setUtcOffset(const DurationSize v) noexcept;
  /**
   * \brief Offset from UTC to use for entire simulation (hours)
   * \return Offset from UTC to use for entire simulation (hours)
   */
  [[nodiscard]] static DurationSize utcOffset() noexcept;
  /**
   * \brief Offset from sunrise at which the day is considered to start (hours)
   * \return Offset from sunrise at which the day is considered to start (hours)
   */
  [[nodiscard]] static DurationSize offsetSunrise() noexcept;
  /**
   * \brief Offset from sunrise at which the day is considered to end (hours)
   * \return Offset from sunrise at which the day is considered to end (hours)
   */
  [[nodiscard]] static DurationSize offsetSunset() noexcept;
  /**
   * \brief Default Percent Conifer to use for M1/M2 fuels where none is specified (%)
   * \return Percent of the stand that is composed of conifer (%)
   */
  [[nodiscard]] static int defaultPercentConifer() noexcept;
  /**
   * \brief Default Percent Dead Fir to use for M3/M4 fuels where none is specified (%)
   * \return Percent of the stand that is composed of dead fir (NOT percent of the fir that is dead)
   * (%)
   */
  [[nodiscard]] static int defaultPercentDeadFir() noexcept;
  /**
   * \brief The maximum fire intensity for the 'low' range of intensity (kW/m)
   * \return The maximum fire intensity for the 'low' range of intensity (kW/m)
   */
  [[nodiscard]] static int intensityMaxLow() noexcept;
  /**
   * \brief The maximum fire intensity for the 'moderate' range of intensity (kW/m)
   * \return The maximum fire intensity for the 'moderate' range of intensity (kW/m)
   */
  [[nodiscard]] static int intensityMaxModerate() noexcept;
  /**
   * \brief Confidence required before simulation stops (% / 100)
   * \return Confidence required before simulation stops (% / 100)
   */
  [[nodiscard]] static ThresholdSize confidenceLevel() noexcept;
  /**
   * \brief Set confidence required before simulation stops (% / 100)
   * \return Set confidence required before simulation stops (% / 100)
   */
  static void setConfidenceLevel(const ThresholdSize value) noexcept;
  /**
   * \brief Salt to use for random seeds
   * \return Salt to use for random seeds
   */
  [[nodiscard]] static size_t salt() noexcept;
  /**
   * \brief Set salt to use for random seeds
   * \return Set salt to use for random seeds
   */
  static void setSalt(const size_t value) noexcept;
  /**
   * \brief Days to output probability contours for (1 is start date, 2 is day after, etc.)
   * \return Days to output probability contours for (1 is start date, 2 is day after, etc.)
   */
  [[nodiscard]] static vector<int> outputDateOffsets();
  /**
   * \brief Set days to output probability contours for (1 is start date, 2 is day after, etc.)
   * \return None
   */
  static void setOutputDateOffsets(const string value);
  /**
   * \brief Whatever the maximum value in the date offsets is
   * \return Whatever the maximum value in the date offsets is
   */
  [[nodiscard]] static int maxDateOffset() noexcept;
  Settings() = delete;
};
}
#endif
