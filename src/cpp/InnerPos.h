/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_INNERPOS_H
#define FS_INNERPOS_H

#include "Settings.h"

namespace fs
{
namespace sim
{
/**
 * \brief The position within a Cell that a spreading point has.
 */
struct InnerPos
{
  /**
   * \brief X coordinate
   */
  double x;
  /**
   * \brief Y coordinate
   */
  double y;
  /**
   * \brief Constructor
   * \param x X coordinate
   * \param y Y coordinate
   */
  InnerPos(
    const double x,
    const double y
  ) noexcept
    : x(x),
      y(y)
  {
  }
  /**
   * \brief Less than operator
   * \param rhs InnerPos to compare to
   * \return Whether or not this is less than the other
   */
  bool
  operator<(
    const InnerPos& rhs
  ) const noexcept
  {
    if (x == rhs.x)
    {
      return y < rhs.y;
    }
    return x < rhs.x;
  }
  /**
   * \brief Equality operator
   * \param rhs InnerPos to compare to
   * \return Whether or not this is equivalent to the other
   */
  bool
  operator==(
    const InnerPos& rhs
  ) const noexcept
  {
    return abs(x - rhs.x) < COMPARE_LIMIT && abs(y - rhs.y) < COMPARE_LIMIT;
  }
};
}
}
#endif
