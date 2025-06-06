/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_INNERPOS_H
#define FS_INNERPOS_H

#include "stdafx.h"

#include "Location.h"
#include "Weather.h"

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
  constexpr BoundedPoint(
    const S& x,
    const S& y
  )
    : xy_(x, y)
  {
  }

  constexpr BoundedPoint(
    const class_type* p
  )
    : BoundedPoint(p->x(), p->y())
  {
  }

  constexpr BoundedPoint(
    const class_type& p
  )
    : BoundedPoint(p.x(), p.y())
  {
  }

  constexpr BoundedPoint() noexcept
    : BoundedPoint(XMin - 1, YMin - 1)
  {
  }

  constexpr class_type&
  operator=(
    const class_type& rhs
  )
  {
    xy_ = rhs.xy_;
    return *this;
  }

  /**
   * \brief Add offset to position and return result
   */
  template <class T, class O>
  [[nodiscard]] constexpr T
  add(
    const O& o
  ) const noexcept
  {
    return static_cast<T>(class_type(xy_.first + o.xy_.first, xy_.second + o.xy_.second));
  }

  inline auto&
  x() const
  {
    return xy_.first;
  }

  inline auto&
  y() const
  {
    return xy_.second;
  }

  CONSTEXPR Location
  location() const
  {
    // HACK: Location is (row, column) and this is (x, y)
    return {static_cast<Idx>(xy_.second), static_cast<Idx>(xy_.first)};
  }

  CONSTEXPR HashSize
  hash() const
  {
    return location().hash();
  }

  template <class T, int _XMin, int _XMax, int _YMin, int _YMax>
  bool
  operator<(
    const BoundedPoint<T, _XMin, _XMax, _YMin, _YMax>& rhs
  ) const
  {
    if (static_cast<S>(xy_.first) == static_cast<S>(rhs.xy_.first))
    {
      return static_cast<S>(xy_.second) < static_cast<S>(rhs.xy_.second);
    }
    return (static_cast<S>(xy_.first) < static_cast<S>(rhs.xy_.first));
  }

  template <class T, int _XMin, int _XMax, int _YMin, int _YMax>
  bool
  operator==(
    const BoundedPoint<T, _XMin, _XMax, _YMin, _YMax>& rhs
  ) const
  {
    return static_cast<S>(xy_.first) == static_cast<S>(rhs.xy_.first)
        && static_cast<S>(xy_.second) == static_cast<S>(rhs.xy_.second);
  }

private:
  pair<S, S> xy_;
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
  using BoundedPoint<DistanceSize, -1, 1, -1, 1>::BoundedPoint;
};

using ROSOffset = std::tuple<Offset>;
using OffsetSet = vector<ROSOffset>;

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
public:
  using BoundedPoint<XYSize, 0, MAX_COLUMNS, 0, MAX_ROWS>::BoundedPoint;

  XYPos(
    const Idx x0,
    const Idx y0,
    const InnerSize x1,
    const InnerSize y1
  )
    : XYPos(
        static_cast<XYSize>(x0) + static_cast<XYSize>(x1),
        static_cast<XYSize>(y0) + static_cast<XYSize>(y1)
      )
  {
  }
};

/**
 * \brief The position within the Environment that a spreading point has.
 */
class CellPos : public BoundedPoint<Idx, 0, MAX_COLUMNS, 0, MAX_ROWS>
{
public:
  using BoundedPoint<Idx, 0, MAX_COLUMNS, 0, MAX_ROWS>::BoundedPoint;
};

static constexpr MathSize
x(
  const auto& p
)
{
  return p.x();
}

static constexpr MathSize
y(
  const auto& p
)
{
  return p.y();
}
}
#endif
