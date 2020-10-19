/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_POINT_H
#define FS_POINT_H
#include "stdafx.h"
namespace fs
{
/**
 * \brief A geographic location in lat/long coordinates.
 */
class Point
{
public:
  /**
   * \brief Constructor
   * \param latitude Latitude (decimal degrees)
   * \param longitude Longitude (decimal degrees)
   */
  constexpr Point(const MathSize latitude, const MathSize longitude) noexcept
    : latitude_(latitude), longitude_(longitude)
  { }
  ~Point() noexcept = default;
  Point(const Point& rhs) noexcept = default;
  Point(Point&& rhs) noexcept = default;
  Point& operator=(const Point& rhs) noexcept = default;
  Point& operator=(Point&& rhs) noexcept = default;
  /**
   * \brief Latitude (decimal degrees)
   * \return Latitude (degrees)
   */
  [[nodiscard]] constexpr MathSize latitude() const noexcept { return latitude_; }
  /**
   * \brief Longitude (decimal degrees)
   * \return Longitude (decimal degrees)
   */
  [[nodiscard]] constexpr MathSize longitude() const noexcept { return longitude_; }

private:
  /**
   * \brief Latitude (decimal degrees)
   */
  MathSize latitude_;
  /**
   * \brief Longitude (decimal degrees)
   */
  MathSize longitude_;
};
}
#endif
