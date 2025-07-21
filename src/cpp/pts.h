/* SPDX-FileCopyrightText: 2005, 2021 Jordan Evens */
/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_PTS_H
#define FS_PTS_H

#include "stdafx.h"
// #define USE_NEW_SPREAD
#ifndef USE_NEW_SPREAD
#undef DEBUG_NEW_SPREAD
#undef DEBUG_NEW_SPREAD_CHECK
#undef DEBUG_NEW_SPREAD_VERBOSE
#endif
#include "CellPoints.h"
#include "InnerPos.h"
#include "IntensityMap.h"
#ifdef USE_NEW_SPREAD
namespace fs::sim
{
using spreading_points_new = map<SpreadKey, vector<pair<HashSize, set<XYPos>>>>;
class Pts
{
public:
  Pts();
  Pts(const BurnedData& unburnable, const XYPos p);
  Pts(
    const BurnedData& unburnable,
    const XYSize x,
    const XYSize y
  )
    : Pts(unburnable, XYPos{x, y})
  {
  }
  Pts&
  insert(
    const XYPos p0
  )
  {
    return insert(p0.x(), p0.y());
  }
  Pts&
  insert(const XYSize x, const XYSize y);
  set<XYPos>
  unique() const noexcept;
  void
  add_unique(const Location& loc, set<XYPos>& into) const noexcept;
  bool
  empty() const;
  size_t
  size() const noexcept;
  inline const array_dists&
  distances() const
  {
    return std::get<0>(cell_pts_);
  }
  inline const array_pts&
  points() const
  {
    return std::get<1>(cell_pts_);
  }
  inline array_dists&
  distances()
  {
    return std::get<0>(cell_pts_);
  }
  inline array_pts&
  points()
  {
    return std::get<1>(cell_pts_);
  }
  bool
  canBurn() const;
private:
  array_cellpts cell_pts_;
  CellPos cell_x_y_;
};
class PtMap
{
public:
  PtMap() = default;
  PtMap(const PtMap& rhs) noexcept = default;
  PtMap(PtMap&& rhs) noexcept = default;
  PtMap&
  operator=(const PtMap& rhs) noexcept = default;
  PtMap&
  operator=(PtMap&& rhs) noexcept = default;
  Pts&
  insert(const BurnedData& unburnable, const XYPos p0);
  Pts&
  insert(
    const BurnedData& unburnable,
    const XYSize x,
    const XYSize y
  )
  {
    return insert(unburnable, XYPos{x, y});
  }
  set<XYPos>
  unique(const HashSize hash_value) const noexcept;
  set<XYPos>
  unique() const noexcept;
  void
  add_unique(set<XYPos>& into) const noexcept;
  set<HashSize>
  keys() const noexcept;
  size_t
  size() const noexcept;
  size_t
  erase(const HashSize hash_value) noexcept;
private:
  map<HashSize, Pts> map_;
};
}
#endif
#endif
