/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <memory>
#include <string>
#include <bitset>
#include "GridMap.h"
namespace tbd
{
namespace topo
{
class Perimeter;
class Cell;
}
namespace sim
{
class ProbabilityMap;
class Model;
using BurnedData = std::bitset<static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS>;
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
  /**
   * \brief Constructor
   * \param model Model to use extent from
   */
  // IntensityMap(const Model& model, topo::Perimeter* perimeter) noexcept;
  explicit IntensityMap(const Model& model) noexcept;
  ~IntensityMap() noexcept;
  IntensityMap(const IntensityMap& rhs);
  IntensityMap(IntensityMap&& rhs) = delete;
  // IntensityMap(IntensityMap&& rhs);
  IntensityMap& operator=(const IntensityMap& rhs) = delete;
  IntensityMap& operator=(IntensityMap&& rhs) noexcept = delete;
  /**
   * \brief Number of rows in this extent
   * \return Number of rows in this extent
   */
  [[nodiscard]] Idx rows() const
  {
    return intensity_max_->rows();
  }
  /**
   * \brief Number of columns in this extent
   * \return Number of columns in this extent
   */
  [[nodiscard]] Idx columns() const
  {
    return intensity_max_->columns();
  }
  /**
   * \brief Set cells in the map to be burned based on Perimeter
   * \param perimeter Perimeter to burn cells based on
   */
  void applyPerimeter(const topo::Perimeter& perimeter) noexcept;
  /**
   * \brief Whether or not the Cell can burn
   * \param location Cell to check
   * \return Whether or not the Cell can burn
   */
  [[nodiscard]] bool canBurn(const topo::Cell& location) const;
  /**
   * \brief Whether or not the Cell with the given hash can burn
   * \param hash Hash for Cell to check
   * \return Whether or not the Cell with the given hash can burn
   */
  //  [[nodiscard]] bool canBurn(HashSize hash) const;
  /**
   * \brief Whether or not the Location can burn
   * \param location Location to check
   * \return Whether or not the Location can burn
   */
  [[nodiscard]] bool hasBurned(const Location& location) const;
  /**
   * \brief Whether or not the Location with the given hash can burn
   * \param hash Hash for Location to check
   * \return Whether or not the Location with the given hash can burn
   */
  //  [[nodiscard]] bool hasBurned(HashSize hash) const;
  /**
   * \brief Whether or not all Locations surrounding the given Location are burned
   * \param location Location to check
   * \return Whether or not all Locations surrounding the given Location are burned
   */
  [[nodiscard]] bool isSurrounded(const Location& location) const;

  /**
   * \brief Update Location with specified values
   * \param location Location to burn
   * \param intensity Intensity to burn with (kW/m)
   * \param ros Rate of spread to check against maximu (m/min)
   * \param raz Spread azimuth for ros
   */
  void burn(const Location& location,
            IntensitySize intensity,
            double ros,
            tbd::wx::Direction raz);
  /**
   * \brief Mark given location as burned
   * \param location Location to burn
   */
  void ignite(const Location& location);
  // void update(const Location& location,
  //             const SpreadInfo& spread_info);
  /**
   * \brief Save contents to an ASCII file
   * \param dir Directory to save to
   * \param base_name Base file name to save to
   */
  void save(const string& dir, const string& base_name) const;
  /**
   * \brief Size of the fire represented by this
   * \return Size of the fire represented by this
   */
  [[nodiscard]] double fireSize() const;
  /**
   * \brief Iterator for underlying GridMap
   * \return Iterator for underlying GridMap
   */
  [[nodiscard]] map<Location, IntensitySize>::const_iterator
    cbegin() const noexcept;
  /**
   * \brief Iterator for underlying GridMap
   * \return Iterator for underlying GridMap
   */
  [[nodiscard]] map<Location, IntensitySize>::const_iterator
    cend() const noexcept;
private:
  /**
   * \brief Model map is for
   */
  const Model& model_;
  /**
   * \brief Map of intensity that cells have burned  at
   */
  unique_ptr<data::GridMap<IntensitySize>> intensity_max_;
  // HACK: just add ROS/RAZ into this object for now
  /**
   * \brief Map of rate of spread/direction that cells have burned with at max ros
   */
  unique_ptr<data::GridMap<double>> rate_of_spread_at_max_;
  unique_ptr<data::GridMap<DegreesSize>> direction_of_spread_at_max_;
  /**
   * \brief bitset denoting cells that can no longer burn
   */
  BurnedData* is_burned_;
};
}
}
