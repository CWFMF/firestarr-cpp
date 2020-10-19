/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_UTM_H
#define FS_UTM_H
#include "stdafx.h"
namespace fs
{
class Point;
/**
 * \brief Calculate the UTM zone for the given meridian
 * \param meridian A MathSize designating the meridian to calculate the UTM zone for (degrees)
 * \return UTM zone for given meridian
 */
[[nodiscard]] constexpr MathSize meridian_to_zone(const MathSize meridian) noexcept
{
  return (meridian + 183.0) / 6.0;
}
/**
 * \brief Determines the central meridian for the given UTM zone.
 * \param zone A MathSize designating the UTM zone, range [1,60].
 * \return The central meridian for the given UTM zone (degrees)
 */
[[nodiscard]] constexpr MathSize utm_central_meridian(const MathSize zone) noexcept
{
  return -183.0 + zone * 6.0;
}
void from_lat_long(const string_view proj4, const Point& point, MathSize* x, MathSize* y);
Point to_lat_long(const string_view proj4, const MathSize x, const MathSize y);
string try_fix_meridian(const string_view proj4);
}
#endif
