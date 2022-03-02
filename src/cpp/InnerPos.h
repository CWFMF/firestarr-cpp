/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
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
  Idx x;
  /**
   * \brief Y coordinate
   */
  Idx y;
  /**
   * \brief X location within cell
   */
  double sub_x;
  /**
   * \brief Y location within cell
   */
  double sub_y;
  /**
   * \brief Create InnerPos from (x, y) and offsets
   * \param x X coordinate
   * \param y Y coordinate
   * \param sub_x Sub-coordinate for X
   * \param sub_y Sub-coordinate for Y
   */
  constexpr static InnerPos create(Idx a, Idx b, double sub_a, double sub_b)
  {
    bool changed = true;
    while (changed)
    {
      changed = false;
      // HACK: rounding error means something + 1 can originally be >0 but then equal 1 exactly
      if (sub_a >= 1)
      {
        a += 1;
        sub_a -= 1;
        changed = true;
      }
      else if (sub_a < 0)
      {
        a -= 1;
        sub_a += 1;
        changed = true;
      }
      if (sub_b >= 1)
      {
        b += 1;
        sub_b -= 1;
        changed = true;
      }
      else if (sub_b < 0)
      {
        b -= 1;
        sub_b += 1;
        changed = true;
      }
    }
    return InnerPos(a, b, sub_a, sub_b);
  }
  /**
   * \brief Less than operator
   * \param rhs InnerPos to compare to
   * \return Whether or not this is less than the other
   */
  bool operator<(const InnerPos& rhs) const noexcept
  {
    if (x == rhs.x)
    {
      if (sub_x == rhs.sub_x)
      {
        if (y == rhs.y)
        {
          if (sub_y == rhs.sub_y)
          {
            // they are "identical" so this is false
            return false;
          }
          return sub_y < rhs.sub_y;
        }
        return y < rhs.y;
      }
      return sub_x < rhs.sub_x;
    }
    return x < rhs.x;
  }
  /**
   * \brief Equality operator
   * \param rhs InnerPos to compare to
   * \return Whether or not this is equivalent to the other
   */
  bool operator==(const InnerPos& rhs) const noexcept
  {
    return (x == rhs.x) && (y == rhs.y) && (sub_x == rhs.sub_x) && (sub_y == rhs.sub_y);
  }
  /**
   * \brief Add offset to position and return result
   */
  InnerPos add(const Offset o) const noexcept { return create(x, y, sub_x + o.x, sub_y + o.y); }
  /**
   * \brief Constructor
   * \param x X coordinate
   * \param y Y coordinate
   * \param sub_x X location within cell
   * \param sub_y Y location within cell
   */
  constexpr InnerPos(const Idx x, const Idx y, const double sub_x, const double sub_y) noexcept
    : x(x), y(y), sub_x(sub_x), sub_y(sub_y)
  {
    logging::check_fatal(
      sub_x >= 1 || sub_x < 0 || sub_y >= 1 || sub_y < 0,
      "Sub-coordinates (%f, %f) are outside cell",
      sub_x,
      sub_y
    );
  }
  /**
   * Copy constructor
   * @param p Object to copy values from
   */
  constexpr InnerPos(const InnerPos& p) noexcept : InnerPos(p.x, p.y, p.sub_x, p.sub_y) { }
};
static constexpr MathSize x(const auto& p) { return p.x; }
static constexpr MathSize y(const auto& p) { return p.y; }
}
#endif
