/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "InnerPos.h"
#ifndef FS_CELLPOINTS_H
#define FS_CELLPOINTS_H
namespace fs
{
static constexpr size_t NUM_DIRECTIONS = 16;
/**
 * Points in a cell furthest in each direction
 */
class CellPoints
{
public:
  using cellpoints_map_type = map<Location, CellPoints>;
  using spreading_points = map<SpreadKey, vector<pair<Cell, CellPoints>>>;
  using array_dists = std::array<pair<double, InnerPos>, NUM_DIRECTIONS>;
  CellPoints() noexcept;
  // HACK: so we can emplace with nullptr
  CellPoints(const CellPoints* rhs) noexcept;
  CellPoints(const vector<InnerPos>& pts) noexcept;
  CellPoints(const double x, const double y) noexcept;
  CellPoints(const InnerPos& p) noexcept;
  CellPoints(CellPoints&& rhs) noexcept;
  CellPoints(const CellPoints& rhs) noexcept;
  CellPoints& operator=(CellPoints&& rhs) noexcept;
  CellPoints& operator=(const CellPoints& rhs) noexcept;
  CellPoints& insert(const double x, const double y) noexcept;
  CellPoints& insert(const InnerPos& p) noexcept;
  template <class _ForwardIterator>
  CellPoints& insert(_ForwardIterator begin, _ForwardIterator end)
  {
    // don't do anything if empty
    if (end != begin)
    {
      auto it = begin;
      // should always be in the same cell so do this once
      const auto cell_x = static_cast<fs::Idx>((*it).x());
      const auto cell_y = static_cast<fs::Idx>((*it).y());
      while (end != it)
      {
        const auto p = *it;
        insert(cell_x, cell_y, p.x(), p.y());
        ++it;
      }
    }
    return *this;
  }
  void add_source(const CellIndex src);
  CellIndex sources() const { return src_; }
  CellPoints& merge(const CellPoints& rhs);
  friend CellPoints merge_cellpoints(const CellPoints& lhs, const CellPoints& rhs);
  set<InnerPos> unique() const noexcept;
  friend const cellpoints_map_type apply_offsets_spreadkey(
    const double duration,
    const OffsetSet& offsets,
    const spreading_points::mapped_type& cell_pts
  );

private:
  static array_dists find_distances(
    const double cell_x,
    const double cell_y,
    const double p_x,
    const double p_y
  ) noexcept;
  CellPoints& insert(
    const double cell_x,
    const double cell_y,
    const double x,
    const double y
  ) noexcept;
  array_dists pts_;
  CellIndex src_;
};
using cellpoints_map_type = CellPoints::cellpoints_map_type;
using spreading_points = CellPoints::spreading_points;
const cellpoints_map_type apply_offsets_spreadkey(
  const double duration,
  const OffsetSet& offsets,
  const spreading_points::mapped_type& cell_pts
);
CellPoints merge_cellpoints(const CellPoints& lhs, const CellPoints& rhs);
}
#endif
