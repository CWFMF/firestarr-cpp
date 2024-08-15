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
  using dist_pt = pair<double, InnerPos>;
  using array_dist_pts = std::array<dist_pt, NUM_DIRECTIONS>;
  using array_dists = std::array<double, NUM_DIRECTIONS>;
  CellPoints() noexcept;
  // HACK: so we can emplace with nullptr
  CellPoints(const CellPoints* rhs) noexcept;
  CellPoints(const InnerSize x, const InnerSize y) noexcept;
  CellPoints(const Idx cell_x, const Idx cell_y) noexcept;
  CellPoints(const InnerPos& p) noexcept;
  CellPoints(CellPoints&& rhs) noexcept;
  CellPoints(const CellPoints& rhs) noexcept;
  CellPoints& operator=(CellPoints&& rhs) noexcept;
  CellPoints& operator=(const CellPoints& rhs) noexcept;
  CellPoints& insert(const InnerSize x, const InnerSize y) noexcept;
  CellPoints& insert(const InnerPos& p) noexcept;
  void add_source(const CellIndex src);
  CellIndex sources() const { return src_; }
  CellPoints& merge(const CellPoints& rhs);
  set<InnerPos> unique() const noexcept;
  bool operator<(const CellPoints& rhs) const noexcept;
  bool operator==(const CellPoints& rhs) const noexcept;
  [[nodiscard]] Location location() const noexcept;
  void clear();
  friend CellPointsMap apply_offsets_spreadkey(
    const DurationSize duration,
    const OffsetSet& offsets,
    const spreading_points::mapped_type& cell_pts
  );
#ifdef DEBUG_POINTS
  bool is_invalid() const;
#endif
  bool empty() const;
  friend CellPointsMap;

private:
  array_dists find_distances(const InnerSize p_x, const InnerSize p_y) const noexcept;
  CellPoints& insert_(const double x, const double y) noexcept;
  array_dist_pts pts_;
  mutable set<InnerPos> pts_unique_;
  // FIX: no point in atomic if not parallel, but need mutex if it is
  mutable bool pts_dirty_;
  // use Idx instead of Location so it can be negative (invalid)
  Idx cell_x_;
  Idx cell_y_;
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
  CellPoints& insert(const InnerSize x, const InnerSize y) noexcept;
  CellPointsMap& merge(const BurnedData& unburnable, const CellPointsMap& rhs) noexcept;
  set<InnerPos> unique() const noexcept;
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
