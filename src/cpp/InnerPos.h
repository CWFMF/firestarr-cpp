/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "stdafx.h"
#include "Cell.h"

namespace tbd
{
using topo::Location;
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
  inline constexpr S x() const noexcept
  {
    return x_y_.first;
  }
  /**
   * \brief Y direction (row)
   */
  inline constexpr S y() const noexcept
  {
    return x_y_.second;
  }
  constexpr BoundedPoint(
    const S x,
    const S y) noexcept
    : x_y_(x, y)
  {
#ifdef DEBUG_GRIDS
    logging::check_fatal(
      (INVALID_Y != y) && (y < YMin || y >= YMax),
      "y %f is out of bounds (%d, %d)",
      y,
      YMin,
      YMax);
    logging::check_fatal(
      (INVALID_X != x) && (x < XMin || x >= XMax),
      "x %f is out of bounds (%d, %d)",
      x,
      XMin,
      XMax);
    logging::check_equal(x_, x, "x_");
    logging::check_equal(y_, y, "y_");
#endif
  }
  constexpr BoundedPoint() noexcept
    : x_y_(XMin - 1, YMin - 1)
  {
  }
  constexpr BoundedPoint(class_type&& rhs) noexcept
    : x_y_(std::move(rhs.x_y_))
  {
  }
  constexpr BoundedPoint(const class_type& rhs) noexcept
    : x_y_(rhs.x_y_)
  {
  }
  class_type& operator=(const class_type& rhs) noexcept
  {
    x_y_ = rhs.x_y_;
    return *this;
  }
  class_type& operator=(class_type&& rhs) noexcept
  {
    x_y_ = rhs.x_y_;
    return *this;
  }
  bool operator<(const class_type& rhs) const noexcept
  {
    return x_y_ < rhs.x_y_;
  }
  /**
   * \brief Equality operator
   * \param rhs BoundedPoint to compare to
   * \return Whether or not this is equivalent to the other
   */
  bool operator==(const class_type& rhs) const noexcept
  {
    return x_y_ == rhs.x_y_;
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
  // NOTE: expecting comparison of a pair to be quicker than two variables
  pair<S, S> x_y_;
};
/**
 * \brief Offset from a position
 */
class Offset
  : public BoundedPoint<DistanceSize, -1, 1, -1, 1>
{
public:
  /**
   * \brief Collection of Offsets
   */
  using OffsetSet = vector<Offset>;
  using BoundedPoint<DistanceSize, -1, 1, -1, 1>::BoundedPoint;
};
using OffsetSet = Offset::OffsetSet;
}
namespace tbd::sim
{
/**
 * \brief The position within a Cell that a spreading point has.
 */
class InnerPos
  : public BoundedPoint<InnerSize, 0, 1, 0, 1>
{
  using BoundedPoint<InnerSize, 0, 1, 0, 1>::BoundedPoint;
};
/**
 * \brief The position within the Environment that a spreading point has.
 */
class XYPos
  : public BoundedPoint<XYSize, 0, MAX_COLUMNS, 0, MAX_ROWS>
{
  using BoundedPoint<XYSize, 0, MAX_COLUMNS, 0, MAX_ROWS>::BoundedPoint;
};
}
