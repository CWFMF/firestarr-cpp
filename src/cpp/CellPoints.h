/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "stdafx.h"
#include "InnerPos.h"

namespace tbd::sim
{
// using sim::CellPoints;
using topo::Cell;
using topo::SpreadKey;
using points_list_type = OffsetSet;
using merged_map_type = map<Location, pair<CellIndex, points_list_type>>;
using spreading_points = map<SpreadKey, vector<pair<Cell, const points_list_type>>>;
using points_type = spreading_points::value_type::second_type;

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
  using cellpoints_map_type = map<Location, pair<CellIndex, CellPoints>>;
  using array_pts = std::array<InnerPos, NUM_DIRECTIONS>;
  using array_dists = std::array<double, NUM_DIRECTIONS>;
  CellPoints() noexcept;
  //   // HACK: so we can emplace with NULL
  //   CellPoints(size_t) noexcept;
  // HACK: so we can emplace with nullptr
  CellPoints(const CellPoints* rhs) noexcept;
  CellPoints(const vector<InnerPos>& pts) noexcept;
  CellPoints(const double x, const double y) noexcept;
  CellPoints(const InnerPos& p) noexcept;
  /**
   * \brief Move constructor
   * \param rhs CellPoints to move from
   */
  CellPoints(CellPoints&& rhs) noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs CellPoints to copy from
   */
  CellPoints(const CellPoints& rhs) noexcept = default;
  /**
   * \brief Move assignment
   * \param rhs CellPoints to move from
   * \return This, after assignment
   */
  CellPoints& operator=(CellPoints&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs CellPoints to copy from
   * \return This, after assignment
   */
  CellPoints& operator=(const CellPoints& rhs) noexcept = default;
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
      const auto cell_x = static_cast<tbd::Idx>((*it).x());
      const auto cell_y = static_cast<tbd::Idx>((*it).y());
      while (end != it)
      {
        const auto p = *it;
        insert(cell_x, cell_y, p.x(), p.y());
        ++it;
      }
    }
    return *this;
  }
  CellPoints& insert(const CellPoints& rhs);
  set<InnerPos> unique() const noexcept;
  const array_pts points() const;
  friend const cellpoints_map_type apply_offsets_spreadkey(
    const double duration,
    const OffsetSet& offsets,
    const points_type& cell_pts);
private:
  CellPoints& insert(const double cell_x, const double cell_y, const double x, const double y) noexcept;
  array_pts pts_;
  array_dists dists_;
  bool is_empty_;
};
using cellpoints_map_type = CellPoints::cellpoints_map_type;

const cellpoints_map_type apply_offsets_spreadkey(
  const double duration,
  const OffsetSet& offsets,
  const points_type& cell_pts);

// const merged_map_type convert_map(const cellpoints_map_type& m);
}
