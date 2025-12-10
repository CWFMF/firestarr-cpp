/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_STARTUP_H
#define FS_STARTUP_H
#include "stdafx.h"
#include "FWI.h"
#include "Point.h"
#include "Weather.h"
namespace fs
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
  [[nodiscard]] constexpr const string_view station() const noexcept { return station_; }
  /**
   * \brief Time the Startup value is for
   * \return Time the Startup value is for
   */
  [[nodiscard]] constexpr const tm& generated() const noexcept { return generated_; }
  /**
   * \brief Point the Startup value is for
   * \return Point the Startup value is for
   */
  [[nodiscard]] constexpr const Point& point() const noexcept { return point_; }
  /**
   * \brief Distance Startup value location is from the requested location (m)
   * \return Distance Startup value location is from the requested location (m)
   */
  [[nodiscard]] constexpr MathSize distanceFrom() const noexcept { return distance_from_; }
  /**
   * \brief Accumulated Precipitation from noon yesterday to start of hourly weather (mm)
   * \return Accumulated Precipitation from noon yesterday to start of hourly weather (mm)
   */
  [[nodiscard]] constexpr const Precipitation& apcpPrev() const noexcept { return apcp_prev; }
  /**
   * \brief Whether or not any Startup values were overridden
   * \return Whether or not any Startup values were overridden
   */
  [[nodiscard]] constexpr bool isOverridden() const noexcept { return is_overridden_; }
  /**
   * \brief Constructor
   * \param station Station indices are from
   * \param generated Date/Time indices are from
   * \param point Point indices were requested for
   * \param distance_from Distance from requested point the weather station is (m)
   * \param ffmc Fine Fuel Moisture Code
   * \param dmc Duff Moisture Code
   * \param dc Drought Code
   * \param apcp_prev Accumulated Precipitation from noon yesterday to start of hourly weather (mm)
   * \param overridden whether or not any Startup values were overridden
   */
  Startup(
    string station,
    const tm& generated,
    const Point& point,
    MathSize distance_from,
    const Ffmc& ffmc,
    const Dmc& dmc,
    const Dc& dc,
    const Precipitation& apcp_prev,
    bool overridden
  ) noexcept;
  Startup(Startup&& rhs) noexcept = default;
  Startup(const Startup& rhs) = default;
  Startup& operator=(Startup&& rhs) noexcept = default;
  Startup& operator=(const Startup& rhs) = default;
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
  Point point_;
  /**
   * \brief Distance actual point for this is from represented Point (m)
   */
  MathSize distance_from_;

public:
  /**
   * \brief Fine Fuel Moisture Code
   */
  Ffmc ffmc;
  /**
   * \brief Duff Moisture Code
   */
  Dmc dmc;
  /**
   * \brief Drought code
   */
  Dc dc;
  /**
   * \brief Accumulated Precipitation from noon yesterday to start of hourly weather (mm)
   */
  Precipitation apcp_prev;

private:
  /**
   * \brief Whether or not any of the indices have been overridden
   */
  bool is_overridden_;
};
}
#endif
