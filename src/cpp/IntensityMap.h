/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_INTENSITYMAP_H
#define FS_INTENSITYMAP_H
#include "stdafx.h"
#include "BurnedData.h"
#include "GridMap.h"
namespace fs
{
class Perimeter;
class Cell;
class ProbabilityMap;
class Model;
/**
 * \brief Represents a map of intensities that cells have burned at for a single Scenario.
 */
class IntensityMap
{
  /**
   * \brief Mutex for parallel access
   */
  mutable mutex mutex_{};

public:
  explicit IntensityMap(const Model& model) noexcept;
  ~IntensityMap() noexcept = default;
  IntensityMap(const IntensityMap& rhs);
  IntensityMap(IntensityMap&& rhs) = delete;
  IntensityMap& operator=(const IntensityMap& rhs) = delete;
  IntensityMap& operator=(IntensityMap&& rhs) noexcept = delete;
  /**
   * \brief Number of rows in this extent
   * \return Number of rows in this extent
   */
  [[nodiscard]] Idx rows() const { return map_.rows(); }
  /**
   * \brief Number of columns in this extent
   * \return Number of columns in this extent
   */
  [[nodiscard]] Idx columns() const { return map_.columns(); }
  /**
   * \brief Set cells in the map to be burned based on Perimeter
   * \param perimeter Perimeter to burn cells based on
   */
  void applyPerimeter(const Perimeter& perimeter) noexcept;
  /**
   * \brief Whether or not the Cell can burn
   * \param location Cell to check
   * \return Whether or not the Cell can burn
   */
  [[nodiscard]] bool canBurn(const Cell& location) const;
  /**
   * \brief Whether or not the Location can burn
   * \param location Location to check
   * \return Whether or not the Location can burn
   */
  [[nodiscard]] bool hasBurned(const Location& location) const;
  /**
   * \brief Whether or not all Locations surrounding the given Location are burned
   * \param location Location to check
   * \return Whether or not all Locations surrounding the given Location are burned
   */
  [[nodiscard]] bool isSurrounded(const Location& location) const;
  /**
   * \brief Burn Location with given intensity
   * \param location Location to burn
   * \param intensity Intensity to burn with (kW/m)
   */
  void burn(const Location& location, IntensitySize intensity);
  /**
   * \brief Save contents to an ASCII file
   * \param dir Directory to save to
   * \param base_name Base file name to save to
   * \return FileList of file names saved to
   */
  [[nodiscard]] FileList save(const string_view dir, const string_view base_name) const;
  /**
   * \brief Size of the fire represented by this
   * \return Size of the fire represented by this
   */
  [[nodiscard]] double fireSize() const;
  /**
   * \brief Iterator for underlying GridMap
   * \return Iterator for underlying GridMap
   */
  [[nodiscard]] map<Location, IntensitySize>::const_iterator cbegin() const noexcept;
  /**
   * \brief Iterator for underlying GridMap
   * \return Iterator for underlying GridMap
   */
  [[nodiscard]] map<Location, IntensitySize>::const_iterator cend() const noexcept;

private:
  /**
   * \brief Model map is for
   */
  const Model& model_;
  /**
   * \brief Map of intensity that cells have burned  at
   */
  GridMap<IntensitySize> map_;
  /**
   * \brief bitset denoting cells that can no longer burn
   */
  BurnedData is_burned_{};
};
}
#endif
