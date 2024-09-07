/* SPDX-FileCopyrightText: 2005, 2021 Jordan Evens */
/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_CELLPOINTS_H
#define FS_CELLPOINTS_H
#include "stdafx.h"
#include "InnerPos.h"
#include "IntensityMap.h"
namespace fs
{
static constexpr size_t NUM_DIRECTIONS = 16;
class CellPointsMap;
/**
 * Points in a cell furthest in each direction
 */
class CellPoints
{
public:
  using spreading_points = map<SpreadKey, vector<pair<Location, CellPoints>>>;
  using array_dists = std::array<DistanceSize, NUM_DIRECTIONS>;
  using array_pts = std::array<InnerPos, NUM_DIRECTIONS>;
  using array_dist_pts = pair<CellPoints::array_dists, CellPoints::array_pts>;
  CellPoints() noexcept;
  // HACK: so we can emplace with nullptr
  CellPoints(const CellPoints* rhs) noexcept;
  CellPoints(const XYSize x, const XYSize y) noexcept;
  CellPoints(const Idx cell_x, const Idx cell_y) noexcept;
  CellPoints(const XYPos& p) noexcept;
  CellPoints(CellPoints&& rhs) noexcept;
  CellPoints(const CellPoints& rhs) noexcept;
  CellPoints& operator=(CellPoints&& rhs) noexcept;
  CellPoints& operator=(const CellPoints& rhs) noexcept;
  CellPoints& insert(const XYSize x, const XYSize y) noexcept;
  CellPoints& insert(const InnerPos& p) noexcept;
  void add_source(const CellIndex src);
  CellIndex sources() const { return src_; }
  CellPoints& merge(const CellPoints& rhs);
  set<XYPos> unique() const noexcept;
  bool operator<(const CellPoints& rhs) const noexcept;
  bool operator==(const CellPoints& rhs) const noexcept;
  [[nodiscard]] Location location() const noexcept;
  void clear();
  friend CellPointsMap apply_offsets_spreadkey(
    const DurationSize duration,
    const OffsetSet& offsets,
    const spreading_points::mapped_type& cell_pts
  );
  bool empty() const;
  friend CellPointsMap;
  // FIX: just access directly for now
public:
  pair<array_dists, array_pts> pts_;

private:
  // use Idx instead of Location so it can be negative (invalid)
  CellPos cell_x_y_;
  CellIndex src_;
};
using spreading_points = CellPoints::spreading_points;
class Scenario;
// map that merges items when try_emplace doesn't insert
class CellPointsMap
{
public:
  CellPointsMap();
  void emplace(const CellPoints& pts);
  CellPoints& insert(const XYSize x, const XYSize y) noexcept;
  CellPoints& insert(const Location& src, const XYSize x, const XYSize y) noexcept;
  CellPointsMap& merge(const BurnedData& unburnable, const CellPointsMap& rhs) noexcept;
  set<XYPos> unique() const noexcept;
  // apply function to each CellPoints within and remove matches
  void remove_if(std::function<bool(const pair<Location, CellPoints>&)> F) noexcept;
  void calculate_spread(
    Scenario& scenario,
    map<SpreadKey, SpreadInfo>& spread_info,
    const DurationSize duration,
    const spreading_points& to_spread,
    const BurnedData& unburnable
  );
  // FIX: public for debugging right now
  // private:
  map<Location, CellPoints> map_;
};
CellPointsMap apply_offsets_spreadkey(
  const DurationSize duration,
  const OffsetSet& offsets,
  const spreading_points::mapped_type& cell_pts
);
}
#endif
