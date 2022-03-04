/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_INNERPOS_H
#define FS_INNERPOS_H
#include "stdafx.h"
#include "Log.h"
namespace fs
{
/**
 * \brief Offset from a position
 */
struct Offset
{
public:
  /**
   * \brief Offset in the x direction (column)
   */
  const double x;
  /**
   * \brief Offset in the y direction (row)
   */
  const double y;
  constexpr Offset(const double a, const double b) noexcept : x(a), y(b) { }
};
/**
 * \brief Collection of Offsets
 */
using OffsetSet = vector<Offset>;
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
   * \brief Less than operator
   * \param rhs InnerPos to compare to
   * \return Whether or not this is less than the other
   */
  bool operator<(const InnerPos& rhs) const noexcept
  {
    if (x == rhs.x)
    {
      if (y == rhs.y)
      {
        // they are "identical" so this is false
        return false;
      }
      return y < rhs.y;
    }
    return x < rhs.x;
  }
  /**
   * \brief Equality operator
   * \param rhs InnerPos to compare to
   * \return Whether or not this is equivalent to the other
   */
  bool operator==(const InnerPos& rhs) const noexcept { return (x == rhs.x) && (y == rhs.y); }
  /**
   * \brief Add offset to position and return result
   */
  [[nodiscard]] constexpr InnerPos add(const Offset o) const noexcept
  {
    return InnerPos(x + o.x, y + o.y);
  }
  /**
   * \brief Constructor
   * \param x X coordinate
   * \param y Y coordinate
   */
  constexpr InnerPos(const double x, const double y) noexcept : x(x), y(y) { }
};
static constexpr MathSize x(const auto& p) { return p.x; }
static constexpr MathSize y(const auto& p) { return p.y; }
}
#endif
