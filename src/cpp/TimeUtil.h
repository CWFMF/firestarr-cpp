/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2024-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_TIMEUTIL_H
#define FS_TIMEUTIL_H

#include <odbcinst.h>
#include <odbcinstext.h>
namespace fs
{
namespace util
{
/**
 * \brief Equality operator
 * \param x First TIMESTAMP_STRUCT
 * \param y Second TIMESTAMP_STRUCT
 * \return Whether or not they are equivalent
 */
[[nodiscard]] constexpr bool
operator==(
  const TIMESTAMP_STRUCT& x,
  const TIMESTAMP_STRUCT& y
) noexcept
{
  return x.year == y.year && x.month == y.month && x.day == y.day && x.hour == y.hour
      && x.minute == y.minute && x.second == y.second && x.fraction == y.fraction;
}
/**
 * \brief Less than operator
 * \param x First TIMESTAMP_STRUCT
 * \param y Second TIMESTAMP_STRUCT
 * \return Whether or not the first is less than the second
 */
[[nodiscard]] constexpr bool
operator<(
  const TIMESTAMP_STRUCT& x,
  const TIMESTAMP_STRUCT& y
) noexcept
{
  if (x.year == y.year)
  {
    if (x.month == y.month)
    {
      if (x.day == y.day)
      {
        if (x.minute == y.minute)
        {
          if (x.second == y.second)
          {
            return x.fraction < y.fraction;
          }
          return x.second < y.second;
        }
        return x.minute < y.minute;
      }
      return x.day < y.day;
    }
    return x.month < y.month;
  }
  return x.year < y.year;
}
/**
 * \brief Greater than operator
 * \param x First TIMESTAMP_STRUCT
 * \param y Second TIMESTAMP_STRUCT
 * \return Whether or not the first is greater than the second
 */
[[nodiscard]] constexpr bool
operator>(
  const TIMESTAMP_STRUCT& x,
  const TIMESTAMP_STRUCT& y
) noexcept
{
  return !(x == y || x < y);
}
/**
 * \brief Convert to local time
 * \param s Input value
 * \param t Output value
 */
void
to_tm(const TIMESTAMP_STRUCT& s, tm* t) noexcept;
/**
 * \brief Convert to GMT
 * \param s Input value
 * \param t Output value
 */
void
to_tm_gm(const TIMESTAMP_STRUCT& s, tm* t) noexcept;
/**
 * \brief Convert to TIMESTAMP_STRUCT
 * \param t Input value
 * \param s Output value
 */
void
to_ts(const tm& t, TIMESTAMP_STRUCT* s) noexcept;
}
}
#endif
