/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_PROBABILITYMAP_H
#define FS_PROBABILITYMAP_H
#include "stdafx.h"
#include "GridMap.h"
#include "Perimeter.h"
#include "Statistics.h"
#include "Util.h"
namespace fs
{
class Model;
class IntensityMap;
/**
 * \brief Map of the percentage of simulations in which a Cell burned in each intensity category.
 */
class ProbabilityMap
{
public:
  ProbabilityMap() = delete;
  ~ProbabilityMap();
  ProbabilityMap(const ProbabilityMap& rhs) noexcept = delete;
  ProbabilityMap(ProbabilityMap&& rhs) noexcept = delete;
  ProbabilityMap& operator=(const ProbabilityMap& rhs) noexcept = delete;
  ProbabilityMap& operator=(ProbabilityMap&& rhs) noexcept = delete;
  /**
   * \brief Constructor
   * \param time Time in simulation this ProbabilityMap represents
   * \param start_time Start time of simulation
   * \param min_value Lower bound of 'low' intensity range
   * \param low_max Upper bound of 'low' intensity range
   * \param med_max Upper bound of 'moderate' intensity range
   * \param max_value Upper bound of 'high' intensity range
   * \param grid_info GridBase to use for extent of this
   */
  ProbabilityMap(
    DurationSize time,
    DurationSize start_time,
    int min_value,
    int low_max,
    int med_max,
    int max_value,
    const GridBase& grid_info
  );
  /**
   * \brief Assign perimeter to use for marking cells as initial perimeter
   * \param perimeter Ignition grid to store for marking in outputs
   */
  void setPerimeter(const Perimeter* const perimeter);
  /**
   * \brief Combine results from another ProbabilityMap into this one
   * \param rhs ProbabilityMap to combine from
   */
  void addProbabilities(const ProbabilityMap& rhs);
  /**
   * \brief Add in an IntensityMap to the appropriate probability grid based on each cell burn
   * intensity
   * \param for_time IntensityMap to add results from
   */
  void addProbability(const IntensityMap& for_time);
  /**
   * \brief Output Statistics to log
   */
  void show() const;
  /**
   * \brief Save total, low, moderate, and high maps, and output information to log
   * \param start_time Start time of simulation
   * \param time Time for these maps
   */
  [[nodiscard]] FileList saveAll(
    string_view output_directory,
    const tm& start_time,
    DurationSize time,
    const bool is_interim
  ) const;
  /**
   * \brief Clear maps and return to initial state
   */
  void reset();
  /**
   * Delete interim output files
   */
  static void deleteInterim();

private:
  /**
   * \brief Create a copy of this that is empty
   * \return New empty Probability with same range bounds and times
   */
  ProbabilityMap* copyEmpty() const;
  /**
   * \brief List of sizes of IntensityMaps that have been added
   * \return List of sizes of IntensityMaps that have been added
   */
  [[nodiscard]] vector<MathSize> getSizes() const;
  /**
   * \brief Generate Statistics on sizes of IntensityMaps that have been added
   * \return Generate Statistics on sizes of IntensityMaps that have been added
   */
  [[nodiscard]] Statistics getStatistics() const;
  /**
   * \brief Number of sizes that have been added
   * \return Number of sizes that have been added
   */
  [[nodiscard]] size_t numSizes() const noexcept;
  /**
   * \brief Save list of sizes
   * \param output_directory Directory to save to
   * \param base_name Base name of file to save into
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList saveSizes(const string_view output_directory, const string_view base_name)
    const;
  /**
   * \brief Save map representing all intensities
   * \param output_directory Directory to save to
   * \param base_name Base file name to save to
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList saveTotal(
    const string_view output_directory,
    const string_view base_name,
    const bool is_interim
  ) const;
  /**
   * \brief Save map representing all intensities occurrence
   * \param output_directory Directory to save to
   * \param base_name Base file name to save to
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList saveTotalCount(
    const string_view output_directory,
    const string_view base_name
  ) const;
  /**
   * \brief Save map representing high intensities
   * \param output_directory Directory to save to
   * \param base_name Base file name to save to
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList saveHigh(const string_view output_directory, const string_view base_name)
    const;
  /**
   * \brief Save map representing moderate intensities
   * \param output_directory Directory to save to
   * \param base_name Base file name to save to
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList saveModerate(
    const string_view output_directory,
    const string_view base_name
  ) const;
  /**
   * \brief Save map representing low intensities
   * \param output_directory Directory to save to
   * \param base_name Base file name to save to
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList saveLow(const string_view output_directory, const string_view base_name)
    const;

private:
  /**
   * \brief Make note of any interim files for later deletion
   */
  [[nodiscard]] FileList record_if_interim(FileList&& files) const;
  /**
   * \brief Save probability file and record filename if interim
   * \return Path for file that was written
   */
  template <class R>
  [[nodiscard]] FileList saveToProbabilityFile(
    const GridMap<size_t>& grid,
    const string_view dir,
    const string_view base_name,
    const R divisor
  ) const
  {
    return record_if_interim(grid.saveToProbabilityFile(dir, base_name, divisor));
  };
  /**
   * \brief Map representing all intensities
   */
  GridMap<size_t> all_;
  /**
   * \brief Map representing high intensities
   */
  GridMap<size_t> high_;
  /**
   * \brief Map representing moderate intensities
   */
  GridMap<size_t> med_;
  /**
   * \brief Map representing low intensities
   */
  GridMap<size_t> low_;
  /**
   * \brief List of sizes for perimeters that have been added
   */
  vector<MathSize> sizes_{};
  /**
   * \brief Time in simulation this ProbabilityMap represents
   */
  const DurationSize time_;
  /**
   * \brief Start time of simulation
   */
  const DurationSize start_time_;
  /**
   * \brief Mutex for parallel access
   */
  mutable mutex mutex_;
  /**
   * \brief Lower bound of 'low' intensity range
   */
  IntensitySize min_value_;
  /**
   * \brief Upper bound of 'high' intensity range
   */
  IntensitySize max_value_;
  /**
   * \brief Upper bound of 'low' intensity range
   */
  const IntensitySize low_max_;
  /**
   * \brief Upper bound of 'moderate' intensity range
   */
  const IntensitySize med_max_;
  /**
   * \brief Initial ignition grid to apply to outputs
   */
  const Perimeter* perimeter_{nullptr};
};
}
#endif
