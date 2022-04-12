/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_STARTUP_H
#define FS_STARTUP_H

#include "stdafx.h"
#include "FWI.h"
#include "Point.h"
#include "Weather.h"
namespace fs
{
namespace wx
{
/**
 * \brief Startup values to initialize a weather stream calculation with.
 */
class Startup
{
public:
  /**
   * \brief Station providing the Startup values
   * \return Station providing the Startup values
   */
  [[nodiscard]] constexpr const string&
  station() const noexcept
  {
    return station_;
  }
  /**
   * \brief Time the Startup value is for
   * \return Time the Startup value is for
   */
  [[nodiscard]] constexpr const tm&
  generated() const noexcept
  {
    return generated_;
  }
  /**
   * \brief Point the Startup value is for
   * \return Point the Startup value is for
   */
  [[nodiscard]] constexpr const topo::Point&
  point() const noexcept
  {
    return point_;
  }
  /**
   * \brief Distance Startup value location is from the requested location (m)
   * \return Distance Startup value location is from the requested location (m)
   */
  [[nodiscard]] constexpr double
  distanceFrom() const noexcept
  {
    return distance_from_;
  }
  /**
   * \brief Fine Fuel Moisture Code
   * \return Fine Fuel Moisture Code
   */
  [[nodiscard]] constexpr const Ffmc&
  ffmc() const noexcept
  {
    return ffmc_;
  }
  /**
   * \brief Duff Moisture Code
   * \return Duff Moisture Code
   */
  [[nodiscard]] constexpr const Dmc&
  dmc() const noexcept
  {
    return dmc_;
  }
  /**
   * \brief Drought Code
   * \return Drought Code
   */
  [[nodiscard]] constexpr const Dc&
  dc() const noexcept
  {
    return dc_;
  }
  /**
   * \brief Accumulated Precipitation at 0800 (mm)
   * \return Accumulated Precipitation at 0800 (mm)
   */
  [[nodiscard]] constexpr const AccumulatedPrecipitation&
  apcp0800() const noexcept
  {
    return apcp_0800_;
  }
  /**
   * \brief Whether or not any Startup values were overridden
   * \return Whether or not any Startup values were overridden
   */
  [[nodiscard]] constexpr bool
  isOverridden() const noexcept
  {
    return is_overridden_;
  }
  /**
   * \brief Constructor
   * \param station Station indices are from
   * \param generated Date/Time indices are from
   * \param point Point indices were requested for
   * \param distance_from Distance from requested point the weather station is (m)
   * \param ffmc Fine Fuel Moisture Code
   * \param dmc Duff Moisture Code
   * \param dc Drought Code
   * \param apcp_0800 Accumulated Precipitation at 0800 (mm)
   * \param overridden whether or not any Startup values were overridden
   */
  Startup(
    string station,
    const tm& generated,
    const topo::Point& point,
    double distance_from,
    const Ffmc& ffmc,
    const Dmc& dmc,
    const Dc& dc,
    const AccumulatedPrecipitation& apcp_0800,
    bool overridden
  ) noexcept;
  /**
   * \brief Move constructor
   * \param rhs Startup to move from
   */
  Startup(Startup&& rhs) noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs Startup to copy from
   */
  Startup(const Startup& rhs) = default;
  /**
   * \brief Move assignment operator
   * \param rhs Startup to move from
   * \return This, after assignment
   */
  Startup&
  operator=(Startup&& rhs) noexcept = default;
  /**
   * \brief Copy assignment operator
   * \param rhs Startup to copy from
   * \return This, after assignment
   */
  Startup&
  operator=(const Startup& rhs) = default;
  /**
   * \brief Destructor
   */
  ~Startup() = default;
private:
  /**
   * \brief Station indices are from
   */
  string station_;
  /**
   * \brief When these indices were observed
   */
  tm generated_;
  /**
   * \brief Point this represents
   */
  topo::Point point_;
  /**
   * \brief Distance actual point for this is from represented Point (m)
   */
  double distance_from_;
  /**
   * \brief Fine Fuel Moisture Code
   */
  Ffmc ffmc_;
  /**
   * \brief Duff Moisture Code
   */
  Dmc dmc_;
  /**
   * \brief Drought code
   */
  Dc dc_;
  /**
   * \brief Accumulated Precipitation at 0800 (mm)
   */
  AccumulatedPrecipitation apcp_0800_;
  /**
   * \brief Whether or not any of the indices have been overridden
   */
  bool is_overridden_;
};
}
}
#endif
