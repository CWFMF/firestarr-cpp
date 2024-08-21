/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_INNERPOS_H
#define FS_INNERPOS_H
#include "stdafx.h"
#include "Cell.h"
namespace fs
{
template <class S, int XMin, int XMax, int YMin, int YMax>
class BoundedPoint
{
protected:
  using class_type = BoundedPoint<S, XMin, XMax, YMin, YMax>;
  static constexpr auto INVALID_X = XMin - 1;
  static constexpr auto INVALID_Y = YMin - 1;

public:
  /**
   * \brief X direction (column)
   */
  inline constexpr S x() const noexcept { return x_; }
  /**
   * \brief Y direction (row)
   */
  inline constexpr S y() const noexcept { return y_; }
  constexpr BoundedPoint(const S x, const S y) noexcept : x_(x), y_(y)
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(
      (INVALID_Y != y) && (y < YMin || y >= YMax), "y %f is out of bounds (%d, %d)", y, YMin, YMax
    );
    logging::check_fatal(
      (INVALID_X != x) && (x < XMin || x >= XMax), "x %f is out of bounds (%d, %d)", x, XMin, XMax
    );
    logging::check_equal(x_, x, "x_");
    logging::check_equal(y_, y, "y_");
#endif
  }
  constexpr BoundedPoint() noexcept : x_(XMin - 1), y_(YMin - 1) { }
  constexpr BoundedPoint(class_type&& rhs) noexcept : BoundedPoint(rhs.x(), rhs.y()) { }
  constexpr BoundedPoint(const class_type& rhs) noexcept : BoundedPoint(rhs.x(), rhs.y()) { }
  class_type& operator=(const class_type& rhs) noexcept
  {
    x_ = rhs.x();
    y_ = rhs.y();
    return *this;
  }
  class_type& operator=(class_type&& rhs) noexcept
  {
    x_ = rhs.x();
    y_ = rhs.y();
    return *this;
  }
  /**
   * \brief Multiply by duration to get total offset over time
   * \param duration time to multiply by
   */
  template <class T>
  constexpr T after(const DurationSize duration) const noexcept
  {
    return static_cast<T>(class_type(x() * duration, y() * duration));
  }
  /**
   * \brief Less than operator
   * \param rhs BoundedPoint to compare to
   * \return Whether or not this is less than the other
   */
  bool operator<(const class_type& rhs) const noexcept
  {
    if (x() == rhs.x())
    {
      if (y() == rhs.y())
      {
        // they are "identical" so this is false
        return false;
      }
      return y() < rhs.y();
    }
    return x() < rhs.x();
  }
  /**
   * \brief Equality operator
   * \param rhs BoundedPoint to compare to
   * \return Whether or not this is equivalent to the other
   */
  bool operator==(const class_type& rhs) const noexcept
  {
    return (x() == rhs.x()) && (y() == rhs.y());
  }
  /**
   * \brief Add offset to position and return result
   */
  template <class T, class O>
  [[nodiscard]] constexpr T add(const O& o) const noexcept
  {
    return static_cast<T>(class_type(x() + o.x(), y() + o.y()));
  }

private:
  S x_;
  S y_;
};
/**
 * \brief Offset from a position
 */
class Offset : public BoundedPoint<DistanceSize, -1, 1, -1, 1>
{
public:
  /**
   * \brief Collection of Offsets
   */
  using OffsetSet = vector<Offset>;
  using BoundedPoint<DistanceSize, -1, 1, -1, 1>::BoundedPoint;
};
using OffsetSet = Offset::OffsetSet;
// define multiplication in other order since equivalent
template <class T>
constexpr T after(const DurationSize duration, const T& o)
{
  return o.after(duration);
}
/**
 * \brief The position within a Cell that a spreading point has.
 */
class InnerPos : public BoundedPoint<InnerSize, 0, 1, 0, 1>
{
  using BoundedPoint<InnerSize, 0, 1, 0, 1>::BoundedPoint;
};
/**
 * \brief The position within the Environment that a spreading point has.
 */
class XYPos : public BoundedPoint<XYSize, 0, MAX_COLUMNS, 0, MAX_ROWS>
{
  using BoundedPoint<XYSize, 0, MAX_COLUMNS, 0, MAX_ROWS>::BoundedPoint;
};
static constexpr MathSize x(const auto& p) { return p.x(); }
static constexpr MathSize y(const auto& p) { return p.y(); }
}
#endif
