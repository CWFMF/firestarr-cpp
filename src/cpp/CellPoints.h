/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "stdafx.h"
#include "InnerPos.h"
#include "IntensityMap.h"

namespace tbd::sim
{
// using sim::CellPoints;
using topo::Cell;
using topo::SpreadKey;

// static constexpr size_t FURTHEST_N = 0;
// static constexpr size_t FURTHEST_NNE = 1;
// static constexpr size_t FURTHEST_NE = 2;
// static constexpr size_t FURTHEST_ENE = 3;
// static constexpr size_t FURTHEST_E = 4;
// static constexpr size_t FURTHEST_ESE = 5;
// static constexpr size_t FURTHEST_SE = 6;
// static constexpr size_t FURTHEST_SSE = 7;
// static constexpr size_t FURTHEST_S = 8;
// static constexpr size_t FURTHEST_SSW = 9;
// static constexpr size_t FURTHEST_SW = 10;
// static constexpr size_t FURTHEST_WSW = 11;
// static constexpr size_t FURTHEST_W = 12;
// static constexpr size_t FURTHEST_WNW = 13;
// static constexpr size_t FURTHEST_NW = 14;
// static constexpr size_t FURTHEST_NNW = 15;
static constexpr size_t NUM_DIRECTIONS = 16;

class CellPointsMap;
/**
 * Points in a cell furthest in each direction
 */
class CellPoints
{
public:
  using spreading_points = map<SpreadKey, vector<pair<Location, CellPoints>>>;
  // using dist_pt = pair<DistanceSize, InnerPos>;
  // using array_dist_pts = std::array<dist_pt, NUM_DIRECTIONS>;
  using array_dists = std::array<DistanceSize, NUM_DIRECTIONS>;
  using array_pts = std::array<InnerPos, NUM_DIRECTIONS>;
  using array_dist_pts = pair<CellPoints::array_dists, CellPoints::array_pts>;
  //   using array_dist_pts = std::array<DistanceSize, NUM_DIRECTIONS>;
  CellPoints() noexcept;
  //   // HACK: so we can emplace with NULL
  //   CellPoints(size_t) noexcept;
  // HACK: so we can emplace with nullptr
  CellPoints(const CellPoints* rhs) noexcept;
  //   CellPoints(const vector<InnerPos>& pts) noexcept;
  CellPoints(const XYSize x, const XYSize y) noexcept;
  CellPoints(const Idx cell_x, const Idx cell_y) noexcept;
  CellPoints(const XYPos& p) noexcept;
  /**
   * \brief Move constructor
   * \param rhs CellPoints to move from
   */
  CellPoints(CellPoints&& rhs) noexcept;
  /**
   * \brief Copy constructor
   * \param rhs CellPoints to copy from
   */
  CellPoints(const CellPoints& rhs) noexcept;
  /**
   * \brief Move assignment
   * \param rhs CellPoints to move from
   * \return This, after assignment
   */
  CellPoints& operator=(CellPoints&& rhs) noexcept;
  /**
   * \brief Copy assignment
   * \param rhs CellPoints to copy from
   * \return This, after assignment
   */
  CellPoints& operator=(const CellPoints& rhs) noexcept;
  CellPoints& insert(const XYSize x, const XYSize y) noexcept;
  CellPoints& insert(const InnerPos& p) noexcept;
  //   template <class _ForwardIterator>
  //   CellPoints& insert(_ForwardIterator begin, _ForwardIterator end)
  //   {
  //     // don't do anything if empty
  //     if (end != begin)
  //     {
  //       auto it = begin;
  //       while (end != it)
  //       {
  //         insert(*it);
  //         ++it;
  //       }
  //     }
  //     return *this;
  //   }
  void add_source(const CellIndex src);
  CellIndex sources() const
  {
    return src_;
  }
  CellPoints& merge(const CellPoints& rhs);
  set<XYPos> unique() const noexcept;
  bool operator<(const CellPoints& rhs) const noexcept;
  bool operator==(const CellPoints& rhs) const noexcept;
  [[nodiscard]] Location location() const noexcept;
  void clear();
  //   const array_pts points() const;
  friend CellPointsMap apply_offsets_spreadkey(
    const DurationSize duration,
    const OffsetSet& offsets,
    const spreading_points::mapped_type& cell_pts);
#ifdef DEBUG_POINTS
  bool is_invalid() const;
  void assert_all_equal(
    const array_dist_pts& pts,
    const InnerSize x0,
    const InnerSize y0) const;
  void assert_all_equal(
    const array_dist_pts& pts,
    const XYSize x,
    const XYSize y) const;
  void assert_all_invalid(const array_dist_pts& pts) const;
#endif
  bool empty() const;
  friend CellPointsMap;
private:
  array_dists find_distances(const InnerPos& p) const noexcept;
  CellPoints& insert_(const XYSize x, const XYSize y) noexcept;
  // FIX: just access directly for now
public:
  pair<array_dists, array_pts> pts_;
private:
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
  CellPoints& insert(const XYSize x, const XYSize y) noexcept;
  CellPoints& insert(const Location& src, const XYSize x, const XYSize y) noexcept;
  CellPointsMap& merge(
    const BurnedData& unburnable,
    const CellPointsMap& rhs) noexcept;
  set<XYPos> unique() const noexcept;
  // apply function to each CellPoints within and remove matches
  void remove_if(std::function<bool(const pair<Location, CellPoints>&)> F) noexcept;
  void calculate_spread(
    Scenario& scenario,
    map<SpreadKey, SpreadInfo>& spread_info,
    const DurationSize duration,
    const spreading_points& to_spread,
    const BurnedData& unburnable);
  // FIX: public for debugging right now
  // private:
  map<Location, CellPoints> map_;
};

CellPointsMap apply_offsets_spreadkey(
  const DurationSize duration,
  const OffsetSet& offsets,
  const spreading_points::mapped_type& cell_pts);
}
