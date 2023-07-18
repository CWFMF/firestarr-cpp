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

#pragma once
#include <memory>
#include <string>
#include "Environment.h"
#include "Grid.h"
namespace tbd::topo
{
/**
 * \brief Information regarding an Environment, such as grids to read and location.
 */
class EnvironmentInfo
{
public:
  /**
   * \brief Load EnvironmentInfo from given rasters
   * \param in_fuel Fuel raster
   * \param in_elevation Elevation raster
   * \return EnvironmentInfo
   */
  [[nodiscard]] static unique_ptr<EnvironmentInfo> loadInfo(const string& in_fuel,
                                                            const string& in_elevation);
  ~EnvironmentInfo();
  /**
   * \brief Construct from given rasters
   * \param in_fuel Fuel raster
   * \param in_elevation Elevation raster
   */
  EnvironmentInfo(const string& in_fuel,
                  const string& in_elevation);
  /**
   * \brief Move constructor
   * \param rhs EnvironmentInfo to move from
   */
  EnvironmentInfo(EnvironmentInfo&& rhs) noexcept = default;
  EnvironmentInfo(const EnvironmentInfo& rhs) = delete;
  /**
   * \brief Move assignment
   * \param rhs EnvironmentInfo to move from
   * \return This, after assignment
   */
  EnvironmentInfo& operator=(EnvironmentInfo&& rhs) noexcept = default;
  EnvironmentInfo& operator=(const EnvironmentInfo& rhs) = delete;
  /**
   * \brief Determine Coordinates in the grid for the Point
   * \param point Point to find Coordinates for
   * \param flipped Whether the grid data is flipped across the horizontal axis
   * \return Coordinates that would be at Point within this EnvironmentInfo, or nullptr if it is not
   */
  [[nodiscard]] unique_ptr<Coordinates> findCoordinates(
    const Point& point,
    bool flipped) const;
  /**
   * \brief Determine FullCoordinates in the grid for the Point
   * \param point Point to find FullCoordinates for
   * \param flipped Whether the grid data is flipped across the horizontal axis
   * \return Coordinates that would be at Point within this EnvironmentInfo, or nullptr if it is not
   */
  [[nodiscard]] unique_ptr<FullCoordinates> findFullCoordinates(
    const Point& point,
    bool flipped) const;
  /**
   * \brief Load the full Environment using the given FuelLookup to determine fuels
   * \param point Origin Point
   * \return
   */
  [[nodiscard]] Environment load(const Point& point) const;
  /**
   * \brief Number of rows in grid
   * \return Number of rows in grid
   */
  [[nodiscard]] constexpr FullIdx calculateRows() const
  {
    return fuel_.calculateRows();
  }
  /**
   * \brief Number of columns in grid
   * \return Number of columns in grid
   */
  [[nodiscard]] constexpr FullIdx calculateColumns() const
  {
    return fuel_.calculateColumns();
  }
  /**
   * \brief Central meridian of UTM projection this uses
   * \return Central meridian of UTM projection this uses
   */
  [[nodiscard]] constexpr double meridian() const
  {
    return fuel_.meridian();
  }
  /**
   * \brief UTM zone for projection this uses
   * \return UTM zone for projection this uses
   */
  [[nodiscard]] constexpr double zone() const
  {
    return fuel_.zone();
  }
  /**
   * \brief UTM projection that this uses
   * \return UTM projection that this uses
   */
  [[nodiscard]] constexpr const string& proj4() const
  {
    return fuel_.proj4();
  }
private:
  /**
   * \brief Information about fuel raster
   */
  data::GridBase fuel_;
  /**
   * \brief Information about elevation raster
   */
  data::GridBase elevation_;
  /**
   * \brief Fuel raster path
   */
  string in_fuel_;
  /**
   * \brief Elevation raster path
   */
  string in_elevation_;
  /**
   * \brief Constructor
   * \param in_fuel Fuel raster path
   * \param in_elevation Elevation raster path
   * \param fuel Information about fuel raster
   * \param elevation Information about elevation raster
   */
  EnvironmentInfo(string in_fuel,
                  string in_elevation,
                  data::GridBase&& fuel,
                  data::GridBase&& elevation) noexcept;
};
}
