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
  using array_dists = std::array<pair<double, InnerPos>, NUM_DIRECTIONS>;
  //   using array_dists = std::array<double, NUM_DIRECTIONS>;
  CellPoints() noexcept;
  //   // HACK: so we can emplace with NULL
  //   CellPoints(size_t) noexcept;
  // HACK: so we can emplace with nullptr
  CellPoints(const CellPoints* rhs) noexcept;
  //   CellPoints(const vector<InnerPos>& pts) noexcept;
  CellPoints(const double x, const double y) noexcept;
  CellPoints(const Idx cell_x, const Idx cell_y) noexcept;
  CellPoints(const InnerPos& p) noexcept;
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
  CellPoints& insert(const double x, const double y) noexcept;
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
  set<InnerPos> unique() const noexcept;
  bool operator<(const CellPoints& rhs) const noexcept;
  bool operator==(const CellPoints& rhs) const noexcept;
  [[nodiscard]] Location location() const noexcept;
  void clear();
  //   const array_pts points() const;
  friend CellPointsMap apply_offsets_spreadkey(
    const double duration,
    const OffsetSet& offsets,
    const spreading_points::mapped_type& cell_pts);
  bool is_invalid() const;
  bool empty() const;
  friend CellPointsMap;
private:
  array_dists find_distances(const double p_x, const double p_y) noexcept;
  CellPoints& insert_(const double x, const double y) noexcept;
  array_dists pts_;
  // use Idx instead of Location    so it can be negative (invalid)
  Idx cell_x_;
  Idx cell_y_;
  CellIndex src_;
};

// map that merges items when try_emplace doesn't insert
class CellPointsMap
{
public:
  CellPointsMap();
  void emplace(const CellPoints& pts);
  CellPoints& insert(const double x, const double y) noexcept;
  CellPointsMap& merge(const CellPointsMap& rhs) noexcept;
  set<InnerPos> unique() const noexcept;
  // apply function to each CellPoints within and remove matches
  void remove_if(std::function<bool(const pair<Location, CellPoints>&)> F);
  // FIX: public for debugging right now
  // private:
  map<Location, CellPoints> map_;
};
using spreading_points = CellPoints::spreading_points;

CellPointsMap apply_offsets_spreadkey(
  const double duration,
  const OffsetSet& offsets,
  const spreading_points::mapped_type& cell_pts);
}
