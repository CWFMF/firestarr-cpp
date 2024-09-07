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
class BoundedPoint : public pair<S, S>
{
protected:
  using class_type = BoundedPoint<S, XMin, XMax, YMin, YMax>;
  static constexpr auto INVALID_X = XMin - 1;
  static constexpr auto INVALID_Y = YMin - 1;

public:
  using pair<S, S>::pair;
  constexpr BoundedPoint() noexcept : BoundedPoint(XMin - 1, YMin - 1) { }
  S x() const { return std::get<0>(*this); }
  S y() const { return std::get<1>(*this); }
  /**
   * \brief Add offset to position and return result
   */
  template <class T, class O>
  [[nodiscard]] constexpr T add(const O& o) const noexcept
  {
    return static_cast<T>(class_type(this->first + o.first, this->second + o.second));
  }
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
/**
 * \brief The position within the Environment that a spreading point has.
 */
class CellPos : public BoundedPoint<Idx, 0, MAX_COLUMNS, 0, MAX_ROWS>
{
  using BoundedPoint<Idx, 0, MAX_COLUMNS, 0, MAX_ROWS>::BoundedPoint;
};
static constexpr MathSize x(const auto& p) { return p.x(); }
static constexpr MathSize y(const auto& p) { return p.y(); }
}
#endif
