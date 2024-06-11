/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "InnerPos.h"
#ifndef FS_CELLPOINTS_H
#define FS_CELLPOINTS_H
namespace fs
{
static constexpr size_t FURTHEST_N = 0;
static constexpr size_t FURTHEST_NNE = 1;
static constexpr size_t FURTHEST_NE = 2;
static constexpr size_t FURTHEST_ENE = 3;
static constexpr size_t FURTHEST_E = 4;
static constexpr size_t FURTHEST_ESE = 5;
static constexpr size_t FURTHEST_SE = 6;
static constexpr size_t FURTHEST_SSE = 7;
static constexpr size_t FURTHEST_S = 8;
static constexpr size_t FURTHEST_SSW = 9;
static constexpr size_t FURTHEST_SW = 10;
static constexpr size_t FURTHEST_WSW = 11;
static constexpr size_t FURTHEST_W = 12;
static constexpr size_t FURTHEST_WNW = 13;
static constexpr size_t FURTHEST_NW = 14;
static constexpr size_t FURTHEST_NNW = 15;
static constexpr size_t NUM_DIRECTIONS = 16;
/**
 * Points in a cell furthest in each direction
 */
class CellPoints
{
public:
  using cellpoints_map_type = map<Location, CellPoints>;
  using spreading_points = map<SpreadKey, vector<pair<Cell, CellPoints>>>;
  using array_pts = std::array<InnerPos, NUM_DIRECTIONS>;
  using array_dists = std::array<double, NUM_DIRECTIONS>;
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
  bool empty() const { return is_empty_; }
  CellPoints& merge(const CellPoints& rhs);
  set<InnerPos> unique() const noexcept;
  const array_pts points() const;
  friend const cellpoints_map_type apply_offsets_spreadkey(
    const double duration,
    const OffsetSet& offsets,
    const spreading_points::mapped_type& cell_pts
  );

private:
  array_dists find_distances(
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
  array_pts pts_;
  array_dists dists_;
  CellIndex src_;
  bool is_empty_;
};
using cellpoints_map_type = CellPoints::cellpoints_map_type;
using spreading_points = CellPoints::spreading_points;
const cellpoints_map_type apply_offsets_spreadkey(
  const double duration,
  const OffsetSet& offsets,
  const spreading_points::mapped_type& cell_pts
);
}
#endif
